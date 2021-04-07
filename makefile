CC = g++
CC_OPTS = -std=c++11 -Werror --pedantic

BUILD_DIR = build

OBJS = socket.o util.o jute.o
SRVR_OBJS = server.o $(OBJS)
CLNT_OBJS = client.o $(OBJS)

server.out client.out: make_build $(CLNT_OBJS) $(SRVR_OBJS)
	$(CC) $(CC_OPTS) $(patsubst %.o, $(BUILD_DIR)/%.o, $(CLNT_OBJS)) -o client.out
	$(CC) $(CC_OPTS) $(patsubst %.o, $(BUILD_DIR)/%.o, $(SRVR_OBJS)) -o server.out

client.o: client.cpp
	$(CC) $(CC_OPTS) -c client.cpp -o $(BUILD_DIR)/client.o

server.o: server.cpp
	$(CC) $(CC_OPTS) -c server.cpp -o $(BUILD_DIR)/server.o

socket.o: socket.hpp socket.cpp
	$(CC) $(CC_OPTS) -c socket.cpp -o $(BUILD_DIR)/socket.o

util.o: util.hpp util.cpp
	$(CC) $(CC_OPTS) -c util.cpp -o $(BUILD_DIR)/util.o

jute.o: jute.h jute.cpp
	$(CC) $(CC_OPTS) -c jute.cpp -o $(BUILD_DIR)/jute.o

clean:
	rm -rf $(BUILD_DIR)/ *.out

make_build:
	mkdir -p build