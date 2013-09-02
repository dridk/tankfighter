
version=2.1
arch=$(uname -m)
specname=sfml2.spec
origname=SFML-$(version)-sources.zip
pkgname=SFML2-$(version)-1.$(arch).rpm

rpmdir=$(shell rpmbuild --showrc |sed -n 's/[-0-9]*: *_topdir[[:space:]]*//p'|sed "s|%{getenv:HOME}|$$HOME|" )

main: $(pkgname)

SFML-$(version)-sources.zip:
	wget -O SFML-$(version)-sources.zip.part -c http://sfml-dev.org/download/sfml/$(version)/$(origname)
	mv SFML-$(version)-sources.zip.part $(origname)

$(pkgname): $(origname)
	cp -f "$(origname)" "$(rpmdir)/SOURCES"
	cp -f "$(specname)" "$(rpmdir)/SPECS"
	rpmbuild -ba "$(rpmdir)/SPECS/$(specname)"
	cp -f "$(rpmdir)/RPMS/$(arch)/$(pkgname)" .

clean:
	rm -f $(pkgname)

distclean: clean
	rm -f $(origname)
