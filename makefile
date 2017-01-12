all:s.out

s.out:server.o service.o user.o #command.o
	g++ -o s.out server.o service.o user.o #command.o

server.o:server.cpp service.h
	g++ -I. -c server.cpp

service.o:service.cpp service.h
	g++ -I. -c service.cpp

user.o: user.cpp user.h
	g++ -I. -c user.cpp

#command.o:command.cpp command.h
#	g++ -I. -c command.cpp

clean:
	rm s.out
	rm server.o
	rm service.o
	rm user.o
#	rm command.o
