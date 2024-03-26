#include "kvdb.h"
/*Управляйте информацией о клиенте.*/
typedef struct kv_server_manage_s {
    int fd;
    int port;
    pthread_mutex_t mutex;
    struct epollop server_eop;
} kv_server_manage_t;

/*Управляйте информацией о сервере.*/
typedef struct kv_client_s {
    int client_fd;
    pthread_t pid;
    struct epollop client_eop;
} kv_client_t;

kv_server_manage_t g_kv_server_m;
/*Функция для обработки сообщения «set», используемого для обработки сообщения об установке пар ключ-значение,
отправленного клиентом, и сохранения пар ключ-значение в базе данных. */
static void kv_do_set_msg(char *msg)
{
    kv_set_msg_t *set_msg = (kv_set_msg_t *)msg;
    short key_len = ntohs(set_msg->key_len);
    short value_len = ntohs(set_msg->value_len);
    char *msg_data = set_msg->data;
    char *key = msg_data;
    char *value = msg_data + key_len + 1;

    KV_DEBUG("kv set msg: key_len %d, value len %d\n", key_len, value_len);
    KV_DEBUG("key[%s], value[%s]\n", key, value);

    kvdb_set_key_value(key, key_len, value, value_len);
    
    return;
}

/*Получаем сообщение «get», отправленное клиентом, и выполняем соответствующие операции обработки*/
static void kv_do_get_msg(int fd, char *msg)
{
    kv_key_msg_t *set_msg = (kv_key_msg_t *)msg;
    short key_len = ntohs(set_msg->key_len);
    char *msg_data = set_msg->data;
    char *key = msg_data;

    KV_DEBUG("kv get msg: key_len %d\n", key_len);
    KV_DEBUG("key[%s]\n", key);

    char *value = NULL;
    int value_len = 0, len;

    kv_value_msg_t *vmsg;
    len = sizeof(kv_value_msg_t);
    
    int ret = kvdb_get_key_value(key, &value, &value_len);
    if (ret == -1) {
        ;
    } else {
        len += value_len;
    }
    vmsg = malloc(len);
    memset(vmsg, 0x00, len);
    vmsg->value_len = htons(value_len);
    memcpy(vmsg->data, value, value_len);
    free(value);

    kv_send_data(fd, KV_MSG_TYPE_RET, vmsg, len);
    free(vmsg);
    
    return;
}
/*Используется для удаления соответствующей пары ключ-значение из базы данных на основе ключа.*/
static void kv_do_del_msg(int fd, char *msg)
{
    kv_key_msg_t *set_msg = (kv_key_msg_t *)msg;
    short key_len = ntohs(set_msg->key_len);
    char *msg_data = set_msg->data;
    char *key = msg_data;

    KV_DEBUG("kv del msg: key_len %d\n", key_len);
    KV_DEBUG("key[%s]\n", key);

    char *value = NULL;
    int value_len = 0, len;

    int ret = kvdb_del_key_value(key);
    if (ret == -1) {
        KV_DEBUG("not find key %s\n", key);
    } else {
        printf("delete key %s success\n", key);
    }
    
    return;
}

static void kv_do_search_msg(int fd, char *msg)
{
    kv_key_msg_t *set_msg = (kv_key_msg_t *)msg;
    short value_len = ntohs(set_msg->key_len);
    char *msg_data = set_msg->data;
    char *value = msg_data;

    KV_DEBUG("kv search msg: value_len %d\n", value_len);
    KV_DEBUG("value[%s]\n", value);

    char *key = NULL;
    int key_len = 0, len;

    kv_value_msg_t *vmsg;
    len = sizeof(kv_value_msg_t);
    
    int ret = kvdb_get_value_key(value, &key, &key_len);
    if (ret == -1) {
        ;
    } else {
        len += key_len;
    }
    vmsg = malloc(len);
    memset(vmsg, 0x00, len);
    vmsg->value_len = htons(key_len);
    memcpy(vmsg->data, key, key_len);
    free(value);

    kv_send_data(fd, KV_MSG_TYPE_RET, vmsg, len);
    free(vmsg);
    
    return;
}

static int kv_thread_do_client_msg(int fd, kv_client_t *client)
{
    int size, ret, count;
	unsigned short left_len;
    char *r_buf;
    char __r_buf[KV_MSG_MAX] = {0};
    char h_buf[KV_HDR_LEN + 1] = {0};
	kv_head_msg_t *msg_hdr;
	unsigned short type;

    /* Read header */
    size = kv_readn(fd, h_buf, KV_HDR_LEN);
    if (size == -1) {
		KV_DEBUG("kv tcp read size 0, client fd over, i am to over\n");
		kv_epoll_delfd(client->client_eop.epfd, fd);
		return 1;/* EOF */
	} else if (size != KV_HDR_LEN) {
		KV_DEBUG("kv read size %d error\n", size);
		return 0;
	}

	msg_hdr = (kv_head_msg_t *)h_buf;
	type = ntohs(msg_hdr->type);
    left_len = ntohs(msg_hdr->len);
	KV_DEBUG("\nkv fd %d get msg type %d, left len %u\n", fd, type, left_len);

    r_buf = __r_buf;
	size = kv_readn(fd, r_buf, left_len);
	if (size == -1) {
		kv_epoll_delfd(client->client_eop.epfd, fd);
		KV_DEBUG("kv read size 0, client fd over, i am to over\n");
		return 1;/* EOF */
	} else if (size != left_len) {
		KV_DEBUG("kv read size %d error\n", size);
		return 0;
	}

	/* do client msg */
	switch (type) {
    case KV_MSG_TYPE_SET:
        kv_do_set_msg(r_buf);
        break;

    case KV_MSG_TYPE_GET:
        kv_do_get_msg(fd, r_buf);
        break;

    case KV_MSG_TYPE_DEL:
        kv_do_del_msg(fd, r_buf);
        break;

    case KV_MSG_TYPE_LOOK:
        kv_do_search_msg(fd, r_buf);
        break;
        
	default:
		break;
	}

	return 0;
}

static void *kv_thread_work_running(void *param)
{
    int ret, i, sockfd, do_ret;
    kv_client_t *client = (kv_client_t *)param;

    KV_DEBUG("new client thread running..........\n");

	pthread_detach(pthread_self());
    (void)kv_epoll_init(&client->client_eop, 32);
    kv_epoll_addfd(client->client_eop.epfd, client->client_fd);

	while (1) {
        /* epoll waits for messages from the client */
        ret = epoll_wait(client->client_eop.epfd, client->client_eop.events,
                    client->client_eop.nevents, -1);
        if (ret < 0) {
            KV_DEBUG("client %d thread epoll wait failure\n", client->client_fd);
            break;
        }

        for (i = 0; i < ret; i++ ) {
            sockfd = client->client_eop.events[i].data.fd;
            /* Client fd can read and process client messages */
            if (client->client_eop.events[i].events & EPOLLIN) {
                do_ret = kv_thread_do_client_msg(sockfd, client);
                if (do_ret == 1) {
                    goto __end;
                }
            } else {
                KV_DEBUG("client %d something event happened ERROR\n", sockfd);
                return NULL;
            }
        }
    }

__end:
	KV_DEBUG("kv client thread %ld end\n", client->pid);
    kv_epoll_delfd(client->client_eop.epfd, client->client_fd);
    close(client->client_fd);
    free(client);
    return NULL;
}

void kv_server_epoll(struct epoll_event *events, int number, int epollfd, int tcp_listenfd)
{
	int i, addr_len;
	int sockfd;
	int connfd;
	unsigned short port;
	struct sockaddr_in peer;
	socklen_t client_addrlength;
	char ipaddr[INET_ADDRSTRLEN] = { 0 };
    kv_client_t *client;

    for (i = 0; i < number; i++ ) {
        sockfd = events[i].data.fd;
        if (events[i].events & EPOLLIN) {
            if (sockfd == tcp_listenfd) {
                client_addrlength = sizeof(peer);

                /* Client connection event, call accept to handle the new client */
                connfd = accept(tcp_listenfd, (struct sockaddr*)&peer, &client_addrlength);
        		port = ntohs(peer.sin_port);

        		memset(ipaddr, 0x00, INET_ADDRSTRLEN);
        		(void)inet_ntop(AF_INET, &peer.sin_addr, ipaddr, INET_ADDRSTRLEN);

                KV_DEBUG("kv server recv a new tcp_client %d, ip %s, port %u\n", connfd, ipaddr, port);

                client = (kv_client_t *)malloc(sizeof(kv_client_t));
                memset(client, 0x00, sizeof(kv_client_t));
                client->client_fd = connfd;
                pthread_create(&client->pid, NULL, kv_thread_work_running, client);
            }
        } else {
            KV_DEBUG("something else happened \n" );
        }
    }

	return;
}

/*Инициализируем TCP-сокет сервера, привязываем его к указанному порту, а затем начинаем прослушивать запросы на соединение. */
static int kv_server_tcp_socket_init(int port)
{
    int ret;
    struct sockaddr_in server_addr;
	struct epoll_event event;

    if ((g_kv_server_m.fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket error");
		return -1;
	}

	memset(&server_addr, 0x00, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	ret = bind(g_kv_server_m.fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
	if (ret < 0){
		perror("bind error\n");
		return -1;
	}

	ret = listen(g_kv_server_m.fd, 200);
	if (ret < 0){
		perror("listen error\n");
		return -1;
	}
	memset(&event, 0, sizeof(event));
    event.data.fd = g_kv_server_m.fd;
    event.events = EPOLLIN;

    ret = epoll_ctl(g_kv_server_m.server_eop.epfd, EPOLL_CTL_ADD, g_kv_server_m.fd, &event);
    if (ret < 0){
		perror("epoll_ctl error\n");
		return -1;
	}

    //kv_epoll_addfd(g_kv_server_m.server_eop.epfd, g_kv_server_m.fd);

    return 0;
}

int main(int argc, char *argv[])
{
    int ret, port;

	if (argc != 2) {
        printf("usge: %s server_port\n", argv[0]);
        return 0;
    }

    g_kv_server_m.port = atoi(argv[1]);
    port = g_kv_server_m.port;

    if (kv_epoll_init(&g_kv_server_m.server_eop, 32) == -1) {
        return -1;
    }

    if (kv_server_tcp_socket_init(port) == -1) {
        return -1;
    }

    kvdb_init();
    pthread_mutex_init(&g_kv_server_m.mutex, NULL);
	KV_DEBUG("Server Started on port %d, Accepting connections\n", port);

	while (1) {
        ret = epoll_wait(g_kv_server_m.server_eop.epfd, g_kv_server_m.server_eop.events,
                    g_kv_server_m.server_eop.nevents, -1);
        if (ret < 0 ) {
            KV_DEBUG( "epoll wait failure\n" );
            break;
        }else if (ret == 0) {
            continue;
        }

        kv_server_epoll(g_kv_server_m.server_eop.events, ret, g_kv_server_m.server_eop.epfd,
            g_kv_server_m.fd);
    }

    return 0;
}

