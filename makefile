CC = g++
CC_OPTS = -std=c++11 -Werror --pedantic

BUILD_DIR = build

TEST_OBJS = test.o socket.o util.o

test.out: make_build $(TEST_OBJS)
	$(CC) $(CC_OPTS) $(patsubst %.o, $(BUILD_DIR)/%.o, $(TEST_OBJS)) -o test.out

test.o: test.cpp
	$(CC) $(CC_OPTS) -c test.cpp -o $(BUILD_DIR)/test.o

socket.o: socket.hpp socket.cpp
	$(CC) $(CC_OPTS) -c socket.cpp -o $(BUILD_DIR)/socket.o

util.o: util.hpp util.cpp
	$(CC) $(CC_OPTS) -c util.cpp -o $(BUILD_DIR)/util.o

clean:
	rm -f $(BUILD_DIR)/*.o *.out
	rm -rf $(BUILD_DIR)/

make_build:
	mkdir -p build