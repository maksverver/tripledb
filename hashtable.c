/*
    TODO: automatically resize buckets table!
*/

#include "hashtable.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

typedef struct ht_entry
{
    struct ht_entry *next;
    
    void *key;
    size_t key_size;

    void *value;
    size_t value_size;
    
} ht_entry_t;


typedef struct ht_bucket
{
    ht_entry_t *first;
} ht_bucket_t;


void ht_create( ht_t *ht,
                unsigned (*hash_func)(const void *data, size_t size) )
{
    unsigned bucket;
    
    ht->hash_func    = hash_func;
    ht->buckets_size = 65531;
    ht->buckets      = (ht_bucket_t*)malloc( sizeof(ht_bucket_t) *
                                             ht->buckets_size );
    assert(ht->buckets);
    for(bucket = 0; bucket < ht->buckets_size; ++bucket)
    {
        ht->buckets[bucket].first = NULL;
    }
}


void ht_destroy( ht_t *ht )
{
    unsigned bucket;
    ht_entry_t *entry, *next_entry;
    
    for(bucket = 0; bucket < ht->buckets_size; ++bucket)
    {
        for( entry = ht->buckets[bucket].first;
             entry != NULL; entry = next_entry )
        {
            next_entry = entry->next;
            free(entry->key);
            free(entry->value);
            free(entry);
        }
    }
    
    free(ht->buckets);
}


const void *ht_put( ht_t *ht,
                    const void *key_data,   size_t key_size,
                    const void *value_data, size_t value_size )
{
    unsigned bucket;
    ht_entry_t *entry;

    entry = (ht_entry_t *)malloc(sizeof(ht_entry_t));
    assert(entry);

    entry->key = malloc(key_size);
    memcpy(entry->key, key_data, key_size);
    entry->key_size = key_size;

    entry->value = malloc(value_size);
    memcpy(entry->value, value_data, value_size);
    entry->value_size = value_size;

    bucket = ht->hash_func(key_data, key_size) % ht->buckets_size;
    entry->next = ht->buckets[bucket].first;
    ht->buckets[bucket].first = entry;

    return entry->value;
}


void *ht_get( const ht_t *ht,
              const void *key_data, size_t key_size,
              size_t *value_size )
{
    unsigned bucket;
    ht_entry_t *entry;
    
    bucket = ht->hash_func(key_data, key_size) % ht->buckets_size;
    
    for(entry = ht->buckets[bucket].first; entry != NULL; entry = entry->next)
    {
        if( entry->key_size == key_size &&
            memcmp(entry->key, key_data, key_size) == 0)
        {
            if(value_size != NULL)
                *value_size = entry->value_size;

            return entry->value;
        }        
    }

    return NULL;
}


void ht_erase( ht_t *ht,
               const void *key_data, size_t key_size )
{
    unsigned bucket;
    ht_entry_t *entry, *last_entry;
    
    bucket = ht->hash_func(key_data, key_size) % ht->buckets_size;
    
    last_entry = NULL;
    for( entry = ht->buckets[bucket].first; entry != NULL;
         last_entry = entry, entry = entry->next )
    {
        if( entry->key_size == key_size &&
            memcmp(entry->key, key_data, key_size) == 0)
        {
            if(last_entry == NULL)
                ht->buckets[bucket].first = entry->next;
            else
                last_entry->next = entry->next;

            free(entry->key);
            free(entry->value);
            free(entry);

            break;
        }
    }
}


ht_it_t ht_iterator( const ht_t *ht )
{
    ht_it_t it;
    
    it.ht     = ht;
    it.bucket = 0;
    it.entry  = NULL;
    
    return it;
}


const void *ht_next( ht_it_t *it,
                     const void **key_data, size_t *key_size,
                     size_t *value_size )
{
    void *value_data;
    
    while(it->entry == NULL)
    {
        if(it->bucket == it->ht->buckets_size)
            return NULL;
            
        it->entry = it->ht->buckets[it->bucket].first;
        ++it->bucket;
    }
    
    if(key_data != NULL)
        *key_data = it->entry->key;
    
    if(key_size != NULL)
        *key_size = it->entry->key_size;
    
    value_data = it->entry->value;
    
    if(value_size != NULL)
        *value_size = it->entry->value_size;

    it->entry = it->entry->next;
    
    return value_data;
}
