
# KEGS Makefile--set all options in the "vars" file!

OBJECTS1 = adb.o clock.o dis.o joystick_driver.o paddles.o scc.o iwm.o moremem.o sim65816.o smartport.o sound.o video.o xdriver.o sound_driver.o
PROJROOT = .

include vars

all: specials $(NAME)$(SUFFIX) partls to_pro dc2raw

clean: specials_clean
	rm -f *.o kegs

specials: $(EXTRA_SPECIALS) 8inst_c 16inst_c size_c

specials_clean:
	rm -f $(EXTRA_SPECIALS) 8inst_c 16inst_c size_c size_s Alib.h


$(NAME)$(SUFFIX): $(OBJECTS) compile_time.o specials
	$(LD) $(LDOPTS) $(OBJECTS) compile_time.o $(LDFLAGS) -o $(NAME)$(SUFFIX) $(XLIBS) $(EXTRA_LIBS) -lm

8inst_s: instable.h
	$(PERL) $(PROJROOT)/make_inst s 8 instable.h > 8inst_s

16inst_s: instable.h
	$(PERL) $(PROJROOT)/make_inst s 16 instable.h > 16inst_s

8inst_c: instable.h
	$(PERL) $(PROJROOT)/make_inst c 8 instable.h > 8inst_c

16inst_c: instable.h
	$(PERL) $(PROJROOT)/make_inst c 16 instable.h > 16inst_c

8size: size_tab.h
	$(PERL) $(PROJROOT)/make_size 8 size_tab.h > 8size

16size: size_tab.h
	$(PERL) $(PROJROOT)/make_size 16 size_tab.h > 16size

size_c: size_tab.h
	$(PERL) $(PROJROOT)/make_size c size_tab.h > size_c

size_s: size_tab.h
	$(PERL) $(PROJROOT)/make_size s size_tab.h > size_s

Alib.h:
	rm -f Alib.h
	$(PERL) $(PROJROOT)/make_alib

engine_s.o: 8inst_s 8size 16inst_s 16size

engine_c.o: 8inst_c 16inst_c

.s.o:
	$(AS) -c $(OPTS) -I. $*.s

.c.o:
	$(CC) $(CCOPTS) $(XOPTS) -c $(OPTS) -I. $*.c

partls: partls.c
	$(CC) $(CCOPTS) $(OPTS) -o partls$(SUFFIX) partls.c

dc2raw: dc2raw.c
	$(CC) $(CCOPTS) $(OPTS) -o dc2raw$(SUFFIX) dc2raw.c

to_pro: prodos.h prodos_protos.h to_pro.c
	$(CC) $(CCOPTS) $(OPTS) -o to_pro$(SUFFIX) to_pro.c

compile_time.o: $(OBJECTS)

# DO NOT DELETE

adb.o: adb.h defc.h defcomm.h iwm.h protos.h
clock.o: defc.h defcomm.h iwm.h protos.h 
dis.o:   defc.h defcomm.h iwm.h protos.h disas.h
scc.o: defc.h defcomm.h iwm.h protos.h scc_driver.h
iwm.o: defc.h defcomm.h iwm.h protos.h iwm_35_525.h
joystick_driver.o: defc.h defcomm.h iwm.h protos.h
moremem.o: defc.h defcomm.h iwm.h protos.h
paddles.o: defc.h defcomm.h iwm.h protos.h
sim65816.o:   defc.h defcomm.h iwm.h protos.h
smartport.o: defc.h defcomm.h iwm.h protos.h
sound.o: defc.h defcomm.h iwm.h protos.h sound.h 
sound_driver.o: defc.h defcomm.h iwm.h protos.h sound.h
video.o:   defc.h defcomm.h iwm.h protos.h superhires.h
xdriver.o:  defc.h defcomm.h iwm.h protos.h protos_xdriver.h
engine_s.o: defs.h defcomm.h op_routs.h defs_instr.h 8inst_s 16inst_s 8size
engine_s.o: 16size size_s
engine_c.o: defc.h defcomm.h op_routs.h defs_instr.h 8inst_c 16inst_c size_c
engine_c.o: protos.h iwm.h protos_engine_c.h
