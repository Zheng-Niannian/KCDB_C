#include "logger.h"

FILE *logger_default_in=NULL;
FILE* logger_default_out=NULL;
FILE* logger_default_err=NULL;

void init_logger(){
    logger_default_in=stdin;
    logger_default_out=stdout;
    logger_default_err=stderr;
}

char* reconstruct_logger_format(const char* prefix,const char* format){
    int len=sizeof(char)*(strlen(format)+1+ strlen(prefix));
    char *buffer=malloc(len);
    if(!buffer){
        fprintf(logger_default_err,"cannot malloc memory for logger\n");
        return NULL;
    }
    memset(buffer,0,len);
    sprintf(buffer,"%s%s",prefix,format);
    return buffer;
}

void log_generic(FILE *stream, const char *prefix, const char *__restrict _format, va_list args) {
    char* newFormat = reconstruct_logger_format(prefix, _format);
    if (!newFormat) return;
    vfprintf(stream, newFormat, args);
    free(newFormat);
}

void log_error(const char *__restrict _format, ...) {
    va_list args;
    va_start(args, _format);
    log_generic(logger_default_err, "[ERROR] ", _format, args);
    va_end(args);
}

void log_warning(const char *__restrict _format, ...) {
    va_list args;
    va_start(args, _format);
    log_generic(logger_default_out, "[WARNING] ", _format, args);
    va_end(args);
}

void log_success(const char *__restrict _format, ...) {
    va_list args;
    va_start(args, _format);
    log_generic(logger_default_out, "[SUCCESS] ", _format, args);
    va_end(args);
}

void log_info(const char *__restrict _format, ...){
    va_list args;
    va_start(args, _format);
    log_generic(logger_default_out, "[INFO] ", _format, args);
    va_end(args);
}
void redirect_log_stream(int deviceId,FILE *__restrict _stream){
    switch (deviceId) {
        case 0: // stdin
            logger_default_in = _stream;
            break;
        case 1: // stdout
            logger_default_out = _stream;
            break;
        case 2: // stderr
            logger_default_err = _stream;
            break;
        default:
            fprintf(logger_default_err, "Invalid device ID\n");
            return;
    }
}