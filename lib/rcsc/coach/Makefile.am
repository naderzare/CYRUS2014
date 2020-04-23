## Process this file with automake to produce Makefile.in

noinst_LTLIBRARIES = librcsc_coach.la

librcsc_coach_la_SOURCES = \
	coach_agent.cpp \
	coach_audio_sensor.cpp \
	coach_command.cpp \
	coach_config.cpp \
	coach_debug_client.cpp \
	global_object.cpp \
	global_visual_sensor.cpp \
	global_world_model.cpp \
	player_type_analyzer.cpp

librcsc_coachincludedir = $(includedir)/rcsc/coach

librcsc_coachinclude_HEADERS = \
	coach_agent.h \
	coach_audio_sensor.h \
	coach_command.h \
	coach_config.h \
	coach_debug_client.h \
	global_object.h \
	global_visual_sensor.h \
	global_world_model.h \
	player_type_analyzer.h

AM_CPPFLAGS = -I$(top_srcdir)
AM_CFLAGS = -Wall -W
AM_CXXFLAGS = -Wall -W
AM_LDLAGS =

CLEANFILES = *~
