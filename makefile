CC = g++
CC_OPTS = -std=c++11 -Werror --pedantic

BUILD_DIR = build

OBJS = socket.o util.o
TEST_OBJS = test.o $(OBJS)
CLNT_OBJS = client.o $(OBJS)

test.out client.out: make_build $(CLNT_OBJS) $(TEST_OBJS)
	$(CC) $(CC_OPTS) $(patsubst %.o, $(BUILD_DIR)/%.o, $(CLNT_OBJS)) -o client.out
	$(CC) $(CC_OPTS) $(patsubst %.o, $(BUILD_DIR)/%.o, $(TEST_OBJS)) -o test.out

client.o: client.cpp
	$(CC) $(CC_OPTS) -c client.cpp -o $(BUILD_DIR)/client.o

test.o: test.cpp
	$(CC) $(CC_OPTS) -c test.cpp -o $(BUILD_DIR)/test.o

socket.o: socket.hpp socket.cpp
	$(CC) $(CC_OPTS) -c socket.cpp -o $(BUILD_DIR)/socket.o

util.o: util.hpp util.cpp
	$(CC) $(CC_OPTS) -c util.cpp -o $(BUILD_DIR)/util.o

clean:
	rm -rf $(BUILD_DIR)/ *.out

make_build:
	mkdir -p build