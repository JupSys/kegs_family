
OBJECTS = adb.o clock.o dis.o engine_s.o scc.o iwm.o moremem.o sim65816.o smartport.o sound.o sound_hp.o video.o xdriver.o
CC = cc -Ae -C
CCOPTS = -O +ESsfc +ESlit
OPTS =  -DNDEBUG
SUFFIX =
CPPOPTS = -DNDEBUG -I.
NAME = kegs
LDFLAGS =
LDOPTS = -z
LD = cc -Ae -C
EXTRA_LIBS = -lc
PROJROOT = .

AS = cc -Ae

XOPTS = -I/usr/include/X11R5
XLIBS = -L/usr/lib/X11R5

all: $(NAME)$(SUFFIX) partls to_pro

specials: 8inst_s 16inst_s 8size 16size 8inst_c 16inst_c size_c Alib.h

specials_clean:
	rm -f 8inst_s 16inst_s 8size 16size 8inst_c 16inst_c size_c Alib.h


$(NAME)$(SUFFIX): $(OBJECTS) compile_time.o
	$(LD) -z $(LDOPTS) $(OBJECTS) compile_time.o $(LDFLAGS) -o $(NAME)$(SUFFIX) $(XLIBS) $(EXTRA_LIBS) -lXext -lX11 -lAlib -lcl -lm

8inst_s: instable.h
	$(PROJROOT)/make_inst s 8 instable.h > 8inst_s

16inst_s: instable.h
	$(PROJROOT)/make_inst s 16 instable.h > 16inst_s

8inst_c: instable.h
	$(PROJROOT)/make_inst c 8 instable.h > 8inst_c

16inst_c: instable.h
	$(PROJROOT)/make_inst c 16 instable.h > 16inst_c

8size: size_tab.h
	sed -e 's/_SYM/_8/' size_tab.h > 8size

16size: size_tab.h
	sed -e 's/_SYM/_16/' size_tab.h > 16size

size_c: size_tab.h
	$(PROJROOT)/make_size c size_tab.h > size_c

Alib.h:
	$(PROJROOT)/make_alib


engine_s.o: 8inst_s 8size 16inst_s 16size

engine_c.o: 8inst_c 16inst_c

.s.o:
	$(AS) -c $(OPTS) -I. $*.s

.c.o:
	$(CC) $(CCOPTS) $(XOPTS) -c $(OPTS) -I. $*.c

.C.o:
	$(CCC) $(CCCOPTS) $(XOPTS) -c $(OPTS) -I. $*.C

partls: partls.c
	$(CC) $(CCOPTS) -o partls partls.c

to_pro: prodos.h prodos_protos.h to_pro.c
	$(CC) $(CCOPTS) -o to_pro to_pro.c

compile_time.o: $(OBJECTS)

