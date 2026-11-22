#ifndef VAULT_AVL_TREE_H
#define VAULT_AVL_TREE_H

#include "backend/BST.h"

class AVLTree : public BST
{
private:
    Node* rotateLeft(Node* pivot, const std::string& kind);
    Node* rotateRight(Node* pivot, const std::string& kind);
    void rotationEvent(Node* pivot, const std::string& kind);
    bool balanced(const Node* node) const;
    int countBelow(int key, bool inclusive) const;

protected:
    void repair(Node* node) override;

public:
    bool primitiveRotate(int pivotId, const std::string& direction, const std::string& kind);
    std::string rotationCase(int pivotId) const;
    IntBuffer kthPath(int k) const;
    int rankWithEvents(int key);
    bool manualRepair(int pivotId, const std::string& kind);
    bool kthSmallest(int k, int& value) const;
    int rank(int value) const;
    int countInRange(int low, int high) const;
    bool validate() const;
};

#endif
