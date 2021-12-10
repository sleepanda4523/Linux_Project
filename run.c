#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>

#define LOGFILE "./log/restart.txt"

typedef char* string;
typedef struct SWINFO
{
    pid_t pid;
    string name;
    string argv[3];
    int count;
    string time;
    char reason[256];
} swinfo;

int list_len=0;

swinfo **initBlock(string filename);
void initProcess(swinfo **list);
void waitProcess(swinfo **list);
void restartProcess(swinfo **list, pid_t pid, int status);
void printProcess(swinfo **list);
void logProcess(swinfo *swblock);
bool checkParent(swinfo **list);

int main(int argc, char **argv)
{
    if(argc != 2) {
        printf("I Need swblock file\n");
        return -1;
    }
    swinfo **Blocklist;
    Blocklist = initBlock(argv[1]);
    initProcess(Blocklist);
    waitProcess(Blocklist);
    return 0;
}

swinfo **initBlock(string filename)
{
    swinfo **list;
    string buf, temp;
    string *listbuf = malloc(sizeof(string) * 5);
    int fd; 
    off_t size; // file full size
    int cnt=0; //Block Count

    if((fd = open(filename, O_RDONLY, 0644)) < 0){
        printf("I don't open %s\n", filename);
        exit(-1);
    }
    if ((size = lseek(fd, 0, SEEK_END)) < 0) {
		printf("lseek error\n");
		exit(-1);
	} //measure file size
    lseek(fd, 0, SEEK_SET); //reset file read point
    buf = malloc(sizeof(char) * size +1); //size plus 1 because '\0'
    if(read(fd, buf, size+1) == -1){
        printf("I don't read %s\n", filename);
        exit(-1);
    }
    //measure swblock
    temp = strtok(buf, "\n");
    for(cnt=0;temp != NULL; cnt++){
        listbuf[cnt] = malloc(sizeof(char)*strlen(temp));
        strcpy(listbuf[cnt], temp);
        temp = strtok(NULL, "\n");
    }
    list_len = cnt;
    // save swblock info
    list = malloc(sizeof(swinfo*) * cnt);
    for(int i=0; i<cnt; i++){
        list[i] = malloc(sizeof(swinfo));
        temp = strtok(listbuf[i], ";");
        list[i]->name = malloc(sizeof(char)*strlen(temp));
        strcpy(list[i]->name, temp);
        for(int j=0;j<3;j++){
            temp = strtok(NULL, ";");
            list[i]->argv[j] = malloc(sizeof(char)*strlen(temp));
            strcpy(list[i]->argv[j], temp);
        }
    }
    // free list
    free(buf);
    for(int i=0;i<cnt; i++){
        free(listbuf[i]);
    }
    free(listbuf);
    close(fd);

    return list;
}

void printProcess(swinfo **list)
{
    system("clear");
    printf("===================================================================================\n");
    printf("|    Name    |    Restart Count    |       Start Time        |      Reason        |\n");
    printf("|------------|---------------------|-------------------------|--------------------|\n");
    for (int i = 0; i < list_len; i++)
    {   
        printf("|%11s |%20d | %23s | %18s |\n", list[i]->name, list[i]->count, list[i]->time, list[i]->reason);
        if (i + 1 == list_len)
            continue;
        else
            printf("|------------|---------------------|-------------------------|--------------------|\n");
    }
    printf("===================================================================================\n");
    return;
}

void initProcess(swinfo **list)
{
    time_t now; //now time
    struct tm *nowtime; //now time 
    for(int i=0;i<list_len;i++){
        list[i]->pid = fork();

        if (list[i]->pid == 0) // if child process
            break;
        if (list[i]->pid == -1)
        { 
            printf("swblock [%d] error\n", i);
            exit(-1);
        }

        now = time(NULL);
        nowtime = localtime(&now);

        list[i]->time = malloc(sizeof(char) * strlen("xxxx.xx.xx. xx:xx:xx"));
        sprintf(list[i]->time, "%04d.%02d.%02d. %02d:%02d:%02d", nowtime->tm_year + 1900, nowtime->tm_mon + 1, nowtime->tm_mday, nowtime->tm_hour, nowtime->tm_min, nowtime->tm_sec);
        // list[i]->reason reset
        memset(list[i]->reason, '\0', sizeof(list[i]->reason));
        strcpy(list[i]->reason, "None.");
    }
    if(checkParent(list)){
        printProcess(list);
    }
    return;
}
void waitProcess(swinfo **list)
{
    if(checkParent(list)){
        pid_t pid;
        int status;
        while(1){
            pid = waitpid(-1, &status, WUNTRACED);
            restartProcess(list, pid, status);
        }
    } else {
        for(int i=0;i<list_len;i++){
            if(list[i]->pid == 0){
                sleep(atoi(list[i]->argv[1]));
                kill(getpid(), atoi(list[i]->argv[2]));
                break;
            }
        }
    }
}

void restartProcess(swinfo **list, pid_t pid, int status)
{
    for(int i=0;i<list_len;i++){
        if(list[i]->pid == pid){
            if(WIFEXITED(status)) {
                sprintf(list[i]->reason, "Exit(%d)", WEXITSTATUS(status) >> 8);
            } 
            else if(WIFSIGNALED(status)){
                int signal = WTERMSIG(status);
                sprintf(list[i]->reason, "Signal(%s)", strsignal(signal));
            }
            list[i]->count += 1;
            list[i]->pid = fork();
            if(checkParent(list)){
                logProcess(list[i]);
                printProcess(list);
            } else {
                sleep(atoi(list[i]->argv[1]));
                kill(getpid(), atoi(list[i]->argv[2]));
            }
            break;
        }
    }
}
void logProcess(swinfo *swblock)
{
    int fd;
    time_t now; //now time
    struct tm *nowtime; //now time 
    string buf = malloc(sizeof(char)*600); //log 

    now = time(NULL);
    nowtime = localtime(&now);

    // write log buf
    sprintf(swblock->time, "%04d.%02d.%02d. %02d:%02d:%02d", nowtime->tm_year + 1900, nowtime->tm_mon + 1, nowtime->tm_mday, nowtime->tm_hour, nowtime->tm_min, nowtime->tm_sec);
    sprintf(buf, "------------------\nSW name : %s\nRestart Time : %s\nReason : %s\nRestart Count : %d\n", swblock->name, swblock->time, swblock->reason, swblock->count);
    
    if((fd = open(LOGFILE, O_RDWR | O_CREAT | O_APPEND, 0644)) < 0){
        printf("I don't open %s\n", LOGFILE);
        exit(-1);
    }
    write(fd,buf,strlen(buf));
    close(fd);
    free(buf);
}
bool checkParent(swinfo **list)
{
    bool test = true;
    int i;
    for (i = 0; i < list_len; i++)
        test = test && list[i]->pid > 0;
    return test;
}