# Vault Heist — Backend ↔ Middleware ↔ Frontend Event Contract

**Status:** Integration contract for implementation. Update together before changing public fields or operation semantics.  
**Applies to:** `include/backend/OperationEvent.h`, `include/middleware/GameEvent.h`, `GameSnapshot.h`, `GameController.h`, `BackendAdapter.h`, `AnimationController.h`.

## 1. Purpose and hard rules

The backend executes **real algorithms** and records the steps that actually happened in its dependency-free `BackendEvent` type. The adapter maps those records to middleware `GameEvent`. Middleware applies game rules, prepares visual states, and queues those steps. The SFML frontend only displays them and forwards player actions.

**Never** generate guessed comparison/rotation paths by inspecting the final tree after an operation. Emit events during the algorithm, or record a faithful intermediate snapshot at the exact step. No independent frontend model of BST/AVL/heap behavior.

Constraints:

- **No enums** or globally declared variables/constants. Event type and operation names are `std::string` values.
- **No STL containers**. Every batch, snapshot and history uses a manually managed array/count/capacity and correct memory ownership.
- Node references use **stable integer IDs**, not `Node*`. Heap references use **zero-based indices**.
- The frontend never owns or modifies backend nodes.
- Event payloads and snapshots must be deterministic for seeded levels and replay.
- Event log emission can be disabled during benchmarks; disabling it must not change algorithm correctness.

## 2. Data shapes

These are reference shapes to agree upon **before implementing**. Keep names and meanings consistent; implement constructors, cleanup and deep copying in the appropriate `.cpp` files. `GameEvent` below belongs to middleware. The backend owns a separate, minimal `BackendEvent` in `include/backend/OperationEvent.h` with the primitive/string payloads it needs; it **never includes the middleware header**. `EventMapper.cpp` translates between the two without rerunning algorithms.

~~~cpp
struct GameEvent
{
    std::string type;
    std::string description;
    std::string detail;

    int nodeId;
    int otherNodeId;
    int parentId;
    int pivotId;

    int indexA;
    int indexB;
    int oldValue;
    int newValue;
    int oldSize;
    int newSize;
};

struct EventBatch
{
    GameEvent* events;
    int count;
    int capacity;
};

struct NodeView
{
    int id;
    int value;
    int parentId;
    int leftId;
    int rightId;
    int height;
    int balanceFactor;
    int subtreeSize;
};

struct TreeSnapshot
{
    NodeView* nodes;
    int nodeCount;
    int rootId;
};

struct HeapSnapshot
{
    int* values;
    int arraySize;
    int heapSize;
    int sortedStart;
};
~~~

Use `-1` for absent node IDs and unused indices; initialize **all** integer fields even if an event does not use them. Use empty strings for unused details. A real value `-1` is allowed: do not use a *value* as a missing-node marker.

One `GameSnapshot` groups only the active structure, current mode, and gameplay HUD fields. The actual implementation can wrap these structs in classes to own memory safely. `GameState` includes act, level, alarm steps/capacity, score, penalties, objective text, completed/failed status and applicable view mode.

**Memory policy:** The owner frees its `new[]` arrays in a destructor; copying must make a deep copy, and assignment must release old memory safely. Expose `const GameSnapshot&` to readers and define when that reference becomes invalid. Do not directly expose mutable buffers to the renderer.

## 3. Operation and UI message contract

Frontend sends a plain `PlayerAction` (suggested fields: `std::string name`, `int value`, `int otherValue`, `int nodeId`, `int otherNodeId`, `int indexA`, `int indexB`) to `GameController::performAction`.

Relevant action names should correspond to the required operation, for example `SEARCH_BST`, `INSERT_BST`, `DELETE_BST`, `SELECT_NODE`, `SELECT_ROTATION`, `BUILD_HEAP`, `HEAPIFY`, `HEAP_SWAP`, `EXTRACT_MAX`, `HEAP_SORT`. Do not introduce new player operations.

Controller returns or exposes:

| Field | Meaning |
|---|---|
| `accepted` | Action is valid for current level/mode and may change state or count as a gameplay attempt. |
| `success` | Underlying command or player answer succeeded. |
| `message` | One short status/error/feedback line. |
| `events` | Exact backend and game events from this action, in order. |
| `before` | Deep pre-action snapshot for visual playback/undo. |
| `after` | Authoritative post-action snapshot. |
| `gameState` | Score/alarm/objective/win/fail status after rule evaluation. |

Do not call the backend directly from an SFML button. Button → UIManager → GameController → BackendAdapter → backend.

While an operation animation is running, block or queue further model-changing input according to one simple documented rule; **default is block until playback completes**. Play/pause/speed may still be used.

## 4. Event catalog (course-required kinds)

Use these **exact string names** in authored code. They are string values, **not enums**. Add only payload needed by the handout's operations.

| Type string | Required data | Visual behavior / game meaning |
|---|---|---|
| `VISIT` | `nodeId` | Pulse/outline room; +1 alarm for chargeable visit. |
| `COMPARE` | `nodeId`, `oldValue` = visited node's key, `newValue` = target/comparison key, outcome in `detail` (`LESS`, `EQUAL`, `GREATER`) | Show comparison and chosen direction; +1 alarm for chargeable compare. |
| `INSERT` | new `nodeId`, `parentId`, `detail` = `LEFT` or `RIGHT` or `ROOT` | Grow node and its edge at correct slot. |
| `UNLINK` | `nodeId`, `parentId`, old side in `detail` | Fade deleted node/edge; redraw correct replacement link. |
| `SWAP_VALUE` | `nodeId`, `otherNodeId`, previous/new values as needed | Animate values crossing, preserving node IDs. |
| `ROTATE` | `pivotId`, `detail` = `LL`, `RR`, `LR` or `RL`; affected links/frame | Animate topology and positions accurately. |
| `UPDATE_HEIGHT` | `nodeId`, `oldValue`, `newValue` | Refresh height badge. |
| `UPDATE_SIZE` | `nodeId`, `oldValue`, `newValue` | Refresh subtree-size badge. |
| `BALANCE_ALERT` | `nodeId`, balance factor in `newValue` | Mark invalid balance with red/label. |
| `HIGHLIGHT_SUBTREE` | `nodeId`, `detail` = `VISITED`, `SKIPPED` or `RESULT` | Highlight traversal or pruned branch. |
| `HEAP_BUILD` | initial array frame, active heap size | Show heap region and start of construction. |
| `HEAP_SWAP` | `indexA`, `indexB`, reason in `detail` | Swap matching cells in **tree and array** as a single animation step. |
| `HEAPIFY_START` | `indexA`, `oldSize`/active `heapSize` | Highlight heapify root and active region. |
| `HEAPIFY_DONE` | `indexA`, active `heapSize` | Show valid restored region. |
| `EXTRACT_MAX` | root index, maximum value, current heap size | Highlight the value being extracted. |
| `MARK_SORTED` | array index and value | Mark sorted suffix visibly. |
| `HEAP_SIZE_CHANGE` | `oldSize`, `newSize` | Update active boundary and sorted region. |
| `HEAP_SORT_COMPLETE` | final sorted-array frame | Confirm sort and unlock final vault when level conditions met. |
| `DONE` | short description | Marks end of one backend operation batch, not an extra gameplay step. |

`HEAP_SWAP` events must preserve the reason (e.g., build/heapify/extraction/sort) in `detail` but that text must not create additional game mechanics.

For event types that need a richer payload (e.g., `ROTATE` topology and `HEAP_SORT_COMPLETE` array), attach a **deep visual frame** after the step, or a manually managed before/after link/value delta owned by the event batch. Do **not** depend on backend pointers remaining alive after deletion.

## 5. Intermediate animation state

The backend normally finishes an operation before the UI begins playback. If the UI renders only the final snapshot, **no meaningful animation is possible**.

Required model:

1. Copy the `before` visual snapshot.
2. Read the next event and its authoritative after-step delta/frame.
3. Highlight and tween the affected nodes/cells for a short duration using frame time.
4. Apply the step's completed visual mutation.
5. Proceed until `DONE` and verify the visual state equals `after`.

For a **comparison** with no structural change, animate highlight/arrow without changing topology. For **insert/delete/rotation**, update links/positions at the event boundary or use stored frame topology. For **heap swaps**, apply the exact same index permutation to tree and array views on the same frame.

Use simple easing (ease-out or smooth start/end), brief pulse/bounce and colors/outlines/text to make changes noticeable. Do not use blocking sleeps, and do not let cosmetic effects reorder events.

### Playback control

- `play`, `pause`, `resume`, `next`, `previous` and `setSpeed` operate on a *single current event log*.
- `update(deltaSeconds)` advances visual time without changing authoritative backend state.
- Manual step/previous uses retained deep keyframes or faithful reversible deltas. Re-entering an earlier step must never duplicate an insertion/extraction.
- Full-level **Replay** (toolbar) separately restores the starting state and shows the entire recorded action sequence; it is not an extra scored attempt.
- **Undo** restores deep pre-action backend/game state and re-synchronizes visible tree/array. Event history is adjusted so future replay does not show an undone action as current.

## 6. Alarm, result and failure ordering

Only the GameController/score responsibility may change the alarm. Renderer and AnimationController may display alarm changes but must not increment counters themselves.

1. Backend event emission decides what comparison/visit actually happened.
2. Controller counts chargeable events once after adapter mapping, applies wrong-answer/rotation/heap penalties from the defined game rules, and updates stars/score.
3. If alarm reaches capacity, flag pending failure, let the **current operation animation finish**, then transition to Result → failure.
4. No further model actions are accepted while failure is pending.
5. Replay and cosmetic sound must not reapply penalties or overwrite saved progress.

No duplicated charges from showing the same event twice on screen.

## 7. Worked event examples

### BST: search for 30 in tree 50 ← 30

~~~text
VISIT(nodeId=1)
COMPARE(nodeId=1, oldValue=50, newValue=30, detail=LESS)
VISIT(nodeId=2)
COMPARE(nodeId=2, oldValue=30, newValue=30, detail=EQUAL)
DONE("Target found")
~~~

Actual event IDs and counts come from the backend implementation. This example shows ordering, not a fixed alarm budget.

### AVL: inserting 10 into 30 → 20 → 10

~~~text
VISIT(30), COMPARE(30)
VISIT(20), COMPARE(20)
INSERT(nodeId=3, parentId=2, detail=LEFT)
UPDATE_HEIGHT(...), BALANCE_ALERT(nodeId=1,...)
ROTATE(pivotId=1, detail=LL, topology frame attached)
UPDATE_HEIGHT(...), UPDATE_SIZE(...)
DONE("AVL insertion complete")
~~~

The correct **post-rotation** snapshot has 20 as root and 10/30 as its children. IDs remain attached to the original node objects.

### Max Heap: one extraction/sort step

~~~text
EXTRACT_MAX(value=maximum)
HEAP_SWAP(indexA=0, indexB=lastActive, detail=EXTRACT)
HEAP_SIZE_CHANGE(oldSize=n, newSize=n-1)
MARK_SORTED(index=oldLast, value=maximum)
HEAPIFY_START(indexA=0, activeSize=n-1)
HEAP_SWAP(indexA=..., indexB=..., detail=SIFT_DOWN)  [if needed]
HEAPIFY_DONE(indexA=0)
DONE("Extraction complete")
~~~

During standard heapsort, the maximum is moved to the sorted suffix rather than being erased from the visible array. For a standalone `extractMax`, the backend's contract decides whether the active array shrinks; the view must honor that contract. Never fake a heap step solely for animation.

## 8. Integration tests (required)

| Test | Pass condition |
|---|---|
| BST path events | Adapter preserves actual backend event sequence and IDs, including missing key. |
| Duplicate insert | No structural insertion, visible feedback, no invented events. |
| Delete two children | Correct successor, value swap/unlink visuals, final BST valid. |
| AVL rotations | All four types generate correct event kinds, pivot IDs, and post-step links. |
| Heap swap | A single swap updates both views identically. |
| Heap Sort | Active heap shrinks; sorted suffix grows; final array ascending. |
| Undo | Restores backend, HUD and visual state with no leaked/dangling nodes. |
| Replay | Produces original visuals and does not recharge alarm or score. |
| Pause/step/speed | No skipped, duplicated or reordered events. |
| Failure during animation | Current animation finishes, then failure screen appears once. |
| Benchmarks without events | Algorithm output identical, event storage disabled. |

## 9. Rules for changing this contract

Only revise an event name/field after checking every backend producer, middleware consumer, frontend renderer and corresponding test. Explain the change in the pull request; update this document first. A coding agent **must not** silently introduce new event types, non-required operations, STL containers, enums or global variables.
