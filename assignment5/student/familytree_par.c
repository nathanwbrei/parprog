#include "familytree.h"
#include <omp.h>

void traverse(tree *node, int numThreads){
	
    if (node != NULL) {
       
       omp_set_nested(1);
       omp_set_max_active_levels(numThreads);
       #pragma omp parallel sections 
       {
            #pragma omp section
            {
                node->IQ = compute_IQ(node->data);
                genius[node->id] = node->IQ;
            }
            #pragma omp section
            traverse(node->right, numThreads);

            #pragma omp section
            traverse(node->left, numThreads); 
        }
    }
}

