#include "backend/BinaryTree.h"
#include <cassert>
#include <iostream>
#include <stdexcept>

class TreeTests
{
public:
    static void equal(const IntBuffer& actual, const int* expected, int count)
    {
        assert(actual.getCount() == count);
        for (int i = 0; i < count; i++)
        {
            assert(actual.get(i) == expected[i]);
        }
    }

    static void run()
    {
        BinaryTree empty;
        assert(empty.height() == 0 && empty.size() == 0 && empty.diameter() == 0);
        assert(empty.isSymmetric() && empty.validate());
        assert(empty.traverse("INORDER", true).getCount() == 0);
        assert(empty.addRoot(-1));
        assert(!empty.addRoot(9));
        assert(empty.height() == 1 && empty.diameter() == 0 && empty.validate());

        int pre[] = {4, 2, 1, 3, 6, 5, 7};
        int in[] = {1, 2, 3, 4, 5, 6, 7};
        int post[] = {1, 3, 2, 5, 7, 6, 4};
        int level[] = {4, 2, 6, 1, 3, 5, 7};
        BinaryTree tree;
        assert(tree.buildFromPreIn(pre, in, 7));
        assert(tree.height() == 3 && tree.diameter() == 4 && tree.validate());
        const std::string orders[4] = {"PREORDER", "INORDER", "POSTORDER", "LEVELORDER"};
        const int* expected[4] = {pre, in, post, level};
        for (int i = 0; i < 4; i++)
        {
            equal(tree.traverse(orders[i]), expected[i], 7);
            equal(tree.traverse(orders[i], true), expected[i], 7);
            const BackendEventBatch& log = tree.getEvents();
            assert(log.getCount() == 8 && log.get(7).type == "DONE");
            for (int j = 0; j < 7; j++)
            {
                assert(log.get(j).type == "VISIT");
            }
        }
        BinaryTree second;
        assert(second.buildFromPostIn(post, in, 7));
        assert(tree.sameTree(second));
        BackendFrame frame = tree.snapshot();
        int leftLeaf = frame.nodes[2].id;
        int rightLeaf = frame.nodes[3].id;
        int lca = -1;
        assert(tree.lowestCommonAncestor(leftLeaf, rightLeaf, lca));
        assert(lca == frame.nodes[1].id);
        IntBuffer path;
        assert(tree.path(leftLeaf, path));
        assert(path.getCount() == 3 && path.get(0) == frame.rootId);
        assert(!tree.path(9999, path) && path.getCount() == 0);
        assert(!tree.lowestCommonAncestor(leftLeaf, 9999, lca));

        BinaryTree copy(tree);
        copy.mirror();
        assert(!copy.sameTree(tree) && copy.validate());
        assert(copy.getEvents().get(1).hasFrame);
        BackendEventBatch saved(copy.getEvents());
        int savedLeft = saved.get(1).frame.nodes[0].leftId;
        copy.mirror();
        assert(copy.sameTree(tree));
        assert(saved.get(1).frame.nodes[0].leftId == savedLeft);
        copy = copy;
        copy = tree;
        copy.clear();
        assert(tree.size() == 7 && copy.size() == 0);

        int badPre[] = {4, 6, 2, 3, 1, 5, 7};
        assert(!tree.buildFromPreIn(badPre, in, 7));
        assert(tree.sameTree(second));
        int duplicate[] = {1, 1};
        assert(!tree.buildFromPreIn(duplicate, duplicate, 2));
        assert(!tree.buildFromPreIn(NULL, in, 2));
        assert(!tree.buildFromPostIn(post, in, -1));

        BinaryTree general;
        assert(general.addRoot(8));
        int rootId = general.snapshot().rootId;
        assert(general.addChild(rootId, true, 3));
        assert(general.addChild(rootId, false, 3));
        assert(general.isSymmetric());
        assert(!general.addChild(rootId, true, 5));
        assert(!general.addChild(9999, false, 1));
        assert(general.validate());
        BinaryTree silent(general);
        silent.setEventRecording(false);
        silent.mirror();
        assert(silent.getEvents().getCount() == 0 && silent.sameTree(general));
        IntBuffer ids = silent.preorder();
        assert(ids.getCount() == silent.size() && ids.get(0) == silent.snapshot().rootId);
        assert(silent.getEvents().getCount() == 0);

        int skew[] = {-5, -2, 0, 8, 10};
        assert(tree.buildFromPreIn(skew, skew, 5));
        assert(tree.height() == 5 && tree.diameter() == 4);
        for (int i = 0; i < 4; i++)
        {
            IntBuffer recursive = tree.traverse(orders[i]);
            IntBuffer iterative = tree.traverse(orders[i], true);
            assert(recursive.getCount() == iterative.getCount());
            for (int j = 0; j < recursive.getCount(); j++)
            {
                assert(recursive.get(j) == iterative.get(j));
            }
        }
        bool threw = false;
        try
        {
            tree.traverse("INVALID");
        }
        catch (const std::invalid_argument&)
        {
            threw = true;
        }
        assert(threw);
        assert(tree.buildFromPreIn(NULL, NULL, 0) && tree.size() == 0);
    }
};

int main()
{
    TreeTests::run();
    std::cout << "BinaryTree: traversal, reconstruction, metrics, paths, mirror, ownership and event tests passed.\n";
    return 0;
}
