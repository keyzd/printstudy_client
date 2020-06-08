CC = gcc
CXX = g++
CFLAGS = -Wall
LDFLAGS = -lfltk

PROGNAME_MAIN = ps_client
PROGNAME2 = cl_print
PROGNAME3 = cl_scan_login
PROGNAME4 = cl_scan_main

SRCMODULES1 = ps_client.cpp
OBJMODULES1 = $(SRCMODULES1:.cpp=.o)

SRCMODULES2 = cl_print.c
OBJMODULES2 = $(SRCMODULES2:.c=.o)

SRCMODULES3 = cl_scan_login.c
OBJMODULES3 = $(SRCMODULES3:.c=.o)

SRCMODULES4 = cl_scan_main.c
OBJMODULES4 = $(SRCMODULES4:.c=.o)

%.o: %.c
	$(CC) $(CFLAGS) $< -c -o $@

%.o: %.cpp
	$(CXX) $(CFLAGS) $< -c -o $@

ps_client: $(OBJMODULES1)
	$(CXX) $(CFLAGS) $(LDFLAGS) $^ -o $@

cl_print: $(OBJMODULES2)
	$(CC) $(CFLAGS) $^ -o $@

cl_scan_login: $(OBJMODULES3)
	$(CC) $(CFLAGS) $^ -o $@

cl_scan_main: $(OBJMODULES4)
	$(CC) $(CFLAGS) $^ -o $@

all: $(PROGNAME_MAIN) $(PROGNAME2) $(PROGNAME3) $(PROGNAME4)

clean:
	rm -f *.o $(PROGNAME_MAIN) $(PROGNAME2) $(PROGNAME3) $(PROGNAME4) 

