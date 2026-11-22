#ifndef VAULT_BINARY_TREE_H
#define VAULT_BINARY_TREE_H

#include "backend/OperationEvent.h"
#include <iosfwd>

class BinaryTree
{
protected:
    struct Node
    {
        int id;
        int value;
        int height;
        int subtreeSize;
        Node* left;
        Node* right;
        Node* parent;
        Node(int nodeId, int key, Node* owner);
    };

    Node* root;
    int nextId;
    BackendEventBatch events;
    bool traversalIds = false;
    IntBuffer traverseNodeIds(const std::string& order, bool iterative);
    static void destroy(Node* node);
    static Node* clone(const Node* node, Node* parent);
    static int nodeHeight(const Node* node);
    static int nodeSize(const Node* node);
    static int countNodes(const Node* node);
    static void refresh(Node* node);
    Node* findId(Node* node, int id) const;
    void emit(const std::string& type, const Node* node, bool frame = false,
              const std::string& detail = "");
    void visit(Node* node, IntBuffer& output);
    void recursiveTraversal(Node* node, const std::string& order, IntBuffer& output);
    void fillFrame(const Node* node, BackendFrame& frame, int& index) const;

private:
    Node* reconstruct(const int* order, const int* in, int first, int last,
                      int& position, bool post, Node* parent, bool& valid);
    bool build(const int* order, const int* in, int count, bool post);
    int diameterHeight(const Node* node, int& longest) const;
    bool pathTo(const Node* node, int id, IntBuffer& output) const;
    void mirrorNodes(Node* node);
    bool equalNodes(const Node* first, const Node* second, bool mirrored) const;
    void writeNode(std::ostream& output, const Node* node) const;
    Node* readNode(std::istream& input, Node* parent, bool& valid, int& count, int& maximumId);

public:
    BinaryTree();
    BinaryTree(const BinaryTree& other);
    BinaryTree& operator=(const BinaryTree& other);
    virtual ~BinaryTree();
    void clear();
    bool addRoot(int value);
    bool addChild(int parentId, bool left, int value);
    bool buildFromPreIn(const int* pre, const int* in, int count);
    bool buildFromPostIn(const int* post, const int* in, int count);
    IntBuffer traverse(const std::string& order, bool iterative = false);
    IntBuffer preorder(bool iterative = false);
    IntBuffer inorder(bool iterative = false);
    IntBuffer postorder(bool iterative = false);
    IntBuffer levelOrder(bool iterative = true);
    bool inspect(int nodeId);
    bool swapChildren(int nodeId);
    IntBuffer pathBetween(int firstId, int secondId) const;
    int height() const;
    int size() const;
    int diameter() const;
    bool path(int nodeId, IntBuffer& output) const;
    bool lowestCommonAncestor(int firstId, int secondId, int& ancestorId) const;
    void mirror();
    bool sameTree(const BinaryTree& other) const;
    bool isSymmetric() const;
    bool validate() const;
    BackendFrame snapshot() const;
    void setEventRecording(bool enabled);
    const BackendEventBatch& getEvents() const;
    std::string serialize() const;
    bool deserialize(const std::string& text);
};

#endif
