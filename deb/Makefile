arch=amd64
MAJOR=0
MINOR=9
RELEASE=0
VER=$(MAJOR).$(MINOR).$(RELEASE)
sdir=tankfighter-$(VER)

main: tankfighter-$(VER)_$(arch).deb
$(sdir): tankfighter-$(VER).orig.tar.gz
	tar xf $<
	cp -rf debian $(sdir)
	sed "s/@MAJOR@/$(MAJOR)/g;s/@MINOR@/$(MINOR)/g;s/@RELEASE@/$(RELEASE)/g" debian/rules.in > "$(sdir)/debian/rules"
	sed "s/@MAJOR@/$(MAJOR)/g;s/@MINOR@/$(MINOR)/g;s/@RELEASE@/$(RELEASE)/g" debian/changelog.in > "$(sdir)/debian/changelog"

tankfighter-$(VER).orig.tar.gz:
	mkdir -p "tmp/$(sdir)"
	(cd ..;sh clean.bat;\
	find . \( -not -path './deb/*' -o -path './deb/debian/*' -o -path './deb/Makefile' \) -a -not -path './.git/*' \
	|pax -d -w)|tar x -C "tmp/$(sdir)"
	tar cfz tankfighter-$(VER).orig.tar.gz -C tmp "$(sdir)"
	rm -rf tmp

tankfighter_$(VER)_$(arch).deb: $(sdir)
	(cd $(sdir); debuild binary)

clean:
	rm -rf $(sdir)
	rm -f tankfighter-$(VER).orig.tar.gz
	rm -f tankfighter_$(VER)_amd64.deb
distclean: clean

