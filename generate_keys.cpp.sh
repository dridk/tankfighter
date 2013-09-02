#!/bin/sh

srcdir="$1"
out="$2"
kbh="$3"

if [ -z "$out" ]; then
	out="keys.cpp"
fi

extract_enum() {
	echo "NamedKey $3[]={"
	sed -n "/ *enum $1/,/};/p"|sed '1d'|sed 's/ *\([^, =]*\)[, =].*/\1/'|sed '/{\|}\|^[[:space:]]*$/d'|sed '/^\(Key\|Button\)Count$/d'|sed "s/.*/{\"&\", (unsigned short)$2::&},/"
	echo "{NULL, 0}"
	echo "};"
}
find_incl() {
local libname="$1"
local headname="$2"
for d in $(pkg-config --cflags "$libname" 2>/dev/null;echo ${SFML_ROOT}/include/;echo ${SFML_ROOT};echo /usr/local/include;echo /usr/local/include/SFML2;echo /usr/include/SFML2;echo /usr/include); do
	d=$(echo "$d"|sed 's/^-I//')
	f="$d/$headname"
	if test -e "$f"; then
		echo "$d/SFML/Window"
	fi
done|tail -1
}
find_keyboard_hpp() {
	path=$(find_incl "sfml-all" "SFML/Window/Keyboard.hpp")
	if [ -z "$path" ]; then
		path16=$(find_incl "sfml-1.6" "SFML/Window/WindowListener.hpp")
		if [ -n "$path16" ]; then
			echo "SFML 1.6 headers found, but SFML >= 2.0 headers required" >&2
		fi
	fi
	echo "$path"
}

if [ -z "$kbh" ]; then
        kbh=$(find_keyboard_hpp)
fi

if [ -n "$kbh" ]; then
	exec > "$out"
	cat "$srcdir/keys.cpp.head"
	cat "$kbh/Keyboard.hpp"|extract_enum "Key" "Keyboard" "key_codemap"
	cat "$kbh/Mouse.hpp"|extract_enum "Button" "Mouse" "mouse_codemap"
	cat "$kbh/Joystick.hpp"|extract_enum "Axis" "Joystick" "joyaxis_codemap"
	cat "$srcdir/keys.cpp.tail"
	exit 0
else
	echo "SFML 2.0 SFML/Window/Keyboard.hpp file not found. If you find it, set \${SFML_ROOT} so that \${SFML_ROOT}/SFML/Window/Keyboard.hpp exists" >&2
	exit 1
fi
