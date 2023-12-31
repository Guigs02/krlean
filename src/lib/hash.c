//
// A hashed lookup mechanism
//
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <hash.h>

//
// hashval
//
// Convert key into hash value
//

__inline unsigned int hashidx(unsigned long key, unsigned int mask) {
    return (key ^ (key >> 2) ^ (key >> 6)) & mask;
}

//
// hash_alloc
//
// Allocate a hash data structure of the given hash size
//
// For speed we always round the hash table size to the nearest power
// of 2 above the requested size.
//

struct hash *hash_alloc(int hashsize) {
    struct hash *h;
    int i = 3, hashlim = 8;

    // Adjust hash size to the next power of 2
    while (hashsize > hashlim) {
        i++;
        hashlim <<= 1;
    }

    h = malloc(sizeof(struct hash) + hashlim * sizeof(struct hash_node *));
    if (h) {
        h->hashsize = hashlim;
        h->hashmask = hashlim - 1;
        memset(&h->buckets, 0, hashlim * sizeof(struct hash_node *));
    }

    return h;
}

//
// hash_insert
//
// Insert a new key/value pair into the hash
//

int hash_insert(struct hash *h, unsigned long key, void *val) {
    struct hash_node *hn;
    unsigned int idx;

    if (!h) return -EINVAL;

    idx = hashidx(key, h->hashmask);
    hn = malloc(sizeof(struct hash_node));
    if (!hn) return -ENOMEM;

    hn->key = key;
    hn->data = val;
    hn->next = h->buckets[idx];
    h->buckets[idx] = hn;

    return 0;
}

//
// hash_delete
//
// Remove node from hash
//

int hash_delete(struct hash *h, unsigned long key) {
    struct hash_node **hnp, *hn;
    unsigned int idx;

    if (!h) return -EINVAL;

    // Walk hash chain list.  Walk both the pointer, as
    // well as a pointer to the previous pointer.  When
    // we find the node, patch out the current node and
    // free it.

    idx = hashidx(key, h->hashmask);
    hnp = &h->buckets[idx];
    hn = *hnp;
    while (hn) {
        if (hn->key == key) {
            *hnp = hn->next;
            free(hn);
            return 0;
        }

        hnp = &hn->next;
        hn = *hnp;
    }

    return -ESRCH;
}

//
// hash_dealloc
//
// Free up the entire hash structure
//

void hash_dealloc(struct hash *h) {
    int x;
    struct hash_node *hn, *hnn;

    for (x = 0; x < h->hashsize; x++) {
        for (hn = h->buckets[x]; hn; hn = hnn) {
            hnn = hn->next;
            free(hn);
        }
    }

    free(h);
}

//
// hash_lookup
//
// Look up a node based on its key
//

void *hash_lookup(struct hash *h, unsigned long key) {
    struct hash_node *hn;
    unsigned int idx;

    if (!h) return NULL;

    idx = hashidx(key, h->hashmask);

    for (hn = h->buckets[idx]; hn; hn = hn->next) {
        if (hn->key == key) return hn->data;
    }

    return NULL;
}

//
// hash_size
//
// Tell how many elements are stored in the hash
//

int hash_size(struct hash *h) {
    int x, cnt = 0;
    struct hash_node *hn;

    for (x = 0; x < h->hashsize; x++) {
        for (hn = h->buckets[x]; hn; hn = hn->next) {
            cnt += 1;
        }
    }

    return cnt;
}

//
// hash_foreach
//
// Enumerate each entry in the hash, invoking a function
//

int hash_foreach(struct hash *h, enumfunc_t f, void *arg) {
    int x;
    int rc;
    struct hash_node *hn;

    for (x = 0; x < h->hashsize; x++) {
        for (hn = h->buckets[x]; hn; hn = hn->next) {
            rc = (*f)(hn->key, hn->data, arg);
            if (rc) return rc;
        }
    }

    return 0;
}
