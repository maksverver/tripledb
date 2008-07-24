#ifndef TRIPLEDB_H_INCLUDED
#define TRIPLEDB_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif


#include <stddef.h>

/*  A node identifier; this should be treated as an opaque data structure.
    Some macros are provided that operate on its contents.  */
typedef struct nid
{
    unsigned index;
    unsigned flags;
} nid_t;

/*  A triple node structure, consisting of the identifiers of the three nodes
    that comprise the triple. */
typedef struct triple
{
    nid_t nodes[3];
} triple_t;


/*  A model handle. */
typedef struct model *model_handle;


/*  Some macro's for manipulating the datatypes declared above follow. */

/* Determines if a node identifier is the NULL node identifier. */
#define NID_IS_NULL(nid) \
    ((nid).index == 0 && (nid).flags == 0)

/* Sets a node identifier to the NULL node identifier. */
#define NID_SET_NULL(nid) \
    { (nid).index = 0; (nid).flags = 0; }

/* Determines if two node identifiers are equal. */
#define NID_IS_EQUAL(nid_a, nid_b) \
    ((nid_a).index == (nid_b).index && (nid_a).flags == (nid_b).flags)

/*  Flag to indicate a node identifier identifies a triple node. */
#define NID_FTRIPLE \
    ((unsigned)1)

/* Determines is a node identifier refers to a triple node. */
#define NID_IS_TRIPLE(nid) \
    (nid.flags & NID_FTRIPLE)

/* Determines if two triples are equal (ie. their respective nodes are equal).
   */
#define TRIPLE_IS_EQUAL(triple_a, triple_b) \
    ( NID_IS_EQUAL((triple_a).nodes[0], (triple_b).nodes[0]) && \
      NID_IS_EQUAL((triple_a).nodes[1], (triple_b).nodes[1]) && \
      NID_IS_EQUAL((triple_a).nodes[2], (triple_b).nodes[2]) )
      
#define TRIPLE_SET_NULL(triple) \
    { NID_SET_NULL((triple).nodes[0]); NID_SET_NULL((triple).nodes[1]); \
      NID_SET_NULL((triple).nodes[0]); }


/*  Initializes the triple database. Before this function is called, no other
    functions declared here may be called. */
void tripledb_initialize();


/*  Finalizes the triple database. After this function is called, no other
    functions declared here may be called. Any open handles and borrowed memory
    buffers must be released before calling this function. */
void tripledb_finalize();


/*  Opens the model with the given name. If 'name' is NULL a new anonymous
    model is opened.

    Returns NULL if the model could not be opened, or a valid model handle that
    must be released with close_model() otherwise. */
model_handle open_model(const char *name);


/*  Closes the model with handle 'model', as returned by an earlier call to
    open_model(). 'model' may be NULL, in which case no action is performed.
    
    If the model is anonymous it is automatically deleted. */
void close_model(model_handle model);


/*  Returns a unique identifier for the given node data. Subsequent calls to
    this function with the same data will return the same identifier. */
nid_t identify_node(const void *data, size_t size);


/*  Returns the triple node identifier for the given triple. Subsequent calls
    to this function with the same triple will return the same identifier. */
nid_t identify_triple(triple_t *triple);


/*  Returns the node data for the non-triple node identifier 'nid'.
    
    If 'data' is not NULL, '*size' should contain the size of the data buffer.
    '*size' will be updated to match the node data size. If the buffer data
    size is large enough to hold the return data, the data buffer is updated
    and 'data' is returned. Otherwise, NULL is returned and the size set in
    'size' can be used to try another call with a large data buffer.

    If 'data' is NULL, a new data buffer containing the node data will be
    returned and '*size' will be set to the node data size. The returned data
    buffer must be freed with free_data(). */
const void *resolve_node(nid_t nid, void *data, size_t *size);


/*  Frees the data returned by resolve_node. */
void free_data(const void *data);


/*  Returns the triple node with the given node identifier. This triple
    contains three node identifiers, each of which may refer to a triple node
    identifier.
    
    'nid' must be a valid triple node identifier. */
triple_t resolve_triple(nid_t nid);


/*  Adds a triple in the given model. If the triple already exists, no
    modifications are made. 'model' must be a valid model handle, 'triple'
    must be a triple node identifier.

    If the triple already existed, no modifications are made.     
    Returns the number of triples added; 0 or 1. */
unsigned add_triple(model_handle model, nid_t nid);


/*  Removes a triple from the given model. 'model' must be a valid model handle
    and 'triple' must be a triple node identifier. If the triple did not exist,
    no modifications are made.
    
    Returns the number of triples removed; 0 or 1. */
unsigned remove_triple(model_handle model, nid_t nid);


/*  Finds a node in the model (given by the valid model handle 'model'), which
    matches the pattern specified in the pointer to the triple structure
    'pattern'. Each of the three fields in this triple may be set to either a
    null node identifier or a valid node identifier.
    
    A node matches the pattern specified by 'triple' if all three of its fields
    match the respective field in 'triple'. Two fields match if they refer to
    the same node, or if at least one of the fields refer to the null node.
    
    Note that found nodes are returned in no particular order.

    'previous' must be set to either a valid triple node identifier (in which
    case the first matching node after this node will be returned) or to the
    null node identifier (in which case the first matching node is returned).
    
    If no (more) matching nodes were found, the null node identifier is returned.
    
    Typically, this function is used in a loop:
        triple_t pattern;
        -- initialize pattern --
        nid_t nid;
        NID_SET_NULL(nid);
        while(nid = find_triple(model, &pattern, nid), !NID_IS_NULL(nid))
        {
            -- process node with identifier 'nid' --
        }
    */
nid_t find_triple(model_handle model, triple_t *pattern, nid_t previous);


/*  Removes all triples from the given model.
    Returns the number of triples removed. */
unsigned empty_model(model_handle model);


/*  Adds all triples in the model referenced by 'source' to the model
    referenced by 'destination'. The destination model may be expanded but
    the source model is never modified. */
void absorb_model(model_handle destination, model_handle source);


#ifdef __cplusplus
}
#endif

#endif /* ndef TRIPLEDB_H_INCLUDED */
