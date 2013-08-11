sh ./generate_keys.cpp.sh keys.cpp "C:\Program Files\Straw\c\include\SFML\Window"
g++ -Os -o tankfighter -Wall parse_json.cpp fc.cpp messages.cpp input.cpp menu.cpp parameters.cpp network_controller.cpp commands.cpp joystick_controller.cpp keys.cpp missile.cpp engine_event.cpp controller.cpp keybmouse_controller.cpp player.cpp texture_cache.cpp entity.cpp wall.cpp load_map.cpp engine.cpp misc.cpp main.cpp json.c geometry.cpp -lsfml-audio-s -lsfml-network-s -lsfml-graphics-s -lsfml-window-s -lsfml-system-s -DSFML_STATIC
