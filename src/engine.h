#ifndef KEGS_ENGINE_H
#define KEGS_ENGINE_H

void fixed_memory_ptrs_init();
word32 get_memory_c(word32 addr, int cycs);
word32 get_memory16_c(word32 addr, int cycs);
word32 get_memory24_c(word32 addr, int cycs);
void set_memory_c(word32 addr, word32 val, int cycs);
void set_memory16_c(word32 addr, word32 val, int cycs);
void set_halt_act(int val);
void clr_halt_act(void);

int enter_engine(Engine_reg *ptr);

#endif /* KEGS_ENGINE_H */
