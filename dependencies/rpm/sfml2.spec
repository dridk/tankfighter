%global debug_package %{nil}

Name:		SFML2
Version:	2.1
Release:	2%{?dist}
Summary:	SFML2

Group:		Application/System
License:	zlib
URL:		http://www.sfml-dev.org/
Source0:	SFML-%{version}-sources.zip

BuildRequires:	cmake
BuildRequires:	libsndfile-devel
BuildRequires:	gcc-c++
Requires:	libsndfile

%if 0%{?suse_version} > 0
BuildRequires:	libjpeg62-devel
Requires:	libjpeg62
BuildRequires:	openal-soft-devel
Requires:	libopenal1-soft
BuildRequires:	xorg-x11-devel
Requires:	xorg-x11
BuildRequires:	Mesa-devel
BuildRequires:	freetype2-devel
BuildRequires:	glew-devel
Requires:	glew
Requires:	Mesa
Requires:	freetype2
%else
Requires:	libjpeg-turbo
BuildRequires:	libjpeg-turbo-devel
BuildRequires:	openal-soft-devel
Requires:	openal-soft
BuildRequires:	libX11-devel
BuildRequires:	libXrandr-devel
Requires:	libX11
Requires:	libXrandr
BuildRequires:	mesa-libGL-devel
BuildRequires:	freetype-devel
BuildRequires:  glew-devel
Requires:	libGLEW
Requires:	mesa-libGL
Requires:	freetype
%endif

%description
SFML2

%prep
%setup -n SFML-%{version}
cd %{_builddir}/SFML-%{version}
mkdir shared
mkdir static

%build
cd shared
cmake .. -DCMAKE_INSTALL_PREFIX=/usr
make %{?_smp_mflags}
cd ../static
cmake .. -DCMAKE_INSTALL_PREFIX=/usr -DBUILD_SHARED_LIBS=false
make %{?_smp_mflags}

%install
cd shared
make install DESTDIR=%{buildroot}
cd ../static
make install DESTDIR=%{buildroot}

%files
%{_libdir}/libsfml-*.so.*
%doc %{_datadir}

%package devel
Summary:	SFML2 devel
Requires:	SFML2

%description devel
SFML2 devel


%files devel
%{_libdir}/pkgconfig
%{_libdir}/libsfml-*.so
%{_libdir}/libsfml-*.a
%{_includedir}

%changelog

