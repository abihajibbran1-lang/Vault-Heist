#include "backend/BST.h"
#include "backend/AVLTree.h"
#include "backend/MaxHeap.h"
#include <fstream>
#include <iostream>
#include <ctime>
#include <iomanip>
#include <cassert>
#include <cstdint>

class Benchmarks
{
private:
    std::uint32_t seed;
    unsigned int random()
    {
        seed = seed * 1664525u + 1013904223u;
        return seed;
    }

    static double seconds(std::clock_t start)
    {
        return static_cast<double>(std::clock() - start) / CLOCKS_PER_SEC;
    }

public:
    Benchmarks() : seed(3017)
    {
    }

    void trees(std::ostream& output)
    {
        output << "n,input,bst_height,avl_height,bst_search_comparisons,avl_search_comparisons,bst_search_seconds,avl_search_seconds,bst_kth_seconds,avl_kth_seconds,avl_insert_rotations,avl_delete_rotations,max_rotations_per_delete,checksum,bst_comparisons_per_search,avl_comparisons_per_search,bst_seconds_per_search,avl_seconds_per_search,avl_rotations_per_insert,avl_rotations_per_delete\n";
        int sizes[3] = {100, 1000, 10000};
        for (int sizeIndex = 0; sizeIndex < 3; sizeIndex++)
        {
            int count = sizes[sizeIndex];
            for (int shape = 0; shape < 2; shape++)
            {
                int* values = new int[count];
                for (int i = 0; i < count; i++)
                {
                    values[i] = i;
                }
                if (shape == 1)
                {
                    for (int i = count - 1; i > 0; i--)
                    {
                        int other = static_cast<int>(random() % (i + 1));
                        int value = values[i];
                        values[i] = values[other];
                        values[other] = value;
                    }
                }
                BST bst;
                AVLTree avl;
                bst.setEventRecording(false);
                avl.setEventRecording(false);
                for (int i = 0; i < count; i++)
                {
                    assert(bst.insert(values[i]) && avl.insert(values[i]));
                }
                assert(bst.validate() && avl.validate());
                assert(bst.getEvents().getCount() == 0 && avl.getEvents().getCount() == 0);
                int bstHeight = bst.height();
                int avlHeight = avl.height();
                long long insertRotations = avl.getRotations();
                int queries[10000];
                for (int i = 0; i < 10000; i++)
                {
                    queries[i] = static_cast<int>(random() % count);
                }
                bst.resetCounters();
                avl.resetCounters();
                std::clock_t start = std::clock();
                for (int i = 0; i < 10000; i++)
                {
                    assert(bst.search(queries[i]));
                }
                double bstSearch = seconds(start);
                start = std::clock();
                for (int i = 0; i < 10000; i++)
                {
                    assert(avl.search(queries[i]));
                }
                double avlSearch = seconds(start);
                long long bstComparisons = bst.getComparisons();
                long long avlComparisons = avl.getComparisons();
                long long checksum = 0;
                start = std::clock();
                for (int i = 0; i < 1000; i++)
                {
                    int value = 0;
                    assert(bst.kthSmallest(queries[i] + 1, value));
                    checksum += value;
                }
                double bstKth = seconds(start);
                long long expected = checksum;
                checksum = 0;
                start = std::clock();
                for (int i = 0; i < 1000; i++)
                {
                    int value = 0;
                    assert(avl.kthSmallest(queries[i] + 1, value));
                    checksum += value;
                }
                double avlKth = seconds(start);
                assert(checksum == expected);
                avl.resetCounters();
                long long maximum = 0;
                for (int i = 0; i < count; i++)
                {
                    long long before = avl.getRotations();
                    assert(avl.deleteKey(values[i]));
                    long long rotations = avl.getRotations() - before;
                    if (rotations > maximum)
                    {
                        maximum = rotations;
                    }
                }
                assert(avl.validate() && avl.size() == 0);
                output << count << ',' << (shape == 0 ? "sorted" : "random") << ',' << bstHeight << ',' << avlHeight
                       << ',' << bstComparisons << ',' << avlComparisons << ',' << bstSearch << ',' << avlSearch
                       << ',' << bstKth << ',' << avlKth << ',' << insertRotations << ',' << avl.getRotations()
                       << ',' << maximum << ',' << checksum
                       << ',' << bstComparisons / 10000.0 << ',' << avlComparisons / 10000.0
                       << ',' << bstSearch / 10000.0 << ',' << avlSearch / 10000.0
                       << ',' << insertRotations / static_cast<double>(count)
                       << ',' << avl.getRotations() / static_cast<double>(count) << '\n';
                delete[] values;
            }
        }
    }

    void heaps(std::ostream& output)
    {
        output << "n,input,build_seconds,build_comparisons,build_swaps,heap_sort_seconds,heap_sort_comparisons,heap_sort_swaps,insertion_sort_seconds,insertion_comparisons,insertion_moves\n";
        int sizes[3] = {100, 1000, 10000};
        const char* kinds[4] = {"random", "sorted", "reverse", "duplicates"};
        for (int sizeIndex = 0; sizeIndex < 3; sizeIndex++)
        {
            int count = sizes[sizeIndex];
            for (int shape = 0; shape < 4; shape++)
            {
                int* values = new int[count];
                int* reference = new int[count];
                for (int i = 0; i < count; i++)
                {
                    values[i] = shape == 0 ? static_cast<int>(random() % (count * 4)) - count :
                                (shape == 1 ? i : (shape == 2 ? count - i : static_cast<int>(random() % 8)));
                    reference[i] = values[i];
                }
                MaxHeap heap;
                heap.setEventRecording(false);
                std::clock_t start = std::clock();
                assert(heap.buildMaxHeap(values, count));
                double buildTime = seconds(start);
                assert(heap.validate());
                long long buildComparisons = heap.getComparisons();
                long long buildSwaps = heap.getSwaps();
                heap.loadArray(values, count);
                heap.resetCounters();
                start = std::clock();
                assert(heap.heapSort());
                double sortTime = seconds(start);
                assert(heap.validate() && heap.getEvents().getCount() == 0);
                BackendFrame sorted = heap.snapshot();
                long long comparisons = 0;
                long long moves = 0;
                start = std::clock();
                for (int i = 1; i < count; i++)
                {
                    int value = reference[i];
                    int j = i - 1;
                    while (j >= 0)
                    {
                        comparisons++;
                        if (reference[j] <= value)
                        {
                            break;
                        }
                        reference[j + 1] = reference[j];
                        moves++;
                        j--;
                    }
                    reference[j + 1] = value;
                }
                double insertionTime = seconds(start);
                for (int i = 0; i < count; i++)
                {
                    assert(reference[i] == sorted.values[i]);
                }
                output << count << ',' << kinds[shape] << ',' << buildTime << ',' << buildComparisons << ',' << buildSwaps
                       << ',' << sortTime << ',' << heap.getComparisons() << ',' << heap.getSwaps() << ',' << insertionTime
                       << ',' << comparisons << ',' << moves << '\n';
                delete[] values;
                delete[] reference;
            }
        }
    }
};

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        std::cerr << "Usage: benchmark OUTPUT_DIRECTORY\n";
        return 1;
    }
    std::ofstream trees((std::string(argv[1]) + "/trees.csv").c_str());
    std::ofstream heaps((std::string(argv[1]) + "/heaps.csv").c_str());
    trees << std::setprecision(9);
    heaps << std::setprecision(9);
    Benchmarks benchmarks;
    benchmarks.trees(trees);
    benchmarks.heaps(heaps);
    if (!trees || !heaps)
    {
        return 1;
    }
    std::cout << "Benchmarks completed with events disabled; results verified and saved.\n";
}
