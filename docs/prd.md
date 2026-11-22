# Vault Heist: The Living Vault — Product Requirements Document
Status: Approved planning baseline (17 November 2026)
Source of truth: Course handout, Project Deliverable 1; decisions recorded with the project owner.
Delivery deadline: 20 November 2026, 11:59 PM (course submission).
Target platform: Windows desktop through Ubuntu WSL/WSLg; C++17 and SFML 2.6.1; developed in VS Code. SFML 2.6.1 is owner-approved and verified locally.
1. Product and experience
Vault Heist is a single-player puzzle-strategy game about navigating and manipulating a vault represented by data structures. The player solves short challenges by clicking rooms, choosing operations, and watching clear step-by-step animations. The emphasis is on doing the algorithms, not on long explanatory lessons.

Approved principles
- Deliver every mandatory requirement in the handout. Simplify implementation only where the handout permits.
- Campaign levels unlock sequentially; Sandbox is available from the main menu at all times.
- Implement and test backend, then middleware, before full frontend implementation. A minimal UI smoke test may happen earlier.
- Show objectives, immediate action feedback, alarms, and results. Keep instructional text short.
- Use a cheerful, bubbly, original interface, inspired by the energy of playful party games but not copied from any existing title.
- Make animations lively and readable; short sound effects are authorized as optional presentation polish. Neither may change game rules.
2. Scope and priorities
Required product scope
1. Four acts with five levels each (20 levels total).
2. General Binary Tree, BST, AVL Tree (including subtree sizes and order statistics), Max Heap, and Heap Sort, implemented from scratch.
3. The complete required operations, validators, event-driven animations, undo, replay, reset, hints, scoring, and alarm behavior.
4. Campaign map, level briefing, gameplay, result screen, Sandbox, Endless Mode, local leaderboard, Settings, and save/load.
5. Heap tree and array representations synchronized during every heap operation.
6. External JSON level data with reproducible seeds; new levels must be addable without code changes.
7. Backend-only unit tests, manual GUI tests, benchmark experiments, and all course submission documents.
Permitted simplifications
- Snapshot-based undo instead of inverse-event undo.
- Minimal but usable Settings and local leaderboard screens.
- Short text hints and feedback instead of tutorials.
- Minimal sound set: click, successful action, error/alarm, rotation/swap, victory/failure. Sounds can be muted or absent without changing behavior.
- Simple but polished shapes, text, and easing; no extra animation framework or external game engine.
Explicitly out of scope
- Multiplayer, online accounts, cloud saves, network APIs, paid assets, extra acts, extra puzzle types, achievements beyond the handout's final-vault badge, and AI-generated gameplay.
- Additional rules, shortcuts, scores, penalties, or operations invented by a coding agent.
3. Core game rules
Alarm and scoring
- Visiting a room and comparing a key are charged events as defined by the handout: +1 alarm step per chargeable visit or comparison. Do not silently count or waive events.
- Each level's alarm capacity is ceil(1.5 * optimalSteps) + 2. The model computes optimalSteps; it is not hard-coded in JSON.
- When the meter reaches its limit, finish the current operation's active animation sequence, then show failure. Do not cut the animation mid-motion or accept another player operation.
- Wrong traversal click: +1 alarm. Wrong rotation choice: +2 alarm and −50 score points. Hint: stars capped at two. Incorrect heap comparison or invalid swap: +1 alarm.
- Raw score uses the handout's max(0, 1000 - 10 * alarmSteps - penalties + bonuses) formula. Any bonus must be explicitly defined by course rules or approved level data; do not invent bonus categories.
- Three stars: at/below optimal steps and no penalties. Two stars: at/below 1.25× optimal and at most one penalty. One star: complete within capacity. Zero: fail.
- On invalid action, show one short reason and apply only the required penalty.
Progress and controls
- Completing the previous level unlocks the next; unfinished levels remain locked. Sandbox never locks.
- Autosave campaign completion, stars and unlocks after each completed level. Include manual load/resume support.
- A replay action resets to the initial level state and reproduces the recorded action/event sequence. A separate per-operation playback control permits stepping/scrubbing event logs as required.
- Undo restores the snapshot taken immediately before the previous accepted player action.
- Hint gives one concise instruction. Hints do not complete the action for the player.
- Mouse interaction is primarily clicks and buttons. Limited dragging is required for reconstruction/rotation tasks; the input system must also support zoom/pan and the required keys.
4. Campaign requirements
Level	Objective and interaction	Acceptance condition
1.1 Patrol route	Click nodes in preorder, inorder, postorder, or level order	Click order matches requested traversal.
1.2 Blueprint reconstruction	Place room tokens using preorder+inorder or postorder+inorder	Reconstructed structure matches hidden tree.
1.3 Alarm spread	Supply node heights bottom-up and tree diameter	Heights and diameter are correct.
1.4 Meeting point	Select two nodes, their LCA, and trace path	Correct LCA and path length.
1.5 Mirror vault	Mirror using child swaps, or permitted Auto	Result equals mirror of starting tree.
2.1 Search heist	Choose left/right at each BST comparison	Reach the target using BST ordering.
2.2 Stash loot	Choose insertion path	New value appears at valid leaf; duplicates rejected visibly.
2.3 Steal and collapse	Identify deletion case; watch successor/value-swap where applicable	BST remains valid after leaf/one-child/two-child deletion.
2.4 Range raid	Query [lo, hi], visualize pruned subtrees	Return exactly values in range using pruning.
2.5 Lazy Architect boss	Search a sorted-insertion, degenerate BST under alarm pressure	Required search objective met; balancing need shown in short transition.
3.1 Read badges	Select nodes with invalid balance factors	All unbalanced nodes selected.
3.2 Manual single repair	Select LL/RR pivot and appropriate rotation	Tree repaired with correct single rotation.
3.3 Manual double repair	Identify LR/RL case, execute both rotations	Correct balanced result.
3.4 Auto repair prediction	Predict rotation before insertion	Prediction checked; AVL automatically rebalances with events.
3.5 Order-statistic boss	Retrieve k-th item and validate subtree sizes/ranks	Correct answer via size augmentation, with invariant checks.
4.1 Activate vault	Perform swaps/build max heap	Every parent is >= existing children.
4.2 Repair locks	Fix disturbed heap with sift-up/sift-down	Heap property restored.
4.3 Extract greatest	Extract maximum repeatedly	Correct maxima extracted; active heap remains valid.
4.4 Race guards	Complete Heap Sort steps under limit	Correct sequence within alarm/operation constraints.
4.5 Final vault boss	Independently sort new array	Ascending sorted array; final vault unlocks.


Non-campaign modes
- Sandbox: accessible immediately. Select General Tree, BST, AVL, or Max Heap; enter values manually, generate a valid example, and invoke all required operations for the chosen structure. Show animations, results, and validation feedback.
- Endless: implement the owner-approved rules in `docs/endless-mode-rules.md`, including rotation prediction, deterministic insert/delete requests, active-time deadlines, pause behavior, survival scoring, local leaderboard and save/resume.
5. Required data structures and algorithms
BinaryTree
Recursive and iterative preorder/inorder/postorder/level-order; reconstruction from preorder+inorder and postorder+inorder; height, diameter, LCA, root-to-node path, mirror, same-tree and symmetry checks.
BST
Insert (reject duplicates), search, delete all three cases, min/max, successor/predecessor, range with pruning, k-th smallest by inorder scan, BST LCA, degeneration demonstration.
AVLTree
Insert, delete, left/right primitive rotations, rebalance LL/RR/LR/RL, height and balance-factor maintenance, subtree size maintenance, kth, rank, range count, balanced validator.
MaxHeap
Zero-based manually allocated array, bottom-up build, sift-up, sift-down/heapify, insert, extract-max, heap sort, and invariant validation. Keep active heap and sorted suffix distinct.
Invariants
- Leaf height is 1; empty subtree height is 0 everywhere.
- Stable unique node IDs for drawings and event tracking; ID is independent of value.
- BST left keys < parent < right; no duplicates.
- AVL balanceFactor = height(left) - height(right), absolute value <= 1, stored heights/sizes accurate.
- Parent links consistent if used; no cycles/shared nodes.
- Heap index rules: parent (i - 1) / 2, left 2*i + 1, right 2*i + 2.
- Validators run after each operation in debug mode and within unit tests.
- Follow the named APIs in the handout wherever legal in C++; see docs/architecture.md for reserved-word and ambiguous-overload exceptions.
6. Functional screens
Screen	Required content
Main Menu	Campaign, Endless, Sandbox, Leaderboard, Settings, Quit.
Campaign Map	Four act groups, sequential unlock states, level nodes, earned stars.
Briefing	Act/level name, short objective, start/back.
Level Play	Tree/heap canvas; alarm, score, objective and step counters; operation toolbar; event log; Undo, Replay, Reset, Hint, speed control.
Result	Success/failure, stars, score, steps vs optimal, penalties, Retry/Next.
Sandbox	Data-structure selector, manual input/operations, example generator, event playback, reset.
Endless	Valid AVL operation stream, alarm/survival feedback, local result.
Leaderboard	Locally saved Endless results.
Settings	Essential controls including sound on/off and animation speed.


Additional GUI behavior from the handout: hover tooltips with key/height/balance/size/depth, drag tokens where required, zoom/pan/fit, Space play/pause, Left/Right step, Z undo, H hint, Esc menu. Color is never the only status indicator.
7. Visual and audio direction
- Reference frame: 1280 × 720. Make the layout legible at this size; resize gracefully where practical.
- Main menu: title along the upper portion, centered vertical button group, static background.
- Personality: bright, bubbly, inviting, cute and coherent. Palette centered on bubblegum pink, coral/red, warm orange, soft yellow, and restrained green. Use color deliberately, not as visual noise.
- Components: rounded buttons with soft depth/shadows, readable labels, clear focus/hover/press states, friendly typography, distinct but consistent panel geometry.
- Gameplay: tree edges remain visually obvious; node positions are stable and labels legible; error, selected, visited, skipped, balanced, target and sorted states each have an additional label/outline/icon cue.
- Animations: soft easing, node pulses on visit, subtle comparison direction, insertion scale-in, unlink fade, swap motion, AVL rotation position tweens, badge changes, matching heap-tree/array swaps, brief win/fail emphasis. Every event remains understandable at normal and slower speed.
- Sound: optional light UI and action cues. Missing audio assets or unavailable SFML Audio must not break gameplay. Sounds must be short, non-intrusive, and switchable in Settings.
- Avoid standard AI-generated dashboard layouts; visual identity should be custom without adding new features.
8. Technical and implementation constraints (non-negotiable)
- All game logic in simple C++; rendering/input in SFML; editor is VS Code on Windows with GCC/MinGW-w64.
- No STL containers/algorithms/adapters anywhere in authored project code: no std::vector, std::list, std::map, std::stack, std::queue, std::priority_queue, or std::sort. std::string is explicitly permitted for text.
- No global variables. No enums (including enum class). Event/operation kinds use strings and/or appropriately scoped class members.
- Standard headers such as <cmath> and <cstdlib> are permitted where relevant. SFML utility and rendering types are allowed for GUI purposes.
- Implement dynamic arrays manually using new[], delete[], capacities, and correct deep copies. Do not shallow-copy raw owning pointers.
- Separate header and source files for major classes. Keep braces on a new line even for functions and control statements. Prefer direct loops, conditionals, descriptive identifiers and straightforward helper functions.
- User-supplied sample code guides naming, indentation and structure; bugs in samples are not conventions.
- Backends must not include SFML or depend on middleware/frontend. Middleware must not draw. Frontend must not implement data-structure algorithms.
- Avoid additional dependencies, files, classes, screens or features unless a requirement or the integration contracts clearly calls for them. Ask the owner before any scope expansion.
9. Events, snapshots and persistence
- Backend operations produce typed-by-string events capturing visits, comparisons, inserts/unlinks, value swaps, rotations, height/size updates, heapify, heap swaps, extraction and completion.
- Middleware exposes a stable GameController entry point, structured operation results, TreeSnapshot/HeapSnapshot views, GameState and animation data. The GUI never receives owning backend node pointers.
- Use manually managed arrays for event batches, visual nodes/edges, history and heap cells. Maintain per-event visual states or sufficiently precise reversible deltas, not only a final snapshot.
- Capture a deep pre-action state for Undo; preserve the starting level state and actions/events for full-level replay.
- Levels are external JSON with seeds. Load with a narrowly scoped parser that does not violate the no-STL rule; reject invalid files with clear errors. Save trees via preorder+null markers and heaps via array+active heap size. Recompute AVL sizes/heights on load and validate.
- The authoritative event contract is docs/event-contract.md.
10. Testing, performance and delivery
Automated model tests
- Tree traversal: empty, one-node, skewed, full; recursive vs iterative.
- Reconstruction round-trip; diameter, LCA, mirror and height conventions.
- BST: duplicate rejection; each deletion case including root/missing key; range/min/max/successor/predecessor.
- AVL: LL/RR/LR/RL; deletion rebalances; size/rank/kth agreement with manually established expected order.
- Heap: empty, singleton, duplicates, random, reverse, pre-heaped; valid after build, insert, heapify, extract; heap sort cases including negatives.
- Middleware: event count/order, stable IDs, deep-copy undo, replay, alarm limits, error/penalty behavior, synchronized heap view.
GUI acceptance
- Complete, fail and retry each of the 20 levels.
- Animations play one operation step at a time without freezing input; paused/speed/step/replay behave correctly.
- Undo never leaves obsolete nodes/array cells; tree and heap array stay synchronized.
- Zoom/pan/fit on a 200-node tree or large heap targets 60 FPS in the specified environment.
- Readability and status cues work without relying on colors alone; silence causes no crashes.
Benchmarks and report
- No GUI and event recording disabled during timing.
- Compare BST/AVL heights for 100, 1,000, 10,000 nodes; 10,000 searches; AVL rotations; 1,000 order-statistic queries.
- Measure heap build and Heap Sort at 100/1,000/10,000 under random, sorted, reverse and duplicate-heavy inputs; include comparisons/swaps and a course-relevant sorting algorithm comparison.
- Deliver complete source, executable, README with run/controls, tests and results, JSON levels and level authoring guide, and an 8–12 page report with architecture, complexity, benchmark charts, GUI captures, all four AVL rotations and heap-operation screenshots.
- Submission is one ZIP named in the handout's prescribed student-ID/section pattern; both members must understand the full code and attend evaluation.
11. Completion criteria and change control
A requirement is complete only when its implementation builds, the relevant test passes, the correct events/snapshot are visible in SFML, and its defined win/loss or output can be demonstrated. Every merged slice must preserve these constraints.

Priority when instructions disagree: (1) official handout and later instructor clarifications; (2) explicitly approved decisions in this PRD; (3) architecture/event contracts; (4) agent suggestions. The owner must approve deviations, especially new features, altered rules, or STL/global/enum usage.

Related files: docs/architecture.md, docs/event-contract.md, docs/development-guide.md, AGENTS.md.
