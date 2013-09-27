arch=$(shell uname -p|sed 's/x86_64/amd64/')
MAJOR=0
MINOR=9
RELEASE=0
VER=$(MAJOR).$(MINOR).$(RELEASE)
sdir=tankfighter-$(VER)

main: tankfighter_$(VER)_$(arch).deb

unpack: tankfighter_$(VER).orig.tar.gz
	rm -rf tankfighter-$(VER)
	tar xf $<
	cp -rf debian $(sdir)
	sed "s/@MAJOR@/$(MAJOR)/g;s/@MINOR@/$(MINOR)/g;s/@RELEASE@/$(RELEASE)/g" debian/rules.in > "$(sdir)/debian/rules" && chmod a+x "$(sdir)/debian/rules"
	sed "s/@MAJOR@/$(MAJOR)/g;s/@MINOR@/$(MINOR)/g;s/@RELEASE@/$(RELEASE)/g" debian/changelog.in > "$(sdir)/debian/changelog"

tankfighter_$(VER).orig.tar.gz:
	(cd ..; sh clean.sh)
	which pax >/dev/null 2>&1 || sudo apt-get install pax
	mkdir -p "tmp/$(sdir)"
	(cd ..;sh clean.bat;\
	find . \( -not -path './deb/*' -o -path './deb/debian/*' -o -path './deb/Makefile' \) -a -not -path './.git/*' -a -not -path './dependencies/deb/SFML-2.1-sources.zip' \
	|pax -d -w)|tar x -C "tmp/$(sdir)"
	tar cfz tankfighter_$(VER).orig.tar.gz -C tmp "$(sdir)"
	rm -rf tmp

has_sfml:
	dpkg --status libsfml2-static >/dev/null 2>&1 \
	|| (cd ../dependencies/deb; make arch=$(arch) && sudo dpkg -i libsfml2-static*_$(arch).deb)

tankfighter_$(VER)_$(arch).deb tankfighter_$(VER).dsc: unpack has_sfml
	echo "Building package tankfighter_$(VER)_$(arch).deb"
	(cd $(sdir);dpkg-checkbuilddeps -B 2>/dev/null) || sudo apt-get install debhelper pkg-config cmake libsfml2-static libfontconfig1-dev mesa-common-dev libgl1-mesa-dev
	(cd $(sdir); debuild -a$(arch))

clean:
	rm -rf $(sdir)
	rm -f tankfighter_$(VER).orig.tar.gz
	rm -f tankfighter_$(VER)*.deb
	rm -f tankfighter_$(VER)*.debian.tar.gz
	rm -f tankfighter_$(VER)*.dsc
	rm -f tankfighter_$(VER)*.build
	rm -f tankfighter_$(VER)*.changes
distclean: clean

