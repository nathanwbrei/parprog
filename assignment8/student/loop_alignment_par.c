#include <omp.h>
#include <stdio.h>

void compute(unsigned long *a, unsigned long *b, unsigned long *c, unsigned long *d, int N, int num_threads) {

	// printf("RUNNING PARALLEL VERSION\n\n");
	
	#pragma omp parallel for num_threads(num_threads) schedule(static)
	for (int i = 1; i < N+1; i++) {
		
		if (i > 1) {
			b[i] = 2 * c[i-1];
			// printf("b(%d) = 2 * c(%d)\n", i,i-1);
		}
		if (i < N) {
			a[i] = d[i] * b[i];
			c[i-1] = a[i] * d[i];
			// printf("a(%d) = d(%d) * b(%d)\n", i,i,i);
			// printf("c(%d) = a(%d) * d(%d)\n", i-1,i,i);
		}
	}
	
		
}
