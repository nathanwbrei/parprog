#include <omp.h>
#include <stdio.h>

void compute(unsigned long **a, unsigned long **b, unsigned long **c, unsigned long **d, int N, int num_threads) {

	for (int j = 1; j < N; j++) {

		#pragma omp parallel for num_threads(num_threads) schedule(static)
		for (int i = 1; i < N; i++) {
			a[i][j] = a[i][j] * b[i][j+1] + d[i][j-1];
			c[i][j] = 3 * d[i][j];
		}

		#pragma omp parallel for num_threads(num_threads) schedule(static)
		for (int i = 1; i < N; i++) {
			b[i][j] = 2 * a[i][j] * c[i-1][j];
			d[i][j] = 2 * c[i-1][j];
		}
	}
}
