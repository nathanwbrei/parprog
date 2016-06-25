#include <stdio.h>
#include <omp.h>

int main(int argc, char ** argv) {

    printf("Hello world\n");

    #pragma omp parallel
    {
        printf("Hello from thread %d\n", omp_get_thread_num());
    }


    printf("Shut down threads. Restarting with more:\n");

    #pragma omp parallel num_threads(7)
    {
        printf("Hello from thread %d\n", omp_get_thread_num());
    }


    printf("Parallel for loop\n");

    #pragma omp parallel
    {
        #pragma omp for
        for (int i=0; i<10; i++) {
            printf("Calculated %d from %d\n", i, omp_get_thread_num());
        }
    }


    printf("Nested loops\n");

    #pragma omp parallel
    {
        for (int i=0; i<3; i++) {
            for (int j=0; j<3; j++) {
                #pragma omp for
                for (int k=0; k<3; k++) {
                    printf("Thread %d processing (%d,%d,%d)\n", omp_get_thread_num(), i,j,k);
                }
            }

        }
    }


}
