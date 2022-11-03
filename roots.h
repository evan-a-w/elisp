#ifndef ROOTS_H
#define ROOTS_H

// frames
void pop_frame(void);
void new_frame(void);
unsigned long push_root_in_frame(unsigned long handle);
unsigned long prof(unsigned long handle);

// stack
unsigned long push_root(unsigned long handle);
unsigned long pro(unsigned long handle);
unsigned long pop_root(void);
void pop_roots(int);
unsigned long roots_size(void);
unsigned long peek_root(void);

// hashmap
unsigned long root_global(unsigned long handle);
void unroot_global(unsigned long handle);
bool rooted_global(unsigned long handle);

#endif
