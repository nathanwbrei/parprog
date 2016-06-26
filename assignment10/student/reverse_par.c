#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include "helper.h"

void reverse(char *str, int strlen)
{
	// parallelize this function and make sure to call reverse_str()
	// on each processor to reverse the substring.
	
	int np, rank;

    MPI_Comm_size(MPI_COMM_WORLD, &np);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int * lengths = malloc(np * sizeof(int));
    int * from_disps = malloc(np * sizeof(int));
    int * to_disps = malloc(np * sizeof(int));
    char * my_str = malloc((strlen/np + 1)*sizeof(char));

//    S T R I N G T O S P L I T
//    S T R I N - G T O S P - L I T
//    N I R T S - P S O T G - T I L 
//    T I L  - P S O T G - N I R T S

    int l = strlen/np + 1;
    int remainder = strlen % l;
    for (int i = 0; i < np; i++) {
        lengths[i] = l;
        from_disps[i] = l * i;
        to_disps[i] = remainder + l * (np - i -2);
    }
    lengths[np-1] = remainder;
    to_disps[np-1] = 0;



    MPI_Scatterv(str, lengths, from_disps, MPI_CHAR, my_str, lengths[rank], MPI_CHAR, 0, MPI_COMM_WORLD);

    printf("Rank %i received %d chars: %d => %d : '%s'\n", rank, lengths[rank], from_disps[rank], to_disps[rank], my_str);

    reverse_str(my_str, lengths[rank]);

    MPI_Gatherv (my_str, lengths[rank], MPI_CHAR, str, lengths, to_disps, MPI_CHAR, 0, MPI_COMM_WORLD);


    free(my_str);    
    free(lengths);
    free(from_disps);
    free(to_disps);
}

