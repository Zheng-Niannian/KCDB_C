#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <emmintrin.h>
#include "art_concurrent.h"

#define IS_ART_LEAF(x) (((uintptr_t)x & 1))
#define SET_ART_LEAF(x) ((void*)((uintptr_t)x | 1))
#define GET_LEAF_RAW_DATA(x) ((art_leaf_node*)((void*)((uintptr_t)x & ~1)))

static art_node *alloc_node(uint8_t type) {
    art_node *node = NULL;
    switch (type) {
        case NODE4:
            node = (art_node *) calloc(1, sizeof(art_node4));
            break;
        case NODE16:
            node = (art_node *) calloc(1, sizeof(art_node16));
            break;
        case NODE48:
            node = (art_node *) calloc(1, sizeof(art_node48));
            break;
        case NODE256:
            node = (art_node *) calloc(1, sizeof(art_node256));
            break;
        default:
            abort();
    }
    node->node_type = type;
    pthread_rwlock_init(&node->lock, NULL);
    return node;
}

int art_tree_init(art_tree *t) {
    t->root = NULL;
    t->size = 0;
    return 0;
}

static void destroy_node(art_node *n) {
    if (!n) return;

    if (IS_ART_LEAF(n)) {
        free(GET_LEAF_RAW_DATA(n));
        return;
    }
    int i, index;
    union {
        art_node4 *p1;
        art_node16 *p2;
        art_node48 *p3;
        art_node256 *p4;
    } p;

    pthread_rwlock_wrlock(&n->lock);

    switch (n->node_type) {
        case NODE4:
            p.p1 = (art_node4 *) n;
            for (i = 0; i < n->children_count; i++) {
                destroy_node(p.p1->children[i]);
            }
            break;

        case NODE16:
            p.p2 = (art_node16 *) n;
            for (i = 0; i < n->children_count; i++) {
                destroy_node(p.p2->children[i]);
            }
            break;

        case NODE48:
            p.p3 = (art_node48 *) n;
            for (i = 0; i < 256; i++) {
                index = ((art_node48 *) n)->children_keys[i];
                if (!index) continue;
                destroy_node(p.p3->children[index - 1]);
            }
            break;

        case NODE256:
            p.p4 = (art_node256 *) n;
            for (i = 0; i < 256; i++) {
                if (p.p4->children_keys[i])
                    destroy_node(p.p4->children_keys[i]);
            }
            break;

        default:
            abort();
    }

    pthread_rwlock_unlock(&n->lock);
    pthread_rwlock_destroy(&n->lock);

    free(n);
}


int art_tree_destroy(art_tree *t) {
    destroy_node(t->root);
    return 0;
}

static art_node **find_children(art_node *n, unsigned char c) {
    int i, mask, bitfield;
    union {
        art_node4 *p1;
        art_node16 *p2;
        art_node48 *p3;
        art_node256 *p4;
    } p;
    switch (n->node_type) {
        case NODE4:
            p.p1 = (art_node4 *) n;
            pthread_rwlock_rdlock(&p.p1->node.lock);
            for (i = 0; i < n->children_count; i++) {
                if (((unsigned char *) p.p1->children_keys)[i] == c) {
                    pthread_rwlock_unlock(&p.p1->node.lock);
                    return &p.p1->children[i];
                }
            }
            pthread_rwlock_unlock(&p.p1->node.lock);
            break;
        case NODE16:
            p.p2 = (art_node16 *) n;
            pthread_rwlock_rdlock(&p.p2->node.lock);
            for (i = 0; i < n->children_count; i++) {
                if (p.p2->children_keys[i] == c) {
                    pthread_rwlock_unlock(&p.p2->node.lock);
                    return &p.p2->children[i];
                }
            }
            pthread_rwlock_unlock(&p.p2->node.lock);
            break;
        case NODE48:
            p.p3 = (art_node48 *) n;
            pthread_rwlock_rdlock(&p.p3->node.lock);
            i = p.p3->children_keys[c];
            if (i) {
                pthread_rwlock_unlock(&p.p3->node.lock);
                return &p.p3->children[i - 1];
            }
            pthread_rwlock_unlock(&p.p3->node.lock);
            break;

        case NODE256:
            p.p4 = (art_node256 *) n;
            pthread_rwlock_rdlock(&p.p4->node.lock);
            if (p.p4->children_keys[c]) {
                pthread_rwlock_unlock(&p.p4->node.lock);
                return &p.p4->children_keys[c];
            }
            pthread_rwlock_unlock(&p.p4->node.lock);
            break;

        default:
            abort();
    }
    return NULL;
}

static inline int min(int a, int b) {
    return (a < b) ? a : b;
}


static int check_prefix(const art_node *n, const unsigned char *key, int key_len, int depth) {
    int max_cmp = min(min(n->part_key_length, MAX_PREFIX_LEN), key_len - depth);
    int index;
    for (index = 0; index < max_cmp; index++) {
        if (n->part_key[index] != key[depth + index])
            return index;
    }
    return index;
}


static int leaf_matches(const art_leaf_node *n, const unsigned char *key, int key_len, int depth) {
    (void) depth;
    if (n->key_length != (uint32_t) key_len) return 1;

    return memcmp(n->key, key, key_len);
}


void *art_search(const art_tree *t, const unsigned char *key, int key_len) {
    art_node **child;
    art_node *n = t->root;
    int prefix_len, depth = 0;
    while (n) {
        if (IS_ART_LEAF(n)) {
            n = (art_node *) GET_LEAF_RAW_DATA(n);
            if (!leaf_matches((art_leaf_node *) n, key, key_len, depth)) {
                return ((art_leaf_node *) n)->value;
            }
            return NULL;
        }
        pthread_rwlock_rdlock(&n->lock);
        if (n->part_key_length) {
            prefix_len = check_prefix(n, key, key_len, depth);
            if (prefix_len != min(MAX_PREFIX_LEN, n->part_key_length)) {
                pthread_rwlock_unlock(&n->lock);
                return NULL;
            }

            depth = depth + n->part_key_length;
        }

        pthread_rwlock_unlock(&n->lock);
        child = find_children(n, key[depth]);
        n = (child) ? *child : NULL;
        depth++;
    }
    return NULL;
}


static art_leaf_node *minimum(const art_node *n) {
    if (!n) return NULL;
    if (IS_ART_LEAF(n)) return GET_LEAF_RAW_DATA(n);
    int index;
    switch (n->node_type) {
        case NODE4: {
            return minimum(((const art_node4 *) n)->children[0]);
        }
        case NODE16:
            return minimum(((const art_node16 *) n)->children[0]);
        case NODE48:
            index = 0;
            while (!((const art_node48 *) n)->children_keys[index]) index++;
            index = ((const art_node48 *) n)->children_keys[index] - 1;
            return minimum(((const art_node48 *) n)->children[index]);
        case NODE256:
            index = 0;
            while (!((const art_node256 *) n)->children_keys[index]) index++;
            return minimum(((const art_node256 *) n)->children_keys[index]);
        default:
            abort();
    }
}

static art_leaf_node *maximum(const art_node *n) {
    if (!n) return NULL;
    if (IS_ART_LEAF(n)) return GET_LEAF_RAW_DATA(n);

    int index;
    switch (n->node_type) {
        case NODE4:
            return maximum(((const art_node4 *) n)->children[n->children_count - 1]);
        case NODE16:
            return maximum(((const art_node16 *) n)->children[n->children_count - 1]);
        case NODE48:
            index = 255;
            while (!((const art_node48 *) n)->children_keys[index]) index--;
            index = ((const art_node48 *) n)->children_keys[index] - 1;
            return maximum(((const art_node48 *) n)->children[index]);
        case NODE256:
            index = 255;
            while (!((const art_node256 *) n)->children_keys[index]) index--;
            return maximum(((const art_node256 *) n)->children_keys[index]);
        default:
            abort();
    }
}

static art_leaf_node *create_leaf_node(const unsigned char *key, int key_len, void *value) {
    art_leaf_node *l = (art_leaf_node *) calloc(1, sizeof(art_leaf_node) + key_len);
    l->value = value;
    l->key_length = key_len;
    memcpy(l->key, key, key_len);
    return l;
}

static int longest_common_prefix(art_leaf_node *l1, art_leaf_node *l2, int depth) {
    int max_cmp = min(l1->key_length, l2->key_length) - depth;
    int index;
    for (index = 0; index < max_cmp; index++) {
        if (l1->key[depth + index] != l2->key[depth + index])
            return index;
    }
    return index;
}

static void copy_node_header(art_node *dest, art_node *src) {
    dest->children_count = src->children_count;
    dest->part_key_length = src->part_key_length;
    memcpy(dest->part_key, src->part_key, min(MAX_PREFIX_LEN, src->part_key_length));
}

static void add_child256(art_node256 *n, art_node **ref, unsigned char c, void *child) {
    (void) ref;
    n->node.children_count++;
    n->children_keys[c] = (art_node *) child;
    pthread_rwlock_unlock(&n->node.lock);
}

static void add_child48(art_node48 *n, art_node **ref, unsigned char c, void *child) {
    if (n->node.children_count < 48) {
        int pos = 0;
        while (n->children[pos]) pos++;
        n->children[pos] = (art_node *) child;
        n->children_keys[c] = pos + 1;
        n->node.children_count++;
        pthread_rwlock_unlock(&n->node.lock);
    } else {
        art_node256 *new_node = (art_node256 *) alloc_node(NODE256);
        for (int i = 0; i < 256; i++) {
            if (n->children_keys[i]) {
                new_node->children_keys[i] = n->children[n->children_keys[i] - 1];
            }
        }
        copy_node_header((art_node *) new_node, (art_node *) n);
        *ref = (art_node *) new_node;
        pthread_rwlock_unlock(&n->node.lock);
        pthread_rwlock_destroy(&n->node.lock);
        free(n);
        pthread_rwlock_wrlock(&new_node->node.lock);
        add_child256(new_node, ref, c, child);
    }
}

static void add_child16(art_node16 *n, art_node **ref, unsigned char c, void *child) {
    if (n->node.children_count < 16) {
        unsigned index = 0;
        while (index < n->node.children_count && c > n->children_keys[index]) {
            index++;
        }

        if (index < n->node.children_count) {
            memmove(n->children_keys + index + 1, n->children_keys + index, (n->node.children_count - index) * sizeof(unsigned char));
            memmove(n->children + index + 1, n->children + index, (n->node.children_count - index) * sizeof(void*));
        }

        n->children_keys[index] = c;
        n->children[index] = (art_node*)child;
        n->node.children_count++;
        pthread_rwlock_unlock(&n->node.lock);
    } else {
        art_node48 *new_node = (art_node48 *) alloc_node(NODE48);
        memcpy(new_node->children, n->children,
               sizeof(void *) * n->node.children_count);
        for (int i = 0; i < n->node.children_count; i++) {
            new_node->children_keys[n->children_keys[i]] = i + 1;
        }
        copy_node_header((art_node *) new_node, (art_node *) n);
        *ref = (art_node *) new_node;
        pthread_rwlock_unlock(&n->node.lock);
        pthread_rwlock_destroy(&n->node.lock);
        free(n);
        pthread_rwlock_wrlock(&new_node->node.lock);
        add_child48(new_node, ref, c, child);
    }
}

static void add_child4(art_node4 *n, art_node **ref, unsigned char c, void *child) {
    if (n->node.children_count < 4) {
        int index;
        for (index = 0; index < n->node.children_count; index++) {
            if (c < n->children_keys[index]) break;
        }

        // Shift to make room
        memmove(n->children_keys + index + 1, n->children_keys + index, n->node.children_count - index);
        memmove(n->children + index + 1, n->children + index,
                (n->node.children_count - index) * sizeof(void *));

        // Insert element
        n->children_keys[index] = c;
        n->children[index] = (art_node *) child;
        n->node.children_count++;
        pthread_rwlock_unlock(&n->node.lock);
    } else {
        art_node16 *new_node = (art_node16 *) alloc_node(NODE16);

        // Copy the child pointers and the key map
        memcpy(new_node->children, n->children,
               sizeof(void *) * n->node.children_count);
        memcpy(new_node->children_keys, n->children_keys,
               sizeof(unsigned char) * n->node.children_count);
        copy_node_header((art_node *) new_node, (art_node *) n);
        *ref = (art_node *) new_node;
        pthread_rwlock_unlock(&n->node.lock);
        pthread_rwlock_destroy(&n->node.lock);
        free(n);
        pthread_rwlock_wrlock(&new_node->node.lock);
        add_child16(new_node, ref, c, child);
    }
}

static void add_children(art_node *n, art_node **ref, unsigned char c, void *child) {
    switch (n->node_type) {
        case NODE4:
            return add_child4((art_node4 *) n, ref, c, child);
        case NODE16:
            return add_child16((art_node16 *) n, ref, c, child);
        case NODE48:
            return add_child48((art_node48 *) n, ref, c, child);
        case NODE256:
            return add_child256((art_node256 *) n, ref, c, child);
        default:
            abort();
    }
}

static int prefix_mismatch(const art_node *n, const unsigned char *key, int key_len, int depth) {
    int max_cmp = min(min(MAX_PREFIX_LEN, n->part_key_length), key_len - depth);
    int index;
    for (index = 0; index < max_cmp; index++) {
        if (n->part_key[index] != key[depth + index])
            return index;
    }

    if (n->part_key_length > MAX_PREFIX_LEN) {
        art_leaf_node *l = minimum(n);
        max_cmp = min(l->key_length, key_len) - depth;
        for (; index < max_cmp; index++) {
            if (l->key[index + depth] != key[depth + index])
                return index;
        }
    }
    return index;
}

static void *
recursive_insert(art_node *n, art_node **ref, const unsigned char *key, int key_len, void *value, int depth, int *old,
                 int replace) {
    if (!n) {
        art_node *node = (art_node *) SET_ART_LEAF(create_leaf_node(key, key_len, value));

        *ref = node;
        return NULL;
    }
    if (IS_ART_LEAF(n)) {
        art_leaf_node *l = GET_LEAF_RAW_DATA(n);
        if (!leaf_matches(l, key, key_len, depth)) {
            *old = 1;
            void *old_val = l->value;
            if (replace) {
                l->value = value;
            }
            return old_val;
        }
        art_node4 *new_node = (art_node4 *) alloc_node(NODE4);
        pthread_rwlock_wrlock(&new_node->node.lock);

        art_leaf_node *l2 = create_leaf_node(key, key_len, value);

        int longest_prefix = longest_common_prefix(l, l2, depth);
        new_node->node.part_key_length = longest_prefix;
        memcpy(new_node->node.part_key, key + depth, min(MAX_PREFIX_LEN, longest_prefix));
        *ref = (art_node *) new_node;
        add_child4(new_node, ref, l->key[depth + longest_prefix], SET_ART_LEAF(l));
        pthread_rwlock_wrlock(&new_node->node.lock);
        add_child4(new_node, ref, l2->key[depth + longest_prefix], SET_ART_LEAF(l2));
        return NULL;
    }
    pthread_rwlock_wrlock(&n->lock);
    if (n->part_key_length) {
        int prefix_diff = prefix_mismatch(n, key, key_len, depth);
        if ((uint32_t) prefix_diff >= n->part_key_length) {
            depth += n->part_key_length;
            pthread_rwlock_unlock(&n->lock);
            goto RECURSE_SEARCH;
        }

        art_node4 *new_node = (art_node4 *) alloc_node(NODE4);
        pthread_rwlock_wrlock(&new_node->node.lock);
        *ref = (art_node *) new_node;
        new_node->node.part_key_length = prefix_diff;
        memcpy(new_node->node.part_key, n->part_key, min(MAX_PREFIX_LEN, prefix_diff));

        if (n->part_key_length <= MAX_PREFIX_LEN) {
            add_child4(new_node, ref, n->part_key[prefix_diff], n);
            n->part_key_length -= (prefix_diff + 1);
            memmove(n->part_key, n->part_key + prefix_diff + 1,
                    min(MAX_PREFIX_LEN, n->part_key_length));
        } else {
            n->part_key_length -= (prefix_diff + 1);
            art_leaf_node *l = minimum(n);
            add_child4(new_node, ref, l->key[depth + prefix_diff], n);
            memcpy(n->part_key, l->key + depth + prefix_diff + 1,
                   min(MAX_PREFIX_LEN, n->part_key_length));
        }

        art_leaf_node *l = create_leaf_node(key, key_len, value);
        pthread_rwlock_wrlock(&new_node->node.lock);
        add_child4(new_node, ref, key[depth + prefix_diff], SET_ART_LEAF(l));
        return NULL;
    }
    pthread_rwlock_unlock(&n->lock);

    RECURSE_SEARCH:;

    art_node **child = find_children(n, key[depth]);
    if (child) {
        return recursive_insert(*child, child, key, key_len, value, depth + 1, old, replace);
    }

    art_leaf_node *l = create_leaf_node(key, key_len, value);
    pthread_rwlock_wrlock(&n->lock);
    add_children(n, ref, key[depth], SET_ART_LEAF(l));
    return NULL;
}


void *art_insert(art_tree *t, const unsigned char *key, int key_len, void *value) {
    int previous_value = 0;
    void *previous_value_ptr = recursive_insert(t->root, &t->root, key, key_len, value, 0, &previous_value, 1);
    if (!previous_value) t->size++;
    return previous_value_ptr;
}


void *art_insert_no_replace(art_tree *t, const unsigned char *key, int key_len, void *value) {
    int previous_value = 0;
    void *previous_value_ptr = recursive_insert(t->root, &t->root, key, key_len, value, 0, &previous_value, 0);
    if (!previous_value) t->size++;
    return previous_value_ptr;
}

static void remove_child256(art_node256 *n, art_node **ref, unsigned char c) {
    n->children_keys[c] = NULL;
    n->node.children_count--;

    if (n->node.children_count == 37) {
        art_node48 *new_node = (art_node48 *) alloc_node(NODE48);
        pthread_rwlock_unlock(&n->node.lock);
        pthread_rwlock_destroy(&n->node.lock);
        *ref = (art_node *) new_node;
        copy_node_header((art_node *) new_node, (art_node *) n);

        int pos = 0;
        for (int i = 0; i < 256; i++) {
            if (n->children_keys[i]) {
                new_node->children[pos] = n->children_keys[i];
                new_node->children_keys[i] = pos + 1;
                pos++;
            }
        }
        free(n);
    } else {
        pthread_rwlock_unlock(&n->node.lock);
    }
}

static void remove_child48(art_node48 *n, art_node **ref, unsigned char c) {
    int pos = n->children_keys[c];
    n->children_keys[c] = 0;
    n->children[pos - 1] = NULL;
    n->node.children_count--;

    if (n->node.children_count == 12) {
        art_node16 *new_node = (art_node16 *) alloc_node(NODE16);
        pthread_rwlock_unlock(&n->node.lock);
        pthread_rwlock_destroy(&n->node.lock);
        *ref = (art_node *) new_node;
        copy_node_header((art_node *) new_node, (art_node *) n);

        int child = 0;
        for (int i = 0; i < 256; i++) {
            pos = n->children_keys[i];
            if (pos) {
                new_node->children_keys[child] = i;
                new_node->children[child] = n->children[pos - 1];
                child++;
            }
        }
        free(n);
    } else {
        pthread_rwlock_unlock(&n->node.lock);
    }
}

static void remove_child16(art_node16 *n, art_node **ref, art_node **l) {
    int pos = l - n->children;
    memmove(n->children_keys + pos, n->children_keys + pos + 1, n->node.children_count - 1 - pos);
    memmove(n->children + pos, n->children + pos + 1, (n->node.children_count - 1 - pos) * sizeof(void *));
    n->node.children_count--;

    if (n->node.children_count == 3) {
        art_node4 *new_node = (art_node4 *) alloc_node(NODE4);
        pthread_rwlock_unlock(&n->node.lock);
        pthread_rwlock_destroy(&n->node.lock);
        *ref = (art_node *) new_node;
        copy_node_header((art_node *) new_node, (art_node *) n);
        memcpy(new_node->children_keys, n->children_keys, 4);
        memcpy(new_node->children, n->children, 4 * sizeof(void *));
        free(n);
    } else {
        pthread_rwlock_unlock(&n->node.lock);
    }
}

static void remove_child4(art_node4 *n, art_node **ref, art_node **l) {
    int pos = l - n->children;
    memmove(n->children_keys + pos, n->children_keys + pos + 1, n->node.children_count - 1 - pos);
    memmove(n->children + pos, n->children + pos + 1, (n->node.children_count - 1 - pos) * sizeof(void *));
    n->node.children_count--;

    if (n->node.children_count == 1) {
        art_node *child = n->children[0];

        if (!IS_ART_LEAF(child)) {
            int prefix = n->node.part_key_length;
            if (prefix < MAX_PREFIX_LEN) {
                n->node.part_key[prefix] = n->children_keys[0];
                prefix++;
            }
            if (prefix < MAX_PREFIX_LEN) {
                int sub_prefix = min(child->part_key_length, MAX_PREFIX_LEN - prefix);
                memcpy(n->node.part_key + prefix, child->part_key, sub_prefix);
                prefix += sub_prefix;
            }

            memcpy(child->part_key, n->node.part_key, min(prefix, MAX_PREFIX_LEN));
            child->part_key_length += n->node.part_key_length + 1;
        }
        log_info("UNLOCK\n");
        pthread_rwlock_unlock(&n->node.lock);
        pthread_rwlock_destroy(&n->node.lock);
        *ref = child;
        free(n);
    } else {
        log_info("UNLOCK\n");
        pthread_rwlock_unlock(&n->node.lock);
    }
}

static void remove_child(art_node *n, art_node **ref, unsigned char c, art_node **l) {
    switch (n->node_type) {
        case NODE4:
            return remove_child4((art_node4 *) n, ref, l);
        case NODE16:
            return remove_child16((art_node16 *) n, ref, l);
        case NODE48:
            return remove_child48((art_node48 *) n, ref, c);
        case NODE256:
            return remove_child256((art_node256 *) n, ref, c);
        default:
            abort();
    }
}

static art_leaf_node *recursive_delete(art_node *n, art_node **ref, const unsigned char *key, int key_len, int depth) {
    if (!n) return NULL;

    if (IS_ART_LEAF(n)) {
        art_leaf_node *l = GET_LEAF_RAW_DATA(n);
        if (!leaf_matches(l, key, key_len, depth)) {
            *ref = NULL;
            return l;
        }
        return NULL;
    }
    log_info("LOCK\n");
    pthread_rwlock_wrlock(&n->lock);
    if (n->part_key_length) {
        int prefix_len = check_prefix(n, key, key_len, depth);
        if (prefix_len != min(MAX_PREFIX_LEN, n->part_key_length)) {
            log_info("UNLOCK\n");
            pthread_rwlock_unlock(&n->lock);
            return NULL;
        }
        depth = depth + n->part_key_length;
    }
    log_info("UNLOCK\n");
    pthread_rwlock_unlock(&n->lock);

    art_node **child = find_children(n, key[depth]);
    if (!child) {
        return NULL;
    }

    if (IS_ART_LEAF(*child)) {
        art_leaf_node *l = GET_LEAF_RAW_DATA(*child);
        if (!leaf_matches(l, key, key_len, depth)) {
            log_info("LOCK\n");
            pthread_rwlock_wrlock(&n->lock);
            remove_child(n, ref, key[depth], child);
//            pthread_rwlock_unlock(&n->lock);
            return l;
        }
        return NULL;

    } else {
//        pthread_rwlock_unlock(&n->lock);
        return recursive_delete(*child, child, key, key_len, depth + 1);
    }
}


void *art_delete(art_tree *t, const unsigned char *key, int key_len) {
    art_leaf_node *l = recursive_delete(t->root, &t->root, key, key_len, 0);
    if (l) {
        t->size--;
        void *old = l->value;
        free(l);
        return old;
    }
    return NULL;
}

static int recursive_iter(art_node *n, art_callback cb, void *data) {
    if (!n) return 0;
    if (IS_ART_LEAF(n)) {
        art_leaf_node *l = GET_LEAF_RAW_DATA(n);
        return cb(data, (const unsigned char *) l->key, l->key_length, l->value);
    }

    int index, res;
    pthread_rwlock_rdlock(&n->lock);
    switch (n->node_type) {
        case NODE4:
            for (int i = 0; i < n->children_count; i++) {
                res = recursive_iter(((art_node4 *) n)->children[i], cb, data);
                if (res) {
                    pthread_rwlock_unlock(&n->lock);
                    return res;
                }
            }
            break;

        case NODE16:
            for (int i = 0; i < n->children_count; i++) {
                res = recursive_iter(((art_node16 *) n)->children[i], cb, data);
                if (res) {
                    pthread_rwlock_unlock(&n->lock);
                    return res;
                }
            }
            break;

        case NODE48:
            for (int i = 0; i < 256; i++) {
                index = ((art_node48 *) n)->children_keys[i];
                if (!index) continue;

                res = recursive_iter(((art_node48 *) n)->children[index - 1], cb, data);
                if (res) {
                    pthread_rwlock_unlock(&n->lock);
                    return res;
                }
            }
            break;

        case NODE256:
            for (int i = 0; i < 256; i++) {
                if (!((art_node256 *) n)->children_keys[i]) continue;
                res = recursive_iter(((art_node256 *) n)->children_keys[i], cb, data);
                if (res) {
                    pthread_rwlock_unlock(&n->lock);
                    return res;
                }
            }
            break;

        default:
            abort();
    }
    pthread_rwlock_unlock(&n->lock);
    return 0;
}


int art_iter(art_tree *t, art_callback cb, void *data) {
    return recursive_iter(t->root, cb, data);
}

static int find_less_callback(void *data, const unsigned char *key, uint32_t key_len, void *value) {
    art_search_ctx *ctx = (art_search_ctx *) data;
    if (memcmp(key, ctx->target_key, min(key_len, ctx->target_key_len)) < 0) {
        if (!ctx->value || memcmp(key, ctx->found_key, ctx->found_key_len) > 0) {
            ctx->value = value;
            ctx->found_key = key;
            ctx->found_key_len = key_len;
        }
    }
    return 0;  
}

static int find_more_callback(void *data, const unsigned char *key, uint32_t key_len, void *value) {
    art_search_ctx *ctx = (art_search_ctx *) data;
    if (memcmp(key, ctx->target_key, min(key_len, ctx->target_key_len)) > 0) {
        if (!ctx->value || memcmp(key, ctx->found_key, ctx->found_key_len) < 0) {
            ctx->value = value;
            ctx->found_key = key;
            ctx->found_key_len = key_len;
        }
    }
    return 0;  
}

void *art_find_less(art_tree *t, const unsigned char *key, int key_len) {
    art_search_ctx ctx = {key, key_len, NULL, 0, NULL};
    art_iter(t, find_less_callback, &ctx);
    return ctx.value;
}


void *art_find_more(art_tree *t, const unsigned char *key, int key_len) {
    art_search_ctx ctx = {key, key_len, NULL, 0, NULL};
    art_iter(t, find_more_callback, &ctx);
    return ctx.value;
}
