
version=2.1
arch=$(shell uname -p|sed 's/x86_64/amd64/')
sdir=libsfml2-$(version)
devarch=:$(arch)

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
	(cd libsfml2-2.1/;dpkg-checkbuilddeps -B 2>/dev/null) || sudo apt-get install \
debhelper pkg-config cmake unzip devscripts
#libx11$(devarch) libxrandr$(devarch) libjpeg$(devarch) libopenal$(devarch) \
libsndfile1$(devarch) libfreetype6$(devarch) libfontconfig1$(devarch) \
mesa-common$(devarch) libgl1-mesa$(devarch) libglew1.6$(devarch)

	(cd $(sdir); debuild -a$(arch))
clean:
	rm -f $(sdir)/SFML-$(version)-sources.zip
	rm -f libsfml2_$(version).orig.tar.gz
	rm -f libsfml2-static_$(version)*.deb
	rm -f libsfml2_$(version)*.dsc
	rm -f libsfml2_$(version)*.debian.tar.gz
	(cd $(sdir); debuild clean)
	rm -f libsfml2_$(version)*.build
	rm -f libsfml2_$(version)*.changes

distclean: clean
	rm -f SFML-$(version)-sources.zip || true
