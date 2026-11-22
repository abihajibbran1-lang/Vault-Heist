#ifndef VAULT_BACKEND_ADAPTER_H
#define VAULT_BACKEND_ADAPTER_H

#include "backend/BinaryTree.h"
#include "backend/BST.h"
#include "backend/AVLTree.h"
#include "backend/MaxHeap.h"
#include "middleware/GameEvent.h"

class BackendAdapter
{
private:
    BinaryTree tree;
    BST bst;
    AVLTree avl;
    MaxHeap heap;
    std::string structure;
    EventBatch events;

public:
    bool validateStructure() const;
    std::string rotationCase(int pivot) const;
    IntBuffer query(const std::string& name, int first = -1, int second = -1) const;
    bool equals(const BackendAdapter& other) const;
    BackendAdapter();
    bool select(const std::string& name);
    bool execute(const PlayerAction& action, std::string& message);
    bool validate() const;
    GameSnapshot snapshot() const;
    const EventBatch& getEvents() const;
    bool contains(int value) const;
    int size() const;
    std::string predict(const std::string& operation, int value) const;
    std::string serializeTree() const;
    bool restoreAVL(const std::string& text);
    bool load(const std::string& name, const IntBuffer& values, const IntBuffer& inorder, bool raw);
};

#endif
