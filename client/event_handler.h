#ifndef KVFILESERVER_EVENT_HANDER_H
#define KVFILESERVER_EVENT_HANDER_H

#include "data_type.h"
#include <stdio.h>
#include<stdlib.h>
#include<string.h>
#include"transfer.h"


typedef struct {
    int exist;
    void (*callback)(ClientTransferState *state,PacketPayload *payload);
} SocketDataHandler;

typedef struct socket_handler_list {
    SocketDataHandler handlers[SOCKET_DATA_HANDLER_SIZE];
    void (*init)(struct socket_handler_list *this);
    void (*handleMessage)(struct socket_handler_list *this,ClientTransferState *state,PacketPayload *payload);
} SocketHandlerList;

void initEventHandler(void);

void setDataHandler(int dataType, void (*callback)(ClientTransferState *state,PacketPayload *payload));

void handleMessage(ClientTransferState *state,PacketPayload *payload);

#endif //KVFILESERVER_EVENT_HANDER_H
