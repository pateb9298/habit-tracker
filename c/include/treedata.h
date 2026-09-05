#ifndef TREEDATA_H
#define TREEDATA_H

typedef int   (*CompareFunc)(void *, void *);
typedef void  (*DestroyFunc)(void *);
typedef void *(*CopyFunc)(void *);
typedef void  (*PrintFunc)(void *);

// --- Example Data Functions for Strings ---

int compare_strings(void *a, void *b);
void destroy_string(void *data);
void *copy_string(void *data);
void print_string(void *data);

#endif