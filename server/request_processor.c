#include"request_processor.h"

//void handleQueryKeyResult(ClientTransferState *state,PacketPayload *payload){
//    QueryKeyResult *result=(QueryKeyResult*)payload->src;
//    char *resultBuffer=(char*)malloc(result->valueLength+1);
//    if(!resultBuffer){
//        fprintf(stdout,"unable to allocate memory for QueryKeyResult.value\n");
//        return;
//    }
//    memset(resultBuffer,0,result->valueLength+1);
//    char *dest= (char *) &result->valueLength;
//    memcpy(resultBuffer,dest+sizeof(result->valueLength),result->valueLength);
//    fprintf(stdout,"QueryKeyResult.value:%s\n",resultBuffer);
//
//
//    free(resultBuffer);
//}
//
//void handleQueryKeyRequest(ClientTransferState *state,PacketPayload *payload){
//    QueryKeyRequest *request=(QueryKeyRequest *)payload->src;
//    char *resultBuffer=(char*)malloc(request->keyLength + 1);
//    if(!resultBuffer){
//        fprintf(stdout,"unable to allocate memory for QueryKeyResult.value\n");
//        return;
//    }
//    memset(resultBuffer, 0, request->keyLength + 1);
//    char *dest= (char *) &request->keyLength;
//    memcpy(resultBuffer, dest+sizeof(request->keyLength), request->keyLength);
//    fprintf(stdout,"QueryKeyRequest.value:%s\n",resultBuffer);
//
//    char returnBuffer[SOCKET_BUFFER_SIZE]={0};
//    sprintf(returnBuffer,"server_received_query_key_request:%s",resultBuffer);
//    QueryKeyResult result={
//            strlen(returnBuffer),returnBuffer
//    };
//    int len=sizeof(result.valueLength)+result.valueLength;
//    char *data=serializeTransferData(&result,len,QUERY_KEY_RESULT);
//    sendSerializeData(state,data,len);
//
//    free(resultBuffer);
//    free(data);
//    fprintf(stdout,"server process QueryKeyRequestSuccess\n");
//}

void handleFindRequest(ClientTransferState *state,PacketPayload *payload){
    TransferCommand transferCommand;
    memset(&transferCommand,0,sizeof(TransferCommand));
    if(payload->fp){
        fclose(payload->fp);
        state->fp=NULL;

        FILE *fp=fopen(state->filename_buffer,"rb+");
        if(!fp){
            fprintf(stdout,"cannot load packet from file:%s\n",state->filename_buffer);
            memset(state->filename_buffer,0,SOCKET_BUFFER_SIZE);
            return;
        }
        memset(state->filename_buffer,0,SOCKET_BUFFER_SIZE);

        PacketHeader header;
        fread(&header,sizeof(PacketHeader),1,fp);
//        if(header.dataType!=FIND_REQUEST){
//            fprintf(stdout,"invalid data type:%d\n",header.dataType);
//            return;
//        }
        fread(&transferCommand,sizeof(transferCommand.totalLength),3,fp);
        fprintf(stdout,"read transfer command from file,totalLength:%d, keyLength:%d, valueLength:%d\n"
                ,transferCommand.totalLength,transferCommand.keyLength,transferCommand.valueLength);
        if(transferCommand.keyLength>0){
            char *key_buffer=malloc(transferCommand.keyLength+1);
            memset(key_buffer,0,transferCommand.keyLength+1);
            if(!key_buffer){
                fprintf(stdout,"cannot allocate memory for key\n");
                return;
            }
            fread(key_buffer,1,transferCommand.keyLength,fp);
            fprintf(stdout,"key:%s\n",key_buffer);
            free(key_buffer);
        }
        if(transferCommand.valueLength>0){
            char *value_buffer=malloc(transferCommand.valueLength+1);
            memset(value_buffer,0,transferCommand.keyLength+1);
            if(!value_buffer){
                fprintf(stdout,"cannot allocate memory for value\n");
                return;
            }
            fread(value_buffer, 1, transferCommand.valueLength, fp);
            fprintf(stdout,"value:%s\n",value_buffer);
            free(value_buffer);
        }
        fclose(fp);
        fprintf(stdout,"finish process find_request from file\n");

    }
    else{
        transferCommand=*(TransferCommand *)payload->src;
        fprintf(stdout,"read transfer command from memory,totalLength:%d, keyLength:%d, valueLength:%d\n"
                ,transferCommand.totalLength,transferCommand.keyLength,transferCommand.valueLength);
        char* key=((char*)payload->src)+sizeof(transferCommand.totalLength)*3;
        if(transferCommand.keyLength>0){
            char *key_buffer=(char*)malloc(sizeof(char)*transferCommand.keyLength+1);
            if(!key_buffer){
                fprintf(stdout,"cannot allocate memory for key\n");
            }
            memset(key_buffer,0,transferCommand.keyLength+1);
            memcpy(key_buffer,key,transferCommand.keyLength);
            fprintf(stdout,"key:%s\n",key_buffer);
            free(key_buffer);
        }
        char *value=((char*)payload->src)+sizeof(transferCommand.totalLength)*3+transferCommand.keyLength;
        if(transferCommand.valueLength>0){
            char *value_buffer=(char*)malloc(sizeof(char) * transferCommand.valueLength + 1);
            if(!value_buffer){
                fprintf(stdout,"cannot allocate memory for key\n");
            }
            memset(value_buffer, 0, transferCommand.valueLength + 1);
            memcpy(value_buffer, value, transferCommand.valueLength);
            fprintf(stdout, "value:%s\n", value_buffer);
            free(value_buffer);
        }

        fprintf(stdout,"finish process find_request from memory\n");
    }
}

void initRequestProcessor(void){
    setDataHandler(FIND_REQUEST,handleFindRequest);
    setDataHandler(SET_REQUEST,handleFindRequest);
    setDataHandler(UPDATE_REQUEST,handleFindRequest);
    setDataHandler(FIND_LESS_REQUEST,handleFindRequest);
    setDataHandler(FIND_MORE_REQUEST,handleFindRequest);
    setDataHandler(DELETE_REQUEST,handleFindRequest);
}
