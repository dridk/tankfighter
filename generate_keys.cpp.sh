#!/bin/sh

out="$1"

if [ -z "$out" ]; then
	out="keys.cpp"
fi

extract_enum() {
	echo "NamedKey $3[]={"
	sed -n "/ *enum $1/,/};/p"|sed '1d'|sed 's/ *\([^, =]*\)[, =].*/\1/'|sed '/{\|}\|^[[:space:]]*$/d'|sed '/^\(Key\|Button\)Count$/d'|sed "s/.*/{\"&\", $2::&},/"
	echo "{NULL, 0}"
	echo "};"
}
find_keyboard_hpp() {
(pkg-config --cflags sfml-all;echo /usr/include)|awk -v RS=' ' 1|sed 's/^-I//'|while read d; do
	f="$d/SFML/Window/Keyboard.hpp"
	if test -e "$f"; then
		echo "$d/SFML/Window"
	fi
done|tail -1
}

kbh=$(find_keyboard_hpp)
if [ -n "$kbh" ]; then
	exec > "$out"
	cat keys.cpp.head
	cat "$kbh/Keyboard.hpp"|extract_enum "Key" "Keyboard" "key_codemap"
	cat "$kbh/Mouse.hpp"|extract_enum "Button" "Mouse" "mouse_codemap"
	cat "$kbh/Joystick.hpp"|extract_enum "Axis" "Joystick" "joyaxis_codemap"
	cat keys.cpp.tail
	exit 0
else
	echo "SFML keyboard file not found" >&2
	exit 1
fi
