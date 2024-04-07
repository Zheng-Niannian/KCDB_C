#ifndef KVFILESERVER_TRANSFER_H
#define KVFILESERVER_TRANSFER_H

#include"data_type.h"
#include<stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

typedef struct client_transfer_state {
    int socket;
    uint32 clientId;
    pthread_t threadId;
    uint32 messageType;
    uint32 unknownMessageLength;
    uint32 unknownMessageType;
    uint32 messageLength;
    uint32 receivedLength;
    uint32 messageId;
    char busy;
    char connect;
    char* buffer;
}ClientTransferState;



void handle_message(char* data, int socket, uint32 count,ClientTransferState* state);

void initClient(void);

void *serializeTransferData(void* src, uint32 len, enum TransferPacketType type);

int sendSerializeData(ClientTransferState*state,void *serializeData,uint32 beforeSerializeLength);

PacketPayload deserializeActualData(ClientTransferState*state,void* header, uint32 len);



void cleanResources();

#endif //KVFILESERVER_TRANSFER_H
