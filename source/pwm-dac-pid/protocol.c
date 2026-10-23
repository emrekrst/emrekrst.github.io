#include "protocol.h"
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
int parse_command(const char* text, command* out) {
    const char* names[]={"SET","AUTO","READ","MONITOR","STOP","KP","KI","KD"};
    while (isspace((unsigned char)*text)) ++text;
    const char* end=text;
    while (*end && !isspace((unsigned char)*end)) ++end;
    unsigned n=(unsigned)(end-text), kind;
    for(kind=0;kind<sizeof names/sizeof names[0];++kind)
        if(strlen(names[kind])==n && strncmp(text,names[kind],n)==0) break;
    if(kind==sizeof names/sizeof names[0]) return 0;
    while(isspace((unsigned char)*end)) ++end;
    long value=0;
    if(kind==CMD_SET || kind==CMD_AUTO || kind>=CMD_KP) {
        if(!*end) return 0;
        char* tail;
        errno=0; value=strtol(end,&tail,10);
        if(tail==end || errno==ERANGE) return 0;
        while(isspace((unsigned char)*tail)) ++tail;
        if(*tail) return 0;
        if(kind>=CMD_KP && (value<0 || value>30000)) return 0;
        if(kind==CMD_SET || kind==CMD_AUTO) {
            long max=kind==CMD_SET ? 255 : 1023;
            if(value<0) value=0;
            if(value>max) value=max;
        }
    } else if(*end) return 0;
    out->kind=(command_kind)kind; out->value=(int)value;
    return 1;
}
int command_feed(command_buffer* b,char byte) {
    if(byte=='\r' || byte=='\n') {
        int result=b->overflow ? -1 : (b->length ? 1 : 0);
        b->text[b->length]='\0'; b->length=0; b->overflow=0;
        return result;
    }
    if(b->length+1<sizeof b->text && !b->overflow) b->text[b->length++]=byte;
    else b->overflow=1;
    return 0;
}
