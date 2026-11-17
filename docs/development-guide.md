# Vault Heist — Development Guide

**Purpose:** Tell developers and IDE agents exactly how to implement the approved project in the existing repository, without inventing new features.  
**Read first:** `AGENTS.md`, `docs/prd.md`, `docs/architecture.md`, `docs/event-contract.md`.

## 1. Development environment

- Windows + VS Code, C++17 or later, GCC/MinGW-w64.
- SFML 3 is the intended GUI library. Use the locally built SFML installation after it is verified.
- Building **SFML itself** using CMake is acceptable because the owner agreed to that one-time step. **Do not convert Vault Heist to a CMake project** without approval. The game can compile using a VS Code build task calling `g++` with include/library paths.
- Development must not require a backend web service, REST API, database, internet account or additional programming language.
- Backend and middleware tests must compile independently from SFML.
- Audio is optional polish; if SFML Audio is not yet built/linked, implement graphical functionality first and leave sound integration disabled until it is available.

## 2. Build in this sequence

| Phase | Work | Done when |
|---|---|---|
| **A. Foundation** | Confirm repository scaffold, GCC and SFML include/lib paths, a minimal SFML window. | VS Code builds/runs a blank window and backend tests can build independently. |
| **B. Contracts** | Finalize dependency-free backend `BackendEvent`, middleware `GameEvent`, manual-array event batches, snapshots, `GameController` and operation results. | A tiny fake-data controller → snapshot → renderer smoke test works; backend needs no middleware headers. |
| **C. Backend: BinaryTree** | Node ownership, all traversals, reconstruction, height/diameter/LCA/path/mirror, tests. | Course API cases pass, leaf height = 1. |
| **D. Backend: BST** | Insert, search, delete, min/max, successor/predecessor, range, kth, tests/events. | Real visit/compare/unlink/swap events; validators pass. |
| **E. Backend: AVL** | Rotations, rebalancing, deletion, size/rank/kth, tests/events. | LL/RR/LR/RL animated metadata paths and invariants pass. |
| **F. Backend: Heap** | Max Heap build/heapify/insert/extract and Heap Sort, tests/events. | All required inputs sorted correctly; active/sorted boundary valid. |
| **G. Middleware** | Adapter, controller, level loading, rules, alarm/score/stars, undo, replay, autosave, Sandbox/Endless. | Model and event integration tested without graphics. |
| **H. Frontend** | UI, campaign, briefing/play/result/screens, tree/heap drawing, event animation, input, zoom/pan/fit. | Every required game mode and level operable through SFML. |
| **I. Polish** | Approved bubbly styling, readable cues, easing, optional sound cues, Settings mute. | Correct animations; no new gameplay rules; silence is safe. |
| **J. Final QA** | Full course test matrix, benchmark run/report, captures, level authoring guide, ZIP preparation. | All mandatory deliverables checked and reproducible. |

Phases can overlap only when the shared contract is stable; the owner explicitly prioritizes **backend and middleware before full frontend**.

## 3. Minimum vertical-slice checklist

For each function/feature:

1. Locate the exact handout requirement and existing header declaration.
2. Write small tests and edge cases first or alongside implementation.
3. Implement the required algorithm in backend **without SFML**.
4. Emit actual events during execution if the algorithm is visualized.
5. Make middleware call the implementation; return valid snapshots/results.
6. If the slice needs GUI behavior, consume snapshots/events in the frontend and animate faithfully.
7. Check undo, replay, invalid input, memory cleanup and validators where applicable.
8. Run build/tests, review git diff, update relevant docs and submit a narrow PR.

Do **not** start a new event type, screen, class hierarchy or level solely because it would be interesting.

## 4. Simple manual memory practices

- Allocate dynamic arrays with `new[]`, track `count` and `capacity` separately, reallocate by creating a larger buffer, copying valid elements, and deleting the old buffer.
- For classes that own arrays or nodes, implement destructor and safe deep-copy behavior (or explicitly disable copying and use a dedicated deep clone).
- Avoid exposing writable internal arrays through public APIs.
- Use `NULL`/absent IDs consistently and initialize every new node field before linking.
- A mutation that fails validation must not leave half-modified structure state.
- When an AVL node moves in a rotation, keep its stable ID but update height, size and links.
- For snapshots, never copy just a raw pointer and then free either owner.
- Avoid new STL-dependent test frameworks; a simple custom assert/check runner is enough if approved.

## 5. How an event-driven action is implemented

~~~text
Click button/node
    -> UIManager creates PlayerAction
    -> GameController checks current level/rules
    -> Middleware snapshots accepted action's old state
    -> BackendAdapter invokes required backend function
    -> Backend returns native BackendEvent records and final state
    -> BackendAdapter/EventMapper converts them to GameEvent records
    -> Controller applies alarm/score/win/lose rules
    -> AnimationController replays actual event frames
    -> TreeRenderer or HeapRenderer draws each frame
~~~

The model may already be at the new state while the view is still animating the old-to-new transition. This is normal. Apply events to a **separate visual playback snapshot**, then verify it matches the final model. Keep model-changing input disabled during playback by default.

### Quick event smoke tests

- BST search visibly visits/compares exactly the nodes traversed.
- AVL LL/RR/LR/RL rotations visibly move original stable IDs; heights/sizes update.
- Heap swaps highlight and exchange matching indices in both tree and array, using a shared backend heap snapshot.
- Undo restores the exact previous model and visual state.
- Full-level replay starts from the original state and does not rescore.

## 6. The JSON level data approach

The handout requires **external JSON** and a reproducible `seed`. Define a minimal documented schema and keep the authoring guide updated. Required concepts include:

| JSON concept | Purpose |
|---|---|
| `act` and `level` | Identify exactly one of the 20 campaign levels. |
| `seed` | Deterministic generation for grading/replay. |
| `structure` | BinaryTree, BST, AVLTree or MaxHeap. |
| `initialValues` / `initialLinks` | Starting array or tree shape where applicable. |
| `objective` and required target values | The required mission's data. |
| `allowedActions` | Limit operations to those needed by that level. |

These are conceptual fields, **not a frozen schema**; finalize exact names with the owner before implementing a parser. Some levels need traversal orders or expected structure; encode only required facts. Do not add an `optimalSteps` field that overrides computation by the model.

Implement a focused JSON reader using allowed text/file operations without an STL JSON library. Invalid or missing files must produce clear feedback, not undefined behavior.

## 7. Feature-specific test matrix

| Component | Essential cases |
|---|---|
| BinaryTree | Empty/one/skewed/full, iterative vs recursive traversals, both reconstruction methods, heights, LCA, diameter, mirror. |
| BST | Reject duplicates; search found/missing; leaf/one-left/one-right/two-child/root deletion; min/max, successor/predecessor, range pruning. |
| AVL | All four rotations (LL/RR/LR/RL), deletion balancing, metadata after every operation, kth/rank/countInRange. |
| Heap | Empty/single/duplicates/reverse/already heap/random, sift up/down, extract max, complete heap sort including negatives. |
| Controller | Only allowed actions, correct alarm and star thresholds, hint cap, penalty, sequential unlock, autosave/load. |
| Animation | Step order, pause, speed, replay, undo, interrupted pending failure, no dangling visual references. |
| Heap rendering | Active heap vs sorted region remains synchronized under all swaps/extractions. |

For bigger manual GUI tests, target a 200-node tree/large heap with readable zoom/pan/fit at about 60 FPS.

## 8. Approved visual polish

Use one cohesive design language across all required screens:

- Bright, cute, bubbly palette (pink, coral/red, orange, yellow, restrained green).
- Static main-menu background; title at top; inviting buttons centered vertically.
- Rounded surfaces, soft shadows, friendly typography, ample whitespace.
- Minimal text: a one-line objective plus immediate feedback.
- Smooth time-based motion on visit, insert, delete, swap, rotation and heap sort.
- Consistent highlight meanings and extra labels/outline/icon indicators for accessibility.
- Optional light click/success/failure/rotation/heap swap sound effects; provide mute in Settings.

Polish **must never** hide an algorithm step, change scoring, add characters/inventory/modes, or depend on downloading assets at runtime.

## 9. Git / collaboration checklist

1. Pull `main` and create a feature branch named for the narrow change.
2. Read task, docs and existing interfaces.
3. Change only necessary files and keep build scripts in sync.
4. Run tests, inspect diff, commit. Use signed-off commits (`git commit -s`) if required by repo/course policy.
5. Open a PR with requirement, files, tests, and caveats. Have partner review it.
6. Merge when passing; update local `main` before the next slice.

For two contributors, do not split work so rigidly that only one person understands the frontend or backend. Both should be able to explain all requirements during the viva.

## 10. Final delivery checklist

- [ ] All 20 campaign levels completed/failed/retried during manual tests.
- [ ] Endless, Sandbox, leaderboard, Settings, save/load work.
- [ ] Required C++ algorithms from scratch; no STL, enums or globals.
- [ ] Both traversal-reconstruction types, AVL order statistics, full Heap Sort implemented.
- [ ] Events, alarm/score, undo/replay and synchronized heap views verified.
- [ ] JSON levels and short new-level guide exist.
- [ ] Unit tests, test report, GUI screenshots and all four AVL rotations captured.
- [ ] Benchmark results taken without GUI/event overhead.
- [ ] README includes compiler/SFML setup instructions and controls.
- [ ] 8–12 page report contains complexity, benchmarks, architecture and GUI/heap visuals.
- [ ] Final executable works on the intended Windows machine.
- [ ] ZIP follows the exact filename required by the handout; both contributors are ready for evaluation.
