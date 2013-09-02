Summary: Multiplayer tank fighting game
Name: tankfighter
Version: 0.9.0
Release: 1%{?dist}
License: MIT
URL: http://gillibert.fr/
Source: tankfighter-%{version}.tar.gz
Group: Game

Buildroot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
BuildRequires: pkgconfig
BuildRequires: SFML2-devel >= 2.0
Requires: SFML2 >= 2.0
BuildRequires: fontconfig-devel
Requires: fontconfig

#%if 0%{?suse_version} > 0
#BuildRequires: glew-devel
#Requires: Mesa
#Requires: freetype2
#%else
#BuildRequires: libGLEW-devel
#Requires: mesa-libGL
#Requires: freetype
#%endif

# Provide lowercase name to help people find the package. 
Provides: tankfighter = %{version}-%{release}

%description
Tankfighter is an open source multiplayer real time video game. It can be played in hot seat mode or on network.

%prep
%setup -q
%build
cmake . -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
make %{?_smp_mflags}

%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,-)
%{_bindir}/*
%{_datadir}/tankfighter-%{version}/*

%changelog
* Wed Aug  28 2013 André Gillibert <MetaEntropy@gmail.com> - 0.9.0
- Initial release.
