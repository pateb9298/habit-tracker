#include <stdlib.h>
#include <stdio.h>
#include "tree.h"

#ifdef GRADER
#include <stdio.h>
static FILE *logfile = NULL;

#define LOG(action) do { \
    if (!logfile) logfile = fopen("grader_log.txt", "a"); \
    if (logfile) { \
        fprintf(logfile, "[GRADER] %s\n", action); \
        fflush(logfile); \
    } \
} while (0)
#else
#define LOG(action)
#endif

// --- Internal Struct Definition ---

struct TreeNode {
    void *data;
    TreeNode *left;
    TreeNode *right;
    int height;
};

// --- Utility Functions ---

static int max(int a, int b) {
    return (a > b) ? a : b;
}

static int height(TreeNode *node) {
    return node ? node->height : -1;
}

static void updateHeight(TreeNode *node) {
    if (node) {
        node->height = 1 + max(height(node->left), height(node->right));
        LOG("updateHeight");
    }
}

static int getBalance(TreeNode *node) {
    int balance = node ? height(node->left) - height(node->right) : 0;
    LOG("getBalance");
    return balance;
}

static TreeNode *rotateRight(TreeNode *y) {
    LOG("rotateRight");
    TreeNode *x = y->left;
    // NOLINTNEXTLINE(clang-analyzer-core.NullDereference)
    TreeNode *T2 = x->right;

    x->right = y;
    y->left = T2;

    updateHeight(y);
    updateHeight(x);

    return x;
}

static TreeNode *rotateLeft(TreeNode *x) {
    LOG("rotateLeft");
    TreeNode *y = x->right;
    TreeNode *T2 = y->left;

    y->left = x;
    x->right = T2;

    updateHeight(x);
    updateHeight(y);

    return y;
}

// --- Recursive Insert ---
// NOLINTNEXTLINE(misc-no-recursion) 
static TreeNode *insertRecursive(Tree *tree, TreeNode *node, void *data) {
    if (!node) {
        LOG("insert: create new node");
        TreeNode *newNode = malloc(sizeof(TreeNode));
        newNode->data = tree->copy(data);
        newNode->left = NULL;
        newNode->right = NULL;
        newNode->height = 0;
        return newNode;
    }

    int cmp = tree->compare(data, node->data);
    if (cmp < 0) {
        LOG("insert: left");
        node->left = insertRecursive(tree, node->left, data);
    } else if (cmp > 0) {
        LOG("insert: right");
        node->right = insertRecursive(tree, node->right, data);
    } else {
        LOG("insert: duplicate");
        return node;
    }

    updateHeight(node);
    int balance = getBalance(node);

    if (balance > 1 && tree->compare(data, node->left->data) < 0) {
        LOG("rebalance: LL");
        return rotateRight(node);
    }

    if (balance < -1 && tree->compare(data, node->right->data) > 0) {
        LOG("rebalance: RR");
        return rotateLeft(node);
    }

    if (balance > 1 && tree->compare(data, node->left->data) > 0) {
        LOG("rebalance: LR");
        node->left = rotateLeft(node->left);
        return rotateRight(node);
    }

    if (balance < -1 && tree->compare(data, node->right->data) < 0) {
        LOG("rebalance: RL");
        node->right = rotateRight(node->right);
        return rotateLeft(node);
    }

    return node;
}

// --- Recursive Destroy ---
// NOLINTNEXTLINE(misc-no-recursion) 
static void destroyRecursive(Tree *tree, TreeNode *node) {
    if (!node) return;
    LOG("destroy node");
    destroyRecursive(tree, node->left);
    destroyRecursive(tree, node->right);
    tree->destroy(node->data);
    free(node);
}

// --- Recursive Search ---
// NOLINTNEXTLINE(misc-no-recursion) 
static TreeNode *findRecursive(Tree *tree, TreeNode *node, void *key) {
    if (!node) return NULL;
    LOG("search node");
    int cmp = tree->compare(key, node->data);
    if (cmp == 0) {
        return node;
    } 
    if (cmp < 0) {
        return findRecursive(tree, node->left, key);
    }
    return findRecursive(tree, node->right, key);
}   

// --- Recursive Print ---

// NOLINTNEXTLINE(misc-no-recursion) 
static void printRecursive(TreeNode *node, PrintFunc print) {
    if (!node) return;
    LOG("print node");
    printRecursive(node->left, print);
    print(node->data);
    printRecursive(node->right, print);
}

// --- Public Tree API ---

Tree *create_tree(CompareFunc compare, DestroyFunc destroy, CopyFunc copy) {
    LOG("create_tree");
    Tree *tree = malloc(sizeof(Tree));
    tree->root = NULL;
    tree->compare = compare;
    tree->destroy = destroy;
    tree->copy = copy;
    return tree;
}

void destroy_tree(Tree *tree) {
    if (!tree) return;
    LOG("destroy_tree");
    destroyRecursive(tree, tree->root);
    free(tree);
    #ifdef GRADER
    if (logfile) {
        fclose(logfile);
        logfile = NULL;
    }
#endif
}

int insert_node(Tree *tree, void *data) {
    if (!tree || !data) return -1;
    LOG("insert_node");
    tree->root = insertRecursive(tree, tree->root, data);
    return 0;
}

TreeNode *find_node(Tree *tree, void *key) {
    LOG("find_node");
    return findRecursive(tree, tree->root, key);
}

void print_in_order(Tree *tree, PrintFunc print_data) {
    if (!tree || !print_data) return;
    LOG("print_in_order");
    printRecursive(tree->root, print_data);
}

// --- Accessors ---

TreeNode *get_root(Tree *tree) {
    LOG("get_root");
    return tree ? tree->root : NULL;
}

void *get_node_data(TreeNode *node) {
    LOG("get_node_data");
    return node ? node->data : NULL;
}

TreeNode *get_left_child(TreeNode *node) {
    LOG("get_left_child");
    return node ? node->left : NULL;
}

TreeNode *get_right_child(TreeNode *node) {
    LOG("get_right_child");
    return node ? node->right : NULL;
}

int get_node_height(TreeNode *node) {
    LOG("get_node_height");
    return height(node);
}