#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "sim-virtual.h"


int main(int argc, char* argv[]) {

    char algorithm[3];
    FILE* log_file;
    int mem_size, frame_size, 
        shift, 
        time,
        page_faults, pages_written,
        pages_occupied;

    // Opening file
    log_file = fopen(argv[2], "r");
    if (!log_file) {
        fprintf(stderr, "error in opening file\n");
        exit(-1);
    }

    // Setting each frame (real page) size
    frame_size = atoi(argv[3]);
    if (frame_size != 8 && frame_size != 16) {
        fprintf(stderr, "frame size is not supported\n");
        exit(-2);
    }

    // Setting memory total size
    mem_size = atoi(argv[4]);
    if (mem_size < 1 || mem_size > 4) {
        fprintf(stderr, "memory size is not supported\n");
        exit(-3);
    }

    // Setting the replace algorithm to be used
    strcpy(algorithm, argv[1]);

    // Creating real pages array
    int num_pages = mem_size * 1024 / frame_size;
    int* rp_array = createPhysicalMemory(num_pages); 

    // Setting shift necessary to obtain page number
    shift = getShift(frame_size);   

    // Creating virtual table
    virtualTable* virtual_table = createVirtualTable(ADDRESS_SIZE - shift);

    // Setting some variables to 0
    page_faults = 0; time = 0; pages_occupied = 0;
    pages_written = 0;


    unsigned addr; char rw; // auxiliary variables
    int pg_index, aux_mem_page;

    while(!feof(log_file)) {
        fscanf(log_file, "%x %c\n", &addr, &rw);
        
        pg_index = addr >> shift;

        // Time to set reference bits to zero
        if (time % num_pages == 0 && strcmp(algorithm, "NRU") == 0) {
            setReferenceBitsToZero(virtual_table, rp_array, num_pages);
        }
        
        // if page is not in memory
        if (virtual_table[pg_index].mem_page == -1) {
            page_faults++; 
        
            // if there are free pages in memory
            if (pages_occupied < num_pages) {
                rp_array[pages_occupied] = pg_index; // frame receives virtual page
                virtual_table[pg_index].mem_page = pages_occupied; // update virtual table entrance

                pages_occupied++;
            }
            // else, some page has to be replaced
            else {
                int removed_page = -1;

                if (strcmp(algorithm, "NRU") == 0) {
                    removed_page = NRU(virtual_table, rp_array, num_pages);
                }
                else if (strcmp(algorithm, "LRU") == 0) {
                    removed_page = LRU(virtual_table, rp_array, num_pages);
                }

                // If page was modified, writes it back in the disk
                if (virtual_table[removed_page].M == 1) pages_written++;

                // Getting real memory page
                aux_mem_page = virtual_table[removed_page].mem_page;

                // Removing virtual page from memory
                virtual_table[removed_page].R = 0;
                virtual_table[removed_page].M = 0;
                virtual_table[removed_page].mem_page = -1;

                // Putting new page in memory
                rp_array[aux_mem_page] = pg_index;
                virtual_table[pg_index].mem_page = aux_mem_page;
            }

        }

        virtual_table[pg_index].R = 1;
        virtual_table[pg_index].last_access = time;
        if (rw == 'W') virtual_table[pg_index].M = 1; 

        time++; 
    }
    
    
    printf("Executando o Simulador...\n"
    "Arquivo de Entrada: %s\n"
    "Tamanho da Memória Fisica: %d MB\n"
    "Tamanho das Páginas: %d KB\n"
    "Algoritmo de Substituição: %s\n"
    "Número de Faltas de Páginas: %d\n"
    "Número de Páginas Escritas: %d\n",
    argv[2], mem_size, frame_size, algorithm, page_faults, pages_written);
    

    // closing file
    fclose(log_file);

    return 0;
}


/*
    Function to create the 
    real pages array
*/
int* createPhysicalMemory(int num_pages) {
    int* rp_array = (int*)malloc(num_pages  * sizeof(int));
    if (!rp_array) {
        fprintf(stderr, "Unable to create physical memory");
        exit(EXIT_FAILURE);
    } 

    for (int i = 0; i < num_pages; i++) rp_array[i] = -1;

    return rp_array;
}


/*
    Creates virtual table
*/
virtualTable* createVirtualTable(int num_pages) {

    int vt_size = (int)pow(2, num_pages);

    virtualTable* virtual_table = (virtualTable*)malloc(vt_size * sizeof(virtualTable));
    if (!virtual_table) {
        fprintf(stderr, "Could not allocate virtual table\n");
        exit(EXIT_FAILURE);
    }

    // inicializing virtual table entrances
    for (int i = 0; i < vt_size; i++) {
        virtual_table[i].NPV = i;
        virtual_table[i].R = 0;
        virtual_table[i].M = 0;
        virtual_table[i].mem_page = -1;
        virtual_table[i].last_access = -1;
    }
    
    return virtual_table;
}


/*
    Returns the shift value needed 
    to obtain a page number
*/
int getShift(int frame_size) {

    int shift = 0;
    frame_size *= 1024; // Obtaining frame size in bytes

    while(frame_size > 1) {
        frame_size /= 2;
        shift++;
    }

    return shift;
}


/*
    Least-Recently Used
    Returns the virtual page index that will be removed
*/
int LRU(virtualTable* virtual_table, int* rp_array, int num_pages) {

    int index_removed = rp_array[0];

    int indexVT;
    // Searching for virtual page to be replaced from memory
    for (int i = 1; i < num_pages; i++) {
        indexVT = rp_array[i];

        if (virtual_table[indexVT].last_access < virtual_table[index_removed].last_access) {
            index_removed = rp_array[i];
        }
    }

    return index_removed;
}


/*
    Not-Recently-Used
    Returns the virtual page index that will be removed
*/
int NRU(virtualTable* virtual_table, int* rp_array, int num_real_pages) {

    int nRnM[num_real_pages], nRM[num_real_pages],
        RnM[num_real_pages], RM[num_real_pages];
    int index_nRnM, index_nRM, index_RnM, index_RM;
    int indexVT, index_removed;


    index_nRnM = index_nRM = index_RnM = index_RM = 0;
    nRnM[0] = nRM[0] = RnM[0] = RM[0] = -1;


    for (int i = 1; i < num_real_pages; i++) {
        indexVT = rp_array[i];

        if (virtual_table[indexVT].R == 0 && virtual_table[indexVT].M == 0) {
             nRnM[index_nRnM] = indexVT; 
             index_nRnM++;
        }
        else if (virtual_table[indexVT].R == 0 && virtual_table[indexVT].M == 1) {
            nRM[index_nRM] = indexVT;
            index_nRM++;
        }
        else if (virtual_table[indexVT].R == 1 && virtual_table[indexVT].M == 0) {
            RnM[index_RnM] = indexVT;
            index_RnM++;
        }
        else if (virtual_table[indexVT].R == 1 && virtual_table[indexVT].M == 1) {
            RM[index_RM] = indexVT;
            index_RM++;
        }
    }

    if (nRnM[0] != -1)     index_removed = LRU(virtual_table, nRnM, index_nRnM);
    else if (nRM[0] != -1) index_removed = LRU(virtual_table, nRM, index_nRM);
    else if (RnM[0] != -1) index_removed = LRU(virtual_table, RnM, index_RnM);
    else if (RM[0] != -1)  index_removed = LRU(virtual_table, RM, index_RM);

    return index_removed;

}


/*
    Auxiliary function that set 
    reference bits of pages 
    that are in memory to zero
*/
void setReferenceBitsToZero(virtualTable* virtual_table, int* rp_array, int num_real_pages) {

    int virtual_index;
    for (int i = 0; i < num_real_pages; i++) {
        virtual_index = rp_array[i];
        if (virtual_index != -1) virtual_table[virtual_index].R = 0;
    }

    return;
}