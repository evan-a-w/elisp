#ifndef ROOTS_H
#define ROOTS_H

#include "garb.h"

#define EXTRA_ROOTS

// extras
void extra_roots(void (*)(handle_t));

// frames
void pop_frame(void);
void new_frame(void);
unsigned long push_root_in_frame(unsigned long handle);
unsigned long prof(unsigned long handle);

// stack
unsigned long push_root(unsigned long handle);
unsigned long pro(unsigned long handle);
unsigned long pop_root(void);
void pop_roots(unsigned long);
unsigned long roots_size(void);

#endif
