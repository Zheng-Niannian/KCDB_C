#ifndef KVFILECLIENT_USER_INTERACTION_H
#define KVFILECLIENT_USER_INTERACTION_H

#include"transfer.h"
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include <stdint.h>

#define MAX_INPUT_LENGTH    (256)
#define CMD_UNKNOWN         (0)

typedef struct {
    uint32_t command_type;
    char* key;
    int key_file;
    char* value;
    int value_file;
    int redirect;
    char* output_file;
} ConsoleCommand;

long getFileSize(const char* filename);

#endif //KVFILECLIENT_USER_INTERACTION_H
