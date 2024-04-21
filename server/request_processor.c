#include"request_processor.h"

art_tree $__art_tree;

void cleanupTransferCommandPayload(TransferCommandPayload* payload){
    if(payload->key!=NULL){
//        free(payload->key);
        payload->key=NULL;
        payload->valueLength=0;
    }
    if(payload->value!=NULL){
//        free(payload->value);
        payload->value=NULL;
        payload->valueLength=0;
    }
    payload->flag=0;
    free(payload);
}

TransferCommandPayload* resolveTransferCommand(ClientTransferState *state,PacketPayload *payload){
    TransferCommandPayload* transferCommandPayload=malloc(sizeof(TransferCommandPayload));
    if(!transferCommandPayload){
        log_error("cannot malloc memory for transfer command payload,abort\n");
        return NULL;
    }
    memset(transferCommandPayload,0,sizeof(TransferCommandPayload));
    TransferCommand transferCommand;
    memset(&transferCommand,0,sizeof(TransferCommand));
    if(payload->fp){
        fclose(payload->fp);
        state->fp=NULL;

        FILE *fp=fopen(state->filename_buffer,"rb+");
        if(!fp){
            log_error("cannot load packet from file:%s\n",state->filename_buffer);
            memset(state->filename_buffer,0,SOCKET_BUFFER_SIZE);
            return transferCommandPayload;
        }
        memset(state->filename_buffer,0,SOCKET_BUFFER_SIZE);

        PacketHeader header;
        fread(&header,sizeof(PacketHeader),1,fp);
        fread(&transferCommand,sizeof(transferCommand.totalLength),3,fp);
        log_info("read transfer command from file,totalLength:%d, keyLength:%d, valueLength:%d\n"
                ,transferCommand.totalLength,transferCommand.keyLength,transferCommand.valueLength);
        if(transferCommand.keyLength>0){
            unsigned char *key_buffer=malloc(transferCommand.keyLength+1);
            memset(key_buffer,0,transferCommand.keyLength+1);
            if(!key_buffer){
                log_error("cannot allocate memory for key\n");
                return transferCommandPayload;
            }
            transferCommandPayload->keyLength=transferCommand.keyLength;
            transferCommandPayload->key=key_buffer;
            fread(key_buffer,1,transferCommand.keyLength,fp);
            log_info("key:%s\n",key_buffer);
//            free(key_buffer);
        }
        if(transferCommand.valueLength>0){
            unsigned char *value_buffer=malloc(transferCommand.valueLength+1);
            memset(value_buffer,0,transferCommand.keyLength+1);
            if(!value_buffer){
                log_error("cannot allocate memory for value\n");
                return transferCommandPayload;
            }
            transferCommandPayload->valueLength=transferCommand.valueLength;
            transferCommandPayload->value=value_buffer;
            fread(value_buffer, 1, transferCommand.valueLength, fp);
            log_info("value:%s\n",value_buffer);

//            free(value_buffer);
        }
        fclose(fp);
        fprintf(stdout,"finish process find_request from file\n");

    }
    else{
        transferCommand=*(TransferCommand *)payload->src;
        log_success("read transfer command from memory,totalLength:%d, keyLength:%d, valueLength:%d\n"
                ,transferCommand.totalLength,transferCommand.keyLength,transferCommand.valueLength);
        char* key=((char*)payload->src)+sizeof(transferCommand.totalLength)*3;
        if(transferCommand.keyLength>0){
            char *key_buffer=(char*)malloc(sizeof(char)*transferCommand.keyLength+1);
            if(!key_buffer){
                log_error("cannot allocate memory for key\n");
                return transferCommandPayload;
            }
            memset(key_buffer,0,transferCommand.keyLength+1);
            memcpy(key_buffer,key,transferCommand.keyLength);
            log_info("key:%s\n",key_buffer);
            transferCommandPayload->keyLength=transferCommand.keyLength;
            transferCommandPayload->key=key_buffer;
//            free(key_buffer);
        }
        char *value=((char*)payload->src)+sizeof(transferCommand.totalLength)*3+transferCommand.keyLength;
        if(transferCommand.valueLength>0){
            char *value_buffer=(char*)malloc(sizeof(char) * transferCommand.valueLength + 1);
            if(!value_buffer){
                log_error("cannot allocate memory for key\n");
                return transferCommandPayload;
            }
            transferCommandPayload->valueLength=transferCommand.valueLength;
            transferCommandPayload->value=value_buffer;
            memset(value_buffer, 0, transferCommand.valueLength + 1);
            memcpy(value_buffer, value, transferCommand.valueLength);
            log_info( "value:%s\n", value_buffer);
//            free(value_buffer);
        }

//        fprintf(stdout,"finish process find_request from memory\n");
    }
    transferCommandPayload->flag=1;
    log_success("Resolve Transfer Command:key:%s, value:%s\n",transferCommandPayload->key,transferCommandPayload->value);
    return transferCommandPayload;
}

void handleFindRequest(ClientTransferState *state,PacketPayload *payload){
    TransferCommandPayload* transferCommandPayload= resolveTransferCommand(state,payload);
    log_info("IN FUNCTION,ret=%p\n",transferCommandPayload);
    if(transferCommandPayload==NULL)return;

    if(!transferCommandPayload->flag){
        cleanupTransferCommandPayload(transferCommandPayload);
        log_error("cannot resolve transfer command payload,abort\n");
        return;
    }
    log_info("FLAG:TRUE\n");

    if(transferCommandPayload->key==NULL){
        log_error("In find request,field key should not be null\n");
        cleanupTransferCommandPayload(transferCommandPayload);
        return;
    }

    if(transferCommandPayload->value!=NULL){
        log_warning("In find request,field value should be null\n");
    }
    log_info("BEGIN_FIND");
    void* result=art_search(&$__art_tree,transferCommandPayload->key,transferCommandPayload->keyLength);

    log_success("FindRequest,key:%s,value:%s\n",transferCommandPayload->key,result);

//    cleanupTransferCommandPayload(transferCommandPayload);

}

void handleSetRequest(ClientTransferState *state,PacketPayload *payload){
    TransferCommandPayload* transferCommandPayload= resolveTransferCommand(state,payload);
    log_info("IN FUNCTION,ret=%p\n",transferCommandPayload);
    if(transferCommandPayload==NULL)return;
    if(!transferCommandPayload->flag){
        cleanupTransferCommandPayload(transferCommandPayload);
        log_error("cannot resolve transfer command payload,abort\n");
        return;
    }

    log_info("FLAG:TRUE\n");

    if(transferCommandPayload->key==NULL){
        log_error("In set request,field key should not be null\n");
        cleanupTransferCommandPayload(transferCommandPayload);
        return;
    }

    if(transferCommandPayload->value==NULL){
        log_error("In set request,field value should not be null\n");
        cleanupTransferCommandPayload(transferCommandPayload);
        return;
    }
    log_info("BEGIN_SET");
    void* result=art_insert_no_replace(&$__art_tree,transferCommandPayload->key,transferCommandPayload->keyLength,(void*)transferCommandPayload->value);

    log_success("SetRequest,key:%s,old value:%s,new value:%s\n",transferCommandPayload->key,result,(char*)transferCommandPayload->value);


//    cleanupTransferCommandPayload(transferCommandPayload);
}

void handleUpdateRequest(ClientTransferState *state,PacketPayload *payload){
    TransferCommandPayload* transferCommandPayload= resolveTransferCommand(state,payload);
    log_info("IN FUNCTION,ret=%p\n",transferCommandPayload);
    if(transferCommandPayload==NULL)return;
    if(!transferCommandPayload->flag){
        cleanupTransferCommandPayload(transferCommandPayload);
        log_error("cannot resolve transfer command payload,abort\n");
        return;
    }

    log_info("FLAG:TRUE\n");

    if(transferCommandPayload->key==NULL){
        log_error("In update request,field key should not be null\n");
        cleanupTransferCommandPayload(transferCommandPayload);
        return;
    }

    if(transferCommandPayload->value==NULL){
        log_error("In update request,field value should not be null\n");
        cleanupTransferCommandPayload(transferCommandPayload);
        return;
    }
    log_info("BEGIN_UPDATE");
    void* result=art_insert(&$__art_tree,transferCommandPayload->key,transferCommandPayload->keyLength,(void*)transferCommandPayload->value);
    log_success("UpdateRequest,key:%s,old value:%s,new value:%s\n",transferCommandPayload->key,result,transferCommandPayload->value);
//    cleanupTransferCommandPayload(transferCommandPayload);
}

void handleFindLessRequest(ClientTransferState *state,PacketPayload *payload){
    TransferCommandPayload* transferCommandPayload= resolveTransferCommand(state,payload);
    log_info("IN FUNCTION,ret=%p\n",transferCommandPayload);
    if(transferCommandPayload==NULL)return;
    if(!transferCommandPayload->flag){
        cleanupTransferCommandPayload(transferCommandPayload);
        log_error("cannot resolve transfer command payload,abort\n");
        return;
    }

    log_info("FLAG:TRUE\n");

    if(transferCommandPayload->key==NULL){
        log_error("In find less request,field key should not be null\n");
        cleanupTransferCommandPayload(transferCommandPayload);
        return;
    }

    if(transferCommandPayload->value!=NULL){
        log_warning("In find less request,field value should be null\n");
    }

    log_info("BEGIN_FIND_LESS");
    void* result=art_find_less(&$__art_tree,transferCommandPayload->key,transferCommandPayload->keyLength);

    log_success("FindLessRequest,key:%s,value:%s\n",transferCommandPayload->key,result,result);

//    cleanupTransferCommandPayload(transferCommandPayload);
}


void handleFindMoreRequest(ClientTransferState *state,PacketPayload *payload){

    TransferCommandPayload *transferCommandPayload= resolveTransferCommand(state,payload);
    log_info("IN FUNCTION,ret=%p\n",transferCommandPayload);
    if(transferCommandPayload==NULL)return;
    if(!transferCommandPayload->flag){
        cleanupTransferCommandPayload(transferCommandPayload);
        log_error("cannot resolve transfer command payload,abort\n");
        return;
    }

    log_info("FLAG:TRUE\n");

    if(transferCommandPayload->key==NULL){
        log_error("In find more request,field key should not be null\n");
        cleanupTransferCommandPayload(transferCommandPayload);
        return;
    }

    if(transferCommandPayload->value!=NULL){
        log_warning("In find more request,field value should be null\n");

    }
    log_info("BEGIN_FIND_MORE");
    void* result=art_find_more(&$__art_tree,transferCommandPayload->key,transferCommandPayload->keyLength);
    log_success("FindMoreRequest,key:%s,value:%s\n",transferCommandPayload->key,result,result);

//    cleanupTransferCommandPayload(transferCommandPayload);
}

void handleDeleteRequest(ClientTransferState *state,PacketPayload *payload){
    TransferCommandPayload* transferCommandPayload= resolveTransferCommand(state,payload);
    log_info("IN FUNCTION,ret=%p\n",transferCommandPayload);
    if(transferCommandPayload==NULL)return;
    if(!transferCommandPayload->flag){
        cleanupTransferCommandPayload(transferCommandPayload);
        log_error("cannot resolve transfer command payload,abort\n");
        return;
    }

    log_info("FLAG:TRUE\n");

    if(transferCommandPayload->key==NULL){
        log_error("In delete request,field key should not be null\n");
        cleanupTransferCommandPayload(transferCommandPayload);
        return;
    }

    if(transferCommandPayload->value!=NULL){
        log_warning("In delete request,field value should be null\n");
    }
    log_info("BEGIN_DELETE");
    void* result=art_delete(&$__art_tree,transferCommandPayload->key,transferCommandPayload->keyLength);
    log_success("DeleteRequest,key:%s,value:%s\n",transferCommandPayload->key,result,result);

//    cleanupTransferCommandPayload(transferCommandPayload);
}

void initRequestProcessor(void){
    art_tree_init(&$__art_tree);
    setDataHandler(FIND_REQUEST,handleFindRequest);
    setDataHandler(SET_REQUEST,handleSetRequest);
    setDataHandler(UPDATE_REQUEST,handleUpdateRequest);
    setDataHandler(FIND_LESS_REQUEST,handleFindLessRequest);
    setDataHandler(FIND_MORE_REQUEST,handleFindMoreRequest);
    setDataHandler(DELETE_REQUEST,handleDeleteRequest);
}
