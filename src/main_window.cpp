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

#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>

#include <pango/pango-font.h>
#include <pango/pango-utils.h>

#include <utility>
#if M_BOOST_GET_VERSION() >= M_GET_VERSION(1, 36, 0)
	#include <boost/unordered_map.hpp>
#else
	#include <map>
#endif

#include <glibmm/main.h>

#include <gtkmm/box.h>
#include <gtkmm/main.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/textview.h>

#include <mlib/gtk/window_settings.hpp>
#include <mlib/fs.hpp>
#include <mlib/misc.hpp>

#include "main_window.hpp"
#include "mplayer.hpp"
#include "subtitles.hpp"



namespace
{
	/// Последовательности байт, соответствующие определенным клавишам.
	const struct Key
	{
		int			gdk_keyval;
		const char*	value;
	} KEYS[] = {
		GDK_Escape			,	"\x1b"					,

		GDK_F1				,	"\x1b\x4f\x50"			,
		GDK_F2				,	"\x1b\x4f\x51"			,
		GDK_F3				,	"\x1b\x4f\x52"			,
		GDK_F4				,	"\x1b\x4f\x53"			,
		GDK_F5				,	"\x1b\x5b\x31\x35\x7e"	,
		GDK_F6				,	"\x1b\x5b\x31\x37\x7e"	,
		GDK_F7				,	"\x1b\x5b\x31\x38\x7e"	,
		GDK_F8				,	"\x1b\x5b\x31\x39\x7e"	,
		GDK_F9				,	"\x1b\x5b\x32\x30\x7e"	,
		GDK_F10				,	"\x1b\x5b\x32\x31\x7e"	,
		GDK_F11				,	"\x1b\x5b\x32\x33\x7e"	,
		GDK_F12				,	"\x1b\x5b\x32\x34\x7e"	,

		GDK_Tab				,	"\x9"					,

		GDK_BackSpace		,	"\x7f"					,
		GDK_Return			,	"\xa"					,

		GDK_Insert			,	"\x1b\x5b\x32\x7e"		,
		GDK_Home			,	"\x1b\x4f\x48"			,
		GDK_Page_Up			,	"\x1b\x5b\x35\x7e"		,
		GDK_Delete			,	"\x1b\x5b\x33\x7e"		,
		GDK_End				,	"\x1b\x4f\x46"			,
		GDK_Page_Down		,	"\x1b\x5b\x36\x7e"		,

		GDK_Left			,	"\x1b\x5b\x44"			,
		GDK_Up				,	"\x1b\x5b\x41"			,
		GDK_Right			,	"\x1b\x5b\x43"			,
		GDK_Down			,	"\x1b\x5b\x42"			,

		GDK_KP_Home			,	"\x1b\x5b\x31\x7e"		,
		GDK_KP_Up			,	"\x1b\x5b\x41"			,
		GDK_KP_Page_Up		,	"\x1b\x5b\x35\x7e"		,
		GDK_KP_Left			,	"\x1b\x5b\x44"			,
		GDK_KP_Begin		,	"\x1b\x5b\x45"			,
		GDK_KP_Right		,	"\x1b\x5b\x43"			,
		GDK_KP_End			,	"\x1b\x5b\x34\x7e"		,
		GDK_KP_Down			,	"\x1b\x5b\x42"			,
		GDK_KP_Page_Down	,	"\x1b\x5b\x36\x7e"		,

		GDK_KP_Insert		,	"\x1b\x5b\x32\x7e"		,
		GDK_KP_Delete		,	"\x1b\x5b\x33\x7e"		,

		GDK_KP_7			,	"\x1b\x5b\x31\x7e"		,
		GDK_KP_8			,	"\x1b\x5b\x41"			,
		GDK_KP_9			,	"\x1b\x5b\x35\x7e"		,
		GDK_KP_4			,	"\x1b\x5b\x44"			,
		GDK_KP_5			,	"\x1b\x5b\x45"			,
		GDK_KP_6			,	"\x1b\x5b\x43"			,
		GDK_KP_1			,	"\x1b\x5b\x34\x7e"		,
		GDK_KP_2			,	"\x1b\x5b\x42"			,
		GDK_KP_3			,	"\x1b\x5b\x36\x7e"		,
		GDK_KP_0			,	"\x1b\x5b\x32\x7e"		,
		0					,	NULL
	};
}



class Subtitles_control: public Gtk::ScrolledWindow
{
	private:
		struct Subtitle
		{
			Subtitle(Time_ms time, Glib::RefPtr<Gtk::TextMark> mark);

			Time_ms						time;
			Glib::RefPtr<Gtk::TextMark>	mark;
		};


	public:
		Subtitles_control(const Subtitles& subtitles);


	private:
		Gtk::TextView*					text_view;
		Glib::RefPtr<Gtk::TextBuffer>	buffer;

		/// Тэг, определяющий параметры отображения субтитров, проигрываемых в
		/// данный момент.
		Glib::RefPtr<Gtk::TextTag>		tag_current;


		/// "Субтитр", выделенный в данный момент в текстовом буфере.
		size_t							cur_id;

		/// Список указателей на субтитры в текстовом буфере.
		std::vector<Subtitle>			subtitles;


	public:
		/// Делает активным субтитр, соответствующий времени offset.
		void			scroll_to(Time_ms time);

	private:
		/// Возвращает итератор для субтитра id.
		Gtk::TextIter	get_iter_for(size_t id) const;

		/// Задает текущий субтитр.
		void			set_current(size_t id);
};



struct Main_window::Private
{
	Private(void);

#if M_BOOST_GET_VERSION() >= M_GET_VERSION(1, 36, 0)
	boost::unordered_map<int, std::string>	key_values;
#else
	std::map<int, std::string>				key_values;
#endif

	std::vector<Subtitles_control*>			controls;
	sigc::connection						time_offset_changed_connection;

	Mplayer									mplayer;

	m::File_holder							nonblock_stdin;
};



// Subtitles_control -->
	Subtitles_control::Subtitle::Subtitle(Time_ms time, Glib::RefPtr<Gtk::TextMark> mark)
	:
		time(time), mark(mark)
	{
	}



	Subtitles_control::Subtitles_control(const Subtitles& subtitles)
	:
		cur_id(0)
	{
		this->set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
		this->set_shadow_type(Gtk::SHADOW_IN);

		this->text_view = Gtk::manage( new Gtk::TextView );
		this->text_view->set_editable(false);
		this->text_view->set_wrap_mode(Gtk::WRAP_WORD);
		this->text_view->property_left_margin() = 3;
		this->add(*this->text_view);

		this->buffer = this->text_view->get_buffer();

		this->tag_current = this->buffer->create_tag();
		this->tag_current->property_scale() = PANGO_SCALE_X_LARGE;
	#if PANGO_VERSION_CHECK(1, 24, 0)
		this->tag_current->property_weight() = PANGO_WEIGHT_MEDIUM;
	#else
		this->tag_current->property_weight() = (PANGO_WEIGHT_NORMAL + PANGO_WEIGHT_SEMIBOLD) / 2;
	#endif

		bool first = true;
		M_FOR_CONST_IT(subtitles.get(), it)
		{
			Glib::RefPtr<Gtk::TextMark> mark = this->buffer->create_mark(this->buffer->end());
			this->buffer->insert(
				this->buffer->end(),
				first ? it->text : "\n" + it->text
			);
			this->subtitles.push_back( Subtitle(it->time, mark) );

			first = false;
		}

		this->set_current(this->cur_id);
	}



	Gtk::TextIter Subtitles_control::get_iter_for(size_t id) const
	{
		if(id >= this->subtitles.size())
			return this->buffer->end();
		else
			return this->buffer->get_iter_at_mark(this->subtitles[id].mark);
	}



	void Subtitles_control::scroll_to(Time_ms time)
	{
		size_t id = this->cur_id;

		if(this->subtitles[id].time < time)
		{
			size_t size = subtitles.size();

			while(++id != size)
				if(this->subtitles[id].time > time)
					break;
			id--;
		}
		else
		{
			while(--id != static_cast<size_t>(-1))
				if(this->subtitles[id].time < time)
					break;
			id++;
		}

		if(id != this->cur_id)
			this->set_current(id);
	}



	void Subtitles_control::set_current(size_t id)
	{
		// Снимаем выделение с текущего субтитра
		this->buffer->remove_all_tags(
			this->get_iter_for(this->cur_id),
			this->get_iter_for(this->cur_id + 1)
		);

		// Изменяем текущий субтитр
		this->cur_id = id;

		Gtk::TextIter next_subtitle_it = this->get_iter_for(this->cur_id + 1);

		// Выделяем текущий субтитр
		this->buffer->apply_tag(
			this->tag_current,
			this->get_iter_for(this->cur_id),
			next_subtitle_it
		);

		// Прокручиваем TextView так, чтобы текущий субтитр находился внизу
		// экрана.
		// -->
			// Почему-то одного раза GTK не достаточно - текст прокручивается,
			// но только частично, не останавливаясь в точности там, где
			// требуется. Судя по тестам, двух раз всегда бывает достаточно.
			for(size_t i = 0; i < 2; i++)
				this->text_view->scroll_to(next_subtitle_it, 0, 1, 1);
		// <--
	}
// Subtitles_control <--



// Private -->
	Main_window::Private::Private(void)
	{
		// Создаем индекс по клавишам -->
		{
			const Key* key = KEYS;

			while(key->value)
			{
				this->key_values.insert(
					std::make_pair(key->gdk_keyval, std::string(key->value)));
				key++;
			}
		}
		// Создаем индекс по клавишам <--
	}
// Private <--



// Main_window -->
	Main_window::Main_window(const std::vector<Subtitles>& subtitles, const std::vector<std::string>& mplayer_args)
	:
		m::gtk::Window(APP_NAME, m::gtk::Window_settings(), 400 * subtitles.size(), 200, 2),
		priv( new Private )
	{
		Gtk::HBox* main_hbox = Gtk::manage( new Gtk::HBox(false, 3) );
		this->add(*main_hbox);

		M_FOR_CONST_IT(subtitles, it)
		{
			Subtitles_control* control = Gtk::manage( new Subtitles_control(*it) );
			main_hbox->pack_start(*control, true, true);
			priv->controls.push_back(control);
		}

		priv->time_offset_changed_connection =
			priv->mplayer.connect_time_offset_changed_handler(
				sigc::mem_fun(*this, &Main_window::on_time_offset_changed_cb)
			);

		this->add_events(Gdk::KEY_PRESS_MASK);
		this->signal_key_press_event().connect(
			sigc::mem_fun(*this, &Main_window::on_key_press_event_cb), false);

		this->signal_delete_event().connect(
			sigc::mem_fun(*this, &Main_window::on_delete_cb));

		priv->mplayer.connect_quit_handler(
			sigc::mem_fun(*this, &Main_window::on_player_closed_cb));

		try
		{
			priv->mplayer.start(mplayer_args);
		}
		catch(m::Exception& e)
		{
			MLIB_W(EE(e));
		}

		// Прослушиваем стандартный ввод -->
		{
			int fd;
			long flags;

			try
			{
				fd = m::unix_dup(STDIN_FILENO);
			}
			catch(m::Exception& e)
			{
				MLIB_W(EE(e));
			}

			priv->nonblock_stdin.set(fd);

			if( ( flags = fcntl(fd, F_GETFL) ) == -1 )
				MLIB_W(__("Can't get flags for the stdin: %1.", EE(errno)));

			if(fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
				MLIB_W(__("Can't set flags for the stdin: %1.", EE(errno)));

			// TODO
			// В gtkmm есть небольшая бага, из-за которой
			// Glib::RefPtr<Glib::IOSource> должен быть удален до того, как
			// завершится Main loop. Т. к. у нас окно живет дольше Main loop'а,
			// то можно пока оставить и так.
			Glib::RefPtr<Glib::IOSource> stdin_io_source =
				Glib::IOSource::create(priv->nonblock_stdin.get(), Glib::IO_IN);
			stdin_io_source->connect(
				sigc::mem_fun(*this, &Main_window::on_stdin_data));
			stdin_io_source->attach();
		}
		// Прослушиваем стандартный ввод <--

		this->show_all();
	}



	bool Main_window::on_delete_cb(GdkEventAny* event)
	{
		priv->time_offset_changed_connection.disconnect();
		this->hide();
		return false;
	}



	bool Main_window::on_key_press_event_cb(const GdkEventKey* event)
	{
		std::string string;

		// Получаем последовательность байт, представляющих нажатую клавишу -->
		{
			M_CONST_ITER_TYPE(priv->key_values) key_it = priv->key_values.find(event->keyval);

			if(key_it != priv->key_values.end())
				string = key_it->second;
			else
			{
				guint32 unicode_val = gdk_keyval_to_unicode(event->keyval);

				if(unicode_val)
					string = U2L(Glib::ustring(1, unicode_val));
			}
		}
		// Получаем последовательность байт, представляющих нажатую клавишу <--

		if(!string.empty())
		{
			try
			{
				priv->mplayer.write_to_stdio(string.data(), string.size());
			}
			catch(m::Exception& e)
			{
				MLIB_SW(__("Unable to write data to the mplayer stdio: %1.", EE(e)));
			}
		}

		return true;
	}



	void Main_window::on_player_closed_cb(void)
	{
		MLIB_D("Player closed.");
		Gtk::Main::quit();
	}



	bool Main_window::on_stdin_data(Glib::IOCondition condition)
	{
		// Вполне достаточно, если учесть то, что пользователь просто нажимает
		// клавиши-команды.
		char buf[10];
		ssize_t readed_bytes;

		MLIB_D("Gotten data from stdin.");

		try
		{
			readed_bytes = m::fs::unix_read(priv->nonblock_stdin.get(), buf, sizeof buf, true);
		}
		catch(m::Exception& e)
		{
			MLIB_SW(__("Error while reading from stdin: %1.", EE(e)));
		}

		try
		{
			if(readed_bytes)
				priv->mplayer.write_to_stdio(buf, readed_bytes);
		}
		catch(m::Exception& e)
		{
			MLIB_SW(__("Unable to write data to the mplayer stdio: %1.", EE(e)));
		}

		return true;
	}



	void Main_window::on_time_offset_changed_cb(void)
	{
		MLIB_D("Current time offset has been changed.");

		Time_ms time = priv->mplayer.get_current_offset();

		M_FOR_IT(priv->controls, it)
			(*it)->scroll_to(time);
	}
// Main_window <--

