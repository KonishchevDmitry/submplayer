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


#ifndef HEADER_SUBTITLES
	#define HEADER_SUBTITLES


	#include <vector>


	/// Представляет из себя файл с субтитрами.
	class Subtitles
	{
		public:
			/// Представляет из себя один субтитр.
			class Subtitle
			{
				public:
					Subtitle(Time_ms time, const std::string& text);


				public:
					Time_ms		time;
					std::string	text;


			#ifdef DEVELOP_MODE
				public:
					void	dump(void) const;
			#endif
			};

			typedef std::vector<Subtitle> Storage;


		private:
			Storage	subtitles;


		public:
			/// Возвращает контейнер с субтитрами.
			const Storage&	get(void) const;

			/// Загружает субтитры из файла.
			void			load(const std::string& file_path) throw(m::Exception);

	#ifdef DEVELOP_MODE
			void			dump(void) const;
	#endif
	};

#endif

