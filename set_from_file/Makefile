COMPILER = g++
FLAGS = -std=c++14 -O3 -Wall -Werror -Wextra

SO_DEPS = $(shell pkg-config --libs --cflags libSimpleAmqpClient msgpack librabbitmq)
SO_DEPS += -lboost_program_options -lpthread -lyaml-cpp

all: set_from_file

clean:
	rm -f set_from_file
	
set_from_file: set_from_file.cpp
	$(COMPILER) $^ -o $@ $(FLAGS) $(SO_DEPS)