#ifndef KEGS_CLOCK_H
#define KEGS_CLOCK_H

double get_dtime(void);
int micro_sleep(double dtime);
void setup_bram(void);
void clock_update(void);

word32 clock_read_c033(void);
word32 clock_read_c034(void);
void clock_write_c033(word32 val);
void clock_write_c034(word32 val);

#endif /* KEGS_CLOCK_H */
