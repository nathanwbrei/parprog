void compute(unsigned long *a, unsigned long *b, unsigned long *c, unsigned long *d, int N, int num_threads) {

	for (int i = 1; i < N; i++) {
		
		a[i] = d[i] * b[i];
		b[i + 1] = 2 * c[i];
		c[i - 1] = a[i] * d[i];
		
	}
}
