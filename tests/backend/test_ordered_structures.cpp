#include "backend/BST.h"
#include "backend/AVLTree.h"
#include "backend/MaxHeap.h"
#include <cassert>
#include <climits>
#include <iostream>

class OrderedTests
{
public:
    static void bst()
    {
        BST tree;
        int result = 0;
        assert(!tree.minimum(result) && !tree.maximum(result));
        assert(!tree.deleteKey(7) && !tree.kthSmallest(1, result));
        int keys[] = {50, 30, 70, 20, 40, 60, 80, 65};
        for (int i = 0; i < 8; i++)
        {
            assert(tree.insert(keys[i]) && tree.validate());
        }
        assert(!tree.insert(50) && tree.size() == 8);
        assert(tree.search(30));
        const BackendEventBatch& log = tree.getEvents();
        assert(log.getCount() == 5);
        assert(log.get(0).type == "VISIT" && log.get(1).type == "COMPARE");
        assert(log.get(1).detail == "LESS" && log.get(3).detail == "EQUAL");
        assert(!tree.search(999));
        assert(tree.minimum(result) && result == 20);
        assert(tree.maximum(result) && result == 80);
        assert(tree.successor(50, result) && result == 60);
        assert(tree.predecessor(50, result) && result == 40);
        assert(!tree.successor(80, result) && !tree.predecessor(20, result));
        assert(!tree.successor(999, result));
        assert(tree.lca(20, 40, result) && result == 30);
        assert(!tree.lca(20, 999, result));
        IntBuffer range = tree.range(35, 65);
        int expected[] = {40, 50, 60, 65};
        assert(range.getCount() == 4);
        for (int i = 0; i < 4; i++)
        {
            assert(range.get(i) == expected[i]);
        }
        assert(tree.range(99, 1).getCount() == 0);
        BackendFrame before = tree.snapshot();
        int rootId = before.rootId;
        assert(tree.deleteKey(50) && tree.validate());
        BackendFrame after = tree.snapshot();
        assert(after.rootId == rootId && after.nodes[0].value == 60);
        assert(tree.deleteKey(20) && tree.validate());
        assert(tree.deleteKey(70) && tree.validate());
        assert(tree.deleteKey(60) && tree.validate());
        assert(tree.insert(INT_MIN) && tree.insert(INT_MAX) && tree.validate());
        assert(tree.minimum(result) && result == INT_MIN);
        assert(tree.maximum(result) && result == INT_MAX);
        BST copy(tree);
        copy = copy;
        copy.clear();
        assert(tree.size() > 0 && tree.validate());

        BST left;
        assert(left.insert(2) && left.insert(1));
        assert(left.deleteKey(2) && left.validate() && left.size() == 1);
        assert(left.deleteKey(1) && left.validate() && left.size() == 0);
        BST right;
        assert(right.insert(1) && right.insert(2));
        assert(right.deleteKey(1) && right.validate());
    }

    static void avl()
    {
        int rotations[4][3] = {{30, 20, 10}, {10, 20, 30}, {30, 10, 20}, {10, 30, 20}};
        std::string kinds[4] = {"LL", "RR", "LR", "RL"};
        for (int i = 0; i < 4; i++)
        {
            AVLTree tree;
            assert(tree.insert(rotations[i][0]));
            int firstId = tree.snapshot().rootId;
            assert(tree.insert(rotations[i][1]));
            assert(tree.insert(rotations[i][2]) && tree.validate());
            BackendFrame frame = tree.snapshot();
            assert(frame.nodes[0].value == 20 && tree.height() == 2);
            bool foundId = false;
            for (int j = 0; j < 3; j++)
            {
                if (frame.nodes[j].id == firstId)
                {
                    foundId = frame.nodes[j].value == rotations[i][0];
                }
            }
            assert(foundId);
            int count = 0;
            const BackendEventBatch& log = tree.getEvents();
            for (int j = 0; j < log.getCount(); j++)
            {
                if (log.get(j).type == "ROTATE")
                {
                    assert(log.get(j).detail == kinds[i] && log.get(j).hasFrame);
                    assert(log.get(j).frame.nodeCount == 3);
                    count++;
                }
            }
            assert(count == (i < 2 ? 1 : 2));
        }

        AVLTree tree;
        tree.setEventRecording(false);
        bool present[201] = {};
        unsigned int seed = 12345;
        for (int operation = 0; operation < 2000; operation++)
        {
            seed = seed * 1664525u + 1013904223u;
            int key = static_cast<int>((seed >> 8) % 201);
            if ((seed & 1u) != 0)
            {
                assert(tree.insert(key - 100) == !present[key]);
                present[key] = true;
            }
            else
            {
                assert(tree.deleteKey(key - 100) == present[key]);
                present[key] = false;
            }
            assert(tree.validate());
            int count = 0;
            for (int i = 0; i <= 200; i++)
            {
                assert(tree.rank(i - 100) == count + (present[i] ? 1 : 0));
                if (present[i])
                {
                    int value = 0;
                    count++;
                    assert(tree.kthSmallest(count, value) && value == i - 100);
                }
            }
            assert(tree.size() == count && tree.countInRange(-100, 100) == count);
        }
        int value = 0;
        assert(!tree.kthSmallest(0, value));
        assert(!tree.kthSmallest(tree.size() + 1, value));
        assert(tree.countInRange(4, 2) == 0);
        assert(tree.countInRange(INT_MIN, INT_MAX) == tree.size());
        AVLTree copy(tree);
        assert(copy.validate());
        for (int i = -100; i <= 100; i++)
        {
            copy.deleteKey(i);
            assert(copy.validate());
        }
        assert(copy.size() == 0 && tree.size() > 0);
    }

    static void heap()
    {
        MaxHeap heap;
        int value = 0;
        assert(heap.validate() && !heap.extractMax(value));
        assert(!heap.heapify(0) && !heap.siftUp(-1));
        int values[] = {3, -4, 7, 7, 0, -8, 15, 2};
        assert(heap.buildMaxHeap(values, 8) && heap.validate());
        assert(heap.insert(100) && heap.validate());
        assert(heap.extractMax(value) && value == 100 && heap.validate());
        MaxHeap copy(heap);
        assert(copy.heapSort() && copy.validate());
        BackendFrame sorted = copy.snapshot();
        assert(sorted.heapSize == 0 && sorted.arraySize == 8);
        int expected[] = {-8, -4, 0, 2, 3, 7, 7, 15};
        for (int i = 0; i < 8; i++)
        {
            assert(sorted.values[i] == expected[i]);
        }
        int swaps = 0;
        const BackendEventBatch& log = copy.getEvents();
        for (int i = 0; i < log.getCount(); i++)
        {
            if (log.get(i).type == "HEAP_SWAP")
            {
                assert(log.get(i).hasFrame && log.get(i).frame.arraySize == 8);
                swaps++;
            }
        }
        assert(swaps > 0);
        assert(!copy.insert(12));
        assert(copy.heapSort() && copy.validate());
        assert(heap.size() == 8 && heap.validate());
        int previous = INT_MAX;
        while (heap.extractMax(value))
        {
            assert(value <= previous && heap.validate());
            previous = value;
        }
        assert(heap.size() == 0);
        assert(heap.buildMaxHeap(NULL, 0) && heap.heapSort() && heap.validate());
        assert(heap.insert(-1) && heap.extractMax(value) && value == -1);
        assert(!heap.buildMaxHeap(NULL, 5) && !heap.buildMaxHeap(values, -1));
        heap.setEventRecording(false);
        assert(heap.buildMaxHeap(values, 8) && heap.heapSort() && heap.getEvents().getCount() == 0);
        for (int count = 1; count <= 150; count++)
        {
            int* input = new int[count];
            for (int i = 0; i < count; i++)
            {
                input[i] = (i * 71 + count * 13) % 31 - 15;
            }
            assert(heap.buildMaxHeap(input, count) && heap.validate());
            assert(heap.heapSort() && heap.validate());
            delete[] input;
        }
    }
};

int main()
{
    OrderedTests::bst();
    OrderedTests::avl();
    OrderedTests::heap();
    std::cout << "BST, AVL and MaxHeap: operations, rotations, invariants, events, copies and stress tests passed.\n";
    return 0;
}
