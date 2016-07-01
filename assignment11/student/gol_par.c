#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <mpi.h>

#include "helper.h"

unsigned int gol(unsigned char *grid, unsigned int dim_x, unsigned int dim_y, unsigned int time_steps)
{
	// READ ME! Parallelize this function to work with MPI. It must work even with a single processor.
	// We expect you to use MPI_Scatterv, MPI_Gatherv, and MPI_Sendrecv to achieve this.
	// MPI_Scatterv/Gatherv are checked to equal np times, and MPI_Sendrecv is expected to equal 2 * np * timesteps
	// That is, top+bottom ghost cells * all processors must execute this command * Sendrecv executed every timestep.

	int rank, np;

	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &np);


	// Calculate domain decomposition
	int * lengths = malloc(sizeof(int) * np);
	int * offsets = malloc(sizeof(int) * np);

	for (int i=0; i<np; i++) {
		lengths[i] = dim_x * (dim_y/np + (i < (dim_y % np)));
	}
	offsets[0] = 0;
	for (int i=1; i<np; i++) {
		offsets[i] = offsets[i-1] + lengths[i-1];
	}


	// Allocate subdomain grid_in, grid_out
	unsigned char *grid_in, *grid_out;
	grid_in  = calloc(sizeof(unsigned char), lengths[rank] + 2*dim_x);
	grid_out = calloc(sizeof(unsigned char), lengths[rank] + 2*dim_x);

	if (grid_in == NULL || grid_out == NULL) exit(EXIT_FAILURE);


	// Scatterv subdomains to local grid_in
	MPI_Scatterv(grid, lengths, offsets, MPI_CHAR, grid_in + dim_x, lengths[rank], MPI_CHAR, 0, MPI_COMM_WORLD);


	for (int t = 0; t < time_steps; ++t) {

		// Send top row to above 
		MPI_Sendrecv(
			grid_in + dim_x, dim_x, MPI_CHAR, (rank - 1 + np)%np, 0,
			grid_in + lengths[rank] + dim_x, dim_x, MPI_CHAR, (rank+1)%np, 0, 
			MPI_COMM_WORLD, MPI_STATUS_IGNORE
		);

		// Send bottom row to below neighbor
		MPI_Sendrecv(
			grid_in + lengths[rank], dim_x, MPI_CHAR, (rank + 1)%np, 0,
			grid_in, dim_x, MPI_CHAR, (rank-1+np)%np, 0,
			MPI_COMM_WORLD, MPI_STATUS_IGNORE
		);

		for (int y = 1; y < lengths[rank]/dim_x + 1; ++y) {
			for (int x = 0; x < dim_x; ++x) {
				evolve(grid_in, grid_out, dim_x, lengths[rank]/dim_x + 2, x, y);
			}
		}
		swap((void**)&grid_in, (void**)&grid_out);
	}

	// Gatherv subdomains back to grid
	MPI_Gatherv(grid_in + dim_x, lengths[rank], MPI_CHAR, grid, lengths, offsets, MPI_CHAR, 0, MPI_COMM_WORLD);

	free(grid_in);
	free(grid_out);
	free(lengths);
	free(offsets);

	return (rank == 0) ? cells_alive(grid, dim_x, dim_y) : 0;
}


