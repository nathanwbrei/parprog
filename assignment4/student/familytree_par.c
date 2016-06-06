#include "familytree.h"
#include <omp.h>

void visit(tree *node, int numThreads) {

    if (node != NULL) {
        node->IQ = compute_IQ(node->data);
        genius[node->id] = node->IQ;

        #pragma omp task firstprivate(node)
        traverse(node->right, numThreads);

        #pragma omp task firstprivate(node)
        traverse(node->left, numThreads);
    }

}


void traverse(tree *node, int numThreads) {
    omp_set_max_active_levels(numThreads);
    #pragma omp parallel 
    {
        #pragma omp single
        {
            visit(node, numThreads);
        }
    }
}
