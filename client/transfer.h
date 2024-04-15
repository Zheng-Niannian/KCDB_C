#ifndef KVFILESERVER_TRANSFER_H
#define KVFILESERVER_TRANSFER_H

#include"data_type.h"
#include<stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include"user_interaction.h"


typedef struct client_transfer_state {
    int socket;
    int32_t clientId;
    pthread_t threadId;
    int32_t messageType;
    int32_t unknownMessageLength;
    int32_t unknownMessageType;
    int32_t messageLength;
    int32_t receivedLength;
    int32_t messageId;
    char busy;
    char connect;
    char* buffer;
    FILE *fp;
    char filename_buffer[SOCKET_BUFFER_SIZE];
}ClientTransferState;

#define TRANSFER_MEMORY_BUFFER_MAX_SIZE     (2*SOCKET_BUFFER_SIZE)



void handle_message(char* data, int socket, int32_t count,ClientTransferState* state);

void initClient(void);

void *serializeTransferData(void* src, int32_t len, enum CommandType type);

int sendSerializeData(ClientTransferState*state,void *serializeData,int32_t beforeSerializeLength);

int sendFileData(ClientTransferState*state,const char* filename);

int sendRawData(ClientTransferState*state,const char*buffer,int32_t len);

PacketPayload deserializeActualData(ClientTransferState*state,void* header, int32_t len);



void cleanResources();

#endif //KVFILESERVER_TRANSFER_H
