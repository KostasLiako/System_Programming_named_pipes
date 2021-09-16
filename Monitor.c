#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/select.h>
#include <signal.h>
#include <sys/epoll.h>
#include "list.h"
#include "functions.h"
#include "citizen.h"
#include "skipList.h"

#define K 16

volatile sig_atomic_t flag1 = 0;
volatile sig_atomic_t flag2 = 0;
volatile sig_atomic_t flag3 = 0;
volatile sig_atomic_t flag4 = 0;
volatile sig_atomic_t flag5 = 0;

void signalHandlerMonitor(int signal, siginfo_t *info, void *content)
{
    if (flag1 == 0)
    {
        //printf("I receive SIGINT signal");
        flag1 = 1;
    }
}
void sigusr1Signal(int signal, siginfo_t *info, void *content)
{
    if (flag2 == 0)
    {
        //printf("I receive SIGUSR1 signal");
        flag2 = 1;
    }
}

void sigusr2Signal(int signal, siginfo_t *info, void *content)
{
    if (flag3 == 0)
    {
        //printf("I receive SIGUSR1 signal");
        flag3 = 1;
    }
}

void sigusr4Signal(int signal, siginfo_t *info, void *content)
{
    if (flag4 == 0)
    {
        //printf("I receive SIGUSR1 signal");
        flag4 = 1;
    }
}

void sigusr5Signal(int signal, siginfo_t *info, void *content)
{
    if (flag5 == 0)
    {
        //printf("I receive SIGUSR1 signal");
        flag5 = 1;
    }
}
int main(int argc, char **argv)
{
    int fdR = 0;
    int fdW = 0;
    int bufferSize = 0;
    int bloomSize = 0;
    char *buffer = NULL;
    int fl = 0;
    char *pathR = argv[2];
    char *pathW = argv[1];
    int counter = 0;
    int numCountries = 3;
    int totals = 0;
    int accepted = 0;
    int rejected = 0;
    int txtFile = 0;

    char *finalMsg = NULL;

    BloomList *bloomList = initBloomList();
    BloomFilter *BloomFilter = NULL;
    DIR *folder = NULL;
    struct dirent *entry;
    char directory[50];
    char line[256];
    Citizen *citizen = NULL;
    HashTable *ht = NULL;
    skipList *slist = NULL;
    strcpy(directory, "input_dir/");

    List *countries = initList();
    List *viruses = initList();
    List *vaccination = initList();
    List *txtFiles = initList();
    listOfSkipList *VaccinatedList = listOfSkipListInit();
    listOfSkipList *NoVaccinatedList = listOfSkipListInit();

    initHashTable(&ht);
    //printf("In the CHILD process %u.\n", getpid());
    if ((fdR = open(pathR, O_RDONLY)) < 0)
    {
        printf("Error opening FIFO in %u.\n", getpid());
        exit(EXIT_FAILURE);
    }

    read(fdR, &bufferSize, sizeof(bufferSize));
    read(fdR, &bloomSize, sizeof(bloomSize));
    read(fdR, &numCountries, sizeof(numCountries));
    read(fdR, &txtFile, sizeof(txtFile));

    struct epoll_event ev, events;
    int nfds, epollfd;

    epollfd = epoll_create1(0);

    ev.events = EPOLLIN;
    ev.data.fd = fdR;

    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, fdR, &ev) == -1)
    {
        printf("error epoll\n");
        exit(1);
    }

    int len = 0;
    while (1)
    {
        nfds = epoll_wait(epollfd, &events, 1, -1);

        if (nfds == 1)
        {
            int n;
            if (len == 0)
            {
                n = read(fdR, &len, sizeof(len));
                if (n == 0)
                    break;
            }
            else
            {
                char country[len];

                n = read(fdR, country, len);
                if (n == 0)
                    break;

                insertNode(countries, country);

                len = 0;
            }
        }
        else
            break;
    }

    close(fdR);

    listNode *tmp;
    tmp = countries->head;

    while (tmp != NULL)
    {
        //initMonitor(tmp->name,directory,ht,viruses,vaccination,countries);
        strcat(directory, tmp->name);

        folder = opendir(directory);

        if (folder == NULL)
        {
            perror("Unable to read directory");
            return (1);
        }

        while ((entry = readdir(folder)))
        {
            char tmpfile[20];
            strcpy(tmpfile, directory);
            strcat(tmpfile, "/");
            strcat(tmpfile, entry->d_name);

            if ((strcmp(entry->d_name, "..") != 0) && (strcmp(entry->d_name, ".") != 0))
            {
                int exist = insertNode(txtFiles, entry->d_name);
                FILE *file = fopen(tmpfile, "r");
                if (file == NULL)
                {
                    fprintf(stderr, "Could not open file");
                }
                BloomFilter = malloc(sizeof(BloomFilter));
                while (fgets(line, sizeof(line), file) != NULL)
                {
                    citizen = malloc(sizeof(Citizen));

                    char *id = strtok(line, " ");
                    char *name = strtok(NULL, " ");
                    char *surname = strtok(NULL, " ");
                    char *country = strtok(NULL, " ");
                    char *citizenAge = strtok(NULL, " ");
                    char *virus = strtok(NULL, " ");
                    char *vacc = strtok(NULL, " ");
                    char *date = strtok(NULL, " ");
                    int age = atoi(citizenAge);

                    initCitizen(citizen, id, name, surname, country, virus, vacc);
                    setCitizenInfo(citizen, id, name, surname, country, age, virus, vacc, date);

                    int virusExist = insertNode(viruses, virus);
                    int vaccinated = insertNode(vaccination, vacc);

                    int insert = insertCitizenHashTable(ht, citizen, countries, viruses, vaccination);
                    if (insert != 0)
                    {
                        increasePopulation(countries, country);
                        increaseAgeArray(countries, country, age);
                        if (virusExist == 1)
                        {
                            initBloomFilter(BloomFilter, bloomSize, viruses, virus);
                            slist = skipListInit(viruses, virus);
                            bloomInsertList(bloomList, BloomFilter, viruses);
                            listOfSkipListInsert(NoVaccinatedList, slist, viruses);
                            listOfSkipListInsert(VaccinatedList, slist, viruses);
                        }
                        //----INSERT BLOOM FILTER---
                        if (strcmp(citizen->vaccinated, "YES") == 0)
                        {
                            bloomInsert(bloomList, citizen->id, K, citizen->virus);
                            insertSkipList(VaccinatedList, citizen, ht);
                        }
                        else
                        {
                            insertSkipList(NoVaccinatedList, citizen, ht);
                        }
                    }

                    free(citizen);
                    //deleteCitizen(citizen);
                }
            }

            // }
        }
        closedir(folder);
        strcpy(directory, "input_dir/");

        tmp = tmp->next;
    }

    struct sigaction signal;

    memset(&signal, 0, sizeof(signal));
    signal.sa_sigaction = signalHandlerMonitor;
    signal.sa_flags = SA_SIGINFO;

    sigaction(SIGINT, &signal, NULL);

    struct sigaction signal2;
    memset(&signal2, 0, sizeof(signal2));
    signal2.sa_sigaction = sigusr1Signal;
    signal2.sa_flags = SA_SIGINFO;
    sigaction(SIGUSR1, &signal2, NULL);

    struct sigaction signal3;
    memset(&signal3, 0, sizeof(signal3));
    signal3.sa_sigaction = sigusr2Signal;
    signal3.sa_flags = SA_SIGINFO;
    sigaction(SIGUSR2, &signal3, NULL);

    struct sigaction signal4;
    memset(&signal4, 0, sizeof(signal4));
    signal4.sa_sigaction = sigusr4Signal;
    signal4.sa_flags = SA_SIGINFO;
    sigaction(SIGKILL, &signal4, NULL);

    struct sigaction signal5;
    memset(&signal5, 0, sizeof(signal5));
    signal5.sa_sigaction = sigusr5Signal;
    signal5.sa_flags = SA_SIGINFO;
    sigaction(SIGTERM, &signal5, NULL);

    while (1)
    {
        //printf("waiting for signal\n");
        if (flag2 == 1)
        {
            totals++;
            char *input;
            if ((fdR = open(pathR, O_RDONLY)) < 0)
            {
                printf("Error opening FIFO in %u.\n", getpid());
                exit(EXIT_FAILURE);
            }
            input = readFifo(&input, fdR, bufferSize);

            close(fdR);
            char *cmd;
            cmd = strtok(input, " ");
            char *id = strtok(NULL, " ");
            char *date = strtok(NULL, " ");
            char *cFrom = strtok(NULL, " ");
            char *cTo = strtok(NULL, " ");
            char *virus = strtok(NULL, " ");

            int exist = searchSkipList(VaccinatedList, id, virus, date);
            if ((fdW = open(pathW, O_WRONLY)) < 0)
            {
                printf("Error opening FIFO in %u.\n", getpid());
                exit(EXIT_FAILURE);
            }
            if (exist == 1)
            {
                accepted++;
                write(fdW, "YES", strlen("YES"));
            }
            else if (exist == 2)
            {
                rejected++;
                write(fdW, "NOM", strlen("NOM"));
            }
            else
            {
                rejected++;
                write(fdW, "NO", strlen("NO"));
            }
            close(fdW);

            flag2 = 0;
        }
        sleep(1);
        if (flag1 == 1)
        {

            char logFile[20];
            strcpy(logFile, "log_file.");
            char pid[20];
            sprintf(pid, "%d", getpid());

            strcat(logFile, pid);
            //printf("log file is %s\n", logFile);

            FILE *file;
            if ((file = fopen(logFile, "w")) == NULL)
            {
                printf("Error opening log file \n");
                break;
            }
            listNode *tmp;
            tmp = countries->head;

            while (tmp != NULL)
            {
                fputs(tmp->name, file);
                fputs("\n", file);
                tmp = tmp->next;
            }
            char total[10];
            char accept[10];
            char reject[10];

            sprintf(accept, "%d", accepted);
            sprintf(total, "%d", totals);
            sprintf(reject, "%d", rejected);
            fputs("TOTAL TRAVEL REQUESTS:", file);
            fputs(total, file);
            fputs("\n", file);
            fputs("ACCEPTED: ", file);
            fputs(accept, file);
            fputs("\n", file);
            fputs("REJECTED: ", file);
            fputs(reject, file);
            fputs("\n", file);
            fclose(file);
            printf("Logs_files created\n");
            break;
        }
        if (flag3 == 1)
        {
            char *msg;
            if ((fdR = open(pathR, O_RDONLY)) < 0)
            {
                printf("Error opening FIFO in %u.\n", getpid());
                exit(EXIT_FAILURE);
            }
            msg = readFifo(&msg, fdR, bufferSize);
            close(fdR);

            char *cmd = strtok(msg, " ");
            char *id = strtok(NULL, " ");

            int exist = idExist(ht, id);
            if (exist == 1)
            {
                searchAllSkipList(VaccinatedList, id, viruses);
                searchAllSkipList(NoVaccinatedList, id, viruses);
            }

            flag3 = 0;
        }

        if (flag4 == 1)
        {
            //printf("EXITING\n");
            //break;

            //SIGKILL SIGNAL CANT BE HANDLED
        }
        if (flag5 == 1)
        {
            
            listNode *tmp;
            tmp = countries->head;

            while (tmp != NULL)
            {
                //initMonitor(tmp->name,directory,ht,viruses,vaccination,countries);
                strcat(directory, tmp->name);

                folder = opendir(directory);

                if (folder == NULL)
                {
                    perror("Unable to read directory");
                    return (1);
                }

                while ((entry = readdir(folder)))
                {
                    char tmpfile[20];
                    strcpy(tmpfile, directory);
                    strcat(tmpfile, "/");
                    strcat(tmpfile, entry->d_name);

                    if ((strcmp(entry->d_name, "..") != 0) && (strcmp(entry->d_name, ".") != 0))
                    {
                        int exist = nodeExist(txtFiles, entry->d_name);
                        if (exist == 0)
                        {
                            FILE *file = fopen(tmpfile, "r");
                            if (file == NULL)
                            {
                                fprintf(stderr, "Could not open file");
                            }
                            BloomFilter = malloc(sizeof(BloomFilter));
                            while (fgets(line, sizeof(line), file) != NULL)
                            {
                                citizen = malloc(sizeof(Citizen));

                                char *id = strtok(line, " ");
                                char *name = strtok(NULL, " ");
                                char *surname = strtok(NULL, " ");
                                char *country = strtok(NULL, " ");
                                char *citizenAge = strtok(NULL, " ");
                                char *virus = strtok(NULL, " ");
                                char *vacc = strtok(NULL, " ");
                                char *date = strtok(NULL, " ");
                                int age = atoi(citizenAge);

                                initCitizen(citizen, id, name, surname, country, virus, vacc);
                                setCitizenInfo(citizen, id, name, surname, country, age, virus, vacc, date);

                                int virusExist = insertNode(viruses, virus);
                                int vaccinated = insertNode(vaccination, vacc);

                                int insert = insertCitizenHashTable(ht, citizen, countries, viruses, vaccination);
                                if (insert != 0)
                                {
                                    increasePopulation(countries, country);
                                    increaseAgeArray(countries, country, age);
                                    if (virusExist == 1)
                                    {
                                        initBloomFilter(BloomFilter, bloomSize, viruses, virus);
                                        slist = skipListInit(viruses, virus);
                                        bloomInsertList(bloomList, BloomFilter, viruses);
                                        listOfSkipListInsert(NoVaccinatedList, slist, viruses);
                                        listOfSkipListInsert(VaccinatedList, slist, viruses);
                                    }
                                    //----INSERT BLOOM FILTER---
                                    if (strcmp(citizen->vaccinated, "YES") == 0)
                                    {
                                        bloomInsert(bloomList, citizen->id, K, citizen->virus);
                                        insertSkipList(VaccinatedList, citizen, ht);
                                    }
                                    else
                                    {
                                        insertSkipList(NoVaccinatedList, citizen, ht);
                                    }
                                }

                                free(citizen);
                                //deleteCitizen(citizen);
                            }
                        }
                    }
                }
                closedir(folder);
                strcpy(directory, "input_dir/");

                tmp = tmp->next;
            }
            printf("Monitor Updated\n");
            flag5 = 0;
        }
    }

    // while(1){
    //     printf("flag is %d",flag);
    //     sleep(3);
    // }

    //printHashTable(ht);
    // char tmpMsg[100];
    // strcpy(tmpMsg,"MESAGE APO CHILDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD");
    // printf("\n\n\n");
    // if ((fdW = open(pathW, O_WRONLY)) < 0)
    // {
    //     printf("Error opening FIFO in %u.\n", getpid());
    //     exit(EXIT_FAILURE);
    // }

    // writeFifo(tmpMsg,fdW,bufferSize);

    //     printf("written from process %u\n",getpid());
    // close(fdW);

    printf("Exiting process %u.\n", getpid());
    return 0;
}
