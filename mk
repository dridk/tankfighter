g++ -O2 -Wall engine_event.cpp controller.cpp keybmouse_controller.cpp player.cpp texture_cache.cpp entity.cpp wall.cpp load_map.cpp engine.cpp misc.cpp main.cpp json.c geometry.cpp -o test `pkg-config --cflags --libs sfml-all` -DSFML2
#g++ main.cpp json.c -o test -I/usr/include -lsfml-audio -lsfml-graphics -lsfml-network -lsfml-system -lsfml-window
