#ifndef VAULT_BST_H
#define VAULT_BST_H

#include "backend/BinaryTree.h"

class BST : public BinaryTree
{
protected:
    long long comparisonCount = 0;
    long long rotationCount = 0;
    Node* locate(int value, bool record);
    void compare(Node* node, int value);
    void updateMetadata(Node* node);
    void rangeNodes(Node* node, int low, int high, IntBuffer& output);
    bool ordered(const Node* node, long long low, long long high) const;
    virtual void repair(Node* node);

public:
    long long getComparisons() const;
    long long getRotations() const;
    void resetCounters();
    bool compareRoom(int id, int key);
    bool insertAt(int parentId, int key);
    virtual bool insert(int value);
    bool search(int value);
    virtual bool deleteKey(int value);
    bool minimum(int& value) const;
    bool maximum(int& value) const;
    bool successor(int key, int& value) const;
    bool predecessor(int key, int& value) const;
    IntBuffer range(int low, int high);
    bool kthSmallest(int k, int& value) const;
    bool lca(int first, int second, int& value);
    bool validate() const;
};

#endif
