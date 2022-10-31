#ifndef ENV_H
#define ENV_H

#include "map.h"
#include "garb.h"
#include "list.h"

// handles must be of const_strings 
handle_t env_push(handle_t env, handle_t key, handle_t val);
handle_t env_pop(handle_t env, handle_t key);
bool env_search(handle_t env, handle_t key, handle_t *save_to);

#endif
