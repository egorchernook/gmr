
.PHONY: all stat

all:
	g++-12 -std=c++20 -O3 -Wall -Wextra -Wshadow -Wconversion -Wpedantic -Werror main.cpp -o main

stat:
	g++-12 -std=c++20 -O2 -Wall -Wextra -Wshadow -Wconversion -Wpedantic -Werror only_stat.cpp -o only_stat