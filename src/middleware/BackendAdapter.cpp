#include "middleware/BackendAdapter.h"
#include <sstream>

bool BackendAdapter::validateStructure() const
{
    if (structure == "MaxHeap")
    {
        BackendFrame frame = heap.snapshot();
        return frame.heapSize >= 0 && frame.heapSize <= frame.arraySize;
    }
    return structure == "AVLTree" ? static_cast<const BST&>(avl).validate() : validate();
}

std::string BackendAdapter::rotationCase(int pivot) const
{
    return avl.rotationCase(pivot);
}

bool BackendAdapter::equals(const BackendAdapter& other) const
{
    return tree.sameTree(other.tree);
}

IntBuffer BackendAdapter::query(const std::string& name, int first, int second) const
{
    IntBuffer result;
    if (name == "KTH_PATH")
    {
        return avl.kthPath(first);
    }
    if (name == "RANK_VALUE")
    {
        result.append(avl.rank(first));
        return result;
    }
    if (name == "UNBALANCED")
    {
        BackendFrame frame = avl.snapshot();
        for (int i = 0; i < frame.nodeCount; i++)
        {
            if (avl.rotationCase(frame.nodes[i].id) != "NONE")
            {
                result.append(frame.nodes[i].id);
            }
        }
        return result;
    }
    BinaryTree copy = structure == "BinaryTree" ? tree : (structure == "BST" ? static_cast<const BinaryTree&>(bst) : static_cast<const BinaryTree&>(avl));
    if (name == "DIAMETER")
    {
        result.append(copy.diameter());
    }
    else if (name == "LCA")
    {
        int id = -1;
        if (copy.lowestCommonAncestor(first, second, id))
        {
            result.append(id);
        }
    }
    else if (name == "PATH_BETWEEN")
    {
        result = copy.pathBetween(first, second);
    }
    else if (name == "SEARCH_PATH" || name == "RANGE_IDS")
    {
        BST ordered = structure == "BST" ? bst : static_cast<const BST&>(avl);
        IntBuffer matches;
        if (name == "SEARCH_PATH")
        {
            ordered.search(first);
            const BackendEventBatch& events = ordered.getEvents();
            for (int i = 0; i < events.getCount(); i++)
            {
                if (events.get(i).type == "VISIT")
                {
                    result.append(events.get(i).nodeId);
                }
            }
        }
        else
        {
            matches = ordered.range(first, second);
            BackendFrame frame = ordered.snapshot();
            for (int i = 0; i < matches.getCount(); i++)
            {
                for (int j = 0; j < frame.nodeCount; j++)
                {
                    if (matches.get(i) == frame.nodes[j].value)
                    {
                        result.append(frame.nodes[j].id);
                    }
                }
            }
        }
    }
    else
    {
        copy.traverse(name);
        const BackendEventBatch& events = copy.getEvents();
        for (int i = 0; i < events.getCount(); i++)
        {
            if (events.get(i).type == "VISIT")
            {
                result.append(events.get(i).nodeId);
            }
        }
    }
    return result;
}

bool BackendAdapter::load(const std::string& name, const IntBuffer& values, const IntBuffer& inorder, bool raw)
{
    if (!select(name))
    {
        return false;
    }
    int count = values.getCount();
    int* input = new int[count];
    int* order = new int[inorder.getCount()];
    for (int i = 0; i < count; i++)
    {
        input[i] = values.get(i);
    }
    for (int i = 0; i < inorder.getCount(); i++)
    {
        order[i] = inorder.get(i);
    }
    bool success = true;
    if (name == "MaxHeap")
    {
        success = raw ? heap.loadArray(input, count) : heap.buildMaxHeap(input, count);
    }
    else if (name == "BinaryTree" || raw)
    {
        BinaryTree* target = name == "BinaryTree" ? &tree : (name == "BST" ? &bst : &avl);
        success = count == inorder.getCount() && target->buildFromPreIn(input, order, count);
    }
    else
    {
        BST* target = name == "BST" ? &bst : &avl;
        for (int i = 0; i < count && success; i++)
        {
            success = target->insert(input[i]);
        }
    }
    delete[] input;
    delete[] order;
    events.clear();
    return success;
}

BackendAdapter::BackendAdapter() : structure("AVLTree")
{
}

bool BackendAdapter::select(const std::string& name)
{
    if (name != "BinaryTree" && name != "BST" && name != "AVLTree" && name != "MaxHeap")
    {
        return false;
    }
    tree = BinaryTree();
    bst = BST();
    avl = AVLTree();
    heap = MaxHeap();
    structure = name;
    events.clear();
    return true;
}

GameSnapshot BackendAdapter::snapshot() const
{
    GameSnapshot output;
    output.structure = structure;
    if (structure == "BinaryTree")
    {
        output.model = tree.snapshot();
    }
    else if (structure == "BST")
    {
        output.model = bst.snapshot();
    }
    else if (structure == "AVLTree")
    {
        output.model = avl.snapshot();
    }
    else
    {
        output.model = heap.snapshot();
    }
    return output;
}

bool BackendAdapter::validate() const
{
    if (structure == "BinaryTree")
    {
        return tree.validate();
    }
    if (structure == "BST")
    {
        return bst.validate();
    }
    if (structure == "AVLTree")
    {
        return avl.validate();
    }
    return heap.validate();
}

bool BackendAdapter::execute(const PlayerAction& action, std::string& message)
{
    events.clear();
    bool success = false;
    IntBuffer values;
    int result = 0;
    if (structure == "MaxHeap" && (action.name == "EXTRACT_SORTED" || action.name == "BUILD_ACTIVE_HEAP" || action.name == "REPAIR_SWAP"))
    {
        success = action.name == "EXTRACT_SORTED" ? heap.extractToSorted() :
                  (action.name == "BUILD_ACTIVE_HEAP" ? heap.rebuildActive() : heap.repairSwap(action.indexA, action.indexB));
        events.map(heap.getEvents());
        message = success ? "Heap step complete" : "Invalid heap step: check the active heap and selected indices";
        return success;
    }
    if (action.name == "ROTATE_PRIMITIVE" && structure == "AVLTree")
    {
        if (action.text.size() < 4 || (action.text.substr(0, 3) != "LR_" && action.text.substr(0, 3) != "RL_"))
        {
            message = "Invalid primitive rotation case";
            return false;
        }
        success = avl.primitiveRotate(action.nodeId, action.text.substr(3), action.text.substr(0, 2));
        events.map(avl.getEvents());
        message = success ? "Primitive rotation complete" : "Invalid pivot or rotation";
        return success;
    }
    if (action.name == "COMPARE_ROOM" || action.name == "INSERT_AT")
    {
        BST* target = structure == "BST" ? &bst : &avl;
        success = action.name == "COMPARE_ROOM" ? target->compareRoom(action.nodeId, action.value) :
                  target->insertAt(action.nodeId, action.value);
        events.map(target->getEvents());
        message = success ? "Path choice accepted" : "Duplicate key or invalid insertion slot";
        return success;
    }
    if (action.name == "SWAP_CHILDREN" && structure == "BinaryTree")
    {
        success = tree.swapChildren(action.nodeId);
        events.map(tree.getEvents());
        message = success ? "Child links swapped" : "Invalid room";
        return success;
    }
    if (action.name == "SELECT_NODE" && structure != "MaxHeap")
    {
        BinaryTree* target = structure == "BinaryTree" ? &tree : (structure == "BST" ? &bst : &avl);
        success = target->inspect(action.nodeId);
        events.map(target->getEvents());
        message = success ? "Room visited" : "Room not found";
        return success;
    }
    if (action.name == "GENERATE")
    {
        IntBuffer initial;
        IntBuffer inorder;
        int keys[7] = {50, 30, 20, 40, 70, 60, 80};
        int sorted[7] = {20, 30, 40, 50, 60, 70, 80};
        for (int i = 0; i < 7; i++)
        {
            initial.append(keys[i]);
            inorder.append(sorted[i]);
        }
        std::string selected = structure;
        success = load(selected, initial, inorder, false);
        message = success ? "Example loaded" : "Example could not be loaded";
        return success;
    }
    if ((action.name == "BUILD_PRE_IN" || action.name == "BUILD_POST_IN" || action.name == "EQUAL") && structure != "MaxHeap")
    {
        BinaryTree* target = structure == "BinaryTree" ? &tree : (structure == "BST" ? &bst : &avl);
        std::string::size_type divider = action.text.find('/');
        if (divider == std::string::npos)
        {
            message = "Enter traversal / inorder, separated by a slash";
            return false;
        }
        IntBuffer first;
        IntBuffer second;
        std::istringstream firstInput(action.text.substr(0, divider));
        std::istringstream secondInput(action.text.substr(divider + 1));
        int key = 0;
        while (firstInput >> key)
        {
            first.append(key);
        }
        while (secondInput >> key)
        {
            second.append(key);
        }
        if (!firstInput.eof() || !secondInput.eof() || first.getCount() != second.getCount() || first.getCount() > 1000)
        {
            message = "Invalid traversal input";
            return false;
        }
        int count = first.getCount();
        int* order = new int[count];
        int* inorder = new int[count];
        for (int i = 0; i < count; i++)
        {
            order[i] = first.get(i);
            inorder[i] = second.get(i);
        }
        if (action.name == "EQUAL")
        {
            BinaryTree candidate;
            success = candidate.buildFromPreIn(order, inorder, count);
            message = success ? (target->sameTree(candidate) ? "Same tree" : "Different tree") : "Invalid traversals";
        }
        else
        {
            success = action.name == "BUILD_PRE_IN" ? target->buildFromPreIn(order, inorder, count) :
                      target->buildFromPostIn(order, inorder, count);
            events.map(target->getEvents());
            message = success ? "Tree reconstructed" : "Invalid traversal pair";
        }
        delete[] order;
        delete[] inorder;
        return success;
    }
    if (action.name == "ROTATE" && structure == "AVLTree")
    {
        success = avl.manualRepair(action.nodeId, action.text);
        events.map(avl.getEvents());
        message = success ? "Tree repaired" : "Incorrect rotation or pivot";
        return success;
    }
    if (structure == "MaxHeap" && (action.name == "HEAP_SWAP" || action.name == "HEAPIFY" ||
        action.name == "SIFT_UP" || action.name == "SORT_STEP" || action.name == "BUILD_HEAP"))
    {
        if (action.name == "HEAP_SWAP")
        {
            success = heap.swapIndices(action.indexA, action.indexB);
        }
        else if (action.name == "HEAPIFY")
        {
            success = heap.heapify(action.indexA);
        }
        else if (action.name == "SIFT_UP")
        {
            success = heap.siftUp(action.indexA);
        }
        else if (action.name == "SORT_STEP")
        {
            success = heap.sortStep();
        }
        else
        {
            BackendFrame frame = heap.snapshot();
            success = heap.buildMaxHeap(frame.values, frame.arraySize);
        }
        events.map(heap.getEvents());
        message = success ? "Heap operation complete" : "Invalid heap action";
        return success;
    }
    if (action.name == "INSERT" || action.name == "DELETE" || action.name == "SEARCH")
    {
        if (structure == "BST" || structure == "AVLTree")
        {
            BST* target = structure == "BST" ? &bst : &avl;
            if (action.name == "INSERT")
            {
                success = target->insert(action.value);
            }
            else if (action.name == "DELETE")
            {
                success = target->deleteKey(action.value);
            }
            else
            {
                success = target->search(action.value);
            }
            events.map(target->getEvents());
        }
        else if (structure == "MaxHeap" && action.name == "INSERT")
        {
            success = heap.insert(action.value);
            events.map(heap.getEvents());
        }
        else if (structure == "BinaryTree" && action.name == "INSERT")
        {
            success = tree.size() == 0 ? tree.addRoot(action.value) :
                      tree.addChild(action.nodeId, action.text != "RIGHT", action.value);
            events.map(tree.getEvents());
        }
        message = success ? "Operation complete" : "Key absent, duplicate, or operation unavailable";
        return success;
    }
    if (action.name == "TRAVERSE")
    {
        BinaryTree* target = &tree;
        if (structure == "BST")
        {
            target = &bst;
        }
        else if (structure == "AVLTree")
        {
            target = &avl;
        }
        if (structure != "MaxHeap")
        {
            values = target->traverse(action.text.empty() ? "INORDER" : action.text);
            events.map(target->getEvents());
            success = true;
        }
    }
    else if (action.name == "MIRROR" && structure == "BinaryTree")
    {
        tree.mirror();
        events.map(tree.getEvents());
        success = true;
    }
    else if (action.name == "HEIGHT" || action.name == "DIAMETER" || action.name == "SYMMETRY")
    {
        BinaryTree* target = structure == "BinaryTree" ? &tree :
                             (structure == "BST" ? &bst : &avl);
        if (structure != "MaxHeap")
        {
            result = action.name == "HEIGHT" ? target->height() :
                     (action.name == "DIAMETER" ? target->diameter() : target->isSymmetric());
            values.append(result);
            success = true;
        }
    }
    else if (action.name == "KTH" && structure != "MaxHeap" && structure != "BinaryTree")
    {
        success = structure == "BST" ? bst.kthSmallest(action.value, result) : avl.kthSmallest(action.value, result);
        if (success)
        {
            values.append(result);
        }
    }
    else if (action.name == "RANK" && structure == "AVLTree")
    {
        values.append(avl.rankWithEvents(action.value));
        events.map(avl.getEvents());
        success = true;
    }
    else if (action.name == "COUNT_RANGE" && structure == "AVLTree")
    {
        values.append(avl.countInRange(action.value, action.otherValue));
        success = true;
    }
    else if ((action.name == "MIN" || action.name == "MAX" || action.name == "SUCCESSOR" ||
              action.name == "PREDECESSOR") && (structure == "BST" || structure == "AVLTree"))
    {
        const BST* target = structure == "BST" ? &bst : &avl;
        if (action.name == "MIN")
        {
            success = target->minimum(result);
        }
        else if (action.name == "MAX")
        {
            success = target->maximum(result);
        }
        else if (action.name == "SUCCESSOR")
        {
            success = target->successor(action.value, result);
        }
        else
        {
            success = target->predecessor(action.value, result);
        }
        if (success)
        {
            values.append(result);
        }
    }
    else if ((action.name == "PATH" || action.name == "LCA") && structure != "MaxHeap")
    {
        BinaryTree* target = structure == "BinaryTree" ? &tree :
                             (structure == "BST" ? &bst : &avl);
        if (action.name == "PATH")
        {
            success = target->path(action.value, values);
        }
        else if (structure == "BinaryTree")
        {
            success = target->lowestCommonAncestor(action.value, action.otherValue, result);
            if (success)
            {
                values.append(result);
            }
        }
        else
        {
            BST* ordered = structure == "BST" ? &bst : &avl;
            success = ordered->lca(action.value, action.otherValue, result);
            events.map(ordered->getEvents());
            if (success)
            {
                values.append(result);
            }
        }
    }
    else if (action.name == "RANGE" && (structure == "BST" || structure == "AVLTree"))
    {
        BST* target = structure == "BST" ? &bst : &avl;
        values = target->range(action.value, action.otherValue);
        events.map(target->getEvents());
        success = true;
    }
    else if (action.name == "EXTRACT_MAX" && structure == "MaxHeap")
    {
        success = heap.extractMax(result);
        if (success)
        {
            values.append(result);
        }
        events.map(heap.getEvents());
    }
    else if (action.name == "HEAP_SORT" && structure == "MaxHeap")
    {
        success = heap.heapSort();
        events.map(heap.getEvents());
    }
    else if (action.name == "VALIDATE")
    {
        success = validate();
    }
    std::ostringstream output;
    output << (success ? "Result:" : "Operation unavailable or invalid input");
    for (int i = 0; i < values.getCount(); i++)
    {
        output << ' ' << values.get(i);
    }
    message = output.str();
    return success;
}

const EventBatch& BackendAdapter::getEvents() const
{
    return events;
}

bool BackendAdapter::contains(int value) const
{
    GameSnapshot current = snapshot();
    for (int i = 0; i < current.model.nodeCount; i++)
    {
        if (current.model.nodes[i].value == value)
        {
            return true;
        }
    }
    return false;
}

int BackendAdapter::size() const
{
    return snapshot().model.nodeCount;
}

std::string BackendAdapter::predict(const std::string& operation, int value) const
{
    AVLTree preview(avl);
    preview.setEventRecording(true);
    if (operation == "INSERT")
    {
        preview.insert(value);
    }
    else
    {
        preview.deleteKey(value);
    }
    const BackendEventBatch& log = preview.getEvents();
    for (int i = 0; i < log.getCount(); i++)
    {
        if (log.get(i).type == "ROTATE")
        {
            return log.get(i).detail;
        }
    }
    return "NONE";
}

std::string BackendAdapter::serializeTree() const
{
    if (structure == "MaxHeap")
    {
        return heap.serialize();
    }
    if (structure == "BST")
    {
        return bst.serialize();
    }
    if (structure == "AVLTree")
    {
        return avl.serialize();
    }
    return tree.serialize();
}

bool BackendAdapter::restoreAVL(const std::string& text)
{
    AVLTree restored;
    if (!restored.deserialize(text) || !restored.validate())
    {
        return false;
    }
    select("AVLTree");
    avl = restored;
    return true;
}
