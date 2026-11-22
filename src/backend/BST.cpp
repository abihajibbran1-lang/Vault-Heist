#include "backend/BST.h"
#include <climits>

long long BST::getComparisons() const
{
    return comparisonCount;
}

long long BST::getRotations() const
{
    return rotationCount;
}

void BST::resetCounters()
{
    comparisonCount = 0;
    rotationCount = 0;
}

bool BST::compareRoom(int id, int key)
{
    events.clear();
    Node* node = findId(root, id);
    if (node == NULL)
    {
        return false;
    }
    compare(node, key);
    emit("DONE", NULL);
    return true;
}

bool BST::insertAt(int parentId, int key)
{
    events.clear();
    Node* current = root;
    Node* parent = NULL;
    while (current != NULL)
    {
        if (current->value == key)
        {
            return false;
        }
        parent = current;
        current = key < current->value ? current->left : current->right;
    }
    if (parent == NULL || parent->id != parentId)
    {
        return false;
    }
    Node* node = new Node(nextId++, key, parent);
    if (key < parent->value)
    {
        parent->left = node;
    }
    else
    {
        parent->right = node;
    }
    emit("INSERT", node, true, key < parent->value ? "LEFT" : "RIGHT");
    repair(parent);
    emit("DONE", NULL, true);
    return true;
}

void BST::compare(Node* node, int value)
{
    comparisonCount++;
    emit("VISIT", node);
    if (events.isEnabled())
    {
        BackendEvent event;
        event.type = "COMPARE";
        event.nodeId = node->id;
        event.oldValue = node->value;
        event.newValue = value;
        if (value < node->value)
        {
            event.detail = "LESS";
        }
        else if (value > node->value)
        {
            event.detail = "GREATER";
        }
        else
        {
            event.detail = "EQUAL";
        }
        events.append(event);
    }
}

BinaryTree::Node* BST::locate(int value, bool record)
{
    Node* current = root;
    while (current != NULL)
    {
        if (record)
        {
            compare(current, value);
        }
        if (current->value == value)
        {
            return current;
        }
        current = value < current->value ? current->left : current->right;
    }
    return NULL;
}

void BST::updateMetadata(Node* node)
{
    int oldHeight = node->height;
    int oldSize = node->subtreeSize;
    refresh(node);
    if (events.isEnabled())
    {
        if (oldHeight != node->height)
        {
            BackendEvent event;
            event.type = "UPDATE_HEIGHT";
            event.nodeId = node->id;
            event.oldValue = oldHeight;
            event.newValue = node->height;
            events.append(event);
        }
        if (oldSize != node->subtreeSize)
        {
            BackendEvent event;
            event.type = "UPDATE_SIZE";
            event.nodeId = node->id;
            event.oldValue = oldSize;
            event.newValue = node->subtreeSize;
            events.append(event);
        }
    }
}

void BST::repair(Node* node)
{
    while (node != NULL)
    {
        updateMetadata(node);
        node = node->parent;
    }
}

bool BST::insert(int value)
{
    events.clear();
    Node* current = root;
    Node* parent = NULL;
    while (current != NULL)
    {
        compare(current, value);
        if (current->value == value)
        {
            emit("DONE", NULL, false, "Duplicate key rejected");
            return false;
        }
        parent = current;
        current = value < current->value ? current->left : current->right;
    }
    Node* node = new Node(nextId++, value, parent);
    std::string side = "ROOT";
    if (parent == NULL)
    {
        root = node;
    }
    else if (value < parent->value)
    {
        parent->left = node;
        side = "LEFT";
    }
    else
    {
        parent->right = node;
        side = "RIGHT";
    }
    // Capture at the actual insertion boundary, before rebalancing.
    emit("INSERT", node, true, side);
    repair(parent);
    emit("DONE", NULL, true);
    return true;
}

bool BST::search(int value)
{
    events.clear();
    bool found = locate(value, true) != NULL;
    emit("DONE", NULL, false, found ? "Target found" : "Key absent");
    return found;
}

bool BST::deleteKey(int value)
{
    events.clear();
    Node* node = locate(value, true);
    if (node == NULL)
    {
        emit("DONE", NULL, false, "Key absent");
        return false;
    }
    if (node->left != NULL && node->right != NULL)
    {
        Node* successor = node->right;
        emit("VISIT", successor);
        while (successor->left != NULL)
        {
            successor = successor->left;
            emit("VISIT", successor);
        }
        int oldValue = node->value;
        node->value = successor->value;
        successor->value = oldValue;
        if (events.isEnabled())
        {
            BackendEvent event;
            event.type = "SWAP_VALUE";
            event.nodeId = node->id;
            event.otherNodeId = successor->id;
            event.oldValue = oldValue;
            event.newValue = node->value;
            event.hasFrame = true;
            event.frame = snapshot();
            events.append(event);
        }
        node = successor;
    }
    Node* parent = node->parent;
    Node* child = node->left != NULL ? node->left : node->right;
    std::string side = "ROOT";
    if (parent == NULL)
    {
        root = child;
    }
    else if (parent->left == node)
    {
        parent->left = child;
        side = "LEFT";
    }
    else
    {
        parent->right = child;
        side = "RIGHT";
    }
    if (child != NULL)
    {
        child->parent = parent;
    }
    // Refresh counts before capturing a frame: snapshot allocation uses root size.
    for (Node* current = parent; current != NULL; current = current->parent)
    {
        updateMetadata(current);
    }
    emit("UNLINK", node, true, side);
    delete node;
    repair(parent);
    emit("DONE", NULL, true);
    return true;
}

bool BST::minimum(int& value) const
{
    Node* node = root;
    if (node == NULL)
    {
        return false;
    }
    while (node->left != NULL)
    {
        node = node->left;
    }
    value = node->value;
    return true;
}

bool BST::maximum(int& value) const
{
    Node* node = root;
    if (node == NULL)
    {
        return false;
    }
    while (node->right != NULL)
    {
        node = node->right;
    }
    value = node->value;
    return true;
}

bool BST::successor(int key, int& value) const
{
    Node* current = root;
    Node* candidate = NULL;
    bool found = false;
    while (current != NULL)
    {
        if (current->value == key)
        {
            found = true;
        }
        if (current->value > key)
        {
            candidate = current;
            current = current->left;
        }
        else
        {
            current = current->right;
        }
    }
    if (found && candidate != NULL)
    {
        value = candidate->value;
        return true;
    }
    return false;
}

bool BST::predecessor(int key, int& value) const
{
    Node* current = root;
    Node* candidate = NULL;
    bool found = false;
    while (current != NULL)
    {
        if (current->value == key)
        {
            found = true;
        }
        if (current->value < key)
        {
            candidate = current;
            current = current->right;
        }
        else
        {
            current = current->left;
        }
    }
    if (found && candidate != NULL)
    {
        value = candidate->value;
        return true;
    }
    return false;
}

void BST::rangeNodes(Node* node, int low, int high, IntBuffer& output)
{
    if (node == NULL)
    {
        return;
    }
    emit("VISIT", node);
    if (node->value > low)
    {
        rangeNodes(node->left, low, high, output);
    }
    else if (node->left != NULL)
    {
        emit("HIGHLIGHT_SUBTREE", node->left, false, "SKIPPED");
    }
    if (node->value >= low && node->value <= high)
    {
        output.append(node->value);
        emit("HIGHLIGHT_SUBTREE", node, false, "RESULT");
    }
    if (node->value < high)
    {
        rangeNodes(node->right, low, high, output);
    }
    else if (node->right != NULL)
    {
        emit("HIGHLIGHT_SUBTREE", node->right, false, "SKIPPED");
    }
}

IntBuffer BST::range(int low, int high)
{
    events.clear();
    IntBuffer output;
    if (low <= high)
    {
        rangeNodes(root, low, high, output);
    }
    emit("DONE", NULL);
    return output;
}

bool BST::kthSmallest(int k, int& value) const
{
    if (k <= 0 || k > size())
    {
        return false;
    }
    Node** stack = new Node*[height()];
    int count = 0;
    Node* current = root;
    while (current != NULL || count > 0)
    {
        if (current != NULL)
        {
            stack[count++] = current;
            current = current->left;
        }
        else
        {
            current = stack[--count];
            k--;
            if (k == 0)
            {
                value = current->value;
                delete[] stack;
                return true;
            }
            current = current->right;
        }
    }
    delete[] stack;
    return false;
}

bool BST::lca(int first, int second, int& value)
{
    events.clear();
    if (locate(first, false) == NULL || locate(second, false) == NULL)
    {
        emit("DONE", NULL, false, "Key absent");
        return false;
    }
    Node* current = root;
    while (current != NULL)
    {
        emit("VISIT", current);
        if (first < current->value && second < current->value)
        {
            current = current->left;
        }
        else if (first > current->value && second > current->value)
        {
            current = current->right;
        }
        else
        {
            value = current->value;
            emit("DONE", current);
            return true;
        }
    }
    return false;
}

bool BST::ordered(const Node* node, long long low, long long high) const
{
    if (node == NULL)
    {
        return true;
    }
    return node->value > low && node->value < high &&
           ordered(node->left, low, node->value) && ordered(node->right, node->value, high);
}

bool BST::validate() const
{
    return BinaryTree::validate() && ordered(root, LLONG_MIN, LLONG_MAX);
}
