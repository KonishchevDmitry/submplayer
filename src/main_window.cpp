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


#include <pango/pango-font.h>
#include <pango/pango-utils.h>

#include <gtkmm/box.h>
#include <gtkmm/main.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/textview.h>

#include <mlib/gtk/window_settings.hpp>

#include "main_window.hpp"
#include "mplayer.hpp"
#include "subtitles.hpp"



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
	Mplayer							mplayer;
	std::vector<Subtitles_control*>	controls;
	sigc::connection				time_offset_changed_connection;
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

		this->signal_delete_event().connect(
			sigc::mem_fun(*this, &Main_window::on_delete_cb)
		);

		priv->mplayer.connect_quit_handler(
			sigc::mem_fun(*this, &Main_window::on_player_closed_cb)
		);

		try
		{
			priv->mplayer.start(mplayer_args);
		}
		catch(m::Exception& e)
		{
			MLIB_W(EE(e));
		}

		this->show_all();
	}



	bool Main_window::on_delete_cb(GdkEventAny* event)
	{
		priv->time_offset_changed_connection.disconnect();
		this->hide();
		return false;
	}



	void Main_window::on_player_closed_cb(void)
	{
		MLIB_D("Player closed.");
		Gtk::Main::quit();
	}



	void Main_window::on_time_offset_changed_cb(void)
	{
		MLIB_D("Current time offset has been changed.");

		Time_ms time = priv->mplayer.get_current_offset();

		M_FOR_IT(priv->controls, it)
			(*it)->scroll_to(time);
	}
// Main_window <--

