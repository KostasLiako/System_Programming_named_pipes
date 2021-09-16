#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include "signalsTravel.h"


extern volatile sig_atomic_t flag1;

void signalHandlerMonitor(int signal,siginfo_t* info,void* content){
    flag1=1;
}