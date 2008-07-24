#ifndef HASHTABLE_H_INCLUDED
#define HASHTABLE_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif


#include <stddef.h>

/*  The hash table type.
    Implementations should not rely on the contents of this type. */
typedef struct ht
{
    unsigned (*hash_func)(const void *data, size_t size);
    struct ht_bucket *buckets;
    unsigned buckets_size;
} ht_t;


/*  A hash table iterator type.
    Implementations should not rely on the contents of this type. */
typedef struct ht_it
{
    const ht_t *ht;
    unsigned bucket;
    struct ht_entry *entry;
} ht_it_t;


/*  Initializes a hash table structure. 'hash_function' refers to a hashing
    function that returns 32-bit hash codes. */
void ht_create( ht_t *ht,
                unsigned (*hash_func)(const void *data, size_t size) );


/*  Finalizes a hash table structure initialized with a previous call to
    'ht_create'. */
void ht_destroy( ht_t *ht );


/*  Adds an entry to the hash table. They key and value data are copied if
    necessary. If an entry with the same key already existed, its value is
    updated.
    
    Invalidates all iterators over this hash table.
    
    Returns a pointer to the value of the item in the hash table. */
const void *ht_put( ht_t *ht,
                    const void *key_data,   size_t key_size,
                    const void *value_data, size_t value_size );


/*  Returns the value of the entry in the hash table with the given key.
    If no such entry exists, NULL is returned. If 'value_size' is non-NULL,
    '*value_size' is set to the value size. */
void *ht_get( const ht_t *ht,
              const void *key_data, size_t key_size,
              size_t *value_size );


/*  Removes the entry with the given key from the hash table. If the entry did
    not exist, no action is performed.
    
    Invalidates all iterators over this hash table. */
void ht_erase( ht_t *ht,
               const void *key_data, size_t key_size );

/*  Returns an iterator over the contents of this hash table and initializes
    it to point to the first entry in the hash table. */
ht_it_t ht_iterator( const ht_t *ht );

/*  Returns the value of the next entry from the hash table. 'it' must be a
    pointer to an iterator previously returned by call to ht_iterator().
    If there is no next entry, NULL is returned and the iterator is
    invalidated. Otherwise, the variables pointed to by 'key_data', 'key_size'
    and 'value_size' are updated to reflect the key data, key size and value
    size respectively. Each of these three arguments may be NULL, in which
    case each of these invidually are ignored.

    The iterator is updated to point to the next entry in the hash table. */
const void *ht_next( ht_it_t *ht,
                     const void **key_data, size_t *key_size,
                     size_t *value_size );


#ifdef __cplusplus
}
#endif

#endif /* ndef HASHTABLE_H_INCLUDED */
