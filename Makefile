LIB=-L$(shell pwd)
INC=-I$(shell pwd)
all:
	g++ -std=c++0x MyDaemon.cpp MyMonitor.cpp MyLog.cpp main.cpp  -L./ -lpthread -o my_daemon 
clean:
	rm -f *.o  
