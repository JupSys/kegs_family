#ifndef KEGS_DIS_H
#define KEGS_DIS_H

extern int g_num_breakpoints;
extern word32 g_breakpts[MAX_BREAK_POINTS];
extern int g_stepping;

void load_roms(void);
void do_go(void);
int do_dis(FILE *outfile, word32 kpc, int accsize, int xsize, int op_provided, word32 instr);
void do_debug_intfc(void);

void halt_printf(const char *fmt, ...);
int enter_debugger(int);

#endif /* KEGS_DIS_H */
