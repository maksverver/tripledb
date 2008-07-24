#include "hash.h"

#define FNV_BASIS_32 ((unsigned)0x811C9DC5)
#define FNV_PRIME_32 ((unsigned)0x01000193)

unsigned hash_fnv1(const void *data, size_t size)
{
    unsigned hash;
    char *bytes;
    
    hash = FNV_BASIS_32;
    bytes = (char *)data;

    while(size--)
    {
        hash *= FNV_PRIME_32;
        hash ^= *bytes++;
    }

    return hash;
}

unsigned hash_fnv1a(const void *data, size_t size)
{
    unsigned hash;
    char *bytes;
    
    hash = FNV_BASIS_32;
    bytes = (char *)data;

    while(size--)
    {
        hash ^= *bytes++;
        hash *= FNV_PRIME_32;
    }

    return hash;
}
