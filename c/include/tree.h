#ifndef TREE_H
#define TREE_H

#include "treedata.h"

// Opaque type
typedef struct TreeNode TreeNode;

typedef struct Tree {
    TreeNode *root;
    CompareFunc compare;
    DestroyFunc destroy;
    CopyFunc copy;
} Tree;

// --- Core Tree Operations ---

/**
 * Creates a new, empty tree with the specified function pointers.
 */
Tree *create_tree(CompareFunc compare, DestroyFunc destroy, CopyFunc copy);

/**
 * Frees all memory used by the tree.
 */
void destroy_tree(Tree *tree);

/**
 * Inserts a new item into the tree.
 * Returns 0 on success, non-zero on failure.
 */
int insert_node(Tree *tree, void *data);

/**
 * Finds a node containing the given key.
 * Returns a pointer to the TreeNode, or NULL if not found.
 */
TreeNode *find_node(Tree *tree, void *key);

/**
 * Prints the tree in-order using the provided print function.
 */
void print_in_order(Tree *tree, PrintFunc print_data);

// --- Tree Accessors ---

/**
 * Returns the root node of the tree.
 */
TreeNode *get_root(Tree *tree);

/**
 * Returns the data stored in a node.
 */
void *get_node_data(TreeNode *node);

/**
 * Returns the left child of a node.
 */
TreeNode *get_left_child(TreeNode *node);

/**
 * Returns the right child of a node.
 */
TreeNode *get_right_child(TreeNode *node);

/**
 * Returns the height of a node.
 */
int get_node_height(TreeNode *node);

#endif