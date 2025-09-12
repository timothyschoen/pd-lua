#include <pthread.h>

pthread_mutex_t lua_global_lock = PTHREAD_MUTEX_INITIALIZER;

#define lua_lock(L)    pthread_mutex_lock(&lua_global_lock)
#define lua_unlock(L)  pthread_mutex_unlock(&lua_global_lock)

#include "lua/onelua.c"
