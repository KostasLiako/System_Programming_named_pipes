#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <dirent.h>
#include "list.h"
#include "citizen.h"



typedef struct monitor{
    pid_t pid;
    char PR_CW[10];
    char PW_CR[10];
    List* countries;
    int numCountries;
}monitor;

void writeFifo(char* msg,int fd,int buffer);
char* readFifo(char** msg,int fd,int bufferSize);
int moreThan6monthsVaccination(Date* date,char* day);
int numOftxtFiles(char* directory,char* country);
#endif