#include "backend/AVLTree.h"

std::string AVLTree::rotationCase(int pivotId) const
{
    Node* node = findId(root, pivotId);
    if (node == NULL)
    {
        return "NONE";
    }
    int balance = nodeHeight(node->left) - nodeHeight(node->right);
    if (balance > 1)
    {
        return nodeHeight(node->left->left) >= nodeHeight(node->left->right) ? "LL" : "LR";
    }
    if (balance < -1)
    {
        return nodeHeight(node->right->right) >= nodeHeight(node->right->left) ? "RR" : "RL";
    }
    return "NONE";
}

bool AVLTree::primitiveRotate(int pivotId, const std::string& direction, const std::string& kind)
{
    events.clear();
    Node* pivot = findId(root, pivotId);
    if (pivot == NULL || (kind != "LR" && kind != "RL") || (direction != "LEFT" && direction != "RIGHT") ||
        (direction == "LEFT" && pivot->right == NULL) || (direction == "RIGHT" && pivot->left == NULL))
    {
        return false;
    }
    Node* top = direction == "LEFT" ? rotateLeft(pivot, kind) : rotateRight(pivot, kind);
    for (Node* node = top->parent; node != NULL; node = node->parent)
    {
        updateMetadata(node);
    }
    emit("DONE", NULL, true);
    return true;
}

IntBuffer AVLTree::kthPath(int k) const
{
    IntBuffer path;
    if (k < 1 || k > size())
    {
        return path;
    }
    Node* node = root;
    while (node != NULL)
    {
        path.append(node->id);
        int left = nodeSize(node->left);
        if (k == left + 1)
        {
            break;
        }
        if (k <= left)
        {
            node = node->left;
        }
        else
        {
            k -= left + 1;
            node = node->right;
        }
    }
    return path;
}

int AVLTree::rankWithEvents(int key)
{
    events.clear();
    int count = 0;
    Node* node = root;
    while (node != NULL)
    {
        compare(node, key);
        if (node->value <= key)
        {
            count += nodeSize(node->left) + 1;
            node = node->right;
        }
        else
        {
            node = node->left;
        }
    }
    emit("DONE", NULL);
    return count;
}

bool AVLTree::manualRepair(int pivotId, const std::string& kind)
{
    events.clear();
    Node* node = findId(root, pivotId);
    if (node == NULL)
    {
        return false;
    }
    int balance = nodeHeight(node->left) - nodeHeight(node->right);
    std::string expected;
    if (balance > 1)
    {
        expected = nodeHeight(node->left->left) >= nodeHeight(node->left->right) ? "LL" : "LR";
    }
    else if (balance < -1)
    {
        expected = nodeHeight(node->right->right) >= nodeHeight(node->right->left) ? "RR" : "RL";
    }
    if (kind != expected || expected.empty())
    {
        return false;
    }
    if (kind == "LL" || kind == "LR")
    {
        if (kind == "LR")
        {
            rotateLeft(node->left, "LR");
        }
        node = rotateRight(node, kind);
    }
    else
    {
        if (kind == "RL")
        {
            rotateRight(node->right, "RL");
        }
        node = rotateLeft(node, kind);
    }
    repair(node->parent);
    emit("DONE", NULL, true);
    return true;
}

void AVLTree::rotationEvent(Node* pivot, const std::string& kind)
{
    rotationCount++;
    if (events.isEnabled())
    {
        BackendEvent event;
        event.type = "ROTATE";
        event.pivotId = pivot->id;
        event.nodeId = pivot->id;
        event.detail = kind;
        event.hasFrame = true;
        event.frame = snapshot();
        events.append(event);
    }
}

BinaryTree::Node* AVLTree::rotateLeft(Node* pivot, const std::string& kind)
{
    Node* top = pivot->right;
    Node* parent = pivot->parent;
    pivot->right = top->left;
    if (top->left != NULL)
    {
        top->left->parent = pivot;
    }
    top->left = pivot;
    pivot->parent = top;
    top->parent = parent;
    if (parent == NULL)
    {
        root = top;
    }
    else if (parent->left == pivot)
    {
        parent->left = top;
    }
    else
    {
        parent->right = top;
    }
    updateMetadata(pivot);
    updateMetadata(top);
    rotationEvent(pivot, kind);
    return top;
}

BinaryTree::Node* AVLTree::rotateRight(Node* pivot, const std::string& kind)
{
    Node* top = pivot->left;
    Node* parent = pivot->parent;
    pivot->left = top->right;
    if (top->right != NULL)
    {
        top->right->parent = pivot;
    }
    top->right = pivot;
    pivot->parent = top;
    top->parent = parent;
    if (parent == NULL)
    {
        root = top;
    }
    else if (parent->left == pivot)
    {
        parent->left = top;
    }
    else
    {
        parent->right = top;
    }
    updateMetadata(pivot);
    updateMetadata(top);
    rotationEvent(pivot, kind);
    return top;
}

void AVLTree::repair(Node* node)
{
    while (node != NULL)
    {
        updateMetadata(node);
        int balance = nodeHeight(node->left) - nodeHeight(node->right);
        if (balance > 1 || balance < -1)
        {
            if (events.isEnabled())
            {
                BackendEvent event;
                event.type = "BALANCE_ALERT";
                event.nodeId = node->id;
                event.newValue = balance;
                events.append(event);
            }
        }
        if (balance > 1)
        {
            if (nodeHeight(node->left->left) < nodeHeight(node->left->right))
            {
                rotateLeft(node->left, "LR");
                node = rotateRight(node, "LR");
            }
            else
            {
                node = rotateRight(node, "LL");
            }
        }
        else if (balance < -1)
        {
            if (nodeHeight(node->right->right) < nodeHeight(node->right->left))
            {
                rotateRight(node->right, "RL");
                node = rotateLeft(node, "RL");
            }
            else
            {
                node = rotateLeft(node, "RR");
            }
        }
        node = node->parent;
    }
}

bool AVLTree::kthSmallest(int k, int& value) const
{
    if (k <= 0 || k > size())
    {
        return false;
    }
    Node* node = root;
    while (node != NULL)
    {
        int leftSize = nodeSize(node->left);
        if (k == leftSize + 1)
        {
            value = node->value;
            return true;
        }
        if (k <= leftSize)
        {
            node = node->left;
        }
        else
        {
            k -= leftSize + 1;
            node = node->right;
        }
    }
    return false;
}

int AVLTree::countBelow(int key, bool inclusive) const
{
    int count = 0;
    Node* node = root;
    while (node != NULL)
    {
        if (node->value < key || (inclusive && node->value == key))
        {
            count += nodeSize(node->left) + 1;
            node = node->right;
        }
        else
        {
            node = node->left;
        }
    }
    return count;
}

int AVLTree::rank(int value) const
{
    return countBelow(value, true);
}

int AVLTree::countInRange(int low, int high) const
{
    if (low > high)
    {
        return 0;
    }
    return countBelow(high, true) - countBelow(low, false);
}

bool AVLTree::balanced(const Node* node) const
{
    if (node == NULL)
    {
        return true;
    }
    int balance = nodeHeight(node->left) - nodeHeight(node->right);
    return balance >= -1 && balance <= 1 && balanced(node->left) && balanced(node->right);
}

bool AVLTree::validate() const
{
    return BST::validate() && balanced(root);
}
