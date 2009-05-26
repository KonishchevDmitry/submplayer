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


#ifndef HEADER_MAIN_WINDOW
	#define HEADER_MAIN_WINDOW

	#include <boost/shared_ptr.hpp>

	#include <mlib/gtk/window.hpp>


	class Subtitles;

	class Main_window: public m::gtk::Window
	{
		private:
			struct Private;


		public:
			Main_window(
				const std::vector<Subtitles>& subtitles,
				const std::vector<std::string>& mplayer_args
			);


		private:
			boost::shared_ptr<Private>	priv;


		private:
			/// Обработчик сигнала на закрытие окна.
			bool	on_delete_cb(GdkEventAny* event);

			/// Обработчик сигнала на закрытие плеера.
			void	on_player_closed_cb(void);

			/// Обработчик сигнала на изменение текущей позиции в проигрываемом
			/// файле.
			void	on_time_offset_changed_cb(void);
	};

#endif

