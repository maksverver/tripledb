#include "tripledb.h"
#include "hash.h"
#include "hashtable.h"

#include <assert.h>
#include <db.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#ifdef THREADSAFE
#include <pthread.h>

#define MUTEX_INIT(mutex) \
    { int result = pthread_mutex_init(&mutex, NULL); assert(result == 0); }

#define MUTEX_DESTROY(mutex) \
    { int result = pthread_mutex_destroy(&mutex); assert(result == 0); }
    
#define MUTEX_LOCK(mutex) \
    { int result = pthread_mutex_lock(&mutex); assert(result == 0); }
    
#define MUTEX_UNLOCK(mutex) \
    { int result = pthread_mutex_unlock(&mutex); assert(result == 0); }
    
#else  /* def THREADSAFE */
#warning "THREADSAFE not defined; the resulting library is not thread safe!"
#define MUTEX_INIT(mutex)    ((void)0)
#define MUTEX_DESTROY(mutex) ((void)0)
#define MUTEX_LOCK(mutex)    ((void)0)
#define MUTEX_UNLOCK(mutex)  ((void)0)
#endif  /* def THREADSAFE */

#include "urlencoding.h"

static DB *nodes, *nodes_index, *triples, *triples_index;
static recno_t last_node, last_triple; 
static ht_t open_models; /* (char*)model_name => (model_t*)model */

#ifdef THREADSAFE
/*  NB. when acquiring multiple locks:
        - nodes_index_mutex must be acquired before nodes_mutex
        - triples_index_mutex must be acquired before triples_mutex
    This way deadlocks can be avoided.
*/
static pthread_mutex_t nodes_mutex, nodes_index_mutex,
                       triples_mutex, triples_index_mutex,
                       models_mutex;
#endif

typedef struct model
{
    DB *triples_index;
    char *name, *filename;
    unsigned references;
#ifdef THREADSAFE
    pthread_mutex_t triples_index_mutex;
#endif
} model_t;

typedef struct triple_entry {
    triple_t triple;
    unsigned index;    
} triple_entry_t;


void tripledb_initialize()
{
    int result;
    DBT key;
    
    assert(sizeof(unsigned) == sizeof(recno_t));
    assert(sizeof(unsigned) == sizeof(size_t));
    assert(sizeof(nid_t) == 2*sizeof(unsigned));
    assert(sizeof(triple_t) == 3*sizeof(nid_t));
    assert(sizeof(triple_entry_t) == sizeof(triple_t) + sizeof(unsigned));

    ht_create(&open_models, hash_fnv1);

    /* Open nodes database. */
    nodes = dbopen("nodes.db", O_CREAT | O_EXLOCK | O_RDWR, 0700, DB_RECNO, NULL);
    assert(nodes);
    nodes_index = dbopen("nodes_index.db", O_CREAT | O_EXLOCK | O_RDWR, 0700, DB_HASH, NULL);
    assert(nodes_index);
    
    /* Look up last node index. */
    result = nodes->seq(nodes, &key, NULL, R_LAST);
    assert(result == 0 || result == 1);
    assert(result != 0 || key.size == sizeof(last_node));
    last_node = ( result == 0 ? *(recno_t*)key.data : 0);
    
    /* Open triples database. */
    triples = dbopen("triples.db", O_CREAT | O_EXLOCK | O_RDWR, 0700, DB_RECNO, NULL);
    assert(triples);
    triples_index = dbopen("triples_index.db", O_CREAT | O_EXLOCK | O_RDWR, 0700, DB_HASH, NULL);
    assert(triples_index);
    
    /* Look up last triple index. */
    result = triples->seq(triples, &key, NULL, R_LAST);
    assert(result == 0 || result == 1);
    assert(result != 0 || key.size == sizeof(last_node));
    last_triple = ( result == 0 ? *(recno_t*)key.data : 0 );
    
    /* Initialize synchronization primitives. */
    MUTEX_INIT(nodes_mutex);
    MUTEX_INIT(nodes_index_mutex);
    MUTEX_INIT(triples_mutex);
    MUTEX_INIT(triples_index_mutex);
}


void tripledb_finalize()
{
    int result;
    
    result = nodes->close(nodes);
    assert(result == 0);
    
    result = nodes_index->close(nodes_index);
    assert(result == 0);
    
    ht_destroy(&open_models);

    /* Finalize synchronization primitives. */
    MUTEX_DESTROY(nodes_mutex);
    MUTEX_DESTROY(nodes_index_mutex);
    MUTEX_DESTROY(triples_mutex);
    MUTEX_DESTROY(triples_index_mutex);
}


nid_t identify_node(const void *data, size_t size)
{
    DBT node_id, node_data;
    int result;
    nid_t nid;
    
    NID_SET_NULL(nid);

    node_data.data = (void*)data;
    node_data.size =  size;
    MUTEX_LOCK(nodes_index_mutex);
    result = nodes_index->get(nodes_index, &node_data, &node_id, 0);
    assert(result == 0 || result == 1);
    
    if(result == 0)
    {
        /* Existing node found. */
        assert(node_id.size == sizeof(size_t));
        nid.index = *(unsigned*)node_id.data;
    }
    else
    {
        /* Create a new node. */
        
        MUTEX_LOCK(nodes_mutex);
        
        nid.index = ++last_node;
        
        node_id.data = &nid.index;
        node_id.size = sizeof(nid.index);
                
        result = nodes->put(nodes, &node_id, &node_data, 0);
        assert(result == 0);

        result = nodes_index->put(nodes_index, &node_data, &node_id, 0);
        assert(result == 0);

        MUTEX_UNLOCK(nodes_mutex);
    }
    MUTEX_UNLOCK(nodes_index_mutex);
    
    return nid;
}


nid_t identify_triple(triple_t *triple)
{
    nid_t nid;
    DBT key, value;
    int result;
    
    nid.flags = NID_FTRIPLE;

    key.data = triple;
    key.size = sizeof(*triple);
    MUTEX_LOCK(triples_index_mutex);
    result = triples_index->get(triples_index, &key, &value, 0);
    assert(result == 0 || result == 1);

    if(result == 0)
    {
        assert(key.size == sizeof(triple_t));
        assert(value.size == sizeof(unsigned));
        
        /* Existing triple identifier found. */
        nid.index = *(unsigned *)value.data;
    }
    else
    {
        /* Create a new triple identifier. */

        MUTEX_LOCK(triples_mutex);
        nid.index = ++last_triple;

        value.data = &nid.index;
        value.size = sizeof(nid.index);

        /* Add the triple to the triple database. */
        result = triples->put(triples, &value, &key, 0);
        assert(result == 0);
        
        /* Add the triple to the triple database index. */
        result = triples_index->put(triples_index, &key, &value, 0);
        assert(result == 0);

        MUTEX_UNLOCK(triples_mutex);
    }
    MUTEX_UNLOCK(triples_index_mutex);
  
    return nid;
}


const void *resolve_node(nid_t nid, void *data, size_t *size)
{
    DBT node_id, node_data;
    int result;
        
    assert(!NID_IS_TRIPLE(nid));
    node_id.data = &nid.index;
    node_id.size = sizeof(nid.index);
    MUTEX_LOCK(nodes_mutex);
    result = nodes->get(nodes, &node_id, &node_data, 0);
    assert(result == 0);
    
    if(data == NULL)
    {
        /* Fill new buffer with node data. */
        void *buffer;
         
        buffer = malloc(node_data.size);
        assert(buffer != NULL);
        memcpy(buffer, node_data.data, node_data.size);
        *size = node_data.size;
        MUTEX_UNLOCK(nodes_mutex);
        
        return buffer;
    }
    else
    {
        if(node_data.size <= *size)
        {
            /* Fill external buffer with node data. */
            memcpy(data, node_data.data, node_data.size);
            *size = node_data.size;
            MUTEX_UNLOCK(nodes_mutex);
            
            return data;
        }
        else
        {
            /* External buffer too small; only set data size. */
            *size = node_data.size;
            MUTEX_UNLOCK(nodes_mutex);

            return NULL;
        }
    }
}


triple_t resolve_triple(nid_t nid)
{
    triple_t triple;
    int result;
    DBT key, value;

    assert(NID_IS_TRIPLE(nid));
    key.data = &nid.index;
    key.size = sizeof(nid.index);
    MUTEX_LOCK(triples_mutex);
    result = triples->get(triples, &key, &value, 0);
    assert(result == 0);
    assert(value.size == sizeof(triple_t));
    triple = *(triple_t *)value.data;
    MUTEX_UNLOCK(triples_mutex);
    
    return triple;
}


void free_data(const void *data)
{
    free((void*)data);
}


model_handle open_model(const char *name)
{
    model_t *model;
    
    MUTEX_LOCK(models_mutex);

    model = NULL;
    if( name != NULL)
    {
        void *p;
        
        p = ht_get( &open_models, name, strlen(name), NULL );
        if(p != NULL)
        {
            /* Open model found. */

            model = *(model_t**)p;
            ++(model->references);
        }
    }

    if(model == NULL)
    {
        /* Create a newly opened model. */
        model = (model_t*)malloc(sizeof(model_t));
        assert(model);
        model->references = 1;
        
        /* Construct filename for this model. */
        if(name == NULL)
        {
            model->name = NULL;
            model->filename = NULL;
        }
        else
        {
            model->name = strdup(name);
            model->filename = (char*)malloc(urlencoded_length(name) + 32);
            assert(model->filename);
            strcpy(model->filename, "model_");
            urlencode(model->filename + strlen("model_"), name);
            strcat(model->filename, "_triples_index.db");
        }
    
        /* Open model database. */
        model->triples_index = dbopen(
            model->filename, O_CREAT | O_EXLOCK | O_RDWR, 0600,
            DB_BTREE, NULL );
        assert(model->triples_index);
    
        /* Initialize synchronization primitives. */
        MUTEX_INIT(model->triples_index_mutex);
        
        if(model->name != NULL)
        {
            /* Register in table of open models. */
            ht_put( &open_models, model->name, strlen(model->name),
                    &model, sizeof(model) );
        }
    }

    MUTEX_UNLOCK(models_mutex);
    
    return model;
}


void close_model(model_handle model)
{
    MUTEX_LOCK(models_mutex);
    if(--model->references != 0)
    {
        /* Do not close the model yet; only flush results. */
        model->triples_index->sync(model->triples_index, 0);
    }
    else
    {
        int result;

        /* Close model database. */    
        result = model->triples_index->close(model->triples_index);
        assert(result == 0);
    
        /* Finalize synchronization primitives. */
        MUTEX_DESTROY(model->triples_index_mutex);
        
        /* Remove file, if the model is empty. */
        if( model->filename != NULL &&
            model->triples_index->seq( model->triples_index,
                                       NULL, NULL, R_FIRST ) == 1 )
        {
            unlink(model->filename);
        }

        if(model->name != NULL)
        {
            /* Unregister in table of open models. */
            ht_erase(&open_models, model->name, strlen(model->name));
        }

        /* Free memory. */
        free(model->name);
        free(model->filename);
        free(model);
    }
    MUTEX_UNLOCK(models_mutex);
}


unsigned add_triple(model_handle model, nid_t nid)
{
    triple_t triple;
    triple_entry_t entry;
    int result, permutation;
    DBT key, value;
    
    assert(NID_IS_TRIPLE(nid));
    triple = resolve_triple(nid);

    entry.index = nid.index;

    key.data = &entry;
    key.size = sizeof(entry);
    value.data = 0;
    value.size = 0;

    /* Add the (partial) triples to the model's triple index. */
    MUTEX_LOCK(model->triples_index_mutex);
    for(permutation = 0; permutation < 8; ++permutation)
    {
        if(permutation & 1)
            entry.triple.nodes[0] = triple.nodes[0];
        else
            NID_SET_NULL(entry.triple.nodes[0]);

        if(permutation & 2)
            entry.triple.nodes[1] = triple.nodes[1];
        else
            NID_SET_NULL(entry.triple.nodes[1]);

        if(permutation & 4)
            entry.triple.nodes[2] = triple.nodes[2];
        else
            NID_SET_NULL(entry.triple.nodes[2]);
        
        result = model->triples_index->put(
            model->triples_index, &key, &value, R_NOOVERWRITE );
        assert(result == 0 || result == 1);
    }
    MUTEX_UNLOCK(model->triples_index_mutex);
    
    return (result == 0) ? 1 : 0;
}


unsigned remove_triple(model_handle model, nid_t nid)
{
    triple_t triple;
    triple_entry_t entry;
    int result, permutation;
    DBT key;
    
    assert(NID_IS_TRIPLE(nid));
    
    triple = resolve_triple(nid);
    
    entry.index = nid.index;
    
    key.data = &entry;
    key.size = sizeof(entry);
        
    /* Remove the partial triples from the triple index. */
    MUTEX_LOCK(model->triples_index_mutex);
    for(permutation = 0; permutation < 8; ++permutation)
    {
        if(permutation & 1)
            entry.triple.nodes[0] = triple.nodes[0];
        else
            NID_SET_NULL(entry.triple.nodes[0]);

        if(permutation & 2)
            entry.triple.nodes[1] = triple.nodes[1];
        else
            NID_SET_NULL(entry.triple.nodes[1]);

        if(permutation & 4)
            entry.triple.nodes[2] = triple.nodes[2];
        else
            NID_SET_NULL(entry.triple.nodes[2]);

        result = model->triples_index->del(
            model->triples_index, &key, 0);
        assert(result == 0 || result == 1);
    }
    MUTEX_UNLOCK(model->triples_index_mutex);
    
    return (result == 0) ? 1 : 0;
}


nid_t find_triple(model_handle model, triple_t *pattern, nid_t previous)
{
    nid_t nid;
    triple_entry_t entry;
    int result;
    DBT key, value;
    
    assert(NID_IS_NULL(previous) || NID_IS_TRIPLE(previous));

    entry.triple = *pattern;
    entry.index  = previous.index + 1;
    
    key.data = &entry;
    key.size = sizeof(entry);
    
    MUTEX_LOCK(model->triples_index_mutex);
    result = model->triples_index->seq( model->triples_index,
                                        &key, &value, R_CURSOR );
    assert(result == 0 || result == 1);
    assert(result != 0 || key.size == sizeof(triple_entry_t));

    if( result == 0 &&
        TRIPLE_IS_EQUAL(((triple_entry_t *)key.data)->triple, *pattern) )
    {
        /* Next triple found. */
        nid.index = ((triple_entry_t *)key.data)->index;
        nid.flags = NID_FTRIPLE;
    }
    else
    {
        /* No more triples found. */
        NID_SET_NULL(nid);
    }
    MUTEX_UNLOCK(model->triples_index_mutex);

    return nid;
}


unsigned empty_model(model_handle model)
{
    int result;
    unsigned removed;
    
    MUTEX_LOCK(model->triples_index_mutex);
    removed = 0;
    while((result = model->triples_index->seq( model->triples_index,
                                               NULL, NULL, R_FIRST )) == 0)
    {
        ++removed;
        model->triples_index->del(model->triples_index, NULL, R_CURSOR);
    }
    assert(result == 1);
    MUTEX_UNLOCK(model->triples_index_mutex);

    return removed;
}


void absorb_model(model_handle destination, model_handle source)
{
    DBT key, value;
    int result;
    
    /* Lock models in fixed order, to avoid dead-locks. */
    if(source->triples_index < destination->triples_index)
    {
        MUTEX_LOCK(source->triples_index_mutex);
        MUTEX_LOCK(destination->triples_index_mutex);
    }
    else
    if(destination->triples_index < source->triples_index)
    {
        MUTEX_LOCK(destination->triples_index_mutex);
        MUTEX_LOCK(source->triples_index_mutex);
    }
    else
    {
        /* Handles are identical. */
        return;
    }
    
    /* Copy triple index contents of source model to destination model. */
    result = source->triples_index->seq( source->triples_index,
                                         &key, &value, R_FIRST );
    while(result == 0)
    {
        destination->triples_index->put( destination->triples_index,
                                         &key, &value, 0 );
        result = source->triples_index->seq( source->triples_index,
                                             &key, &value, R_NEXT );
    }
    assert(result == 1);

    /* Unlock models; order does not matter. */        
    MUTEX_UNLOCK(source->triples_index_mutex);
    MUTEX_UNLOCK(destination->triples_index_mutex);
}
