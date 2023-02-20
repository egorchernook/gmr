all:
	g++-12 -std=c++20 -O3 -Wall -Wextra -Wshadow -Wconversion -Wpedantic -Werror main.cpp -o main

stat:
	g++-12 -std=c++20 -O3 -Wall -Wextra -Wshadow -Wconversion -Wpedantic -Werror stat.cpp -o stat