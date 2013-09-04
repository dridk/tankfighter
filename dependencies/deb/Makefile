
version=2.1
arch=$(shell uname -p|sed 's/x86_64/amd64/')
sdir=libsfml2-$(version)

main: libsfml2-static_$(version)_$(arch).deb

SFML-$(version)-sources.zip:
	wget -O SFML-$(version)-sources.zip.part -c http://sfml-dev.org/download/sfml/2.1/SFML-2.1-sources.zip
	mv SFML-$(version)-sources.zip.part SFML-2.1-sources.zip
libsfml2_$(version).orig.tar.gz: SFML-$(version)-sources.zip
	tar cfz $@ $<
$(sdir)/SFML-$(version)-sources.zip: SFML-$(version)-sources.zip
	cp -f $< $@

libsfml2-static_$(version)_$(arch).deb libsfml2_$(version).dsc: $(sdir)/SFML-$(version)-sources.zip libsfml2_$(version).orig.tar.gz
	echo "Building package libsfml2-static_$(version)_$(arch).deb"
	(cd libsfml2-2.1/;dpkg-checkbuilddeps -B 2>/dev/null) || sudo apt-get install debhelper pkg-config cmake unzip libx11-dev libxrandr-dev, libjpeg-dev libopenal-dev libsndfile1-dev libfreetype6-dev libfontconfig1-dev mesa-common-dev libgl1-mesa-dev libglew1.6-dev

	(cd $(sdir); debuild)
clean:
	rm -f $(sdir)/SFML-$(version)-sources.zip
	rm -f libsfml2_$(version).orig.tar.gz
	rm -f libsfml2-static_$(version)_*.deb
	rm -f libsfml2_$(version).dsc
	rm -f libsfml2_$(version).debian.tar.gz
	(cd $(sdir); debuild clean)
	rm -f libsfml2_$(version)_*.build
	rm -f libsfml2_$(version)_*.changes

distclean: clean
	rm -f SFML-$(version)-sources.zip || true
