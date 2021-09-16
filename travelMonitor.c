#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <dirent.h>
#include <signal.h>
#include <time.h>
#include "functions.h"
#include "list.h"
#include "citizen.h"
#include "skipList.h"

volatile sig_atomic_t flag = 0;
void signalHandlerTravel(int signal, siginfo_t *info, void *content)
{
    if (flag == 0)
    {

        //  printf("i receive SIGINT signal \n");
        flag = 1;
    }
}

int main(int argc, char **argv)
{
    srand(time(NULL));
    pid_t pid;
    ssize_t elements;
    int numMonitor = 0;
    char pathname[] = "PR_CWX";
    char pathname2[] = "PW_CRX";
    char *directory = NULL;
    char *buffer = NULL;
    DIR *inputDir = NULL;
    int files = 0;
    struct dirent *dir;
    char **PR_CW = NULL;
    char **PW_CR = NULL;
    char **countriesNames = NULL;
    char *receiveMsg = NULL;

    int totals = 0;
    int accepted = 0;
    int rejected = 0;

    int status = 0;
    //initialize numMonitors bloomSize and input directory
    numMonitor = atoi(argv[2]);
    int bloomSize = atoi(argv[6]);
    int bufferSize = atoi(argv[4]);
    directory = argv[8];
    monitor child[numMonitor];
    List *countries = initList();
    Request *requests = initRequests();

    //check for validation of buffer size and bloom size and numMonitors
    if ((bloomSize == 0) || (bufferSize == 0) || (numMonitor == 0))
    {
        printf("Give at least 1 byte\n");
    }

    //initiliaze countries for each child
    for (int i = 0; i < numMonitor; i++)
    {
        child[i].countries = initList();
    }

    //save countries number and names
    inputDir = opendir(directory);
    if (inputDir == NULL)
    {
        perror("Unable to read directory1");
        return (1);
    }

    while ((dir = readdir(inputDir)))
    {

        if ((strcmp(dir->d_name, "..") != 0) && (strcmp(dir->d_name, ".") != 0))
        {
            files++;
            int exist = insertNode(countries, dir->d_name);
        }
    }
    closedir(inputDir);
    //printf("In directory inluded %d files\n", files);

    countriesNames = (char **)malloc(sizeof(char *) * files);

    inputDir = opendir(directory);
    if (inputDir == NULL)
    {
        perror("Unable to read directory");
        return (1);
    }
    int j = 0;
    int counter = 0;
    while ((dir = readdir(inputDir)))
    {

        if ((strcmp(dir->d_name, "..") != 0) && (strcmp(dir->d_name, ".") != 0))
        {
            if (counter == numMonitor)
                counter = 0;
            countriesNames[j] = strdup(dir->d_name);
            child[counter].numCountries++;
            counter++;
            j++;
        }
    }
    closedir(inputDir);

    int txtFiles = numOftxtFiles(directory, countriesNames[0]);
    //printf("txt files is %d\n",txtFiles);

    //creating paths for monitor processes.
    PR_CW = malloc(numMonitor * sizeof(char *));
    PW_CR = malloc(numMonitor * sizeof(char *));
    for (int i = 0; i < numMonitor; i++)
    {
        PR_CW[i] = malloc((strlen(pathname) + 1) * sizeof(char));
        PW_CR[i] = malloc((strlen(pathname2) + 1) * sizeof(char));
    }
    for (int digit = 0; digit < numMonitor; digit++)
    {
        pathname[5] = digit + '0';
        pathname2[5] = digit + '0';
        strcpy(PR_CW[digit], pathname);
        strcpy(PW_CR[digit], pathname2);
        strcpy(child[digit].PR_CW, PR_CW[digit]);
        strcpy(child[digit].PW_CR, PW_CR[digit]);
    }

    int fdR[numMonitor];
    int fdW[numMonitor];

    buffer = malloc(bufferSize);

    //create named pipes for each monitor process
    for (int i = 0; i < numMonitor; i++)
    {

        if ((mkfifo(child[i].PR_CW, 0666)) < 0)
        {
            perror("Pipe error:");
            exit(1);
        }

        if ((mkfifo(child[i].PW_CR, 0666)) < 0)
        {
            perror("Pipe error:");
            exit(1);
        }
    }

    //create childs and exec
    for (int i = 0; i < numMonitor; i++) // loop will run n times (n=5)
    {
        if ((pid = fork()) == 0) // child process
        {
            // printf("[son] pid %d from [parent] pid %d\n", getpid(), getppid());
            char *args[] = {"./Monitor", child[i].PR_CW, child[i].PW_CR, NULL};
            execv(args[0], args);
            exit(0);
        }

        child[i].pid = pid;
    }

    // //-------PARENT CODE--------
    int num;
    //send buffer size bloom size and number of countries for each child.
    for (int i = 0; i < numMonitor; i++)
    {
        char msg[100];

        if ((fdW[i] = open(child[i].PW_CR, O_WRONLY)) < 0)
        {
            printf("Error opening FIFO in %u.\n", getpid());
            exit(EXIT_FAILURE);
        }
    }
    for (int i = 0; i < numMonitor; i++)
    {

        write(fdW[i], &bufferSize, sizeof(bufferSize));
        write(fdW[i], &bloomSize, sizeof(bloomSize));
        write(fdW[i], &child[i].numCountries, sizeof(child[i].numCountries));
        write(fdW[i],&txtFiles,sizeof(txtFiles));
    }
    usleep(500);

    inputDir = opendir(directory);
    if (inputDir == NULL)
    {
        perror("Unable to read directory1");
        return (1);
    }
    int counter2 = 0;
    int monitor = 0;
    //send the countries how manage each monitor;
    while ((dir = readdir(inputDir)))
    {

        if ((strcmp(dir->d_name, "..") != 0) && (strcmp(dir->d_name, ".") != 0))
        {
            monitor = counter2 % numMonitor;
            int size = strlen(dir->d_name) + 1;

            if (write(fdW[monitor], &size, sizeof(size)) == -1)
                printf("error writting\n");

            if (write(fdW[monitor], dir->d_name, size) == -1)
                printf("error writting\n");
            int exist = insertNode(child[monitor].countries, dir->d_name);

            counter2++;
        }
    }
    closedir(inputDir);

    for (int i = 0; i < numMonitor; i++)
    {
        close(fdW[i]);
    }

    struct sigaction signal;

    memset(&signal, 0, sizeof(signal));
    signal.sa_sigaction = signalHandlerTravel;
    signal.sa_flags = SA_SIGINFO;
    sigaction(SIGINT, &signal, NULL);




    struct sigaction signal2;
    memset(&signal2, 0, sizeof(signal2));
    signal2.sa_sigaction = signalHandlerTravel;
    signal2.sa_flags = SA_SIGINFO; 
    sigaction(SIGQUIT, &signal2, NULL);

    // signal(SIGINT,signalHandler);

    while (1)
    {
        char *cmd;
        char input[100];

        printf("Give us an input\n");
        fgets(input, sizeof(input), stdin);
        input[strlen(input) - 1] = '\0';

        char *msg = strdup(input);

        sleep(1);
        if (flag == 1)
        {
            
            char logFile[20];
            strcpy(logFile, "log_file.");
            char pid[20];
            sprintf(pid, "%d", getpid());

            strcat(logFile, pid);
            //printf("log file is %s\n", logFile);

            FILE *fl;
            if ((fl = fopen(logFile, "w")) == NULL)
            {
                printf("Error opening log file \n");
                break;
            }

            for(int i=0;i<files;i++){
                fputs(countriesNames[i],fl);
                fputs("\n",fl);
            }

            char total[10];
            char accept[10];
            char reject[10];

            sprintf(accept,"%d",accepted);
            sprintf(total,"%d",totals);
            sprintf(reject,"%d",rejected);

             fputs("TOTAL TRAVEL REQUESTS:",fl);
             fputs(total,fl);
             fputs("\n",fl);
             fputs("ACCEPTED: ",fl);
             fputs(accept,fl);
             fputs("\n",fl);
             fputs("REJECTED: ",fl);
             fputs(reject,fl);
             fputs("\n",fl);
             fclose(fl);
            break;
        }
        cmd = strtok(input, " ");
        if (strcmp(cmd, "/travelRequest") == 0)
        {

            char *id = strtok(NULL, " ");
            char *date = strtok(NULL, " ");
            char *cFrom = strtok(NULL, " ");
            char *cTo = strtok(NULL, " ");
            char *virus = strtok(NULL, " ");
            totals++;

            int temp = 0;

            for (int i = 0; numMonitor; i++)
            {
                int exist = nodeExist(child[i].countries, cFrom);
                if (exist == 1)
                {
                    temp = i;
                    break;
                }
            }

            kill(child[temp].pid, SIGUSR1);
            int fd;
            if ((fd = open(child[temp].PW_CR, O_WRONLY)) < 0)
            {
                printf("Error opening FIFO in %u.\n", getpid());
                exit(EXIT_FAILURE);
            }
            writeFifo(msg, fd, bufferSize);
            close(fd);
            int fdR;
            if ((fdR = open(child[temp].PR_CW, O_RDONLY)) < 0)
            {
                printf("Error opening FIFO \n");
                exit(EXIT_FAILURE);
            }

            char *result;
            result = readFifo(&result, fdR, bufferSize);
            //printf("result is %s\n",result);
            close(fdR);

            if (strcmp(result, "NO") == 0)
            {
                printf("REQUEST REJECTED – YOU ARE NOT VACCINATED\n");
                increaseRejectedRequest(countries, cTo);
                insertRequest(requests, cTo, date, 0, virus);
                rejected++;
            }
            if (strcmp(result, "YES") == 0)
            {
                printf("REQUEST ACCEPTED – HAPPY TRAVELS\n");
                increaseAcceptedRequest(countries, cTo);
                insertRequest(requests, cTo, date, 1, virus);
                accepted++;
            }

            if (strcmp(result, "NOM") == 0)
            {
                printf("REQUEST REJECTED – YOU WILL NEED ANOTHER VACCINATION BEFORE TRAVEL DATE\n");
                increaseRejectedRequest(countries, cTo);
                insertRequest(requests, cTo, date, 0, virus);
                rejected++;
            }

            sleep(1);

            //printf("%s %s %s %s %s \n",id,date,cFrom,cTo,virus);
        }
        if (strcmp(cmd, "/searchVaccinationStatus") == 0)
        {
            char *id = strtok(NULL, " ");

            for (int i = 0; i < numMonitor; i++)
            {
                kill(child[i].pid, SIGUSR2);
            }

            for (int i = 0; i < numMonitor; i++)
            {

                if ((fdW[i] = open(child[i].PW_CR, O_WRONLY)) < 0)
                {
                    printf("Error opening FIFO in %u.\n", getpid());
                    exit(EXIT_FAILURE);
                }
            }

            for (int i = 0; i < numMonitor; i++)
            {

                writeFifo(msg, fdW[i], bufferSize);
            }

            for (int i = 0; i < numMonitor; i++)
            {
                close(fdW[i]);
            }
        }
        if (strcmp(cmd, "/travelStats") == 0)
        {
            char *virus = strtok(NULL, " ");
            char *date1 = strtok(NULL, " ");
            char *date2 = strtok(NULL, " ");
            char *country = strtok(NULL, " ");
            int t = 0, a = 0, r = 0;
            char *day1 = strtok(date1, "-");
            char *month1 = strtok(NULL, "-");
            char *year1 = strtok(NULL, "-");
            char *day2 = strtok(date2, "-");
            char *month2 = strtok(NULL, "-");
            char *year2 = strtok(NULL, "-");

            if (country == NULL)
            {
                RequestNode *tmp;
                tmp = requests->head;
                while (tmp != NULL)
                {
                    if (strcmp(tmp->virus, virus) == 0)
                    {
                        t++;
                        if (tmp->accepted == 1)
                            a++;
                        if (tmp->rejected == 1)
                            r++;
                    }
                    tmp = tmp->next;
                }
                printf("TOTAL REQUEST: %d\n", t);
                printf("ACCEPTED: %d\n", a);
                printf("REJECTED: %d\n", r);
            }
            else
            {
                RequestNode *tmp;
                tmp = requests->head;
                while (tmp != NULL)
                {
                    if ((strcmp(tmp->virus, virus) == 0) && (strcmp(tmp->cTo, country) == 0))
                    {
                        t++;
                        if (tmp->accepted == 1)
                            a++;
                        if (tmp->rejected == 1)
                            r++;
                    }
                    tmp = tmp->next;
                }
                printf("TOTAL REQUEST: %d\n", t);
                printf("ACCEPTED: %d\n", a);
                printf("REJECTED: %d\n", r);
            }
        }
        if (strcmp(cmd, "/addVaccinationRecords") == 0)
        {
            char *country = strtok(NULL, " ");
            char path[100];
            strcpy(path, directory);
            strcat(path, "/");
            strcat(path, country);
            strcat(path, "/");
            strcat(path, country);
            char number[10];
            sprintf(number, "%d", txtFiles + 1);
            strcat(path, "-");
            strcat(path, number);
            strcat(path, ".txt");
            //printf("path is %s\n", path);
            txtFiles++;

            FILE *file = fopen(path, "w");

            for(int i=0;i<10;i++){
            int random = rand()%1001;
            char id[10];
            sprintf(id,"%d",random);
            fputs(id, file);
            fputs(" KOSTAS ", file);
            fputs("LIAKO ", file);
            fputs(country, file);
            fputs(" 23 ", file);
            fputs("COVID-19 ", file);
            fputs("NO", file);
            fputs("\n",file);
            }

            fclose(file);

            int temp = 0;

            for (int i = 0; numMonitor; i++)
            {
                int exist = nodeExist(child[i].countries,country);
                if (exist == 1)
                {
                    temp = i;
                    break;
                }
            }

            kill(child[temp].pid, SIGTERM);
        }

        if (strcmp(input, "/exit") == 0)
        {
            printf("Exit\n");
            for(int i=0;i<numMonitor;i++){
                kill(child[i].pid,SIGKILL);
            }
            //free memory

            
            
            break;
        }
    }


    return 0;
}
