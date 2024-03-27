
#ifndef __KV_DB_H__
#define __KV_DB_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>

#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/socket.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>

#include <time.h>
#include <sys/time.h>
#include <sys/timerfd.h>

#include <pthread.h>
#include <asm/byteorder.h>

#define _KV_DEBUG__
#ifdef _KV_DEBUG__
#define KV_DEBUG(kvt, args...) do {\
		printf(kvt, ##args); \
} while (0)
#else
#define KV_DEBUG(kvt, args...)
#endif
// Структура, представляющая обработчик epoll
struct epollop {
	struct epoll_event *events;
	int nevents;
	int epfd;
};
// Функция установки неблокирующего режима для сокета
static inline void kv_setnonblocking(int fd)
{
	int flags;

	if ((flags = fcntl(fd, F_GETFL, NULL)) < 0)
    {
        printf("Fd %d NoBlock fcntl F_GETFD Failed\n", fd);

		return;
	}

	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        printf("Fd %d NoBlock fcntl F_SETFL Failed\n", fd);

		return;
	}

	return;
}
// Функция установки неблокирующего режима для сокета
static inline void kv_epoll_addfd(int epollfd, int fd)
{
    struct epoll_event event;

    kv_setnonblocking(fd);

    event.data.fd = fd;
    event.events = EPOLLIN;

    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);

	return;
}
// Функция удаления сокета из epoll и его закрытия
static inline void kv_epoll_delfd(int epollfd, int fd)
{
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL);

	close(fd);

	return;
}
// Функция инициализации epoll
static inline int kv_epoll_init(struct epollop *eop, int nevent)
{
    int len;

    eop->nevents = nevent;

    len = eop->nevents * sizeof(struct epoll_event);
	eop->events = malloc(len);
	if (eop->events == NULL) {
		printf("out of memory\n");
		return -1;
	}

    memset(eop->events, 0x00, len);
	eop->epfd = epoll_create(10000);
	if (eop->epfd == -1) {
        free(eop->events);
        eop->events = NULL;
		perror("epoll creat error");
		return -1;
	}

    return 0;
}
// Типы сообщений
#define KV_MSG_TYPE_RET         1
#define KV_MSG_TYPE_SET         2
#define KV_MSG_TYPE_GET         3
#define KV_MSG_TYPE_DEL         4
#define KV_MSG_TYPE_LOOK        5

#define KV_MSG_MAX	            8192

// Заголовок сообщения
typedef struct kv_head_msg {
    unsigned short type;
    unsigned short len;//not inclde head
    char data[0];
} kv_head_msg_t;
#define KV_HDR_LEN			 sizeof(kv_head_msg_t)

typedef unsigned char u8;
// Структура сообщения с возвращаемым значением
typedef struct kv_value_msg_s {
    short value_len;
    short value1_len;
    char  data[0]; //value_len + 1 + value1_len
} kv_value_msg_t;

// Структура сообщения для установки пары ключ-значение
typedef struct kv_set_msg_s {
    short key_len;
    short value_len;
    char    data[0]; //key_len + 1 + value_len + 1
} kv_set_msg_t;

//get    KEY
//delete KEY
//search VALUE
// Структура сообщения для ключа
typedef struct kv_key_msg_s {
    short key_len;
    short pad;
    char data[0]; //key_len + 1
} kv_key_msg_t;
// Функция чтения n байтов из сокета
static inline int kv_readn(int fd, void *buf, int n)
{
	int	nleft;
	int	nread;
	char	*ptr;

	ptr = (char *)buf;
	nleft = n;

	while (nleft > 0) {
		if ((nread = read(fd, ptr, nleft)) < 0) {
			if (errno == EINTR || errno == EAGAIN) {
				nread = 0;		
			} else {
				return 0;
			}
		} else if (nread == 0) {
			return -1;				
        }

		nleft -= nread;
		ptr   += nread;
	}

	return (n - nleft);
}
// Функция записи n байтов в сокет
static inline int kv_writen(int fd, void *buf, int n)
{
	int		nleft;
	int		nwritten;
	char	*ptr;

	ptr = (char *)buf;
	nleft = n;

	while (nleft > 0) {
		if ((nwritten = write(fd, ptr, nleft)) <= 0) {
			if (nwritten < 0 && errno == EINTR) {
				nwritten = 0;		
			} else {
				perror("writen error");
				return -1;			
			}
		}

		nleft -= nwritten;
		ptr   += nwritten;
	}

	return n;
}
// Функция чтения сообщения из сокета
static inline int kv_read_msg(int fd, kv_value_msg_t **msg)
{
    int size;
	unsigned short left_len;
	kv_head_msg_t *msg_hdr;
	//unsigned short msg_type;
    kv_value_msg_t *ret_msg;
    char r_buf[KV_HDR_LEN + 1] = {0};
    

	size = kv_readn(fd, r_buf, KV_HDR_LEN);
	if (size <= 0) {
		printf("kv read size 0, client fd over, i am to over\n");
		return 0;/* EOF */
	}

	msg_hdr = (kv_head_msg_t *)r_buf;
	//msg_type = ntohs(msg_hdr->type);
    left_len = ntohs(msg_hdr->len);   
	if (left_len) {
        ret_msg = malloc(sizeof(kv_value_msg_t) + left_len);
        memset(ret_msg, 0x00, sizeof(kv_value_msg_t) + left_len);
		size = kv_readn(fd, ret_msg, left_len);
		if (size <= 0) {
			printf("kv read size 0, client fd over, i am to over\n");
			return -1;
		} else if (size != left_len) {
			printf("kv read size %d error\n", size);
			return -1;
		}

        *msg = ret_msg;
        return left_len - (int)sizeof(kv_value_msg_t);
	}

	return left_len;
}

static inline int kv_send_data(int fd, unsigned short type, void *data, unsigned int data_len)
{
	int buf_len, ret;
    char *buf;
    kv_head_msg_t *msg_hdr;
	char __data_buf[KV_HDR_LEN] = {0};

	if (data && data_len) {
		buf_len = data_len + KV_HDR_LEN + 1;
    	buf = (char *)malloc(buf_len);
    	if (buf == NULL) {
        	KV_DEBUG("send buf out of memory\n");
        	return -1;
    	}
    	memset(buf, 0x00, buf_len);
	} else {
		buf = __data_buf;
	}

    msg_hdr = (kv_head_msg_t *)buf;
    msg_hdr->type = htons(type);
	msg_hdr->len = htons(data_len);

	if (data && data_len) {
		memcpy(buf + KV_HDR_LEN, data, data_len);
	}

    ret = kv_writen(fd, buf, data_len + KV_HDR_LEN);
	KV_DEBUG("send fd %d type %d len %d, writen %d\n", fd, type, data_len, ret);

	if (data && data_len) {
    	free(buf);
	}

    return ret;
}

extern void kvdb_init(void);
void kvdb_set_key_value(char *key, int key_len, char *value, int value_len);
int kvdb_get_key_value(char *key, char **value, int *value_len);
int kvdb_del_key_value(char *key);
int kvdb_get_value_key(char *value, char **key, int *key_len);

#endif /* __KV_DB_H__ */

