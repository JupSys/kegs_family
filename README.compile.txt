# $Id: README.compile.txt,v 1.22 2020/12/11 22:37:52 kentd Exp $

General build instructions:
--------------------------

KEGS only supports Mac OS X and Linux compiles for now.  Win32 support will
be added back at a future date.

You need to build with a make utility.  There's a default Makefile, which
should work for nearly any environment.  The Makefile includes a file called
"vars" which defines the platform-dependent variables.  You need to make
vars point to the appropriate file for your machine.

Mac OS X  build instructions (the default):
------------------------------------------

KEGS is easy to compile, but you must install XCode first.  Go to the Apple
App store, select Xcode, and install it.  You then must execute the Xcode.app
and agree to terms.  It will want to download additional components, you
need those, too.

Then, cd to the src directory of the KEGS release and type "make -j 20".
KEGS requires perl to be in your path (or just edit the vars file to give
the full path to wherever you installed perl).  Perl version 4 or 5 is
fine.

After the "make" has finished, it will create the application KEGSMAC.

To run, see README.mac.

Linux build instructions:
------------------------

Use the vars_x86linux file with:

rm vars; ln -s vars_x86linux vars
make -j 20

The resulting executable is called "xkegs".

The build scripts assume perl is in your path. If it is somewhere else,
you need to edit the "PERL = perl" line in the Makefile it point
to the correct place.

KEGS by default requires Pulse Audio to compile.  To compile to use /dev/dsp
(not really supported by any current Linux as far as I know), edit
vars to remove pulseaudio_driver.o, the -lpulse of EXTRA_LIBS, and
remove -DPULSE_AUDIO from CCOPTS.


Mac X11 build instructions:
----------------------------

Use the vars_mac_x vars file by:

rm vars; ln -s vars_mac_x vars
make -j 20

The build assumes you've installed XQuartz and run it to initialize itself.

The executable is called xkegs.  KEGS generally does the fastest display
updates in X11 mode--but requires Xshm.  Mac misconfigures the shared memory
limits to be too small to be used as Xshm memory (4MB limit, we need 40MB for
a retina display), so do:

sudo sysctl  kern.sysv.shmmax=67108864          # One segment max: 64MB
sudo sysctl  kern.sysv.shmall=131072            # in 4KB pages Total mem: 512MB


Other platform "C" build instructions:
-------------------------------------

If you are porting to an X-windows and Unix-based machine, it should be
easy.  Start with vars_x86linux.  Remove things causing problems, add things
as needed.  Good luck!

