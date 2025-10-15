#
# FILE        : Makefile
# SUBJECT     : Makefile for the MailFlux project.
# AUTHOR      : (C) Copyright 2025 by Peter Chapin
#

THREAD_FLAGS = -pthread
CURSES_LIB   = -lncurses

CPPFLAGS=-Wall -g -DDEBUG -std=c++20 $(THREAD_FLAGS)
OBJS = MailFlux.o         \
	ClientConnection.o \
	config.o           \
	Console.o          \
	Message.o          \
	ServerConnection.o \
	Spool.o            \
	support.o

all:		MailFlux

MailFlux:	$(OBJS)
	g++ -g $(THREAD_FLAGS) -o MailFlux $(OBJS) $(CURSES_LIB)

MailFlux.o:	MailFlux.cpp Console.hpp ServerConnection.hpp

ClientConnection.o: ClientConnection.cpp \
		ClientConnection.hpp \
		Message.hpp \
		istring.hpp

config.o:	config.cpp config.hpp

Console.o:	Console.cpp Console.hpp

Message.o:	Message.cpp Message.hpp istring.hpp

ServerConnection.o:	ServerConnection.cpp \
		ServerConnection.hpp \
		Console.hpp \
		Message.hpp \
		istring.hpp \
		Spool.hpp

Spool.o:	Spool.cpp \
		Spool.hpp \
		ClientConnection.hpp \
		config.hpp \
		Console.hpp \
		Message.hpp

support.o:	support.cpp support.hpp

#
# Various items.
#

clean:
	rm -f MailFlux *.o core *~

docs:
	doxygen
