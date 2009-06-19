/**************************************************************************
*                                                                         *
*   submplayer - Simple MPlayer wrapper for subtitles watching            *
*   http://sourceforge.net/projects/submplayer                            *
*                                                                         *
*   Copyright (C) 2009, Konishchev Dmitry                                 *
*   http://konishchevdmitry.blogspot.com/                                 *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
**************************************************************************/


#include <fcntl.h>
#include <unistd.h>

#include <cmath>

#include <memory>

#include <boost/noncopyable.hpp>
#include <boost/ref.hpp>
#include <boost/thread.hpp>

#include <sigc++/connection.h>
#include <sigc++/slot.h>

#include <glibmm/dispatcher.h>
#include <glibmm/regex.h>

#include <mlib/fs.hpp>

#include "mplayer.hpp"



namespace aux {

class Mplayer_impl: public boost::noncopyable
{
	public:
		Mplayer_impl(void);
		~Mplayer_impl(void);


	private:
		/// Регулярное выражение для вычленения текущей позиции в проигрываемом
		/// файле из вывода MPlayer'а.
		Glib::RefPtr<Glib::Regex>	offset_regex;


		/// Блокирует доступ к:
		///   current_time_offset
		mutable
		boost::mutex				mutex;

		/// Текущая позиция в проигрываемом в данный момент файле.
		Time_ms						current_time_offset;


		/// Сигнал на изменения текущей позиции в проигрываемом файле.
		Glib::Dispatcher			offset_changed_signal;

		/// Сигнал на завершение работы mplayer'а.
		Glib::Dispatcher			mplayer_quit_signal;


		/// Файловый дескриптор стандатного ввода MPlayer'а.
		m::File_holder				mplayer_stdin;

		/// Файловый дескриптор стандатного вывода MPlayer'а.
		m::File_holder				mplayer_stdout;

		/// Поток, осуществляющий работу с MPlayer'ом.
		std::auto_ptr<
			boost::thread>			mplayer_thread;


	public:
		/// Подключает обработчик сигнала на закрытие MPlayer'а.
		sigc::connection	connect_quit_handler(const sigc::slot<void>& slot);

		/// Подключает обработчик сигнала на изменения текущей позиции в
		/// проигрываемом файле.
		sigc::connection	connect_time_offset_changed_handler(const sigc::slot<void>& slot);

		/// Возвращает текущую позицию в проигрываемом файле.
		Time_ms				get_current_offset(void) const;

		/// Запускает MPlayer.
		void				start(const std::vector<std::string>& args) throw(m::Exception);

		/// Записывает данные в стандартный поток ввода MPlayer'а.
		void				write_to_stdio(const void* data, size_t size) throw(m::Exception);

	private:
		/// Обрабатывает полученную от MPlayer'а логическую строку.
		void				process_string(const std::string& string);


	public:
		/// Поток, осуществляющий работу с MPlayer'ом.
		void	operator()(void);
};



Mplayer_impl::Mplayer_impl(void)
:
	offset_regex(Glib::Regex::create(
		"^A:\\s*(\\d+\\.\\d+)\\s+V:\\s*\\d+\\.\\d+\\s+")),
	current_time_offset(0)
{
}



Mplayer_impl::~Mplayer_impl(void)
{
	if(this->mplayer_thread.get())
		this->mplayer_thread->join();
}



sigc::connection Mplayer_impl::connect_quit_handler(const sigc::slot<void>& slot)
{
	return this->mplayer_quit_signal.connect(slot);
}



sigc::connection Mplayer_impl::connect_time_offset_changed_handler(const sigc::slot<void>& slot)
{
	return this->offset_changed_signal.connect(slot);
}



Time_ms Mplayer_impl::get_current_offset(void) const
{
	boost::mutex::scoped_lock lock(this->mutex);
	return this->current_time_offset;
}



void Mplayer_impl::process_string(const std::string& string)
{
	Time_ms offset;


	Glib::StringArrayHandle matches = this->offset_regex->split(string);

	if(matches.size() < 2)
	{
		MLIB_D(_C("Gotten non-time-offset MPlayer output string '%1'.", string));
		return;
	}

	MLIB_D(_C("Gotten time offset MPlayer output string '%1'.", string));


	offset = static_cast<Time_ms>(
		boost::lexical_cast<double>( *(matches.begin() + 1) ) * 1000);

	if(!std::isnormal(offset))
		offset = 0;

	MLIB_D(_C("Gotten cur time offset: %1.", offset));


	{
		bool changed;

		{
			boost::mutex::scoped_lock lock(this->mutex);
			changed = offset != this->current_time_offset;
			this->current_time_offset = offset;
		}

		if(changed)
			this->offset_changed_signal();
	}
}



void Mplayer_impl::start(const std::vector<std::string>& args) throw(m::Exception)
{
	if(this->mplayer_thread.get())
		M_THROW(_("MPlayer is already started."));


	m::File_holder child_stdin;
	m::File_holder child_stdout;

	// Создаем средства коммуникации между MPlayer'ом и нашей программой -->
	{
		// Генерирует m::Exception
		std::pair<int, int> pipe_fds = m::unix_pipe();
		this->mplayer_stdin.set(pipe_fds.second);
		child_stdin.set(pipe_fds.first);
	}
	{
		// Генерирует m::Exception
		std::pair<int, int> pipe_fds = m::unix_pipe();
		this->mplayer_stdout.set(pipe_fds.first);
		child_stdout.set(pipe_fds.second);
	}
	// Создаем средства коммуникации между MPlayer'ом и нашей программой <--

	if(m::unix_fork())
	{
		// Родительский процесс

		this->mplayer_thread = std::auto_ptr<boost::thread>(
			new boost::thread(boost::ref(*this))
		);
	}
	else
	{
		// Дочерний процесс

		try
		{
			// Генерирует m::Exception
			m::unix_dup(child_stdin.get(), STDIN_FILENO);
			child_stdin.reset();

			// Генерирует m::Exception
			m::unix_dup(child_stdout.get(), STDOUT_FILENO);
			child_stdout.reset();

			// Закрываем все открытые файловые дескрипторы
			// Генерирует m::Exception
			m::close_all_fds();

			// Генерирует m::Sys_exception
			m::unix_execvp("mplayer", args);
		}
		catch(m::Sys_exception& e)
		{
			MLIB_W(__("Starting MPlayer failed: %1.", EE(e)));
		}
		catch(m::Exception& e)
		{
			MLIB_W(__("Starting MPlayer failed. %1", EE(e)));
		}
	}
}



void Mplayer_impl::write_to_stdio(const void* data, size_t size) throw(m::Exception)
{
	m::fs::unix_write(this->mplayer_stdin.get(), data, size);
}



void Mplayer_impl::operator()(void)
{
	try
	{
		long flags;
		int read_fd = this->mplayer_stdout.get();

		char byte;
		char buf[PIPE_BUF];
		ssize_t readed_bytes;
		std::string output;


		if( ( flags = fcntl(read_fd, F_GETFL) ) == -1 )
			MLIB_E(__("Can't get flags for a pipe: %1.", EE(errno)));

		// Ждем, пока MPlayer выдаст какие-либо данные
		while(m::fs::unix_read(read_fd, &byte, sizeof byte))
		{
		#ifndef DEVELOP_MODE
			try
			{
				m::fs::unix_write(STDOUT_FILENO, &byte, sizeof byte);
			}
			catch(m::Exception& e)
			{
				MLIB_W(__("Can't write data to stdout: %1.", EE(e)));
			}
		#endif
			output += byte;

			// Получаем все данные, которые есть на данный момент -->
				if(fcntl(read_fd, F_SETFL, flags | O_NONBLOCK) == -1)
					MLIB_E(__("Can't set flags for a pipe: %1.", strerror(errno)));

				while( (readed_bytes = m::fs::unix_read(read_fd, buf, sizeof buf, true)) )
				{
					#ifndef DEVELOP_MODE
						try
						{
							m::fs::unix_write(STDOUT_FILENO, buf, readed_bytes);
						}
						catch(m::Exception& e)
						{
							MLIB_W(__("Can't write data to stdout: %1.", EE(e)));
						}
					#endif
					output.append(buf, readed_bytes);
				}

				if(fcntl(read_fd, F_SETFL, flags) == -1)
					MLIB_E(__("Can't set flags for a pipe: %1.", strerror(errno)));
			// Получаем все данные, которые есть на данный момент <--

			// Построчно обрабатываем полученные данные -->
			{
				const char* data = output.data();
				size_t size = output.size();
				size_t start_pos = 0;

				for(size_t i = 0; i < size; i++)
				{
					switch(data[i])
					{
						// В зависимости от типа терминала MPlayer по разному
						// разделяет строки + прибавляет различные управляющие
						// символы.
						case '\r':
						case '\n':
							if(start_pos != i)
								this->process_string(std::string(data + start_pos, data + i));
							start_pos = i + 1;
							break;

						default:
							break;
					}
				}

				// Вырезаем обработанные данные
				if(start_pos)
					output.erase(0, start_pos);
			}
			// Построчно обрабатываем полученные данные <--

			if(output.size() > sizeof buf)
				M_THROW(__("invalid output - no any line delimiter over %1 bytes", output.size()));
		}

		this->mplayer_quit_signal();
	}
	catch(m::Exception& e)
	{
		MLIB_W(__("Error while reading MPlayer output: %1.", EE(e)));
	}
}

}



// Mplayer -->
	Mplayer::Mplayer(void)
	:
		impl(boost::shared_ptr<Mplayer_impl>(new Mplayer_impl))
	{
	}



	sigc::connection Mplayer::connect_quit_handler(const sigc::slot<void>& slot)
	{
		return this->impl->connect_quit_handler(slot);
	}



	sigc::connection Mplayer::connect_time_offset_changed_handler(const sigc::slot<void>& slot)
	{
		return this->impl->connect_time_offset_changed_handler(slot);
	}



	Time_ms Mplayer::get_current_offset(void) const
	{
		return this->impl->get_current_offset();
	}



	void Mplayer::start(const std::vector<std::string>& args) throw(m::Exception)
	{
		this->impl->start(args);
	}



	void Mplayer::write_to_stdio(const void* data, size_t size) throw(m::Exception)
	{
		this->impl->write_to_stdio(data, size);
	}

// Mplayer <--

