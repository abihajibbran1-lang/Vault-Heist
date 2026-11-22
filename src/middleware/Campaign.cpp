#include "middleware/GameController.h"
#include "middleware/JsonDocument.h"
#include "middleware/SaveFile.h"
#include <cmath>
#include <fstream>
#include <cstdio>
#include <sstream>

class CampaignTools
{
public:
    static const BackendNodeView* node(const BackendFrame& frame, int id)
    {
        for (int i = 0; i < frame.nodeCount; i++)
        {
            if (frame.nodes[i].id == id)
            {
                return &frame.nodes[i];
            }
        }
        return NULL;
    }

    static int idFor(const BackendFrame& frame, int value)
    {
        for (int i = 0; i < frame.nodeCount; i++)
        {
            if (frame.nodes[i].value == value)
            {
                return frame.nodes[i].id;
            }
        }
        return -1;
    }

    static bool contains(const IntBuffer& values, int value)
    {
        for (int i = 0; i < values.getCount(); i++)
        {
            if (values.get(i) == value)
            {
                return true;
            }
        }
        return false;
    }

    static int charges(const EventBatch& events)
    {
        int count = 0;
        for (int i = 0; i < events.getCount(); i++)
        {
            if (events.get(i).type == "VISIT" || events.get(i).type == "COMPARE")
            {
                count++;
            }
        }
        return count;
    }

    static bool run(BackendAdapter& model, const PlayerAction& action, OperationResult& result)
    {
        bool success = model.execute(action, result.message);
        result.events.append(model.getEvents());
        return success;
    }

    static std::string orderAt(const std::string& orders, int round)
    {
        std::string::size_type start = 0;
        while (round-- > 0)
        {
            start = orders.find('|', start);
            if (start == std::string::npos)
            {
                return "";
            }
            start++;
        }
        std::string::size_type end = orders.find('|', start);
        return orders.substr(start, end == std::string::npos ? end : end - start);
    }
};

bool GameController::levelAvailable(int act, int level) const
{
    if (act < 1 || act > 4 || level < 1 || level > 5)
    {
        return false;
    }
    int index = (act - 1) * 5 + level - 1;
    return index == 0 || campaignStars[index - 1] > 0;
}

int GameController::levelStars(int act, int level) const
{
    if (act < 1 || act > 4 || level < 1 || level > 5)
    {
        return 0;
    }
    return campaignStars[(act - 1) * 5 + level - 1];
}

void GameController::loadCampaignProgress()
{
    JsonDocument document;
    IntBuffer stars;
    int version = 0;
    if (!document.load(saveDirectory + "/campaign-progress.json"))
    {
        return;
    }
    if (document.integer("version", version) && version == 2 &&
        document.only("|version||stars|") && document.array("stars", stars) && stars.getCount() == 20)
    {
        bool gap = false;
        for (int i = 0; i < 20; i++)
        {
            if (stars.get(i) < 0 || stars.get(i) > 3 || (gap && stars.get(i) > 0))
            {
                return;
            }
            gap = gap || stars.get(i) == 0;
        }
        for (int i = 0; i < 20; i++)
        {
            campaignStars[i] = stars.get(i);
        }
    }
    else if (version == 1)
    {
        int first = 0;
        if (document.integer("level1Stars", first) && first >= 0 && first <= 3)
        {
            campaignStars[0] = first;
        }
    }
}

bool GameController::saveCampaignProgress()
{
    std::string path = saveDirectory + "/campaign-progress.json";
    std::ofstream output((path + ".tmp").c_str());
    output << "{\"version\":2,\"stars\":[";
    for (int i = 0; i < 20; i++)
    {
        output << (i == 0 ? "" : ",") << campaignStars[i];
    }
    output << "]}\n";
    output.close();
    return !output.fail() && SaveFile::commit(path + ".tmp", path);
}

bool GameController::loadLevel(int act, int level)
{
    if (!levelAvailable(act, level))
    {
        state.message = "Complete the previous level first";
        return false;
    }
    JsonDocument document;
    IntBuffer values;
    IntBuffer inorder;
    IntBuffer targets;
    int fileAct = 0;
    int fileLevel = 0;
    int seed = 0;
    int caseSize = 0;
    std::string title;
    std::string kind;
    std::string objective;
    std::string hint;
    std::string orders;
    std::string path = "data/levels/act" + std::to_string(act) + "-level" + std::to_string(level) + ".json";
    if (!document.load(path) ||
        !document.only("|act||level||seed||title||kind||objective||hint||values||inorder||targets||orders||caseSize|") ||
        !document.integer("act", fileAct) || !document.integer("level", fileLevel) ||
        fileAct != act || fileLevel != level || !document.integer("seed", seed) || seed < 0 ||
        !document.string("title", title) || !document.string("kind", kind) ||
        !document.string("objective", objective) || !document.string("hint", hint) ||
        !document.string("orders", orders) || !document.array("values", values) ||
        !document.array("inorder", inorder) || !document.array("targets", targets) ||
        values.getCount() < (act == 3 ? 2 : 6) || values.getCount() > 15)
    {
        state.message = "Missing or invalid level data: " + path;
        return false;
    }
    document.integer("caseSize", caseSize);
    const std::string supported = "|TRAVERSAL|RECONSTRUCT|HEIGHTS|LCA_PATH|MIRROR|SEARCH|INSERT_PATH|DELETE_CASE|RANGE|AVL_BADGES|AVL_SINGLE|AVL_DOUBLE|AVL_PREDICT|AVL_ORDER|HEAP_BUILD|HEAP_REPAIR|HEAP_EXTRACT|HEAP_RACE|HEAP_FINAL|";
    if (supported.find("|" + kind + "|") == std::string::npos ||
        ((kind == "SEARCH" || kind == "INSERT_PATH" || kind == "DELETE_CASE") && targets.getCount() == 0) ||
        ((kind == "LCA_PATH" || kind == "RANGE") && targets.getCount() != 2))
    {
        state.message = "Invalid objective or targets";
        return false;
    }
    BackendAdapter candidate;
    bool raw = kind == "AVL_BADGES" || kind == "AVL_SINGLE" || kind == "AVL_DOUBLE" || act == 4;
    IntBuffer firstValues;
    IntBuffer firstInorder;
    int count = caseSize > 0 ? caseSize : values.getCount();
    if (count > values.getCount() || (caseSize > 0 && values.getCount() % caseSize != 0))
    {
        return false;
    }
    for (int i = 0; i < count; i++)
    {
        firstValues.append(values.get(i));
        if (inorder.getCount() > i)
        {
            firstInorder.append(inorder.get(i));
        }
    }
    if (!candidate.load(act == 1 ? "BinaryTree" : (act == 2 ? "BST" : (act == 3 ? "AVLTree" : "MaxHeap")), firstValues, firstInorder, raw) ||
        !(raw ? candidate.validateStructure() : candidate.validate()))
    {
        state.message = "Invalid campaign tree";
        return false;
    }
    IntBuffer route;
    int optimal = 0;
    if (act == 4)
    {
        if (kind == "HEAP_REPAIR" && caseSize <= 0)
        {
            return false;
        }
        int rounds = kind == "HEAP_REPAIR" ? values.getCount() / caseSize : 1;
        for (int round = 0; round < rounds; round++)
        {
            BackendAdapter solver;
            IntBuffer input;
            IntBuffer empty;
            int size = kind == "HEAP_REPAIR" ? caseSize : values.getCount();
            if (size <= 0 || (kind == "HEAP_REPAIR" && targets.getCount() != rounds))
            {
                return false;
            }
            for (int i = 0; i < size; i++)
            {
                input.append(values.get(round * size + i));
            }
            solver.load("MaxHeap", input, empty, true);
            PlayerAction step;
            std::string ignored;
            if (kind == "HEAP_REPAIR")
            {
                step.name = round == 0 ? "HEAPIFY" : "SIFT_UP";
                step.indexA = targets.get(round);
                if (!solver.execute(step, ignored) || !solver.validate())
                {
                    return false;
                }
                optimal += CampaignTools::charges(solver.getEvents());
            }
            else
            {
                if (!solver.validate())
                {
                    step.name = "BUILD_ACTIVE_HEAP";
                    solver.execute(step, ignored);
                    optimal += CampaignTools::charges(solver.getEvents());
                }
                if (kind != "HEAP_BUILD")
                {
                    while (solver.snapshot().model.heapSize > 0)
                    {
                        step.name = "EXTRACT_SORTED";
                        if (!solver.execute(step, ignored))
                        {
                            return false;
                        }
                        if (!solver.validate())
                        {
                            step.name = "HEAPIFY";
                            step.indexA = 0;
                            solver.execute(step, ignored);
                            optimal += CampaignTools::charges(solver.getEvents());
                        }
                    }
                }
            }
        }
    }
    else if (kind == "AVL_BADGES")
    {
        route = candidate.query("UNBALANCED");
        optimal = route.getCount();
    }
    else if (kind == "AVL_SINGLE" || kind == "AVL_DOUBLE")
    {
        if (caseSize != 3 || inorder.getCount() != values.getCount())
        {
            return false;
        }
        for (int round = 0; round < values.getCount() / caseSize; round++)
        {
            IntBuffer input;
            IntBuffer order;
            for (int i = 0; i < caseSize; i++)
            {
                input.append(values.get(round * caseSize + i));
                order.append(inorder.get(round * caseSize + i));
            }
            BackendAdapter solver;
            if (!solver.load("AVLTree", input, order, true) || !solver.validateStructure())
            {
                return false;
            }
            int pivot = solver.snapshot().model.rootId;
            std::string required = solver.rotationCase(pivot);
            if ((kind == "AVL_SINGLE" && required != "LL" && required != "RR") ||
                (kind == "AVL_DOUBLE" && required != "LR" && required != "RL"))
            {
                return false;
            }
            PlayerAction repair;
            repair.name = "ROTATE";
            repair.nodeId = pivot;
            repair.text = required;
            std::string ignored;
            if (!solver.execute(repair, ignored) || !solver.validate())
            {
                return false;
            }
            optimal++; // Required pivot inspection for the case choice.
            if (kind == "AVL_DOUBLE")
            {
                for (int i = 0; i < solver.getEvents().getCount(); i++)
                {
                    if (solver.getEvents().get(i).type == "ROTATE")
                    {
                        optimal++; // One inspected pivot per real primitive rotation.
                    }
                }
            }
        }
    }
    else if (kind == "AVL_PREDICT")
    {
        if (caseSize != 2 || targets.getCount() != values.getCount() / caseSize)
        {
            return false;
        }
        for (int round = 0; round < targets.getCount(); round++)
        {
            IntBuffer input;
            IntBuffer empty;
            input.append(values.get(round * 2));
            input.append(values.get(round * 2 + 1));
            BackendAdapter solver;
            solver.load("AVLTree", input, empty, false);
            PlayerAction insert;
            insert.name = "INSERT";
            insert.value = targets.get(round);
            std::string ignored;
            if (!solver.execute(insert, ignored) || !solver.validate())
            {
                return false;
            }
            optimal += CampaignTools::charges(solver.getEvents());
        }
    }
    else if (kind == "AVL_ORDER")
    {
        if (targets.getCount() != 2)
        {
            return false;
        }
        route = candidate.query("KTH_PATH", targets.get(0));
        BackendAdapter solver = candidate;
        PlayerAction rank;
        rank.name = "RANK";
        rank.value = targets.get(1);
        std::string ignored;
        solver.execute(rank, ignored);
        optimal = route.getCount() + CampaignTools::charges(solver.getEvents()) + 1;
    }
    else if (kind == "TRAVERSAL")
    {
        for (int round = 0; ; round++)
        {
            std::string order = CampaignTools::orderAt(orders, round);
            if (order.empty())
            {
                break;
            }
            if (order != "PREORDER" && order != "INORDER" && order != "POSTORDER" && order != "LEVELORDER")
            {
                state.message = "Invalid traversal order";
                return false;
            }
            IntBuffer part = candidate.query(order);
            for (int i = 0; i < part.getCount(); i++)
            {
                route.append(part.get(i));
            }
        }
        optimal = route.getCount();
    }
    else if (kind == "HEIGHTS" || kind == "MIRROR")
    {
        route = candidate.query("POSTORDER");
        optimal = route.getCount() + (kind == "HEIGHTS" ? 1 : 0);
    }
    else if (kind == "RECONSTRUCT")
    {
        optimal = 2 * values.getCount();
    }
    else if (kind == "LCA_PATH")
    {
        GameSnapshot original = candidate.snapshot();
        int first = CampaignTools::idFor(original.model, targets.get(0));
        int second = CampaignTools::idFor(original.model, targets.get(1));
        IntBuffer lca = candidate.query("LCA", first, second);
        IntBuffer pathIds = candidate.query("PATH_BETWEEN", first, second);
        if (lca.getCount() != 1 || pathIds.getCount() == 0)
        {
            return false;
        }
        route.append(first);
        route.append(second);
        route.append(lca.get(0));
        for (int i = 0; i < pathIds.getCount(); i++)
        {
            route.append(pathIds.get(i));
        }
        optimal = route.getCount() + 1; // Trace visits plus the path-length answer.
    }
    else if (kind == "SEARCH" || kind == "INSERT_PATH")
    {
        if (candidate.contains(targets.get(0)) != (kind == "SEARCH"))
        {
            state.message = "Search or insertion target is inconsistent";
            return false;
        }
        route = candidate.query("SEARCH_PATH", targets.get(0));
        optimal = 2 * route.getCount();
    }
    else if (kind == "DELETE_CASE")
    {
        BackendAdapter solver = candidate;
        for (int i = 0; i < targets.getCount(); i++)
        {
            PlayerAction action;
            action.name = "DELETE";
            action.value = targets.get(i);
            std::string ignored;
            if (!solver.execute(action, ignored))
            {
                return false;
            }
            optimal += CampaignTools::charges(solver.getEvents());
        }
    }
    else if (kind == "RANGE")
    {
        route = candidate.query("RANGE_IDS", targets.get(0), targets.get(1));
        PlayerAction action;
        action.name = "RANGE";
        action.value = targets.get(0);
        action.otherValue = targets.get(1);
        std::string ignored;
        candidate.execute(action, ignored);
        optimal = CampaignTools::charges(candidate.getEvents()) + route.getCount();
    }
    if (optimal <= 0)
    {
        state.message = "Level has no valid solution";
        return false;
    }
    backend = candidate;
    initialBackend = candidate;
    campaignRoute = route;
    campaignTargets = targets;
    campaignValues = values;
    campaignInorder = inorder;
    campaignCaseSize = caseSize;
    campaignKind = kind;
    campaignTitle = title;
    campaignHint = hint;
    campaignOrders = orders;
    clearHistory();
    state = GameState();
    state.mode = "CAMPAIGN";
    state.status = "PLAYING";
    state.act = act;
    state.level = level;
    state.objective = objective;
    state.optimalSteps = optimal;
    state.alarmCapacity = static_cast<int>(std::ceil(1.5 * optimal)) + 2;
    state.score = 1000;
    initialState = state;
    if (kind == "RECONSTRUCT")
    {
        backend.select("BinaryTree");
        initialBackend = candidate; // Hidden reference; reset below uses an empty live model.
    }
    snapshot = backend.snapshot();
    EventBatch empty;
    animator.load(snapshot, empty, snapshot);
    return true;
}

OperationResult GameController::campaignAction(const PlayerAction& action)
{
    OperationResult result;
    result.before = snapshot;
    result.after = snapshot;
    result.gameState = state;
    if (!focused || state.status != "PLAYING" || !animator.isFinished())
    {
        result.message = "Wait for playback before the next action";
        return result;
    }
    const BackendNodeView* clicked = CampaignTools::node(snapshot.model, action.nodeId);
    if (action.name == "SELECT_NODE" && clicked == NULL)
    {
        result.message = "Choose a room";
        return result;
    }
    // Selecting a height-answer room is UI state only, not a scored attempt.
    if (campaignKind == "HEIGHTS" && action.name == "SELECT_NODE")
    {
        state.selectedId = action.nodeId;
        result.accepted = true;
        result.success = true;
        result.message = "Enter this room's height, then Submit";
        result.gameState = state;
        return result;
    }
    bool allowed =
        (campaignKind.find("AVL_") == 0 && (action.name == "SELECT_NODE" || action.name == "SELECT_PIVOT" ||
         action.name == "ROTATION_CASE" || action.name == "ROTATE_PRIMITIVE" || action.name == "SUBMIT")) ||
        (campaignKind.find("HEAP_") == 0 && (action.name == "SELECT_HEAP_INDEX" || action.name == "BUILD_ACTIVE_HEAP" ||
         action.name == "HEAPIFY" || action.name == "SIFT_UP" || action.name == "REPAIR_SWAP" || action.name == "EXTRACT_SORTED")) ||
        ((campaignKind == "TRAVERSAL" || campaignKind == "LCA_PATH") && (action.name == "SELECT_NODE" || action.name == "SUBMIT")) ||
        (campaignKind == "RECONSTRUCT" && action.name == "PLACE_TOKEN") ||
        (campaignKind == "HEIGHTS" && action.name == "SUBMIT") ||
        (campaignKind == "MIRROR" && (action.name == "SELECT_NODE" || action.name == "AUTO")) ||
        ((campaignKind == "SEARCH" || campaignKind == "INSERT_PATH") && action.name == "DIRECTION") ||
        (campaignKind == "DELETE_CASE" && action.name == "DELETE_CASE") ||
        (campaignKind == "RANGE" && (action.name == "SUBMIT" || action.name == "SELECT_NODE"));
    if (!allowed)
    {
        result.message = "Use the actions shown for this objective";
        return result;
    }
    BackendAdapter before = backend;
    GameState oldState = state;
    result.accepted = true;
    bool complete = false;
    PlayerAction operation;
    operation.name = "SELECT_NODE";
    operation.nodeId = action.nodeId;
    if (campaignKind.find("HEAP_") == 0)
    {
        heapCampaignAction(action, result, complete);
    }
    else if (campaignKind.find("AVL_") == 0)
    {
        avlCampaignAction(action, result, complete);
    }
    else if (campaignKind == "TRAVERSAL" || campaignKind == "LCA_PATH")
    {
        if (state.cursor < campaignRoute.getCount() && action.name == "SELECT_NODE")
        {
            result.success = action.nodeId == campaignRoute.get(state.cursor);
            if (result.success)
            {
                CampaignTools::run(backend, operation, result);
                state.cursor++;
            }
            complete = campaignKind == "TRAVERSAL" && state.cursor == campaignRoute.getCount();
        }
        else if (campaignKind == "LCA_PATH" && state.cursor == campaignRoute.getCount() && action.name == "SUBMIT")
        {
            operation.nodeId = snapshot.model.rootId;
            CampaignTools::run(backend, operation, result);
            result.success = action.value == campaignRoute.getCount() - 4;
            complete = result.success;
        }
        else
        {
            result.accepted = false;
        }
    }
    else if (campaignKind == "HEIGHTS")
    {
        if (state.cursor < campaignRoute.getCount())
        {
            int required = campaignRoute.get(state.cursor);
            const BackendNodeView* node = CampaignTools::node(snapshot.model, required);
            operation.nodeId = action.nodeId != -1 ? action.nodeId : (state.selectedId == -1 ? required : state.selectedId);
            CampaignTools::run(backend, operation, result);
            result.success = operation.nodeId == required && action.value == node->height;
            if (result.success)
            {
                state.cursor++;
                state.selectedId = -1;
            }
        }
        else
        {
            operation.nodeId = snapshot.model.rootId;
            CampaignTools::run(backend, operation, result);
            result.success = action.value == backend.query("DIAMETER").get(0);
            complete = result.success;
        }
    }
    else if (campaignKind == "MIRROR")
    {
        if (action.name == "AUTO")
        {
            result.success = true;
            while (state.cursor < campaignRoute.getCount())
            {
                operation.nodeId = campaignRoute.get(state.cursor);
                operation.name = "SELECT_NODE";
                result.success = CampaignTools::run(backend, operation, result) && result.success;
                operation.name = "SWAP_CHILDREN";
                result.success = CampaignTools::run(backend, operation, result) && result.success;
                state.cursor++;
            }
        }
        else
        {
            result.success = action.nodeId == campaignRoute.get(state.cursor);
            if (result.success)
            {
                CampaignTools::run(backend, operation, result);
                operation.name = "SWAP_CHILDREN";
                CampaignTools::run(backend, operation, result);
                state.cursor++;
            }
        }
        BackendAdapter mirrored = initialBackend;
        PlayerAction mirror;
        mirror.name = "MIRROR";
        std::string ignored;
        mirrored.execute(mirror, ignored);
        complete = state.cursor == campaignRoute.getCount() && backend.equals(mirrored);
    }
    else if (campaignKind == "RECONSTRUCT")
    {
        GameSnapshot target = initialBackend.snapshot();
        const BackendNodeView* slot = CampaignTools::node(target.model, action.nodeId);
        result.success = slot != NULL && slot->value == action.value &&
                         !CampaignTools::contains(state.answers, action.nodeId);
        if (result.success && slot->parentId != -1 && !CampaignTools::contains(state.answers, slot->parentId))
        {
            result.success = false;
            result.message = "Place the parent room before its child";
        }
        if (result.success)
        {
            operation.name = "INSERT";
            operation.value = action.value;
            operation.nodeId = -1;
            if (slot->parentId != -1)
            {
                const BackendNodeView* parent = CampaignTools::node(target.model, slot->parentId);
                operation.nodeId = CampaignTools::idFor(snapshot.model, parent->value);
                operation.text = parent->leftId == slot->id ? "LEFT" : "RIGHT";
            }
            result.success = CampaignTools::run(backend, operation, result);
            if (result.success)
            {
                GameSnapshot built = backend.snapshot();
                operation.name = "SELECT_NODE";
                operation.nodeId = CampaignTools::idFor(built.model, action.value);
                CampaignTools::run(backend, operation, result);
                state.answers.append(slot->id);
                state.cursor++;
                if (state.answers.getCount() == target.model.nodeCount && backend.equals(initialBackend))
                {
                    if (state.stage == 0)
                    {
                        state.stage = 1;
                        state.answers.clear();
                        backend.select("BinaryTree");
                        result.message = "Preorder blueprint solved. Now use postorder + inorder.";
                    }
                    else
                    {
                        complete = true;
                    }
                }
            }
        }
    }
    else if (campaignKind == "SEARCH" || campaignKind == "INSERT_PATH")
    {
        int id = campaignRoute.get(state.cursor);
        const BackendNodeView* current = CampaignTools::node(snapshot.model, id);
        int key = campaignTargets.get(0);
        std::string expected = key < current->value ? "LEFT" : (key > current->value ? "RIGHT" : "FOUND");
        if (action.text != "LEFT" && action.text != "RIGHT" && action.text != "FOUND")
        {
            result.accepted = false;
        }
        else
        {
            operation.name = "COMPARE_ROOM";
            operation.nodeId = id;
            operation.value = key;
            CampaignTools::run(backend, operation, result);
            result.success = action.text == expected;
            if (result.success)
            {
                state.cursor++;
                if (state.cursor == campaignRoute.getCount())
                {
                    if (campaignKind == "INSERT_PATH")
                    {
                        operation.name = "INSERT_AT";
                        result.success = CampaignTools::run(backend, operation, result);
                    }
                    complete = result.success;
                }
            }
        }
    }
    else if (campaignKind == "DELETE_CASE")
    {
        int key = campaignTargets.get(state.cursor);
        const BackendNodeView* target = CampaignTools::node(snapshot.model, CampaignTools::idFor(snapshot.model, key));
        if (target == NULL)
        {
            result.accepted = false;
        }
        else
        {
            int children = (target->leftId != -1 ? 1 : 0) + (target->rightId != -1 ? 1 : 0);
            std::string expected = children == 0 ? "LEAF" : (children == 1 ? "ONE_CHILD" : "TWO_CHILDREN");
            result.success = action.text == expected;
            operation.name = result.success ? "DELETE" : "SEARCH";
            operation.value = key;
            CampaignTools::run(backend, operation, result);
            if (result.success)
            {
                state.cursor++;
                complete = state.cursor == campaignTargets.getCount() && backend.validate();
            }
        }
    }
    else if (campaignKind == "RANGE")
    {
        if (action.name == "SUBMIT")
        {
            operation.name = "RANGE";
            operation.value = action.value;
            operation.otherValue = action.otherValue;
            CampaignTools::run(backend, operation, result);
            result.success = action.value == campaignTargets.get(0) && action.otherValue == campaignTargets.get(1);
            if (result.success)
            {
                state.stage = 1;
                state.answers.clear();
                complete = campaignRoute.getCount() == 0;
            }
        }
        else if (state.stage == 1)
        {
            CampaignTools::run(backend, operation, result);
            result.success = CampaignTools::contains(campaignRoute, action.nodeId) &&
                             !CampaignTools::contains(state.answers, action.nodeId);
            if (result.success)
            {
                state.answers.append(action.nodeId);
                complete = state.answers.getCount() == campaignRoute.getCount();
            }
        }
        else
        {
            result.accepted = false;
            result.message = "Submit the requested bounds first";
        }
    }
    if (!result.accepted)
    {
        backend = before;
        state = oldState;
        return result;
    }
    PlayerAction recorded = action;
    if (campaignKind.find("HEAP_") == 0)
    {
        recorded.value = action.indexA >= 0 ? action.indexA : action.value;
        recorded.otherValue = action.indexB >= 0 ? action.indexB : action.otherValue;
    }
    if (campaignKind == "HEIGHTS" && action.name == "SUBMIT")
    {
        recorded.nodeId = operation.nodeId;
    }
    remember(before, recorded, oldState);
    int charge = CampaignTools::charges(result.events);
    if (!result.success)
    {
        state.penalties++;
        // An actual visit/comparison already charges a wrong numeric/path answer.
        // Wrong traversal/placement clicks without model events cost exactly one.
        if (charge == 0)
        {
            charge = campaignKind == "AVL_SINGLE" || campaignKind == "AVL_DOUBLE" || campaignKind == "AVL_PREDICT" ? 2 : 1;
        }
        if (campaignKind == "AVL_SINGLE" || campaignKind == "AVL_DOUBLE" || campaignKind == "AVL_PREDICT")
        {
            state.pointPenalties += 50;
        }
        if (result.message.empty())
        {
            result.message = "Incorrect answer. Try again.";
        }
    }
    state.alarmSteps += charge;
    state.score = 1000 - 10 * state.alarmSteps - state.pointPenalties;
    if (state.score < 0)
    {
        state.score = 0;
    }
    if (state.alarmSteps >= state.alarmCapacity)
    {
        state.status = "PENDING_FAILURE";
    }
    else if (complete)
    {
        state.status = "PENDING_SUCCESS";
    }
    history[historyCount - 1].playback = result.events;
    state.message = result.message;
    result.after = backend.snapshot();
    snapshot = result.after;
    animator.load(result.before, result.events, result.after);
    finishCampaignAction();
    result.gameState = state;
    return result;
}

void GameController::finishCampaignAction()
{
    if (!animator.isFinished())
    {
        return;
    }
    if (state.status == "PENDING_FAILURE")
    {
        state.failed = true;
        state.stars = 0;
        state.status = "RESULT";
    }
    else if (state.status == "PENDING_SUCCESS")
    {
        state.completed = true;
        state.status = "RESULT";
        state.stars = state.alarmSteps <= state.optimalSteps && state.penalties == 0 ? 3 :
                      (state.alarmSteps <= 1.25 * state.optimalSteps && state.penalties <= 1 ? 2 : 1);
        if (state.hinted && state.stars > 2)
        {
            state.stars = 2;
        }
        if (restoringCampaign)
        {
            return;
        }
        int index = (state.act - 1) * 5 + state.level - 1;
        if (campaignStars[index] < state.stars)
        {
            campaignStars[index] = state.stars;
        }
        if (!saveCampaignProgress())
        {
            state.message = "Level completed; progress could not be saved";
        }
    }
}

bool GameController::requestHint(std::string& message)
{
    if (state.mode != "CAMPAIGN" || state.status != "PLAYING")
    {
        return false;
    }
    state.hinted = true;
    message = campaignHint;
    state.message = message;
    return true;
}

const std::string& GameController::getCampaignTitle() const
{
    return campaignTitle;
}

bool GameController::saveCampaign()
{
    if (state.mode != "CAMPAIGN" || state.status != "PLAYING" || !animator.isFinished())
    {
        state.message = "Finish playback before saving an active level";
        return false;
    }
    std::string path = saveDirectory + "/campaign-resume.json";
    std::ofstream output((path + ".tmp").c_str());
    output << "{\"version\":2,\"act\":" << state.act << ",\"level\":" << state.level
           << ",\"hinted\":" << (state.hinted ? 1 : 0) << ",\"selectedId\":" << state.selectedId
           << ",\"alarmSteps\":" << state.alarmSteps << ",\"tree\":\"" << backend.serializeTree()
           << "\",\"actions\":\"";
    for (int i = 0; i < historyCount; i++)
    {
        const PlayerAction& action = history[i].action;
        output << action.name << ' ' << action.value << ' ' << action.otherValue << ' '
               << action.nodeId << ' ' << (action.text.empty() ? "~" : action.text) << ' ';
    }
    output << "\",\"stars\":[";
    for (int i = 0; i < 20; i++)
    {
        output << (i == 0 ? "" : ",") << campaignStars[i];
    }
    output << "]}\n";
    output.close();
    bool success = !output.fail() && SaveFile::commit(path + ".tmp", path);
    state.message = success ? "Campaign checkpoint saved" : "Could not save campaign checkpoint";
    return success;
}

bool GameController::resumeCampaign()
{
    JsonDocument document;
    int act = 0;
    int level = 0;
    int hinted = 0;
    int selected = -1;
    int alarm = 0;
    int version = 0;
    std::string actions;
    std::string tree;
    IntBuffer stars;
    if (!document.load(saveDirectory + "/campaign-resume.json") ||
        !document.only("|version||act||level||hinted||selectedId||alarmSteps||tree||actions||stars|") ||
        !document.integer("version", version) || (version != 1 && version != 2) || !document.integer("act", act) ||
        !document.integer("level", level) || !document.integer("hinted", hinted) || (hinted != 0 && hinted != 1) ||
        !document.integer("selectedId", selected) || !document.integer("alarmSteps", alarm) ||
        !document.string("tree", tree) || !document.string("actions", actions))
    {
        state.message = "Missing or corrupt campaign checkpoint";
        return false;
    }
    GameController candidate(saveDirectory);
    candidate.restoringCampaign = true;
    if (version == 2)
    {
        if (!document.array("stars", stars) || stars.getCount() != 20)
        {
            state.message = "Invalid checkpoint progress";
            return false;
        }
        bool gap = false;
        for (int i = 0; i < 20; i++)
        {
            if (stars.get(i) < 0 || stars.get(i) > 3 || (gap && stars.get(i) > 0))
            {
                state.message = "Invalid checkpoint unlock sequence";
                return false;
            }
            gap = gap || stars.get(i) == 0;
            if (candidate.campaignStars[i] < stars.get(i))
            {
                candidate.campaignStars[i] = stars.get(i);
            }
        }
    }
    if (!candidate.loadLevel(act, level))
    {
        state.message = "Checkpoint level is unavailable";
        return false;
    }
    std::istringstream input(actions);
    int count = 0;
    PlayerAction action;
    while (input >> action.name)
    {
        if (!(input >> action.value >> action.otherValue >> action.nodeId >> action.text) || ++count > 500)
        {
            state.message = "Invalid checkpoint actions";
            return false;
        }
        if (action.text == "~")
        {
            action.text.clear();
        }
        if (!candidate.campaignAction(action).accepted)
        {
            state.message = "Checkpoint action cannot be restored";
            return false;
        }
        candidate.update(1000);
    }
    if (!input.eof() || candidate.state.status != "PLAYING" || candidate.state.alarmSteps != alarm ||
        candidate.backend.serializeTree() != tree ||
        (selected != -1 && (candidate.snapshot.structure == "MaxHeap" ?
            selected < 0 || selected >= candidate.snapshot.model.arraySize : CampaignTools::node(candidate.snapshot.model, selected) == NULL)))
    {
        state.message = "Checkpoint state failed validation";
        return false;
    }
    clearHistory();
    history = new HistoryStep[candidate.historyCount];
    historyCount = candidate.historyCount;
    historyCapacity = historyCount;
    for (int i = 0; i < historyCount; i++)
    {
        history[i] = candidate.history[i];
    }
    backend = candidate.backend;
    initialBackend = candidate.initialBackend;
    initialState = candidate.initialState;
    campaignRoute = candidate.campaignRoute;
    campaignTargets = candidate.campaignTargets;
    campaignValues = candidate.campaignValues;
    campaignInorder = candidate.campaignInorder;
    campaignCaseSize = candidate.campaignCaseSize;
    campaignKind = candidate.campaignKind;
    campaignTitle = candidate.campaignTitle;
    campaignHint = candidate.campaignHint;
    campaignOrders = candidate.campaignOrders;
    state = candidate.state;
    for (int i = 0; i < 20; i++)
    {
        if (campaignStars[i] < candidate.campaignStars[i])
        {
            campaignStars[i] = candidate.campaignStars[i];
        }
    }
    state.selectedId = selected;
    state.hinted = hinted == 1;
    snapshot = backend.snapshot();
    EventBatch empty;
    animator.load(snapshot, empty, snapshot);
    state.message = "Campaign checkpoint restored";
    return true;
}

const std::string& GameController::getCampaignKind() const
{
    return campaignKind;
}

std::string GameController::briefingInstructions() const
{
    if (campaignKind == "HEAP_BUILD")
    {
        return "Select a parent and its larger child in either the tree or array, then Swap.\nYou can also use Build Heap to see bottom-up construction.\nFinish when every parent is at least as large as its children.";
    }
    if (campaignKind == "HEAP_REPAIR")
    {
        return "Repair both disturbed heaps: sift down at index 0, then sift up at index 6.\nSelect the index in the tree/array, or type its index in Input.\nOnly the current active heap can be changed; invalid swaps add one alarm step.";
    }
    if (campaignKind.find("HEAP_") == 0)
    {
        return "Build a valid Max Heap, then Extract Maximum into the sorted suffix.\nWhen the active heap is disturbed, select index 0 and Heapify before extracting again.\nRepeat until every array cell is sorted ascending. Sorted cells stay locked.";
    }
    if (campaignKind == "TRAVERSAL")
    {
        return "Click the rooms in the requested traversal order. Complete all four rounds.\nWatch the instruction line for the next order; a wrong click adds one alarm step.";
    }
    if (campaignKind == "RECONSTRUCT")
    {
        return "Drag a numbered token into its empty tree slot. Place parents before children.\nUse preorder + inorder first, then postorder + inorder. Solve both blueprints.";
    }
    if (campaignKind == "HEIGHTS")
    {
        return "Work from the leaves upward: click the requested room, enter its height, Submit.\nA leaf has height 1. Finally enter the diameter as the number of edges.";
    }
    if (campaignKind == "LCA_PATH")
    {
        return "Click the two requested endpoints, then their lowest common ancestor.\nTrace the path from the first room to the second, including both endpoints.\nFinish by entering its length in edges.";
    }
    if (campaignKind == "MIRROR")
    {
        return "Click every room bottom-up in postorder to swap its left and right children.\nAuto completes the remaining swaps if you prefer. The final tree must be mirrored.";
    }
    if (campaignKind == "SEARCH" || campaignKind == "INSERT_PATH")
    {
        return "Compare the target with the current room, then choose Left, Right or Found.\nSmaller keys go left; larger keys go right. Follow the whole correct path.\nInsertion creates a new room only when you reach its valid empty slot.";
    }
    if (campaignKind == "DELETE_CASE")
    {
        return "For each requested treasure, choose Leaf, One Child or Two Children.\nWatch the real unlink or successor/value-swap animation. Solve all three deletions.";
    }
    if (campaignKind == "RANGE")
    {
        return "Enter the two requested bounds and Submit. Watch which subtrees are skipped.\nThen click exactly the returned rooms in any order; duplicates do not count.";
    }
    if (campaignKind == "AVL_BADGES")
    {
        return "Each room shows its balance factor: height(left) minus height(right).\nClick every room outside -1, 0, 1. Do not select balanced rooms or repeat answers.";
    }
    if (campaignKind == "AVL_SINGLE")
    {
        return "Drag a subtree onto the unbalanced pivot, then choose LL or RR.\nLL means a right rotation; RR means a left rotation. Complete all four examples.\nA wrong pivot/case adds 2 alarm steps and costs 50 points.";
    }
    if (campaignKind == "AVL_DOUBLE")
    {
        return "Drag onto the unbalanced pivot and identify LR or RL.\nThen drag onto each primitive pivot and choose Rotate Left or Rotate Right.\nLR: left at the left child, then right at the root. RL reverses these directions.";
    }
    if (campaignKind == "AVL_PREDICT")
    {
        return "Before each displayed insertion, predict NONE, LL, RR, LR or RL.\nCorrect predictions apply the real insertion and animate all rotations.\nWrong predictions leave the tree unchanged and cost 2 alarm steps and 50 points.";
    }
    return "Use subtree sizes to click the path to the requested k-th smallest treasure.\nThen enter the inclusive rank of the displayed key and the root subtree size.\nThe model validates heights, sizes, links and balance before success.";
}

GameSnapshot GameController::blueprint() const
{
    return initialBackend.snapshot();
}

std::string GameController::campaignInstruction() const
{
    if (campaignKind.find("HEAP_") == 0)
    {
        if (snapshot.model.heapSize == 0)
        {
            return "Sorting complete: all treasures are in ascending order.";
        }
        if (campaignKind == "HEAP_BUILD")
        {
            return "Build a Max Heap: select parent/child indices and Swap, or Build Heap.";
        }
        if (campaignKind == "HEAP_REPAIR")
        {
            return state.stage == 0 ? "Round 1: repair downward from index 0." : "Round 2: repair upward from index 6.";
        }
        return "Active heap: " + std::to_string(snapshot.model.heapSize) + " / sorted cells: " +
            std::to_string(snapshot.model.arraySize - snapshot.model.heapSize) +
            (backend.validate() ? " — extract the maximum next." : " — restore the heap before extracting.");
    }
    if (campaignKind == "AVL_BADGES")
    {
        return "Click every room with |balance factor| > 1. Selected: " + std::to_string(state.answers.getCount());
    }
    if (campaignKind == "AVL_SINGLE" || campaignKind == "AVL_DOUBLE")
    {
        return "Case " + std::to_string(state.stage + 1) + "/" + std::to_string(campaignValues.getCount() / campaignCaseSize) +
            (state.choice.empty() ? ": drag a subtree onto its pivot, then choose the case." : ": drag onto the next pivot, then choose a primitive rotation.");
    }
    if (campaignKind == "AVL_PREDICT")
    {
        return state.stage < campaignTargets.getCount() ? "Before inserting " + std::to_string(campaignTargets.get(state.stage)) +
            ", predict NONE, LL, RR, LR or RL." : "Prediction rounds complete.";
    }
    if (campaignKind == "AVL_ORDER")
    {
        return state.stage == 0 ? "Follow subtree sizes: click the path to the " + std::to_string(campaignTargets.get(0)) + "-th smallest treasure." :
            (state.stage == 1 ? "Enter rank(" + std::to_string(campaignTargets.get(1)) + ") including the key itself, then Submit." : "Enter the root subtree size to validate the augmentation.");
    }
    if (campaignKind == "TRAVERSAL")
    {
        return CampaignTools::orderAt(campaignOrders, state.cursor / snapshot.model.nodeCount);
    }
    if (campaignKind == "RECONSTRUCT")
    {
        std::string order = state.stage == 0 ? "PREORDER" : "POSTORDER";
        GameSnapshot original = initialBackend.snapshot();
        std::string text = order + ": ";
        IntBuffer ids = initialBackend.query(order);
        for (int i = 0; i < ids.getCount(); i++)
        {
            text += std::to_string(CampaignTools::node(original.model, ids.get(i))->value) + " ";
        }
        text += " / INORDER: ";
        ids = initialBackend.query("INORDER");
        for (int i = 0; i < ids.getCount(); i++)
        {
            text += std::to_string(CampaignTools::node(original.model, ids.get(i))->value) + " ";
        }
        return text;
    }
    if (campaignKind == "HEIGHTS" && state.cursor < campaignRoute.getCount())
    {
        int id = campaignRoute.get(state.cursor);
        const BackendNodeView* required = CampaignTools::node(snapshot.model, id);
        return "Bottom-up: click room " + std::to_string(required->value) + ", enter its height, Submit.";
    }
    if (campaignKind == "HEIGHTS")
    {
        return "Now enter the tree diameter (number of edges), then Submit.";
    }
    if (campaignKind == "LCA_PATH")
    {
        if (state.cursor < 2)
        {
            return "Select room " + std::to_string(campaignTargets.get(state.cursor));
        }
        if (state.cursor == 2)
        {
            return "Click the lowest common ancestor (meeting room).";
        }
        if (state.cursor < campaignRoute.getCount())
        {
            return "Trace the path from the first room to the second, including both endpoints.";
        }
        return "Enter the path length in edges, then Submit.";
    }
    if (campaignKind == "MIRROR")
    {
        return "Click each room in postorder to swap its children, or Auto.";
    }
    if (campaignKind == "SEARCH" || campaignKind == "INSERT_PATH")
    {
        if (state.cursor >= campaignRoute.getCount())
        {
            return "Path complete.";
        }
        const BackendNodeView* current = CampaignTools::node(snapshot.model, campaignRoute.get(state.cursor));
        return "Target " + std::to_string(campaignTargets.get(0)) + " / current room " +
               std::to_string(current->value) + ": Left, Right or Found?";
    }
    if (campaignKind == "DELETE_CASE")
    {
        return state.cursor < campaignTargets.getCount() ?
            "Delete " + std::to_string(campaignTargets.get(state.cursor)) + ": choose its deletion case." : "All deletion cases complete.";
    }
    if (campaignKind == "RANGE")
    {
        return state.stage == 0 ? "Enter " + std::to_string(campaignTargets.get(0)) + " " +
            std::to_string(campaignTargets.get(1)) + " and Submit." :
            "Click exactly the returned rooms (any order).";
    }
    return state.objective;
}
