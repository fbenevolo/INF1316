#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define ADDRESS_SIZE 32


struct virtualTable {
    int NPV;
    int R;
    int M;
    int last_access;
    int mem_page;
} typedef virtualTable;


int* createPhysicalMemory(int num_pages);
virtualTable* createVirtualTable(int num_pages);
int getShift(int frame_size);

// replace page algorithms
int LRU(virtualTable* virtual_table, int* rp_array, int num_real_pages);
int NRU(virtualTable* virtual_table, int* rp_array, int num_real_pages);