#include "event_handler.h"

SocketHandlerList list;

void $__initHandlerList(struct socket_handler_list *it);
void $__handleMessage(struct socket_handler_list *it,ClientTransferState *state,PacketPayload *payload);

void initEventHandler(void){
    $__initHandlerList(&list);
}

void setDataHandler(int dataType, void (*callback)(ClientTransferState *state,PacketPayload *payload)) {
    if(list.handlers[dataType].exist){
        printf("target data type has already bind a handler\n");
        return;
    }
    SocketDataHandler handler={
            .exist=1,
            .callback=callback
    };
    printf("bind handler for dataType:%d\n",dataType);
    list.handlers[dataType]=handler;
}

void handleMessage(ClientTransferState *state,PacketPayload *payload){
    if(payload==NULL){
        fprintf(stdout,"payload must not be NULL\n");
        return;
    }
    printf("payload data type:%u\n",payload->dataType);
    list.handleMessage(&list,state,payload);
}

void $__initHandlerList(struct socket_handler_list *it){
    memset((void*)it->handlers,0,sizeof(SocketDataHandler)*SOCKET_DATA_HANDLER_SIZE);
    it->handleMessage=$__handleMessage;
}

void $__handleMessage(struct socket_handler_list *it,ClientTransferState *state,PacketPayload *payload){
    if(!it->handlers[payload->dataType].exist){
        printf("no suitable handler for dataType:%d,cancel handle operation.\n", payload->dataType);
        return;
    }
    it->handlers[payload->dataType].callback(state,payload);
}
