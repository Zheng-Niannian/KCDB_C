
#include "kvdb.h"
#include <stddef.h>
// Функция предварительной загрузки
static inline void prefetch(const void *x) {;}

#define container_of(ptr, type, member) 			\
        (type *)( (char *)ptr - offsetof(type,member) )

#define list_entry(ptr, type, member) \
	container_of(ptr, type, member)
// Структура для узла двусвязного списка
struct list_head {
	struct list_head *next, *prev;
};
// Инициализация списка
static inline void INIT_LIST_HEAD(struct list_head *list)
{
	list->next = list;
	list->prev = list;
}

static inline void __list_add(struct list_head *new,
			      struct list_head *prev,
			      struct list_head *next)
{
	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
}
// Добавление узла в начало списка
static inline void list_add(struct list_head *new, struct list_head *head)
{
	__list_add(new, head, head->next);
}
// Добавление узла в конец списка
static inline void list_add_tail(struct list_head *new, struct list_head *head)
{
	__list_add(new, head->prev, head);
}
// Удаление узла из списка
static inline void __list_del(struct list_head * prev, struct list_head * next)
{
	next->prev = prev;
	prev->next = next;
}

static inline void list_del(struct list_head *entry)
{
	__list_del(entry->prev, entry->next);
}
// Проверка списка на пустоту
static inline int list_empty(const struct list_head *head)
{
	return head->next == head;
}
// Макрос для перебора элементов списка с доступом к данным
#define list_for_each_entry(pos, head, member)				\
	for (pos = list_entry((head)->next, typeof(*pos), member);	\
	     prefetch(pos->member.next), &pos->member != (head); 	\
	     pos = list_entry(pos->member.next, typeof(*pos), member))
// Макрос для перебора элементов списка с доступом к данным и безопасным удалением
#define list_for_each_entry_safe(pos, n, head, member)			\
	for (pos = list_entry((head)->next, typeof(*pos), member),	\
		n = list_entry(pos->member.next, typeof(*pos), member);	\
	     &pos->member != (head); 					\
	     pos = n, n = list_entry(n->member.next, typeof(*n), member))
// Структура элемента базы данных ключ-значение
typedef struct kvdb_struct_s {
    short key_len;
    short value_len;
    char *key;
    char *value;
    struct list_head list;
} kvdb_struct;
// Голова списка для хранения базы данных
struct list_head kvdb_head;
pthread_mutex_t kvdb_mutex;
// Поиск элемента по ключу
kvdb_struct *kvdb_find_key(char *key)
{
    kvdb_struct *kvdb;

    list_for_each_entry(kvdb, &kvdb_head, list) {
        if (strcmp(kvdb->key, key) == 0) {
            return kvdb;
        }
    }

    return NULL;
}
// Получение значения по ключу
int kvdb_get_key_value(char *key, char **value, int *value_len)
{
    kvdb_struct *kvdb;

    pthread_mutex_lock(&kvdb_mutex);
    kvdb = kvdb_find_key(key);
    if (kvdb == NULL) {
        *value = NULL;
        *value_len = 0;
        pthread_mutex_unlock(&kvdb_mutex);
        return -1;
    }
    char *data =  malloc(kvdb->value_len);
    memcpy(data, kvdb->value, kvdb->value_len);
    *value_len = kvdb->value_len;
    *value = data;
    
    pthread_mutex_unlock(&kvdb_mutex);

    return 0;
}
// Получение ключа по значению
int kvdb_get_value_key(char *value, char **key, int *key_len)
{
    kvdb_struct *kvdb;
    int find = 0, ret;
    
    pthread_mutex_lock(&kvdb_mutex);
    list_for_each_entry(kvdb, &kvdb_head, list) {
        if (strcmp(kvdb->value, value) == 0) {
            find = 1;
            break;
        }
    }

    if (find) {
        char *data =  malloc(kvdb->key_len);
        memcpy(data, kvdb->value, kvdb->key_len);
        *key_len = kvdb->key_len;
        *key = data;
        ret = 0;
    } else {
        *key_len = 0;
        *key = NULL;
        ret = -1;
    }
    
    pthread_mutex_unlock(&kvdb_mutex);

    return ret;
}

int kvdb_del_key_value(char *key)
{
    kvdb_struct *kvdb;

    pthread_mutex_lock(&kvdb_mutex);
    kvdb = kvdb_find_key(key);
    if (kvdb == NULL) {
        pthread_mutex_unlock(&kvdb_mutex);
        return -1;
    }
    list_del(&kvdb->list);
    pthread_mutex_unlock(&kvdb_mutex);

    free(kvdb->key);
    free(kvdb->value);
    free(kvdb);

    return 0;
}

void kvdb_set_key_value(char *key, int key_len, char *value, int value_len)
{
    kvdb_struct *kvdb;

    pthread_mutex_lock(&kvdb_mutex);
    kvdb = kvdb_find_key(key);
    if (kvdb) {
        free(kvdb->value);
        kvdb->value_len = value_len;
        kvdb->value = malloc(kvdb->value_len);
        memcpy(kvdb->value, value, kvdb->value_len);
        pthread_mutex_unlock(&kvdb_mutex);
        KV_DEBUG("kvdb set key[%s] change value to [%s]\n", kvdb->key, kvdb->value);
        return;
    }
    pthread_mutex_unlock(&kvdb_mutex);

    kvdb = malloc(sizeof(kvdb_struct));
    kvdb->key_len = key_len;
    kvdb->value_len = value_len;
    kvdb->key = malloc(kvdb->key_len + 1);
    memset(kvdb->key, 0x00, kvdb->key_len + 1);
    memcpy(kvdb->key, key, kvdb->key_len);
    kvdb->value = malloc(kvdb->value_len);
    memcpy(kvdb->value, value, kvdb->value_len);

    KV_DEBUG("kvdb set key_len %d, value len %d\n", kvdb->key_len, kvdb->value_len);
    KV_DEBUG("kvdb set key[%s], value[%s]\n", kvdb->key, kvdb->value);

    pthread_mutex_lock(&kvdb_mutex);
    list_add_tail(&kvdb->list, &kvdb_head);
    pthread_mutex_unlock(&kvdb_mutex);

    return;
}

void kvdb_init(void)
{
    INIT_LIST_HEAD(&kvdb_head);
    pthread_mutex_init(&kvdb_mutex, NULL);

    return;
}
