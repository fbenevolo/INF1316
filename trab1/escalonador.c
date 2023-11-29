#include "data.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <sys/wait.h>


int filterInvalidProcesses(Process* p_data, Process* rt_data, int size);
void allocateProcess(Process* array_data, Process* p_data, int* index);
void swap(Process* xp, Process* yp);
void selectionSort(Process arr[], int n);

char* args[] = {NULL};


int main(void) {

    int seg_processes, seg_num_p;
    struct timeval T;
    int sec, rt_executing = 0, rr_executing = 0;
    int time_limit = 0;

    // retrieving access to shared memory area
    seg_processes = shmget(SHM_P, sizeof(Process), IPC_CREAT | 0666);
    seg_num_p     = shmget(SHM_NUM, sizeof(int), IPC_CREAT | 0666);

    // attaching to shared memory retrieved segments
    Process* p_data = (Process*) shmat(seg_processes, 0, 0);
    int* num_p      = (int*) shmat(seg_num_p, 0, 0);

    // creates processes array
    Process rt_data[MAX];
    Process rr_data[MAX];
    int num_rt, num_rr;

    num_rt = num_rr = 0;

    while (time_limit < LIMIT) {

        if (num_rt + num_rr != *num_p) { // if there are processes to be scheduled
            selectionSort(rt_data, num_rt); // sort array according to times 
            if (p_data->policy == REAL_TIME) {
                if (filterInvalidProcesses(p_data, rt_data, num_rt) == 0) { // verify if process is valid
                    allocateProcess(&rt_data[num_rt], p_data, &num_rt);
                }
                else printf("Could not allocate %s. It's time conflicts with %s\n",
                p_data->program, rt_data[num_rt-1].program);
            }
            else {
                allocateProcess(&rr_data[num_rr], p_data, &num_rr);
            }
        }

        gettimeofday(&T, NULL);
        sec = T.tv_sec % 60;

        printf("%d\n", sec);

        // search for real time processes
        for (int i = 0; i < num_rt; i++) {
            if (sec == (rt_data[i].begin + rt_data[i].duration) - 1) {
                rt_executing = 0;
                kill(rt_data[i].pid, SIGSTOP);

                //printf("stopping %s\n", rt_data[i].program);
            }

            if (sec == rt_data[i].begin) {
                rt_executing = 1;
                kill(rt_data[i].pid, SIGCONT);

                printf("executing %s\n", rt_data[i].program);
            }
        }

        // round robin
        // test if real time is executing
        if (rt_executing == 0 && num_rr > 0) {
            
            int last = rr_executing-1; 
            if (rr_executing == num_rr) rr_executing = 0;
            if (rr_executing == 0) last = num_rr-1;

            kill(rr_data[last].pid, SIGSTOP);
            kill(rr_data[rr_executing].pid, SIGCONT);

            //printf("stopping %s\n", rr_data[last].program);
            printf("executing %s\n", rr_data[rr_executing].program);

            rr_executing++;
        }
        

        sleep(1);
        time_limit++;
    }


    shmdt(p_data);
    shmdt(num_p);

    shmctl(seg_processes, IPC_RMID, 0);
    shmctl(seg_num_p, IPC_RMID, 0);


    return 0;
}


/*
    Function that verifies and removes processes 
    under one of the two conditions
        - Process begin plus duration time exceeds 60s
        - Process execution time conflicts with previous processes

    Returns a new array with only valid processes
*/
int filterInvalidProcesses(Process* p_data, Process* rt_data, int size) {

    if (p_data->begin + p_data->duration > 60) return 1;
    
    for (int i = 0; i < size; i++) {
        if (p_data->index > rt_data[i].index) {
            if (p_data->begin >= rt_data[i].begin &&
            p_data->begin <= rt_data[i].begin + rt_data[i].duration - 1) {
                return 1;
            }
        }
    }

    return 0;
}


/*
    Allocate process in it's own structure
*/
void allocateProcess(Process* array_data, Process* p_data, int* index) {
    
    strcpy(array_data->program, p_data->program);
    array_data->index = p_data->index; 
    array_data->policy = p_data->policy;
    array_data->begin = p_data->begin;
    array_data->duration = p_data->duration;

    array_data->pid = fork();
    if (array_data->pid == 0) { 
        execvp(array_data->program, args);
    }
    kill(array_data->pid, SIGSTOP);

    (*index)++;
}


/*
    Auxiliary function to 
    swap elements
*/
void swap(Process* xp, Process* yp) { 
    Process temp = *xp; 
    *xp = *yp; 
    *yp = temp; 
} 
  
/*
    Function to perform selection sort
*/
void selectionSort(Process arr[], int n) { 
    int i, j, min_idx; 
  
    // One by one move boundary of 
    // unsorted subarray 
    for (i = 0; i < n - 1; i++) { 
        // Find the minimum element in 
        // unsorted array 
        min_idx = i; 
        for (j = i + 1; j < n; j++) 
            if (arr[j].begin < arr[min_idx].begin) 
                min_idx = j; 
  
        // Swap the found minimum element 
        // with the first element 
        swap(&arr[min_idx], &arr[i]); 
    } 
} 