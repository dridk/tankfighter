#!/bin/sh

out="$1"

if [ -z "$out" ]; then
	out="keys.cpp"
fi

extract_enum() {
	sed -n '/ *enum Key/,/};/p'|sed -n '/,/p'|sed 's/ *\([^, =]*\)[, =].*/\1/'|(cat keys.cpp.head;sed 's/.*/{"Key&", Keyboard::&},/';cat keys.cpp.tail)
}
find_keyboard_hpp() {
(pkg-config --cflags sfml-all;echo /usr/include)|awk -v RS=' ' 1|sed 's/^-I//'|while read d; do
	f="$d/SFML/Window/Keyboard.hpp"
	if test -e "$f"; then
		echo "$f"
	fi
done|tail -1
}

kbh=$(find_keyboard_hpp)
if [ -n "$kbh" ]; then
	cat "$kbh"|extract_enum > "$out"
	exit 0
else
	echo "SFML keyboard file not found" >&2
	exit 1
fi
