# Implementation status — 20 November 2026

The complete game is not finished. SFML 2.6.1 is the approved and tested GUI
target. The previous Endless specification blocker is resolved by the supplied
owner-approved rules, now stored fully in docs/endless-mode-rules.md.

## Implemented and tested

- BinaryTree, BST, AVL and MaxHeap algorithm foundation, manually owned arrays,
  stable node IDs, real backend events and deep frames.
- Middleware BackendAdapter, event mapping, snapshots and frame-based animation
  with play/pause, speed and retained previous/next frames.
- Endless state machine: fixed initial tree, deterministic valid mixed requests,
  five first-rotation predictions, exact 8/7/6/5/4/3-second arrival schedule,
  timeout precedence, active-time clock and focus/menu/animation pauses.
- Wrong predictions never mutate the live AVL. Successful operations validate
  ordering, balance, heights, sizes and links. Technical errors do not score.
- Stable local top-10 leaderboard with operations as secondary tie breaker;
  failure writes once; exits/resumes/previews do not add scores.
- Versioned JSON save/resume at operation boundaries. Trees use preorder IDs
  and keys with null markers; next-ID allocation, shape and metadata survive
  reconstruction. PRNG state, counters, request and remaining deadline persist.
  Resume starts paused; failed-run checkpoints are invalidated.
- Sandbox operation selector: tree/BST/AVL/heap operations, example generation,
  traversal/reconstruction, queries, deep undo, reset and deterministic replay.
  Required query-event coverage and campaign-specific controls remain pending.
- SFML main menu, Endless gameplay/pause/result, Sandbox, leaderboard and
  settings; rounded buttons, hover metadata, tree zoom/pan, event pulses,
  tree insertion/rotation/value-swap/deletion presentation, matching heap
  swap movement in tree and array. Sound is optional and has no loaded assets.
- VS Code WSL build/test/debug configuration and LF handling.

## Verification actually performed

The backend and middleware suites passed under GCC 15.2.0 with
-Wall -Wextra -Wpedantic -Werror, AddressSanitizer, UndefinedBehaviorSanitizer and
leak detection. The commands are:

    SANITIZE=1 ASAN_OPTIONS=detect_leaks=1 bash tests/run.sh
    bash run.sh --build-only
    DISPLAY=:0 ./build/vault-heist --smoke-test
    DISPLAY=:0 ./build/vault-heist --smoke-endless

The tests include tree edge cases, invalid reconstruction, all BST deletion
cases, all four AVL rotations, 2,000 AVL mutations checked against an expected
set, heap extraction/sorting with duplicates/negatives, deep ownership and
stable IDs. Middleware tests include 400 paired fixed-seed Endless rounds, all
five prediction cases, the node limit, deadline/early-answer behavior, schedule
boundaries, time freezes, leaderboard persistence/order/top-10 capacity, save
boundaries, paused exact resume, malformed JSON/tree rejection, playback
scrubbing and Sandbox undo/replay/heap restoration.

The integrated menu and Endless windows launched and closed normally under
WSLg. Their actual screenshots were captured and visually inspected:
build/menu-smoke.png and build/endless-smoke.png. This is not evidence that every
mouse/keyboard path or campaign level has been manually tested.

## Deterministic generator contract

Each draw advances a per-run uint32 LCG with a = 1664525 and c = 1013904223.
Bounded draws use rejection sampling to avoid modulo bias. Every request first
draws an operation roll in [0,99], even at forced size boundaries, then draws
either an insertion candidate in [1,999] or a deletion index into a manually
allocated sorted array of current keys. Insertion collisions scan upward,
wrapping to 1, without extra draws. Previewing a request runs the real backend
on a deep copy and does not draw random numbers or change live state.

## Remaining work

All twenty Binary Tree, BST, AVL and heap levels (1.1–4.5) now have external JSON definitions,
distinct objective handlers and playable screens. Reconstruction includes token
dragging and two traversal-pair rounds; BST insertion is guided without charging
duplicate comparisons. Deletion covers all three cases; range query preserves
actual pruning events. The corridor boss uses a fitted tree view.

Campaign scoring, delayed results, stars, sequential unlocking, saved progress,
hints, undo/replay/reset, and validated active-level checkpoint/resume are
implemented for all twenty levels. A focused sanitizer suite solved, failed and
retried each level; checked model-computed optimal scores, replay without
recharging, progress reload, checkpoint restore/undo, and the hint star cap.

AVL 3.1 identifies all unbalanced rooms; 3.2 covers LL/RR twice each; 3.3 requires
case identification and two separately performed primitive LR/RL rotations,
twice each; 3.4 predicts four real insertion repairs; 3.5 uses an augmented kth
path, traced inclusive rank and subtree-size validation. Wrong rotation choices
cost exactly +2 alarm and 50 points. Checkpoints restore partial double repairs.

All campaign entries, retries and resumes now show a wrapped objective/control
briefing and wait for "Okay, start level". AVL views show balance/size badges and
selected-room outlines. The focused campaign suite passed with sanitizers,
including mid-rotation resume, optimal scores and saved unlocking.

Heap 4.1 constructs a Max Heap, 4.2 exercises downward and upward repair,
4.3 retains extracted maxima in the sorted suffix, 4.4 runs the extraction/
heapify race and 4.5 completes the new-array sorting workflow and final-vault
badge. Selection works in both views; sorted cells are locked. Checkpoints
preserve active size and all array values, including disturbed intermediate
states. Final extraction emits HEAP_SORT_COMPLETE exactly once.

The full backend, Endless/Sandbox and twenty-level campaign suites passed with
address/undefined-behavior sanitizers and leak detection. Model-only benchmarks
were actually measured at 100/1,000/10,000 elements with events disabled. The
twelve-page report (revised 22 November 2026) and raw CSVs are under docs/. Actual heap and four-rotation
captures are under docs/captures. See docs/test-results.txt for the verified
scope; diagnostic GUI captures use temporary save directories.

Remaining verification/limitations: exhaustive manual GUI paths and 200-node
60-FPS acceptance; native Windows deployment; optional sound assets; full
per-node playback for some nonmutating Sandbox queries. Campaign fixtures are
explicit reproducible arrays rather than runtime seed-driven randomization.
The review archive needs the real student IDs/section and contribution details
before being used as an official course submission.

The supplied 12-page assignment PDF has been read completely. It requires
inclusive AVL rank (keys smaller than or equal to the argument); implementation
and tests were corrected accordingly. No specification blocker is being claimed
for the remaining campaign implementation.

No new STL containers/algorithms, global variables, enums, game modes or
unapproved gameplay rules were introduced. No remote Git changes were made.
