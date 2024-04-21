#include "user_interaction.h"

extern ClientTransferState clientTransferState;

int parseCommand(char* input, ConsoleCommand* cmd) {
    char* token = strtok(input, " \n");
    if (token == NULL) {
        printf("No command entered.\n");
        return 0;
    }
    memset(cmd, 0, sizeof(ConsoleCommand));
    cmd->command_type = CMD_UNKNOWN;

    if (strcmp(token, "find") == 0) cmd->command_type = FIND_REQUEST;
    else if (strcmp(token, "set") == 0) cmd->command_type = SET_REQUEST;
    else if (strcmp(token, "update") == 0) cmd->command_type = UPDATE_REQUEST;
    else if (strcmp(token, "find_less") == 0) cmd->command_type = FIND_LESS_REQUEST;
    else if (strcmp(token, "find_more") == 0) cmd->command_type = FIND_MORE_REQUEST;
    else if (strcmp(token, "delete") == 0) cmd->command_type = DELETE_REQUEST;
    else if (strcmp(token, "help") == 0) cmd->command_type = HELP_COMMAND;
    else if (strcmp(token, "exit") == 0) cmd->command_type = EXIT_COMMAND;

    while (token != NULL) {
        token = strtok(NULL, " \n");
        if (token == NULL) break;

        if (strcmp(token, "-f") == 0) {
            token = strtok(NULL, " \n");
            if (token == NULL) {
                printf("-f flag provided but no file specified.\n");
                return 0;
            }
            if (!cmd->key) {
                cmd->key = strdup(token);
                cmd->key_file = 1;
            }
            else if (!cmd->value) {
                cmd->value = strdup(token);
                cmd->value_file = 1;
            }
        }
        else if (strcmp(token, ">") == 0) {
            cmd->redirect = 1;
            token = strtok(NULL, " \n");
            if (token == NULL) {
                printf("> flag provided but no output file specified.\n");
                return 0;
            }
            cmd->output_file = strdup(token);
        }
        else if (!cmd->key) {
            cmd->key = strdup(token);
        }
        else if (!cmd->value && !cmd->redirect) {
            cmd->value = strdup(token);
        }
    }
    return 1;
}

void printCommand(const ConsoleCommand* cmd) {
    const char* commandTypeStr;
    switch (cmd->command_type) {
        case FIND_REQUEST:    commandTypeStr = "find"; break;
        case SET_REQUEST:  commandTypeStr = "set"; break;
        case UPDATE_REQUEST:  commandTypeStr = "update"; break;
        case FIND_LESS_REQUEST:  commandTypeStr = "find_less"; break;
        case FIND_MORE_REQUEST:  commandTypeStr = "find_more"; break;
        case DELETE_REQUEST:  commandTypeStr = "delete"; break;
        case HELP_COMMAND:    commandTypeStr = "help"; break;
        default:          commandTypeStr = "unknown"; break;
    }

    printf("Command Type: %s\n", commandTypeStr);
    printf("Key: %s\n", cmd->key ? cmd->key : "NULL");
    printf("Key is File: %d\n", cmd->key_file);
    printf("Value: %s\n", cmd->value ? cmd->value : "NULL");
    printf("Value is File: %d\n", cmd->value_file);
    printf("Redirect: %d\n", cmd->redirect);
    printf("Output File: %s\n", cmd->output_file ? cmd->output_file : "NULL");
}

long getFileSize(const char* filename){
    FILE * fp = fopen(filename, "rb");
    if(!fp){
        return -1;
    }
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fclose(fp);
    return size;
}

void displayHelp() {
    printf("Commands and their formats:\n");
    printf("Query Commands: find set update find_less find_more delete\n");
    printf("Other Commands: help exit\n\n");

    printf("Format of Query Commands:\n");
    printf("command [-f] key [-f] [value] [> output]\n");
    printf("Examples:\n");
    printf("  find name - Searches for the value associated with 'name'\n");
    printf("  find -f a.txt - Searches for the value with 'a.txt' content as the key\n");
    printf("  create name 1 - Creates a key-value pair with 'name' as key and '1' as value\n");
    printf("  create -f a.txt -f value.txt - Creates a KV where both key and value are contents of 'a.txt' and 'value.txt' respectively\n");
    printf("  find -f a.txt > result.txt - Searches for the value with 'a.txt' content as the key and saves it to 'result.txt'\n\n");

    printf("Other Commands:\n");
    printf("  help - Displays this help message including command formats and how to exit the program.\n");
    printf("  exit - Exits the program.\n");
}

void executeCommand(const ConsoleCommand* cmd){
    if(cmd->command_type==EXIT_COMMAND){
        exit(EXIT_SUCCESS);
    }
    if(cmd->command_type==HELP_COMMAND){
        displayHelp();
        return;
    }

    TransferCommand transferCommand;
    int32_t len=sizeof(transferCommand.totalLength)*3;
    char* header=serializeTransferData((void*)cmd,len,cmd->command_type);
    if(!header){
        fprintf(stdout,"cannot serialize ConsoleCommand\n");
        return;
    }
    for(int i=0;i<len+sizeof(PacketHeader);i++){
        printf("%d ",header[i]);
    }
    printf("\n");
    int bytes=sendSerializeData(&clientTransferState,header,len);
    fprintf(stdout,"send header byte count:%d\n",bytes);
    if(cmd->key_file){
        sendFileData(&clientTransferState,cmd->key);
    }else{
        if(cmd->key){
            sendRawData(&clientTransferState,cmd->key,strlen(cmd->key));
        }
    }

    if(cmd->value_file){
        sendFileData(&clientTransferState,cmd->value);
    }else{
        if(cmd->value){
            sendRawData(&clientTransferState,cmd->value,strlen(cmd->value));
        }
    }
}
void *user_interaction_thread(void *arg) {
    char message[MAX_INPUT_LENGTH];
    ConsoleCommand cmd;
    while(clientTransferState.connect) {
        fprintf(stdout,"client: ");
        fgets(message, MAX_INPUT_LENGTH, stdin);
        if(message[0]=='\n'){
            continue;
        }
        if(!parseCommand(message, &cmd)){
            fprintf(stdout,"Invalid Command\n");
            continue;
        }
        printCommand(&cmd);
        executeCommand(&cmd);

        if (cmd.key && cmd.key_file) free(cmd.key);
        if (cmd.value && cmd.value_file) free(cmd.value);
        if (cmd.output_file) free(cmd.output_file);
    }
    fprintf(stdout,"sendMsg thread exit normally\n");
    return NULL;
}
