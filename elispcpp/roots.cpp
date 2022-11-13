#include <vector>

#include "garb.h"
#include "roots.h"

static std::vector<unsigned long> frames;
static std::vector<handle_t> roots;

#ifndef EXTRA_ROOTS
void extra_roots(void (*f)(handle_t)) {}
#endif

void pop_frame(void) {
    unsigned long size = frames.back();
    frames.pop_back();
    pop_roots(size);
}

void new_frame(void) {
    frames.push_back(0);
}

unsigned long push_root_in_frame(unsigned long handle) {
    frames.back()++;
    return push_root(handle);
}

unsigned long prof(unsigned long handle) {
    return push_root_in_frame(handle);
}

// stack
unsigned long push_root(unsigned long handle) {
    roots.push_back(handle);
    return handle;
}

unsigned long pro(unsigned long handle) {
    return push_root(handle);
}

unsigned long pop_root(void) {
    unsigned long handle = roots.back();
    roots.pop_back();
    return handle;
}
void pop_roots(unsigned long num) {
    roots.resize(roots.size() - num);
}

unsigned long roots_size(void) {
    return roots.size();
}
