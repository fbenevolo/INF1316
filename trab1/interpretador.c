#include "data.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <sys/stat.h>

/*
    THE INTEPRETER DOES DO FOLLOWING STEPS
    - Reads file that contains the programs to be scheduled
    - Inserts programs data into processes struct
    - Executes scheduler
*/


int main(void) {

    FILE* fp;

    if (!(fp = fopen("exec.txt", "r"))) {
        fprintf(stderr, "could not open file\n");
        exit(-1);
    }

    // command and args to run the scheduler
    char* command = "./escalonador";
    char* args[] = {"./escalonador", NULL};

    // creating shared memory areas
    int seg_process, seg_num_p;

    seg_process = shmget(SHM_P, sizeof(Process), IPC_CREAT | 0666);
    seg_num_p   = shmget(SHM_NUM, sizeof(int), IPC_CREAT | 0666);

    if (!(seg_process) || !(seg_num_p)) {
        fprintf(stderr, "Could not alocate shared memory\n");
        exit(-2);
    }

    // creating attachments to shared memory segments
    Process* p_data = (Process*) shmat(seg_process, 0, 0);
    int* num_p      = (int*) shmat(seg_num_p, 0, 0);

    // auxiliary variables
    char name[20], policy;
    int b, e, read, lines=0;

    if (fork() == 0) {
        while(!feof(fp)) {
            read = fscanf(fp, "%*s %s %c=%d D=%d\n", name, &policy, &b, &e);
            
            strcpy(p_data->program, "./");
            strcat(p_data->program, name);
            p_data->index = lines;
            p_data->pid = -1;

            if (policy == 'I') { // real time
                p_data->policy = REAL_TIME;
                p_data->begin = b;
                p_data->duration = e;
            }
            else { // round robin
                p_data->policy = ROUND_ROBIN;
                p_data->begin = -1;
                p_data->duration = -1;
            }

            lines++;
            sleep(1);
        }
    }
    // executes scheduler
    else execvp(command, args);

    *num_p = lines-1;

    fclose(fp);

    
    shmdt(p_data);
    shmdt(num_p);

    for(EVER);

    return 0;
}
