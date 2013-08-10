#!/bin/sh
./generate_keys.cpp.sh
g++ -Os -Wall -o tankfighter *.cpp *.c `pkg-config --cflags --libs sfml-all` -DSFML2
