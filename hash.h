#ifndef HASH_H_INCLUDED
#define HASH_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif


#include <stddef.h>


/* 32-bit FNV-1 hash */
unsigned hash_fnv1(const void *data, size_t size);

/* 32-bit FNV-1a hash */
unsigned hash_fnv1a(const void *data, size_t size);


#ifdef __cplusplus
}
#endif

#endif /* ndef HASH_H_INCLUDED */
