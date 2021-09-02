#ifndef RUNTIME_COMMON_H
#define RUNTIME_COMMON_H

#include "types.h"

//Stanza Alloc
void* stz_malloc (stz_long size);
void stz_free (void* ptr);

#endif
