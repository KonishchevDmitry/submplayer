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


#include <unistd.h>

#include <algorithm>
#include <fstream>
#include <functional>
#include <vector>

#include <glibmm/convert.h>
#include <glibmm/regex.h>

#include "subtitles.hpp"



namespace
{
	// Перекодирует строку с текстом субтитров в UTF-8, пытаясь подобрать
	// правильную кодировку.
	Glib::ustring	smart_convert(const std::string& string);


	Glib::ustring smart_convert(const std::string& string)
	{
		try
		{
			return Glib::locale_to_utf8(string);
		}
		catch(Glib::ConvertError&)
		{
			try
			{
				return Glib::convert(string, "UTF-8", "CP1251");
			}
			catch(Glib::ConvertError&)
			{
				return _("[Invalid encoding]");
			}
		}
	}
}



Subtitles::Subtitle::Subtitle(Time_ms time, const std::string& text)
:
	time(time), text(text)
{
}



#ifdef DEVELOP_MODE
	void Subtitles::Subtitle::dump(void) const
	{
		MLIB_D(_C("%1: '%2'", time, text));
	}
#endif



const Subtitles::Storage& Subtitles::get(void) const
{
	return this->subtitles;
}



void Subtitles::load(const std::string& file_path) throw(m::Exception)
{
	Time_ms time = 0;
	std::string text;
	Glib::ustring line;
	size_t line_num = 1;

	char buf[PIPE_BUF];
	enum { GET_SUBTITLE, GET_TIME, GET_TEXT } state = GET_SUBTITLE;

	Glib::RefPtr<Glib::Regex> tag_regex = Glib::Regex::create("</{0,1}[a-zA-Z]>");
	Glib::RefPtr<Glib::Regex> empty_line_regex = Glib::Regex::create("^\\s*$");
	Glib::RefPtr<Glib::Regex> id_regex = Glib::Regex::create("^\\s*\\d+\\s*$");
	Glib::RefPtr<Glib::Regex> time_regex = Glib::Regex::create(
		"^\\s*(\\d{1,2}):(\\d{1,2}):(\\d{1,2}),(\\d{1,3})\\s+-{1,2}>\\s+\\d{1,2}:\\d{1,2}:\\d{1,2},\\d{1,3}\\s*$"
	);


	this->subtitles.clear();

	// Открываем файл -->
		std::ifstream file;

		file.open(U2L(file_path).c_str(), std::ios::in);
		if(!file.is_open())
			M_THROW(EE(errno));

		file.exceptions(file.eofbit | file.failbit | file.badbit);
	// Открываем файл <--

	while(file.good())
	{
		// Считываем очередную строку из файла -->
		{
			size_t size = 0;

			try
			{
				char byte;
				bool work = true;

				while(work)
				{
					if(size + 1 >= sizeof buf)
						M_THROW(__("line %1 is too big", line_num));

					file.get(byte);

					switch(byte)
					{
						case '\r':
						{
							file.get(byte);

							if(byte != '\n')
								file.unget();
						}
						case '\n':
							line = smart_convert(std::string(buf, buf + size));
							work = false;
							break;

						default:
							buf[size++] = byte;
							break;
					}
				}
			}
			catch(m::Exception& e)
			{
				// Чтобы деструктор не сгенерировал исключение
				file.exceptions(file.goodbit);
				throw;
			}
			catch(std::ofstream::failure& e)
			{
				// Чтобы деструктор не сгенерировал исключение
				file.exceptions(file.goodbit);

				if(file.bad())
					M_THROW(EE(errno));
				else
				{
					if(state == GET_SUBTITLE || GET_TEXT)
						line = smart_convert(std::string(buf, buf + size));
					else
						M_THROW(__("unexpected end of file at line %1 ('%2')", line_num, line));
				}
			}
		}
		// Считываем очередную строку из файла <--

		// Парсим полученную строку -->
			switch(state)
			{
				case GET_SUBTITLE:
				{
					bool dont_break = false;

					if(!empty_line_regex->match(line))
					{
						if(id_regex->match(line))
							state = GET_TIME;
						// Иногда субтитры не имеют идентификатора
						else if(time_regex->match(line))
						{
							state = GET_TIME;
							dont_break = true;
						}
						else
							M_THROW(__("invalid line %1 ('%2')", line_num, line));
					}

					if(!dont_break)
						break;
				}

				case GET_TIME:
				{
					Glib::StringArrayHandle matches = time_regex->split(line);

					if(matches.size() < 5)
						M_THROW(__("invalid line %1 ('%2')", line_num, line));
					else
					{
						Time_ms hours = boost::lexical_cast<int>(matches.data()[1]);
						Time_ms minutes = boost::lexical_cast<int>(matches.data()[2]);
						Time_ms seconds = boost::lexical_cast<int>(matches.data()[3]);
						Time_ms mseconds = boost::lexical_cast<int>(matches.data()[4]);

						if(
							hours < 0 ||
							minutes < 0 || minutes > 59 ||
							seconds < 0 || seconds > 59 ||
							mseconds < 0 || mseconds > 999
						)
							M_THROW(__("invalid line %1 ('%2')", line_num, line));


						Time_ms gotten_time = ( (hours * 60 + minutes) * 60 + seconds ) * 1000 + mseconds;

						if(gotten_time < time)
						{
							MLIB_SW(__(
								"Gotten smaller time offset than previous at line %1 in subtitles file '%2'.",
								line_num, file_path
							));
						}
						else
							time = gotten_time;
					}

					state = GET_TEXT;
				}
				break;

				case GET_TEXT:
				{
					if(empty_line_regex->match(line))
					{
						if(!text.empty())
						{
							this->subtitles.push_back(Subtitle(time, text));
							text = "";
						}

						state = GET_SUBTITLE;
					}
					else
					{
						line = tag_regex->replace(line, 0, "", static_cast<Glib::RegexMatchFlags>(0));

						if(!text.empty())
							text += "\n";

						text += line;
					}
				}
				break;

				default:
					MLIB_LE();
					break;
			}
		// Парсим полученную строку <--

		line_num++;
	}

	if(this->subtitles.empty())
		M_THROW(_("there is no subtitles in this file"));
}



#ifdef DEVELOP_MODE
	void Subtitles::dump(void) const
	{
		std::for_each(
			this->subtitles.begin(), this->subtitles.end(),
			std::mem_fun_ref(&Subtitle::dump)
		);
	}
#endif

