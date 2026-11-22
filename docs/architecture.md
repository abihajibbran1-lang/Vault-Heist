# Vault Heist — Technical Architecture

**Companion documents:** `docs/prd.md`, `docs/event-contract.md`, `docs/development-guide.md`, `docs/endless-mode-rules.md`, `AGENTS.md`.
**Implementation principle:** One C++ desktop executable with three layers, **not** a client/server application.

## 1. Boundaries and dependency direction

~~~text
SFML Frontend (input, layout, drawing, animation presentation, sounds)
                    |
                    v
Middleware (GameController, level rules, snapshots, events, scoring,
            replay/undo history, animation scheduling)
                    |
                    v
Backend (BinaryTree, BST, AVLTree, MaxHeap, validators)
~~~

Dependency rules:

1. Backend includes only approved C++ headers and its own data structures. **No SFML**, no UI references.
2. Middleware calls backend APIs, enforces level rules and exposes plain C++ operation results, events and snapshots. **No SFML rendering calls.**
3. Frontend receives immutable snapshots/event batches from middleware, handles clicks and drawing, and sends commands to GameController. **No BST, AVL, tree, or heap algorithms in the frontend.**
4. One authoritative backend instance per active game mode/level. No parallel UI copy of the algorithms. Visual playback state is allowed solely to show intermediate event frames.
5. No global variables, enums, STL containers or STL algorithms. Use `std::string` for text and manual arrays for collections.

## 2. Repository layout (retain the existing scaffold)

~~~text
VaultHeist/
├── .vscode/
│   ├── tasks.json
│   ├── launch.json
│   ├── c_cpp_properties.json
│   └── settings.json
├── assets/
│   ├── fonts/
│   ├── images/
│   └── sounds/
├── data/
│   ├── levels/
│   └── saves/
├── docs/
│   ├── prd.md
│   ├── architecture.md
│   ├── event-contract.md
│   └── development-guide.md
├── include/
│   ├── backend/
│   │   ├── BinaryTree.h
│   │   ├── BST.h
│   │   ├── AVLTree.h
│   │   ├── MaxHeap.h
│   │   └── OperationEvent.h  (add for backend-native events)
│   ├── middleware/
│   │   ├── GameController.h
│   │   ├── GameSnapshot.h
│   │   ├── GameEvent.h
│   │   ├── BackendAdapter.h
│   │   └── AnimationController.h
│   └── frontend/
│       ├── UIManager.h
│       ├── TreeRenderer.h
│       ├── HeapRenderer.h
│       ├── Button.h
│       └── Screen.h
├── src/
│   ├── backend/
│   │   ├── BinaryTree.cpp
│   │   ├── BST.cpp
│   │   ├── AVLTree.cpp
│   │   ├── MaxHeap.cpp
│   │   └── OperationEvent.cpp  (only if buffer implementation needs it)
│   ├── middleware/
│   │   ├── GameController.cpp
│   │   ├── BackendAdapter.cpp
│   │   ├── EventMapper.cpp
│   │   └── AnimationController.cpp
│   ├── frontend/
│   │   ├── UIManager.cpp
│   │   ├── TreeRenderer.cpp
│   │   ├── HeapRenderer.cpp
│   │   └── Button.cpp
│   └── main.cpp
├── tests/
│   ├── backend/
│   └── middleware/
├── .gitignore
├── README.md
└── AGENTS.md
~~~

These are **reserved responsibilities**, not instructions to fill every placeholder file with invented code. `OperationEvent.h` is the one explicitly identified additional interface needed to let the backend emit events **without including middleware headers**. Add its `.cpp` only if necessary. Add other files only when needed for a handout requirement or testability. The developer guide explains the implementation order.

## 3. Backend contract

**BinaryTree:** own dynamically allocated nodes; traversal/reconstruction, height, diameter, LCA, paths, mirror, symmetry/equality. Node fields minimally include stable `id`, `value`, `left` and `right`; `parent` if used.

**BST:** ordered insert/search/delete (duplicate rejection), min/max, successor/predecessor, range query, k-th smallest, BST LCA. Emit comparison/path and structural events during the actual algorithm.

**AVLTree:** extends or reuses BST behavior while maintaining node `height` and `subtreeSize`, rotations, kth, rank and range count. Every structural change updates metadata and logs rotation/height/size events.

**MaxHeap:** owns manual zero-based array, `heapSize`, and capacity. Implements build, siftUp/siftDown, insert, extractMax, heapSort, invariant checks. Events reference array indices; renderer derives tree positions from indices.

**Validator:** validate after mutations in debug builds and in every unit-test family. Required checks include ordering, stored metadata, AVL balance, optional parent links, no cycles/shared nodes, heap property and active/sorted partition.

**Backend-native events:** place a small `BackendEvent`/`BackendEventBatch` definition in `include/backend/OperationEvent.h`. It uses only approved C++ types and manual arrays, so BinaryTree/BST/AVL/MaxHeap can emit events without depending on `include/middleware`. The adapter converts those events into middleware `GameEvent`; do not make the backend include `middleware/GameEvent.h`.

**Function-name constraints:** use the handout's method names wherever C++ permits. The handout's `delete(key)` cannot be a method named `delete` because `delete` is a C++ keyword; use `deleteKey(key)` (or an owner-approved existing equivalent), and document the mapping. Its two `build(pre, in)` / `build(post, in)` operations cannot be overloaded if both argument types are identical; use distinctly named `buildFromPreIn` and `buildFromPostIn` (or one clear selector parameter). Do not silently omit either required behavior.

**Conventions:** leaf height = 1, empty height = 0. Choose and document diameter as number of **edges** on the longest path, consistently in code/tests/UI. No duplicate BST/AVL keys. Stable IDs must not change when values swap or nodes rotate.

## 4. Middleware responsibilities

| Existing file | Responsibility |
|---|---|
| `GameController.h/.cpp` | One public entry point for UI commands; active mode/level, rules, operation results, score/alarm, status and snapshots. |
| `BackendAdapter.h/.cpp` | Map GameController commands into correct backend API calls; convert nodes/heap array to read-only snapshots without exposing pointers. |
| `GameSnapshot.h` | Plain C++ `NodeView`, `EdgeView`, `HeapView` and `GameState` payloads with manual arrays/counts. No SFML geometry types. |
| `GameEvent.h` | Plain `GameEvent`, event batch, and string event-kind contract. |
| `EventMapper.cpp` | Convert dependency-free `BackendEvent` records into middleware `GameEvent` records and visual frames; preserve exact order and identifiers. |
| `AnimationController.h/.cpp` | Event queue/play/pause/step/speed/replay timing and current visual playback state; no rendering. |

Required shared responsibilities can initially be private helpers in these classes. Add dedicated `LevelManager`, `ScoreSystem`, `SaveManager`, `UndoManager`, `Validator`, or `LayoutEngine` files only when those responsibilities grow and the handout requires their separation. Obtain owner approval before reorganizing the scaffold.

### Recommended controller-facing operations (design contract, not final signatures)

~~~cpp
class GameController
{
public:
    bool loadLevel(int act, int level);
    bool selectMode(const std::string& mode);
    OperationResult performAction(const PlayerAction& action);
    bool undo();
    bool replayLevel();
    bool resetLevel();
    bool requestHint(std::string& message);
    const GameSnapshot& getSnapshot() const;
    const GameState& getState() const;
    const EventBatch& getEvents() const;
};
~~~

`PlayerAction` contains a string operation name plus relevant node IDs/values/indices. `OperationResult` explicitly states accepted/success/error message; detailed shapes are specified in `docs/event-contract.md`. All commands are validated against active level and mode before invoking the backend.

### Example: BST insertion flow

1. UI collects an integer and sends `INSERT_BST` through `performAction`.
2. Controller rejects the command if the current level forbids it, an animation is playing, or the value duplicates an existing key.
3. Before an **accepted mutating action**, middleware stores a deep copy of the backend state for undo.
4. Backend inserts and records its actual visit/compare/insert events using `BackendEvent`.
5. Adapter maps those events to `GameEvent`, and supplies post-operation model snapshot and result.
6. Controller applies alarm/score rules exactly once for chargeable events, checks win/failure after sequence, and records replay history.
7. AnimationController begins from the pre-action visual state and applies each event in order until the post-action snapshot is reached.
8. Frontend draws current playback state every frame; it does **not** change the backend.

Failed validation never mutates state. A wrong *gameplay* answer can still be an accepted action with an assignment-defined penalty.

## 5. Snapshot and memory ownership

### Model snapshots

Use manually managed arrays, for example:

~~~cpp
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

Document who owns each array and when it is freed. Prefer one `GameSnapshot` owner with a destructor, deep-copy constructor and copy-assignment implementation (or explicitly prohibit copying and provide `clone`/`clear`). Expose `const` references to UI wherever practical. Set absent IDs to `-1` consistently. Initialize all fields.

### Undo and replay

- **Undo:** save deep pre-action state including tree/heap, relevant game score/alarm and current level action state. Restore this as one coherent unit; drop/realign pending animations.
- **Replay level:** record starting state/seed and accepted player actions with their event batches. Restore initial state and replay deterministic actions and visuals. Replay must not award extra score, spend alarm, or save duplicate progress.
- **Event scrubber:** retain visual keyframes or sufficient event deltas so previous/next/scrub can reproduce correct intermediate frames. Merely jumping to a final snapshot is not step-by-step animation.
- **Ownership:** no storing raw pointers to nodes in event history or UI. After deletion, a stale pointer must never be dereferenced.

## 6. Frontend architecture

- `UIManager`: screen routing and SFML input dispatch; chooses current screen, passes player intent to GameController.
- `Screen`: common screen behavior only where actually needed; use a straightforward class/struct, not a complex inheritance framework.
- `Button`: focus/hover/click, rounded presentation, shadow, label; provides input results only.
- `TreeRenderer`: render nodes, edges, badges, highlights, selection and layout; calculate positions in frontend/layout helper.
- `HeapRenderer`: render the **same** HeapSnapshot as both a tree and array, including sorted region, highlights and swaps.
- `main.cpp`: initialize window, controller/UI, handle events, call time-based updates, draw, display. No data structure algorithms.

SFML is for rendering, input, clock and optionally audio. The actual installed graphics/audio modules must match the local compiler. **Building SFML from source with CMake is separate from building this game**; the game can use VS Code `tasks.json` and GCC directly, as preferred.

The approved local frontend target is SFML 2.6.1 in `/usr/local`, using Ubuntu
WSL/WSLg and C++17. Endless behavior is specified in `docs/endless-mode-rules.md`.

The GUI's visual state may lag behind the actual backend state during animation. It starts with the pre-action snapshot, applies event-defined changes, then must equal the authoritative post-action snapshot at operation completion. Never render only the final backend state during an animated operation.

### Lively but controlled animation

Use non-blocking frame-time updates (never `sleep`). Short scale/pulse/fade/tween effects help show visits, inserts, deletions, rotations, swaps and completion. At most one authoritative event advances at a time; affected nodes can move together within one event. Maintain labels/edge connections during motion. Pause, speed changes and replay must remain correct. Sound effects are presentation only and must degrade safely when disabled/unavailable.

## 7. JSON, saves and benchmarks

- `data/levels/*.json` declares act, level, seed, input values/tree shape, required operation/objective and allowed player actions. Use a documented narrow schema; do not hard-code level content in C++.
- The model computes `optimalSteps`; files must not assert a score advantage. For generated levels use the seed reproducibly.
- With the no-STL rule, create a small parser for the documented schema using permitted C/C++ text/file functions. Reject unexpected fields, malformed types and invalid ranges; no dependency-heavy JSON framework.
- Save campaign unlocks/stars, seed and Endless state. Serialize tree preorder with null markers; heap values plus active size; recompute AVL metadata and validate on load.
- Benchmarks run without SFML and with event emission disabled. Measure the cases in the PRD; do not perform GUI measurements inside algorithm timing.

## 8. Build and verification boundaries

1. Backend can compile and test without SFML.
2. Middleware can compile and test without opening a window.
3. Frontend integrates through headers in `include/middleware/`.
4. `.vscode/tasks.json` explicitly compiles and links every implemented `.cpp` (update it whenever files are added). `.vscode/launch.json` points to the actual executable.
5. No unapproved package manager, alternate compiler toolchain or extra build system. Keep Windows/MinGW setup documented in `README.md` once verified.
6. Run relevant unit tests after each change and a graphical smoke test after integration.

## 9. Architecture acceptance criteria

- Backend can be used entirely without SFML.
- The UI can demonstrate a model operation through `GameController` without dereferencing a backend node pointer.
- Event IDs survive AVL rotations and BST value swaps.
- Undo/replay restore backend state *and* matching visible state.
- Every Heap Swap changes tree and array highlights/values together.
- Event logging can be disabled for benchmarks without changing algorithm results.
- All code respects the no-STL/no-global/no-enum constraint and the approved personal style.
