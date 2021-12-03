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

typedef char* string;
typedef struct SWINFO
{
    pid_t pid;
    string name;
    string argv[3];
    int count;
    string time;
    string reason;
} swinfo;

swinfo **initBlock(string filename);
void initProcess(swinfo **list);
void waitProcess(swinfo **list);
void restartProcess(swinfo **list, pid_t pid, int status);
void printProcess(swinfo **list);

int main(int argc, char **argv)
{
    if(argc != 2) {
        printf("I Need swBlock Filename\n");
        return -1;
    }
    swinfo **Blocklist;
    
    Blocklist = initBlock(argv[1]);
    //initProcess(Blocklist);
    //waitProcess(Blocklist);
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

    if(fd = open(filename, O_RDONLY, 0644) == -1){
        printf("I don't open %s\n", filename);
        exit(-1);
    }
    size = lseek(fd, 0, SEEK_END); //measure file size
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
    }
    // save swblock info
    list = malloc(sizeof(swinfo*)*cnt);
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
    for(int i;temp != NULL; i++){
        free(listbuf[i]);
    }
    free(listbuf);
    close(fd);
}
