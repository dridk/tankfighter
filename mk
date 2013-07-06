g++ engine.cpp main.cpp json.c geometry.cpp -o test `pkg-config --cflags --libs sfml-all` -DSFML2
#g++ main.cpp json.c -o test -I/usr/include -lsfml-audio -lsfml-graphics -lsfml-network -lsfml-system -lsfml-window
