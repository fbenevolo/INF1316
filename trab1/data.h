#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h> 

// shared memory keys
#define SHM_P 0x1000
#define SHM_NUM 0x5000

// useful constants
#define MAX 10
#define EVER ;;
#define VALID 1
#define INVALID 0
#define LIMIT 120
#define PROGRAM_MAX_NAME 30
#define REAL_TIME 1 
#define ROUND_ROBIN 2

// processes data
struct process {
    char program[PROGRAM_MAX_NAME];
    int policy;
    int index;
    int begin;
    int duration;
    pid_t pid;
} typedef Process;