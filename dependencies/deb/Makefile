
version=2.1
arch=amd64
sdir=libsfml2-$(version)

main: libsfml2-static_$(version)_$(arch).deb

SFML-$(version)-sources.zip:
	wget -O SFML-$(version)-sources.zip.part -c http://sfml-dev.org/download/sfml/2.1/SFML-2.1-sources.zip
	mv SFML-$(version)-sources.zip.part SFML-2.1-sources.zip
libsfml2_$(version).orig.tar.gz: SFML-$(version)-sources.zip
	tar cfz $@ $<
$(sdir)/SFML-$(version)-sources.zip: SFML-$(version)-sources.zip
	cp -f $< $@

libsfml2-static_$(version)_$(arch).deb: $(sdir)/SFML-$(version)-sources.zip
	(cd $(sdir); debuild binary)
clean:
	rm -f $(sdir)/SFML-$(version)-sources.zip
	rm -f libsfml2_$(version).orig.tar.gz
	rm -f libsfml2-static_$(version)_$(arch).deb
	(cd $(sdir); debuild clean)
	rm -f libsfml2_$(version)_$(arch).build

distclean: clean
	rm -f SFML-$(version)-sources.zip || true
