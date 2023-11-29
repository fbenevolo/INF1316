#include <stdio.h>
#include <unistd.h>

int main(void) {

    int vetor[3] = {1, 2, 3};
    int size = 3;
    int i = 0;

    while(1) {

        int previous = i-1;
        if (i == size) i = 0;
        if (i == 0) previous = size-1;

        printf("%d %d \n", i, previous);

        i++;
        sleep(1);
    }

}