Summary: Kent's Emulated GS; Apple //GS Emulator
Name: kegs
%define version 0.71
Version: %{version}
Release: 1
Group: Applications/Emulators
Source: http://kegs.sourceforge.net/kegs.%{version}.tar.gz
URL: http://kegs.sourceforge.net/kegs/
Packager: Jonathan Stark <kegs@starks.org>
Copyright: GPL
BuildRoot: /tmp/kegs
Provides: kegs

%description

KEGS is an Apple //gs emulator for X windows, but optimized for HP
workstations.  KEGS is now portable to other platforms--Linux on
Intel x86, Linux on PowerPC machines, Solaris, and OS/2.

KEGS uses X-Windows for its display (you can send the display to another
machine) and emulates all Apple //gs sounds accurately.  It supports
limited serial port emulation through sockets.  Audio works for some
people under Linux.

The ROMs and GS/OS (the Apple //gs operating system) are not included
with KEGS since they are not freely distributable.  KEGS is a little
user-hostile now, so if something doesn't work, let me know what went
wrong, and I'll try to help you out.

Kent Dickey
kentd@cup.hp.com
%prep
%setup -n kegs.%{version}
cd src; rm -f vars; ln -s vars_x86linux vars
%build
cd src
make
make partls
make to_pro

%install
rm -rf $RPM_BUILD_ROOT
install -d $RPM_BUILD_ROOT/usr/bin
install -d $RPM_BUILD_ROOT/usr/doc

install src/partls src/to_pro kegs $RPM_BUILD_ROOT/usr/bin
#install CHANGES BUGS Copyright README TODO $RPM_BUILD_ROOT/usr/doc

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%doc CHANGES README.kegs README.linux.partitions INTERNALS.overview INTERNALS.iwm INTERNALS.xdriver README.linux.rpm kegs_conf font.65sim README.linux.rpm
/usr/bin/to_pro
/usr/bin/partls
/usr/bin/kegs

%changelog
* Wed Nov 20 2002 Jonathan Stark <kegs@starks.org>
- v0.71 packaged
* Wed Dec 29 1999 Jonathan Stark <stark@starks.org>
- v0.57 packaged
* Mon Nov  1 1999 Jonathan Stark <stark@starks.org>
- v0.56 packaged
* Tue Oct 19 1999 Jonathan Stark <stark@starks.org>
- v0.55 packaged
* Sun Oct 17 1999 Jonathan Stark <stark@starks.org>
- packaged for RedHat 6.1

