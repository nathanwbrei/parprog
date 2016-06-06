#include "familytree.h"
#include <omp.h>

void traverse(tree *node, int numThreads) {

    if (node != NULL) {
        node->IQ = compute_IQ(node->data);
        genius[node->id] = node->IQ;

        #pragma omp task firstprivate(node)
        traverse(node->right, numThreads);

        #pragma omp task firstprivate(node)
        traverse(node->left, numThreads);
    }

}

