#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <dirent.h>
#include "functions.h"
#include <signal.h>

// #include "citizen.h"
// #include "list.h"


void writeFifo(char* input,int fd,int buffer){

    size_t size;
    int chunks;
    int lastBytes;
    int i;

  
    size= (strlen(input)+1)/sizeof(char);
    lastBytes=size%buffer; //who much bytes is less than buffer size 
    chunks=size/buffer; //split message into chunks

    for(i=0;i<(chunks+1);i++){  //send message (chunks +1) times

        if(lastBytes==0 && i<chunks){
            write(fd,&input[i*buffer],buffer);
        }

        else if(lastBytes!=0){

            if(i == chunks) write(fd,&input[i*buffer],lastBytes);
            else write(fd,&input[i*buffer],buffer);
        }
    }
    

}

char* readFifo(char** input,int fd,int bufferSize){
    char buffer[bufferSize];
    int i;
    int bytesForRead;
    strcpy(buffer,"");

    i=read(fd,buffer,bufferSize);
    bytesForRead=i;

    (*input)=malloc((i+1)*sizeof(char));
    strcpy((*input),"");
    while(i!=0){
        strncat((*input),buffer,i);
        i=read(fd,buffer,bufferSize); //when i==0 we havent more bytes to read
        if(i!=0){
            bytesForRead+=i;
            (*input)=realloc((*input),(bytesForRead+i+1)*sizeof(char));
        }
    }
    strncat((*input),"\0",1);//set the terminate string character
    return (*input);



}

int moreThan6monthsVaccination(Date* date,char* travelday){
    char* day=strtok(travelday,"-");
    char* month=strtok(NULL,"-");
    char* year=strtok(NULL,"-");

    int Day=atoi(day);
    int Month=atoi(month);
    int Year=atoi(year);

    if((Year-date->year)>=2) return 1;
    if(Year==date->year){
        if((Month-date->month)>6) return 1;
    }
    return 0;

    
}

int numOftxtFiles(char* directory,char* country){
    char* tmpdir=strdup(directory);
    strcat(tmpdir,"/");
    strcat(tmpdir,country);
    int counter=0;
    struct dirent *dir;
    DIR* inputDir = opendir(tmpdir);
    if (inputDir == NULL)
    {
        perror("Unable to read directory1");
        return (1);
    }
    while ((dir = readdir(inputDir)))
    {

        if ((strcmp(dir->d_name, "..") != 0) && (strcmp(dir->d_name, ".") != 0)){
            //printf("dir->name %s\n",dir->d_name);
            counter++;
        }
    }
    closedir(inputDir);
    return counter;


}



