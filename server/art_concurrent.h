#include <stdint.h>
#include <pthread.h>
#include"logger.h"

#ifndef ART_H
#define ART_H

#define NODE4   1
#define NODE16  2
#define NODE48  3
#define NODE256 4

#define MAX_PREFIX_LEN 10

typedef struct {
    const unsigned char *target_key;
    int target_key_len;
    const unsigned char *found_key;
    uint32_t found_key_len;
    void *value;
} art_search_ctx;

typedef int(*art_callback)(void *data, const unsigned char *key, uint32_t key_len, void *value);

typedef struct {
    uint32_t part_key_length;
    uint8_t node_type;
    uint8_t children_count;
    unsigned char part_key[MAX_PREFIX_LEN];
    pthread_rwlock_t lock;
} art_node;


typedef struct {
    art_node node;
    unsigned char children_keys[4];
    art_node *children[4];
} art_node4;


typedef struct {
    art_node node;
    unsigned char children_keys[16];
    art_node *children[16];
} art_node16;


typedef struct {
    art_node node;
    unsigned char children_keys[256];
    art_node *children[48];
} art_node48;


typedef struct {
    art_node node;
    art_node *children_keys[256];
} art_node256;


typedef struct {
    void *value;
    uint32_t key_length;
    unsigned char key[];
} art_leaf_node;


typedef struct {
    art_node *root;
    uint64_t size;
} art_tree;


int art_tree_init(art_tree *t);


int art_tree_destroy(art_tree *t);


inline uint64_t art_size(art_tree *t) {
    return t->size;
}


void *art_insert(art_tree *t, const unsigned char *key, int key_len, void *value);


void *art_insert_no_replace(art_tree *t, const unsigned char *key, int key_len, void *value);


void *art_delete(art_tree *t, const unsigned char *key, int key_len);


void *art_search(const art_tree *t, const unsigned char *key, int key_len);


int art_iter(art_tree *t, art_callback cb, void *data);



void *art_find_less(art_tree *t, const unsigned char *key, int key_len);


void *art_find_more(art_tree *t, const unsigned char *key, int key_len);


#endif
