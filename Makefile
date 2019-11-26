CC = g++
CXX = g++

INCS = -I.
CXXFLAGS = -Wall -std=c++11 -g $(INCS)
CXXFLAGS = -Wall -std=c++11 -g $(INCS)

OBJ = *.o
IOH = whatsappio.h
IOCPP = whatsappio.cpp
IOSRC = whatsappio.cpp whatsappio.h
IOOBJ = whatsappio.o
SERVERSRC = whatsappServer.cpp
SERVEROBJ = whatsappServer.o
CLIENTSRC = whatsappClient.cpp
CLIENTOBJ = whatsappClient.o

SERVEREXE = whatsappServer
CLIENTEXE = whatsappClient
TARGETS = $(SERVEREXE) $(CLIENTEXE)

TAR = tar
TARFLAGS = -cvf
TARNAME = ex4.tar
TARSRCS = $(IOSRC) $(SERVERSRC) $(CLIENTSRC) Makefile README

all: $(TARGETS)

$(SERVEREXE): $(SERVEROBJ) $(IOOBJ)
	$(CC) $(SERVEROBJ) $(IOOBJ) -o $(SERVEREXE)
	
$(CLIENTEXE): $(CLIENTOBJ) $(IOOBJ)
	$(CC) $(CLIENTOBJ) $(IOOBJ) -o $(CLIENTEXE)

$(IOOBJ): $(IOSRC)
	$(CC) $(CXXFLAGS) -c $(IOCPP) -o $(IOOBJ)
	
$(SERVEROBJ): $(IOH) $(SERVERSRC)
	$(CC) $(CXXFLAGS) -c $(SERVERSRC) -o $(SERVEROBJ)
	
$(CLIENTOBJ): $(IOH) $(CLIENTSRC)
	$(CC) $(CXXFLAGS) -c $(CLIENTSRC) -o $(CLIENTOBJ)

clean:
	$(RM) $(OBJ) $(TARGETS) *~ *core

tar:
	$(TAR) $(TARFLAGS) $(TARNAME) $(TARSRCS)
