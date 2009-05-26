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


#ifndef HEADER_MPLAYER
	#define HEADER_MPLAYER

	#include <boost/noncopyable.hpp>
	#include <boost/shared_ptr.hpp>

	#include <sigc++/connection.h>
	#include <sigc++/slot.h>


	namespace aux { class Mplayer_impl; }

	/// Представляет из себя запущенную копию MPlayer'а, за которой мы
	/// наблюдаем.
	class Mplayer: public boost::noncopyable
	{
		private:
			typedef aux::Mplayer_impl Mplayer_impl;


		public:
			Mplayer(void);


		private:
			boost::shared_ptr<Mplayer_impl>	impl;


		public:
			/// Подключает обработчик сигнала на закрытие MPlayer'а.
			sigc::connection	connect_quit_handler(const sigc::slot<void>& slot);

			/// Подключает обработчик сигнала на изменения текущей позиции в
			/// проигрываемом файле.
			sigc::connection	connect_time_offset_changed_handler(const sigc::slot<void>& slot);

			/// Возвращает текущую позицию в проигрываемом файле.
			Time_ms				get_current_offset(void) const;

			/// Запускает Mplayer.
			void				start(const std::vector<std::string>& args) throw(m::Exception);
	};

#endif

