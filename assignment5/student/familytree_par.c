#include "familytree.h"

void traverse(tree *node, int numThreads){
	
    #pragma omp sections
    {
        if (node != NULL) {
            node->IQ = compute_IQ(node->data);
            genius[node->id] = node->IQ;

            #pragma omp section
            traverse(node->right, numThreads);

            #pragma omp section
            traverse(node->left, numThreads); 
        }
    }
}

