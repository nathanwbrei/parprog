#include "familytree.h"
#include <omp.h>

void visit(tree *node) {

    if (node != NULL) {
        node->IQ = compute_IQ(node->data);
        genius[node->id] = node->IQ;

        #pragma omp task firstprivate(node)
        visit(node->right);

        #pragma omp task firstprivate(node)
        visit(node->left);
    }

}


void traverse(tree *node, int numThreads) {
    omp_set_nested(1);
    omp_set_max_active_levels(numThreads);
    #pragma omp parallel 
    {
        #pragma omp single
        {
            visit(node);
        }
    }
}
