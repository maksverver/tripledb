#include "tripledb.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

static char
    a[] = "Dit is een test.",
    b[] = "Korter.",
    c[] = { '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0' };

static size_t
    la = sizeof(a) - 1,
    lb = sizeof(b) - 1,
    lc = sizeof(c);

int main()
{
    nid_t nid_a, nid_b, nid_c, tid[6], nid;
    size_t size;
    void *buffer;
    const void *result;
    model_handle model_a, model_b;
    triple_t triple;
     
    buffer = malloc(4096);
    tripledb_initialize();

           
    /* Test identify_node() */
    
    nid_a = identify_node(a, la);
    assert(!NID_IS_NULL(nid_a));
    nid_b = identify_node(b, lb);
    assert(!NID_IS_NULL(nid_b));
    nid_c = identify_node(c, lc);
    assert(!NID_IS_NULL(nid_c));
    
    assert(!NID_IS_EQUAL(nid_a, nid_b));
    assert(!NID_IS_EQUAL(nid_a, nid_c));
    assert(!NID_IS_EQUAL(nid_a, nid_b));
    
    /* Test resolve_node() */
    result = resolve_node(nid_a, NULL, &size);
    assert(size == la); assert(memcmp(result, a, la) == 0);
    free_data(result);
    result = resolve_node(nid_b, NULL, &size);
    assert(size == lb); assert(memcmp(result, b, lb) == 0);
    free_data(result);
    result = resolve_node(nid_c, NULL, &size);
    assert(size == lc); assert(memcmp(result, c, lc) == 0);
    free_data(result);

    size = 4096; result = resolve_node(nid_a, buffer, &size);
    assert(result == buffer); assert(size == la); assert(memcmp(result, a, la) == 0);
    size = 4096; result = resolve_node(nid_b, buffer, &size);
    assert(result == buffer); assert(size == lb); assert(memcmp(result, b, lb) == 0);
    size = 4096; result = resolve_node(nid_c, buffer, &size);
    assert(result == buffer); assert(size == lc); assert(memcmp(result, c, lc) == 0);

    size = 0; result = resolve_node(nid_b, buffer, &size);
    assert(result == NULL); assert(size == lb);
    result = resolve_node(nid_b, buffer, &size);
    assert(result == buffer); assert(size == lb); assert(memcmp(result, b, lb) == 0);
    result = resolve_node(nid_a, buffer, &size);
    assert(result == NULL); assert(size == la);
    
    /* Adding nodes to different models. */
    model_a = open_model("a");
    model_b = open_model("b");
    assert(model_a != model_b);
    
    /* Add a few triples to model A. */
    triple.nodes[0] = nid_a; triple.nodes[1] = nid_b; triple.nodes[2] = nid_a;     /* A,B,A */
    tid[0] = identify_triple(&triple); add_triple(model_a, tid[0]);
    assert(!NID_IS_NULL(tid[0])); assert(NID_IS_TRIPLE(tid[0]));
    triple.nodes[0] = nid_a; triple.nodes[1] = nid_b; triple.nodes[2] = nid_b;     /* A,B,B */
    tid[1] = identify_triple(&triple); add_triple(model_a, tid[1]);
    assert(!NID_IS_NULL(tid[0])); assert(NID_IS_TRIPLE(tid[1]));
    triple.nodes[0] = nid_a; triple.nodes[1] = nid_b; triple.nodes[2] = nid_c;     /* A,B,C */
    tid[2] = identify_triple(&triple); add_triple(model_a, tid[2]);
    assert(!NID_IS_NULL(tid[0])); assert(NID_IS_TRIPLE(tid[2]));
    triple.nodes[0] = nid_c; triple.nodes[1] = nid_c; triple.nodes[2] = nid_c;     /* C,C,C */
    tid[3] = identify_triple(&triple); add_triple(model_a, tid[3]);
    assert(!NID_IS_NULL(tid[3])); assert(NID_IS_TRIPLE(tid[3]));
    
    /* Add a few more triples to model B. */
    triple.nodes[0] = nid_a; triple.nodes[1] = nid_b; triple.nodes[2] = nid_c;     /* A,B,C */
    tid[0] = identify_triple(&triple); add_triple(model_b, tid[0]); assert(!NID_IS_NULL(tid[0]));
    assert(NID_IS_EQUAL(tid[0], tid[2]));
    triple.nodes[0] = nid_a; triple.nodes[1] = nid_c; triple.nodes[2] = nid_b;     /* A,C,B */
    tid[1] = identify_triple(&triple); add_triple(model_b, tid[1]); assert(!NID_IS_NULL(tid[1]));
    assert(!NID_IS_EQUAL(tid[1], tid[0]));
    triple.nodes[0] = nid_b; triple.nodes[1] = nid_a; triple.nodes[2] = nid_c;     /* B,A,C */
    tid[2] = identify_triple(&triple); add_triple(model_b, tid[2]); assert(!NID_IS_NULL(tid[2]));
    assert(!NID_IS_EQUAL(tid[2], tid[0])); assert(!NID_IS_EQUAL(tid[2], tid[1]));
    triple.nodes[0] = nid_b; triple.nodes[1] = nid_c; triple.nodes[2] = nid_a;     /* B,C,A */
    tid[3] = identify_triple(&triple); add_triple(model_b, tid[3]); assert(!NID_IS_NULL(tid[3]));
    assert(!NID_IS_EQUAL(tid[3], tid[0])); assert(!NID_IS_EQUAL(tid[3], tid[1]));
    assert(!NID_IS_EQUAL(tid[3], tid[2]));
    triple.nodes[0] = nid_c; triple.nodes[1] = nid_a; triple.nodes[2] = nid_b;     /* C,A,B */
    tid[4] = identify_triple(&triple); add_triple(model_b, tid[4]); assert(!NID_IS_NULL(tid[4]));
    assert(!NID_IS_EQUAL(tid[4], tid[0])); assert(!NID_IS_EQUAL(tid[4], tid[1]));
    assert(!NID_IS_EQUAL(tid[4], tid[2])); assert(!NID_IS_EQUAL(tid[4], tid[3]));
    triple.nodes[0] = nid_c; triple.nodes[1] = nid_b; triple.nodes[2] = nid_a;     /* C,B,A */
    tid[5] = identify_triple(&triple); add_triple(model_b, tid[5]); assert(!NID_IS_NULL(tid[5]));
    assert(!NID_IS_EQUAL(tid[5], tid[0])); assert(!NID_IS_EQUAL(tid[5], tid[1]));
    assert(!NID_IS_EQUAL(tid[5], tid[2])); assert(!NID_IS_EQUAL(tid[5], tid[3]));
    assert(!NID_IS_EQUAL(tid[5], tid[4]));
    
    /* Test resolve_triple. */
    triple = resolve_triple(tid[0]);                                   /* A,B,C */
    assert(NID_IS_EQUAL(triple.nodes[0], nid_a) &&
           NID_IS_EQUAL(triple.nodes[1], nid_b) &&
           NID_IS_EQUAL(triple.nodes[2], nid_c) );
    triple = resolve_triple(tid[1]);                                   /* A,C,B */
    assert(NID_IS_EQUAL(triple.nodes[0], nid_a) &&
           NID_IS_EQUAL(triple.nodes[1], nid_c) &&
           NID_IS_EQUAL(triple.nodes[2], nid_b) );
    triple = resolve_triple(tid[2]);                                   /* B,A,C */
    assert(NID_IS_EQUAL(triple.nodes[0], nid_b) &&
           NID_IS_EQUAL(triple.nodes[1], nid_a) &&
           NID_IS_EQUAL(triple.nodes[2], nid_c) );
    triple = resolve_triple(tid[3]);                                   /* B,C,A */
    assert(NID_IS_EQUAL(triple.nodes[0], nid_b) &&
           NID_IS_EQUAL(triple.nodes[1], nid_c) &&
           NID_IS_EQUAL(triple.nodes[2], nid_a) );
    triple = resolve_triple(tid[4]);                                   /* C,A,B */
    assert(NID_IS_EQUAL(triple.nodes[0], nid_c) &&
           NID_IS_EQUAL(triple.nodes[1], nid_a) &&
           NID_IS_EQUAL(triple.nodes[2], nid_b) );
    triple = resolve_triple(tid[5]);                                   /* C,B,A */
    assert(NID_IS_EQUAL(triple.nodes[0], nid_c) &&
           NID_IS_EQUAL(triple.nodes[1], nid_b) &&
           NID_IS_EQUAL(triple.nodes[2], nid_a) );
    
    /* TODO: Test reified statements. */
    
    /* TODO: Test find and iterators. */
    /* NB. This exploits some undocumented ordering behaviour! */
    NID_SET_NULL(triple.nodes[0]);
    triple.nodes[1] = nid_a;
    NID_SET_NULL(triple.nodes[2]);
    NID_SET_NULL(nid);
    nid = find_triple(model_b, &triple, nid);
    assert(NID_IS_EQUAL(nid, tid[2]));
    nid = find_triple(model_b, &triple, nid);
    assert(NID_IS_EQUAL(nid, tid[4]));
    nid = find_triple(model_b, &triple, nid);
    assert(NID_IS_NULL(nid));

    NID_SET_NULL(triple.nodes[0]);
    triple.nodes[0] = nid_c;
    triple.nodes[1] = nid_a;
    triple.nodes[2] = nid_b;
    NID_SET_NULL(nid);
    nid = find_triple(model_b, &triple, nid);
    assert(NID_IS_EQUAL(nid, tid[4]));
    nid = find_triple(model_b, &triple, nid);
    assert(NID_IS_NULL(nid));

    NID_SET_NULL(triple.nodes[0]);
    triple.nodes[0] = nid_a;
    triple.nodes[1] = nid_b;
    NID_SET_NULL(triple.nodes[2]);
    NID_SET_NULL(nid);
    nid = find_triple(model_b, &triple, nid);
    assert(NID_IS_EQUAL(nid, tid[0]));
    nid = find_triple(model_b, &triple, nid);
    assert(NID_IS_NULL(nid));
    
    /* Remove all triples from model B */
    empty_model(model_b);
    
    /* Remove all triples from model A */
    NID_SET_NULL(triple.nodes[0]);
    NID_SET_NULL(triple.nodes[1]);
    NID_SET_NULL(triple.nodes[2]);
    NID_SET_NULL(nid);
    while(nid = find_triple(model_a, &triple, nid), !NID_IS_NULL(nid))
    {
        remove_triple(model_a, nid);
    }
    
    /* Test absorb_model. */
    triple.nodes[0] = nid_a; triple.nodes[1] = nid_b; triple.nodes[2] = nid_a;     /* a <-- A,B,A */
    tid[0] = identify_triple(&triple); add_triple(model_a, tid[0]);
    triple.nodes[0] = nid_c; triple.nodes[1] = nid_b; triple.nodes[2] = nid_a;     /* a <-- C,B,A */
    tid[1] = identify_triple(&triple); add_triple(model_a, tid[1]);
    triple.nodes[0] = nid_b; triple.nodes[1] = nid_a; triple.nodes[2] = nid_c;     /* b <-- B,A,C */
    tid[2] = identify_triple(&triple); add_triple(model_b, tid[2]);
    
    absorb_model(model_b, model_a);
    NID_SET_NULL(triple.nodes[0]);
    NID_SET_NULL(triple.nodes[1]);
    NID_SET_NULL(triple.nodes[2]);
    NID_SET_NULL(nid);
    nid = find_triple(model_b, &triple, nid);
    assert(NID_IS_EQUAL(nid, tid[0]));
    nid = find_triple(model_b, &triple, nid);
    assert(NID_IS_EQUAL(nid, tid[2]));
    nid = find_triple(model_b, &triple, nid);
    assert(NID_IS_EQUAL(nid, tid[1]));
    nid = find_triple(model_b, &triple, nid);
    assert(NID_IS_NULL(nid));
    
    absorb_model(model_a, model_b);
    NID_SET_NULL(nid);
    nid = find_triple(model_a, &triple, nid);
    assert(NID_IS_EQUAL(nid, tid[0]));
    nid = find_triple(model_a, &triple, nid);
    assert(NID_IS_EQUAL(nid, tid[2]));
    nid = find_triple(model_a, &triple, nid);
    assert(NID_IS_EQUAL(nid, tid[1]));
    nid = find_triple(model_a, &triple, nid);
    assert(NID_IS_NULL(nid));
    
    empty_model(model_a);
    empty_model(model_b);

    close_model(model_a);
    close_model(model_b);
    free(buffer);

    tripledb_finalize();
    
    return 0;
}

