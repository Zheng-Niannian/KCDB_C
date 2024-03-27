
#include "kvdb.h"

typedef struct kvc_client_manage_s {
    int fd;// Файловый дескриптор сокета
    struct sockaddr_in serveraddr;// Адрес сервера
} kvc_client_manage_t;

kvc_client_manage_t g_kvc_client_m;

/*Инициализируем TCP-сокет клиента и подключаемся к указанному IP-адресу и порту сервера. */
int kvc_client_tcp_socket_init(char *server_ip, unsigned short port)
{
    struct hostent *hp;

    if ((g_kvc_client_m.fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket error");
        return -1;
    }// Создание сокета

    memset(&g_kvc_client_m.serveraddr, 0, sizeof (g_kvc_client_m.serveraddr));
    g_kvc_client_m.serveraddr.sin_family = AF_INET;
    g_kvc_client_m.serveraddr.sin_port = htons(port);//Инициализация структуры адреса сервера
	
	/*Преобразование IP-адресов в порядок сетевых байтов*/
    if (inet_aton(server_ip, &g_kvc_client_m.serveraddr.sin_addr) == 0) {
        hp = gethostbyname(server_ip);
        if (hp == NULL) {
            perror("gethostbyname");
            return -1;
        }
        g_kvc_client_m.serveraddr.sin_addr = *(struct in_addr *)hp->h_addr;
    }

    if (connect(g_kvc_client_m.fd, (struct sockaddr *)&g_kvc_client_m.serveraddr, 
            sizeof(g_kvc_client_m.serveraddr)) == -1) {
        perror("connect");
        return -1;
    }

    KV_DEBUG("KV client Connected TCP to %s, port %u.\n", server_ip, port);

    return g_kvc_client_m.fd;
}

int kvc_getlinestr(char s[], int lim)
{
 	int c, i;

	memset(s, 0x00, lim);// Очистка строки

 	for (i = 0; i < lim-1 && (c = getchar()) != EOF && c != '\n'; ++i)// Чтение строки из ввода
 		s[i] = c;

 	//if (c == '\n') {
 	//	s[i] = c;
 	//	++i;
 	//}

 	s[i] = '\0';// Установка нулевого символа в конце строки

	return i;
}

static void kvc_help(void)
{
    printf("-------------------SUPPORT CMD ----------------------\n");
    printf("set/SET KEY VALUE     : set KEY VALUE, VALUE is a string or integer, converted by the user\n"
           "get/GET KEY           : get value for KEY\n"
           "delete/DELETE KEY     : delete KEY\n"
           "search/SEARCH VALUE   : Returns the KEY of the closest smaller and larger values\n"
           "quit/QUIT             : QUIT and exit\n");
    printf("input your cmd:\n");
    printf("-----------------------------------------------\n");

    return;
}

static void kvc_set_key_value(int fd, char *key, int key_len, char *value, int value_len)
{
    printf("kvc_set_key_value: set %s(%d) %s(%d)\n", key, key_len, value, value_len);
    
    int msg_len = sizeof(kv_set_msg_t) + key_len + value_len  + 2;
    kv_set_msg_t *msg = malloc(msg_len);
    memset(msg, 0x00, msg_len);
    msg->key_len = htons(key_len);
    msg->value_len = htons(value_len);

    int offest = 0;
    memcpy(msg->data + offest, key, key_len);

    offest = key_len + 1;
    memcpy(msg->data + offest, value, value_len);

    kv_send_data(fd, KV_MSG_TYPE_SET, msg, msg_len);
    free(msg);

    return;
}

static void kvc_del_key_value(int fd, char *key, int key_len)
{
    int msg_len = sizeof(kv_key_msg_t) + key_len + 1;
    kv_key_msg_t *msg = malloc(msg_len);
    memset(msg, 0x00, msg_len);
    msg->key_len = htons(key_len);

    int offest = 0;
    memcpy(msg->data, key, key_len);

    kv_send_data(fd, KV_MSG_TYPE_DEL, msg, msg_len);
    free(msg);

    return;
}

static void kvc_get_key_value(int fd, char *key, int key_len)
{
    int msg_len = sizeof(kv_key_msg_t) + key_len + 1;
    kv_key_msg_t *msg = malloc(msg_len);
    memset(msg, 0x00, msg_len);
    msg->key_len = htons(key_len);

    int offest = 0;
    memcpy(msg->data, key, key_len);

    kv_send_data(fd, KV_MSG_TYPE_GET, msg, msg_len);
    free(msg);

    kv_value_msg_t *value_msg;
    int ret = kv_read_msg(fd, &value_msg);
    if (ret <= 0) {
        printf("key:%s, not get value:\n", key);
        return;
    }

    short value_len = ntohs(value_msg->value_len);
    printf("get KEY:%s\n", key);
    printf("value len %d, data:[%s]\n", value_len, value_msg->data);
    free(value_msg);
    
    return;
}
/*Поиск ключа, соответствующего определенному значению*/
static void kvc_search_key_value(int fd, char *value, int value_len)
{
    int msg_len = sizeof(kv_key_msg_t) + value_len + 1;
    kv_key_msg_t *msg = malloc(msg_len);
    memset(msg, 0x00, msg_len);
    msg->key_len = htons(value_len);

    int offest = 0;
    memcpy(msg->data, value, value_len);

    kv_send_data(fd, KV_MSG_TYPE_LOOK, msg, msg_len);
    free(msg);

    kv_value_msg_t *value_msg;
    int ret = kv_read_msg(fd, &value_msg);
    if (ret <= 0) {
        printf("value:%s, not find key:\n", value);
        return;
    }

    short key_len = ntohs(value_msg->value_len);
    printf("get value:%s\n", value);
    printf("key len %d, key:[%s]\n", key_len, value_msg->data);
    free(value_msg);
    
    return;
}

int main(int argc, char *argv[])
{
    int port, ret;
    char *server_ip, *str, *str_begin;

	if (argc != 3) {
        printf("usge: %s server_ip server_port\n", argv[0]);
        return 0;
    }

    server_ip = argv[1];
    port = atoi(argv[2]);
    
    ret = kvc_client_tcp_socket_init(server_ip, port);
    if (ret == -1) {
        return -1;
    }

    char *key, *value;
    int key_len, value_len;
    char cmd[512] = {0};
    int len;
    
    while (1) {
        kvc_help();
        ret = kvc_getlinestr(cmd, 512);
        if (ret == -1  && len == 0) {
            return 0;
        }
        if (len == 0) {
            continue;
        }
        
        str = cmd;
        //skip 
        while (*str != 0 && strchr(" ", *str))
                str++;
        if (*str == 0) {
            printf("%s input error\n", cmd);
            continue;
        }
        if (strncasecmp(str, "quit", 4) == 0) {
            printf("GoodBye\n");
            exit(0);
        }

        if (strncasecmp(str, "set", 3) == 0) {
            str += 3;
            while (*str != 0 && strchr(" ", *str))
                str++;
            if (*str == 0) {
                printf("%s input error\n", cmd);
                continue;
            }
            
            //KEY
            str_begin = str;
            while (*str != 0 && !strchr(" ", *str))
                str++;
            key = str_begin;
            key_len = str - str_begin;
            *str++ = 0;

            
            while (*str != 0 && strchr(" ", *str))
                str++;
            if (*str == 0) {
                printf("%s input error\n", cmd);
                continue;
            }
            str_begin= str;
            while (*str != 0 && !strchr(" ", *str))
                str++;
            //VALUE
            value = str_begin;
            value_len = str - str_begin;
            printf("set %s(%d) %s(%d)\n", key, key_len, value, value_len);
            kvc_set_key_value(g_kvc_client_m.fd, key, key_len, value, value_len);
        }else if (strncasecmp(str, "get", 3) == 0) {
            str += 3;
            while (*str != 0 && strchr(" ", *str))
                str++;
            if (*str == 0) {
                printf("%s input error\n", cmd);
                continue;
            }
            //KEY
            str_begin = str;
            while (*str != 0 && !strchr(" ", *str))
                str++;
            key = str_begin;
            key_len = str - str_begin;
            *str++ = 0;

            printf("get %s, len %d\n", key, key_len);
            kvc_get_key_value(g_kvc_client_m.fd, key, key_len);
        } else if (strncasecmp(str, "delete", 6) == 0) {
            str += 6;
            while (*str != 0 && strchr(" ", *str))
                str++;
            if (*str == 0) {
                printf("%s input error\n", cmd);
                continue;
            }
            
            //KEY
            str_begin = str;
            while (*str != 0 && !strchr(" ", *str))
                str++;
            key = str_begin;
            key_len = str - str_begin;
            *str++ = 0;

            printf("delete %s, len %d\n", key, key_len);
            kvc_del_key_value(g_kvc_client_m.fd, key, key_len);
        } else if (strncasecmp(str, "search", 6) == 0) {
            str += 6;
            while (*str != 0 && strchr(" ", *str))
                str++;
            if (*str == 0) {
                printf("%s input error\n", cmd);
                continue;
            }
            
            //VALUE
            str_begin = str;
            while (*str != 0 && !strchr(" ", *str))
                str++;
            value = str_begin;
            value_len = str - str_begin;
            *str++ = 0;

            printf("search %s, value_len %d\n", value, value_len);
            kvc_search_key_value(g_kvc_client_m.fd, value, value_len);
        }
    }

    return 0;
}

