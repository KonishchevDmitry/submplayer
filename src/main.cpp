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


#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#include <clocale>

#include <iostream>
#include <memory>

#include <boost/filesystem.hpp>

#include <gtkmm/main.h>
#include <gtkmm/stock.h>

#include <mlib/fs.hpp>

#include "main_window.hpp"
#include "mplayer.hpp"
#include "subtitles.hpp"



namespace
{
	void sigchld_handler(int signal_no);
	void usage(void);
	void warning_function(const char* file, const int line, const std::string& title, const std::string& message);



	void sigchld_handler(int signal_no)
	{
		wait(NULL);
	}



	void warning_function(const char* file, const int line, const std::string& title, const std::string& message)
	{
		std::cerr
			#ifdef DEBUG_MODE
				<< U2L(m::get_log_debug_prefix(file, line))
			#endif
			<< "W: ";

		if(!title.empty())
			std::cerr << "[" << U2L(title) << "] ";

		std::cerr << U2L(message) << std::endl;

		std::cerr.flush();

		exit(EXIT_FAILURE);
	}



	void usage(void)
	{
		std::cout << U2L(__(
			"Usage:\n"
			"%1 video_file [other mplayer options]",
			APP_UNIX_NAME
		)) << std::endl;

		exit(EXIT_FAILURE);
	}
}



int main(int argc, char *argv[])
{
	m::set_warning_function(warning_function);

	// Устанавливаем обработчики сигналов -->
	{
		struct sigaction sig_action;

		// SIGPIPE -->
			sig_action.sa_handler = SIG_IGN;
			sigemptyset(&sig_action.sa_mask);
			sig_action.sa_flags = 0;

			sigaction(SIGPIPE, &sig_action, NULL);
		// SIGPIPE <--

		// SIGCHLD -->
			sig_action.sa_handler = &sigchld_handler;
			sigemptyset(&sig_action.sa_mask);
			sig_action.sa_flags = 0;

			sigaction(SIGCHLD, &sig_action, NULL);
		// SIGCHLD <--
	}
	// Устанавливаем обработчики сигналов <--

	// gettext -->
		setlocale(LC_ALL, "");

		#ifdef ENABLE_NLS
			if(!bindtextdomain(APP_UNIX_NAME, APP_LOCALE_PATH))
				MLIB_D(_C("Unable to bind text domain: %1.", EE(errno)));
			else if(!bind_textdomain_codeset(APP_UNIX_NAME, "UTF-8"))
				MLIB_D(_C("Unable to bind text domain codeset: %1.", EE(errno)));
			else if(!textdomain(APP_UNIX_NAME))
				MLIB_D(_C("Unable to set text domain: %1.", EE(errno)));
		#endif
	// gettext <--


	std::vector<Subtitles> subtitles;
	std::vector<std::string> mplayer_args;

	// Получаем все необходимые нам данные -->
	{
		std::string file_to_play;
		std::vector<std::string> subtitles_paths;

		// Парсим аргументы командной строки -->
			MLIB_D("Parsing command line args...");

			if(argc < 2)
				usage();

			{
				char* const* arg = argv + 1;

				while(*arg)
				{
					MLIB_D(_C("Gotten arg: '%1'.", *arg));

					if(**arg != '-' && file_to_play.empty())
						file_to_play = L2U(*arg);
					mplayer_args.push_back(L2U(*arg));

					arg++;
				}
			}

			if(file_to_play.empty())
				usage();

			MLIB_D(_C("File to play: '%1'.", file_to_play));
		// Парсим аргументы командной строки <--

		// Получаем список файлов с субтитрами, соответствующими проигрываемому
		// файлу.
		// -->
		{
			MLIB_D(_C("Finding subtitles for '%1'...", file_to_play));

			Path file_dir_path = Path(file_to_play).dirname();
			Glib::ustring play_name = m::fs::strip_extension(Path(file_to_play).basename()).uppercase();

			try
			{
				for(
					fs::directory_iterator it(U2L(file_dir_path.string()));
					it != fs::directory_iterator(); it++
				)
				{
					Glib::ustring file_name = L2U( Path(it->path()).basename() );
					MLIB_D(_C("Gotten file: '%1'.", file_name));

					if(!m::fs::check_extension(file_name, "srt"))
						continue;

					file_name = m::fs::strip_extension(file_name);

					if(file_name.size() >= play_name.size() && file_name.substr(0, play_name.size()).uppercase() == play_name)
					{
						MLIB_D("This file is subtitles file for our playing file.");
						subtitles_paths.push_back(L2U( it->path().string() ));
					}
				}
			}
			catch(fs::filesystem_error& e)
			{
				MLIB_W(__("Error while reading directory '%1': %2.", file_dir_path, EE(errno)));
			}
		}
		// <--

		// Загружаем все необходимые субтитры -->
			M_FOR_CONST_IT(subtitles_paths, it)
			{
				subtitles.push_back(Subtitles());

				try
				{
					subtitles.back().load(*it);
				#ifdef DEVELOP_MODE
					subtitles.back().dump();
				#endif
				}
				catch(m::Exception& e)
				{
					MLIB_SW(__("Error while reading subtitles file '%1': %2.", *it, EE(e)));
					subtitles.pop_back();
				}
			}
		// Загружаем все необходимые субтитры <--
	}
	// Получаем все необходимые нам данные <--

	// Начинаем работу -->
		if(subtitles.empty())
		{
			try
			{
				m::unix_execvp("mplayer", mplayer_args);
			}
			catch(m::Sys_exception& e)
			{
				MLIB_W(__("Starting MPlayer failed: %1.", EE(e)));
			}
		}
		else
		{
			Glib::thread_init();

			std::auto_ptr<Gtk::Main> gtk_main = std::auto_ptr<Gtk::Main>(new Gtk::Main(argc, argv));
			Glib::set_prgname(APP_UNIX_NAME);
			Glib::set_application_name(APP_NAME);

			Main_window window(subtitles, mplayer_args);
			subtitles.clear();
			Gtk::Main::run();
		}
	// Начинаем работу <--

	MLIB_D("Exiting...");

    return EXIT_SUCCESS;
}

