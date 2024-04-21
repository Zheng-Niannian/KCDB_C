

#ifndef KVFILESERVER_LOGGER_H
#define KVFILESERVER_LOGGER_H

#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include <stdarg.h>

void init_logger();
void log_error(const char *__restrict _format, ...);
void log_warning(const char *__restrict _format, ...);
void log_success(const char *__restrict _format, ...);
void log_info(const char *__restrict _format, ...);
void redirect_log_stream(int deviceId,FILE *__restrict _stream);

#endif //KVFILESERVER_LOGGER_H
