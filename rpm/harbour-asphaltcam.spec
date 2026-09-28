Name:       harbour-asphaltcam

Summary:    Looping dash cam with crash lock
Version:    1.0
Release:    1
License:    MIT
URL:        https://github.com/coding-on-slate/asphaltcam
Source0:    %{name}-%{version}.tar.bz2
Requires:   sailfishsilica-qt5
Requires:   qt5-qtpositioning
Requires:   qt5-qtmultimedia
Requires:   nemo-qml-plugin-configuration-qt5
Requires:   nemo-qml-plugin-dbus-qt5
Requires:   qt5-qtdeclarative-import-sensors
Requires:   libkeepalive
BuildRequires:  pkgconfig(sailfishapp) >= 1.0.2
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5Qml)
BuildRequires:  pkgconfig(Qt5Quick)
BuildRequires:  pkgconfig(Qt5Positioning)
BuildRequires:  pkgconfig(Qt5Multimedia)
BuildRequires:  pkgconfig(mlite5)
BuildRequires:  desktop-file-utils
BuildRequires:  cmake

%description
AsphaltCam records a looping dash-cam video, locks clips on a crash or
manual save, and shows GPS, speed, and a timestamp overlay during playback.

%prep
%setup -q -n %{name}-%{version}

%build
# Drop in-source CMake state left by a previous architecture.
rm -rf CMakeCache.txt CMakeFiles CMakeTmp Makefile cmake_install.cmake \
       harbour-asphaltcam_autogen asphaltcam_autogen QtCreatorDeployment.txt

%cmake

%make_build

%install
%make_install


desktop-file-install --delete-original       \
  --dir %{buildroot}%{_datadir}/applications             \
   %{buildroot}%{_datadir}/applications/*.desktop

%files
%defattr(-,root,root,-)
%{_bindir}/%{name}
%{_datadir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/hicolor/*/apps/%{name}.png

