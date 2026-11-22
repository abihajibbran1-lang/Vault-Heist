#include "backend/BinaryTree.h"
#include <stdexcept>
#include <sstream>
#include <climits>

BinaryTree::Node::Node(int nodeId, int key, Node* owner)
    : id(nodeId), value(key), height(1), subtreeSize(1), left(NULL), right(NULL), parent(owner)
{
}

BinaryTree::BinaryTree() : root(NULL), nextId(1)
{
}

BinaryTree::BinaryTree(const BinaryTree& other)
    : root(NULL), nextId(other.nextId), events(other.events)
{
    root = clone(other.root, NULL);
}

BinaryTree& BinaryTree::operator=(const BinaryTree& other)
{
    if (this != &other)
    {
        Node* replacement = clone(other.root, NULL);
        try
        {
            events = other.events;
        }
        catch (...)
        {
            destroy(replacement);
            throw;
        }
        destroy(root);
        root = replacement;
        nextId = other.nextId;
    }
    return *this;
}

BinaryTree::~BinaryTree()
{
    destroy(root);
}

void BinaryTree::destroy(Node* node)
{
    if (node != NULL)
    {
        destroy(node->left);
        destroy(node->right);
        delete node;
    }
}

BinaryTree::Node* BinaryTree::clone(const Node* node, Node* parent)
{
    if (node == NULL)
    {
        return NULL;
    }
    Node* copy = new Node(node->id, node->value, parent);
    try
    {
        copy->left = clone(node->left, copy);
        copy->right = clone(node->right, copy);
    }
    catch (...)
    {
        destroy(copy);
        throw;
    }
    copy->height = node->height;
    copy->subtreeSize = node->subtreeSize;
    return copy;
}

int BinaryTree::nodeHeight(const Node* node)
{
    return node == NULL ? 0 : node->height;
}

int BinaryTree::nodeSize(const Node* node)
{
    return node == NULL ? 0 : node->subtreeSize;
}

void BinaryTree::refresh(Node* node)
{
    if (node != NULL)
    {
        int left = nodeHeight(node->left);
        int right = nodeHeight(node->right);
        node->height = 1 + (left > right ? left : right);
        node->subtreeSize = 1 + nodeSize(node->left) + nodeSize(node->right);
    }
}

void BinaryTree::clear()
{
    destroy(root);
    root = NULL;
    // IDs are not reused in the lifetime of an active model.
    events.clear();
}

BinaryTree::Node* BinaryTree::findId(Node* node, int id) const
{
    if (node == NULL || node->id == id)
    {
        return node;
    }
    Node* left = findId(node->left, id);
    if (left != NULL)
    {
        return left;
    }
    return findId(node->right, id);
}

void BinaryTree::emit(const std::string& type, const Node* node, bool frame,
                      const std::string& detail)
{
    if (!events.isEnabled())
    {
        return;
    }
    BackendEvent event;
    event.type = type;
    event.detail = detail;
    if (node != NULL)
    {
        event.nodeId = node->id;
        event.newValue = node->value;
        event.parentId = node->parent == NULL ? -1 : node->parent->id;
    }
    event.hasFrame = frame;
    if (frame)
    {
        event.frame = snapshot();
    }
    events.append(event);
}

bool BinaryTree::addRoot(int value)
{
    events.clear();
    if (root != NULL)
    {
        return false;
    }
    root = new Node(nextId++, value, NULL);
    emit("INSERT", root, true, "ROOT");
    emit("DONE", NULL);
    return true;
}

bool BinaryTree::addChild(int parentId, bool left, int value)
{
    events.clear();
    Node* parent = findId(root, parentId);
    if (parent == NULL || (left && parent->left != NULL) || (!left && parent->right != NULL))
    {
        return false;
    }
    Node* node = new Node(nextId++, value, parent);
    if (left)
    {
        parent->left = node;
    }
    else
    {
        parent->right = node;
    }
    for (Node* current = parent; current != NULL; current = current->parent)
    {
        refresh(current);
    }
    emit("INSERT", node, true, left ? "LEFT" : "RIGHT");
    emit("DONE", NULL);
    return true;
}

BinaryTree::Node* BinaryTree::reconstruct(const int* order, const int* in, int first,
                                         int last, int& position, bool post,
                                         Node* parent, bool& valid)
{
    if (first > last || !valid)
    {
        return NULL;
    }
    int key = order[position];
    position += post ? -1 : 1;
    int split = first;
    while (split <= last && in[split] != key)
    {
        split++;
    }
    if (split > last)
    {
        valid = false;
        return NULL;
    }
    Node* node = new Node(nextId++, key, parent);
    try
    {
        if (post)
        {
            node->right = reconstruct(order, in, split + 1, last, position, post, node, valid);
            node->left = reconstruct(order, in, first, split - 1, position, post, node, valid);
        }
        else
        {
            node->left = reconstruct(order, in, first, split - 1, position, post, node, valid);
            node->right = reconstruct(order, in, split + 1, last, position, post, node, valid);
        }
    }
    catch (...)
    {
        destroy(node);
        throw;
    }
    refresh(node);
    return node;
}

bool BinaryTree::build(const int* order, const int* in, int count, bool post)
{
    events.clear();
    if (count < 0 || (count > 0 && (order == NULL || in == NULL)))
    {
        return false;
    }
    // Unique reconstruction needs distinct tokens in both traversal sequences.
    for (int i = 0; i < count; i++)
    {
        bool found = false;
        for (int j = 0; j < count; j++)
        {
            if (order[i] == in[j])
            {
                found = true;
            }
            if (j < i && (order[i] == order[j] || in[i] == in[j]))
            {
                return false;
            }
        }
        if (!found)
        {
            return false;
        }
    }
    int position = post ? count - 1 : 0;
    bool valid = true;
    Node* replacement = reconstruct(order, in, 0, count - 1, position, post, NULL, valid);
    if (!valid)
    {
        destroy(replacement);
        return false;
    }
    destroy(root);
    root = replacement;
    emit("DONE", NULL, true, "Reconstruction complete");
    return true;
}

bool BinaryTree::buildFromPreIn(const int* pre, const int* in, int count)
{
    return build(pre, in, count, false);
}

bool BinaryTree::buildFromPostIn(const int* post, const int* in, int count)
{
    return build(post, in, count, true);
}

void BinaryTree::visit(Node* node, IntBuffer& output)
{
    output.append(traversalIds ? node->id : node->value);
    emit("VISIT", node);
}

void BinaryTree::recursiveTraversal(Node* node, const std::string& order, IntBuffer& output)
{
    if (node == NULL)
    {
        return;
    }
    if (order == "PREORDER")
    {
        visit(node, output);
    }
    recursiveTraversal(node->left, order, output);
    if (order == "INORDER")
    {
        visit(node, output);
    }
    recursiveTraversal(node->right, order, output);
    if (order == "POSTORDER")
    {
        visit(node, output);
    }
}

IntBuffer BinaryTree::traverse(const std::string& order, bool iterative)
{
    events.clear();
    if (order != "PREORDER" && order != "INORDER" && order != "POSTORDER" && order != "LEVELORDER")
    {
        throw std::invalid_argument("Unknown traversal order");
    }
    IntBuffer output;
    if (!iterative && order != "LEVELORDER")
    {
        recursiveTraversal(root, order, output);
    }
    else if (root != NULL)
    {
        Node** pending = new Node*[size()];
        try
        {
            int count = 0;
            if (order == "LEVELORDER" || order == "PREORDER")
            {
                int front = 0;
                pending[count++] = root;
                while ((order == "LEVELORDER" && front < count) ||
                       (order == "PREORDER" && count > 0))
                {
                    Node* node = order == "LEVELORDER" ? pending[front++] : pending[--count];
                    visit(node, output);
                    if (order == "LEVELORDER")
                    {
                        if (node->left != NULL)
                        {
                            pending[count++] = node->left;
                        }
                        if (node->right != NULL)
                        {
                            pending[count++] = node->right;
                        }
                    }
                    else
                    {
                        if (node->right != NULL)
                        {
                            pending[count++] = node->right;
                        }
                        if (node->left != NULL)
                        {
                            pending[count++] = node->left;
                        }
                    }
                }
            }
            else
            {
                Node* current = root;
                Node* previous = NULL;
                while (current != NULL || count > 0)
                {
                    if (current != NULL)
                    {
                        pending[count++] = current;
                        current = current->left;
                    }
                    else
                    {
                        Node* top = pending[count - 1];
                        if (order == "POSTORDER" && top->right != NULL && previous != top->right)
                        {
                            current = top->right;
                        }
                        else
                        {
                            count--;
                            visit(top, output);
                            previous = top;
                            if (order == "INORDER")
                            {
                                current = top->right;
                            }
                        }
                    }
                }
            }
        }
        catch (...)
        {
            delete[] pending;
            throw;
        }
        delete[] pending;
    }
    emit("DONE", NULL);
    return output;
}

int BinaryTree::height() const
{
    return nodeHeight(root);
}

IntBuffer BinaryTree::traverseNodeIds(const std::string& order, bool iterative)
{
    bool previous = traversalIds;
    traversalIds = true;
    try
    {
        IntBuffer result = traverse(order, iterative);
        traversalIds = previous;
        return result;
    }
    catch (...)
    {
        traversalIds = previous;
        throw;
    }
}

IntBuffer BinaryTree::preorder(bool iterative)
{
    return traverseNodeIds("PREORDER", iterative);
}

IntBuffer BinaryTree::inorder(bool iterative)
{
    return traverseNodeIds("INORDER", iterative);
}

IntBuffer BinaryTree::postorder(bool iterative)
{
    return traverseNodeIds("POSTORDER", iterative);
}

IntBuffer BinaryTree::levelOrder(bool iterative)
{
    return traverseNodeIds("LEVELORDER", iterative);
}

bool BinaryTree::inspect(int nodeId)
{
    events.clear();
    Node* node = findId(root, nodeId);
    if (node == NULL)
    {
        return false;
    }
    emit("VISIT", node);
    emit("DONE", NULL);
    return true;
}

bool BinaryTree::swapChildren(int nodeId)
{
    events.clear();
    Node* node = findId(root, nodeId);
    if (node == NULL)
    {
        return false;
    }
    Node* temporary = node->left;
    node->left = node->right;
    node->right = temporary;
    emit("HIGHLIGHT_SUBTREE", node, true, "VISITED");
    emit("DONE", NULL);
    return true;
}

int BinaryTree::size() const
{
    return nodeSize(root);
}

int BinaryTree::diameterHeight(const Node* node, int& longest) const
{
    if (node == NULL)
    {
        return 0;
    }
    int left = diameterHeight(node->left, longest);
    int right = diameterHeight(node->right, longest);
    if (left + right > longest)
    {
        longest = left + right;
    }
    return 1 + (left > right ? left : right);
}

int BinaryTree::diameter() const
{
    int longest = 0;
    diameterHeight(root, longest);
    return longest;
}

bool BinaryTree::pathTo(const Node* node, int id, IntBuffer& output) const
{
    if (node == NULL)
    {
        return false;
    }
    if (node->id == id || pathTo(node->left, id, output) || pathTo(node->right, id, output))
    {
        output.append(node->id);
        return true;
    }
    return false;
}

bool BinaryTree::path(int nodeId, IntBuffer& output) const
{
    output.clear();
    IntBuffer reverse;
    if (!pathTo(root, nodeId, reverse))
    {
        return false;
    }
    for (int i = reverse.getCount() - 1; i >= 0; i--)
    {
        output.append(reverse.get(i));
    }
    return true;
}

bool BinaryTree::lowestCommonAncestor(int firstId, int secondId, int& ancestorId) const
{
    ancestorId = -1;
    IntBuffer first;
    IntBuffer second;
    if (!path(firstId, first) || !path(secondId, second))
    {
        return false;
    }
    for (int i = 0; i < first.getCount() && i < second.getCount(); i++)
    {
        if (first.get(i) != second.get(i))
        {
            break;
        }
        ancestorId = first.get(i);
    }
    return true;
}

void BinaryTree::mirrorNodes(Node* node)
{
    if (node != NULL)
    {
        emit("VISIT", node);
        Node* temporary = node->left;
        node->left = node->right;
        node->right = temporary;
        emit("HIGHLIGHT_SUBTREE", node, true, "VISITED");
        mirrorNodes(node->left);
        mirrorNodes(node->right);
    }
}

IntBuffer BinaryTree::pathBetween(int firstId, int secondId) const
{
    IntBuffer first;
    IntBuffer second;
    IntBuffer result;
    if (!path(firstId, first) || !path(secondId, second))
    {
        return result;
    }
    int shared = 0;
    while (shared < first.getCount() && shared < second.getCount() && first.get(shared) == second.get(shared))
    {
        shared++;
    }
    for (int i = first.getCount() - 1; i >= shared - 1; i--)
    {
        result.append(first.get(i));
    }
    for (int i = shared; i < second.getCount(); i++)
    {
        result.append(second.get(i));
    }
    return result;
}

void BinaryTree::mirror()
{
    events.clear();
    mirrorNodes(root);
    emit("DONE", NULL, true);
}

bool BinaryTree::equalNodes(const Node* first, const Node* second, bool mirrored) const
{
    if (first == NULL || second == NULL)
    {
        return first == second;
    }
    return first->value == second->value &&
           equalNodes(first->left, mirrored ? second->right : second->left, mirrored) &&
           equalNodes(first->right, mirrored ? second->left : second->right, mirrored);
}

bool BinaryTree::sameTree(const BinaryTree& other) const
{
    return equalNodes(root, other.root, false);
}

bool BinaryTree::isSymmetric() const
{
    return root == NULL || equalNodes(root->left, root->right, true);
}

void BinaryTree::fillFrame(const Node* node, BackendFrame& frame, int& index) const
{
    if (node != NULL)
    {
        BackendNodeView& view = frame.nodes[index++];
        view.id = node->id;
        view.value = node->value;
        view.parentId = node->parent == NULL ? -1 : node->parent->id;
        view.leftId = node->left == NULL ? -1 : node->left->id;
        view.rightId = node->right == NULL ? -1 : node->right->id;
        view.height = node->height;
        view.subtreeSize = node->subtreeSize;
        fillFrame(node->left, frame, index);
        fillFrame(node->right, frame, index);
    }
}

int BinaryTree::countNodes(const Node* node)
{
    if (node == NULL)
    {
        return 0;
    }
    return 1 + countNodes(node->left) + countNodes(node->right);
}

BackendFrame BinaryTree::snapshot() const
{
    BackendFrame frame;
    // A mutation frame may precede the metadata-update events.
    frame.nodeCount = countNodes(root);
    frame.rootId = root == NULL ? -1 : root->id;
    frame.nodes = new BackendNodeView[frame.nodeCount];
    int index = 0;
    fillFrame(root, frame, index);
    return frame;
}

bool BinaryTree::validate() const
{
    // Check links before recursion, so cycles/shared nodes cannot recurse forever.
    if (root == NULL)
    {
        return true;
    }
    Node** pending = new Node*[size() + 1];
    int count = 1;
    pending[0] = root;
    bool valid = root->parent == NULL;
    for (int i = 0; valid && i < count; i++)
    {
        Node* node = pending[i];
        Node* children[2] = {node->left, node->right};
        for (int j = 0; valid && j < 2; j++)
        {
            Node* child = children[j];
            if (child != NULL)
            {
                if (child->parent != node || count >= size())
                {
                    valid = false;
                    break;
                }
                for (int k = 0; k < count; k++)
                {
                    if (pending[k] == child || pending[k]->id == child->id)
                    {
                        valid = false;
                    }
                }
                if (valid)
                {
                    pending[count++] = child;
                }
            }
        }
    }
    for (int i = count - 1; valid && i >= 0; i--)
    {
        Node* node = pending[i];
        int left = nodeHeight(node->left);
        int right = nodeHeight(node->right);
        valid = node->id > 0 && node->id < nextId &&
                node->height == 1 + (left > right ? left : right) &&
                node->subtreeSize == 1 + nodeSize(node->left) + nodeSize(node->right);
    }
    valid = valid && count == size();
    delete[] pending;
    return valid;
}

void BinaryTree::setEventRecording(bool enabled)
{
    events.setEnabled(enabled);
}

const BackendEventBatch& BinaryTree::getEvents() const
{
    return events;
}

void BinaryTree::writeNode(std::ostream& output, const Node* node) const
{
    if (node == NULL)
    {
        output << "# ";
        return;
    }
    output << node->id << ' ' << node->value << ' ';
    writeNode(output, node->left);
    writeNode(output, node->right);
}

std::string BinaryTree::serialize() const
{
    std::ostringstream output;
    output << nextId << ' ';
    writeNode(output, root);
    return output.str();
}

BinaryTree::Node* BinaryTree::readNode(std::istream& input, Node* parent, bool& valid,
                                      int& count, int& maximumId)
{
    std::string token;
    if (!(input >> token))
    {
        valid = false;
        return NULL;
    }
    if (token == "#")
    {
        return NULL;
    }
    std::istringstream identifier(token);
    int id = 0;
    int value = 0;
    char extra = 0;
    if (!(identifier >> id) || (identifier >> extra) || id <= 0 || id == INT_MAX ||
        !(input >> value) || ++count > 10000)
    {
        valid = false;
        return NULL;
    }
    if (id > maximumId)
    {
        maximumId = id;
    }
    Node* node = new Node(id, value, parent);
    try
    {
        node->left = readNode(input, node, valid, count, maximumId);
        if (valid)
        {
            node->right = readNode(input, node, valid, count, maximumId);
        }
    }
    catch (...)
    {
        destroy(node);
        throw;
    }
    refresh(node);
    return node;
}

bool BinaryTree::deserialize(const std::string& text)
{
    std::istringstream input(text);
    int restoredNext = 0;
    if (!(input >> restoredNext) || restoredNext < 1)
    {
        return false;
    }
    bool valid = true;
    int count = 0;
    int maximumId = 0;
    BinaryTree candidate;
    candidate.root = candidate.readNode(input, NULL, valid, count, maximumId);
    candidate.nextId = restoredNext;
    std::string extra;
    if (!valid || maximumId >= restoredNext || (input >> extra) || !candidate.validate())
    {
        return false;
    }
    *this = candidate;
    return true;
}
