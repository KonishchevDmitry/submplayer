SUBDIRS = mlib

bin_PROGRAMS = submplayer

submplayer_SOURCES = \
	common.hpp \
	main.cpp \
	main_window.cpp \
	main_window.hpp \
	mplayer.cpp \
	mplayer.hpp \
	subtitles.cpp \
	subtitles.hpp

submplayer_DEPENDENCIES = @APP_DEPENDENCIES@
submplayer_CPPFLAGS = @APP_CPPFLAGS@ -D APP_LOCALE_PATH='"$(localedir)"'
submplayer_LDADD = @APP_LDADD@

