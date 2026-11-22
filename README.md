# Vault Heist: The Living Vault

C++ data structures visualization game under development. Endless AVL survival
and Sandbox now have an SFML frontend backed by tested algorithms and middleware.
All twenty Binary Tree, BST, AVL and heap campaign levels (Acts 1–4) are playable from
Campaign on the main menu, with sequential unlocking, stars and checkpoints.
The release includes measured benchmarks, test results, real screenshots and a
twelve-page project report, revised on 22 November 2026. Remaining verification limits are listed in that report.
See [implementation status](docs/implementation-status.md) for the tested scope
and unresolved specification questions.

## Current verified environment

Use Ubuntu WSL with GCC 15.2.0 and C++17. The existing runner explicitly links
SFML 2.6.1 from `/usr/local/include` and `/usr/local/lib`. The system also has
SFML 3.0.2 registered with `pkg-config`; its flags must not be mixed with the
2.6.1 installation. SFML 2.6.1 is the owner-approved frontend target.
No additional packages are required for the backend tests.

Open this `Vault-Heist` directory through VS Code's WSL connection to use the
included Linux build and debug tasks. F5 additionally requires `/usr/bin/gdb`.

## Build and run

From the repository directory in Ubuntu WSL:

```bash
bash run.sh --build-only
bash run.sh
```

The script compiles all `src/*.cpp` files recursively with C++17, warnings and
debug symbols, using `include/` and the explicit SFML 2.6.1 paths. Output is
`build/vault-heist`. This opens the graphical main menu. Menu and Endless screen
smoke checks were run under WSLg and their actual captures are in
`build/menu-smoke.png` and `build/endless-smoke.png`.

## Controls and current modes

Campaign levels 1.1–4.5 have individual objectives and JSON definitions in
`data/levels/`. Complete a level to unlock the next, including the transition
between acts. Every level shows a briefing with its objective, controls and an
"Okay, start level" confirmation before gameplay. Retry and Resume also show
the briefing. Earned stars are loaded automatically on startup. Save stores
an active-level checkpoint; Resume Campaign restores its tree, answers, action
history, alarm and hint usage. Existing one-level progress files migrate to the
new twenty-slot progress format without losing the first level's stars.
New checkpoints also carry unlock progress, so an older progress file does not
prevent a valid checkpoint from resuming. Saves normally replace a temporary
file; when Windows/OneDrive denies replacement, a verified copy fallback keeps
the previous bytes in a `.bak` file for recovery.

Binary Tree: click traversal rooms; drag reconstruction tokens into the empty
slots (place a parent before its children); enter bottom-up heights and diameter;
select the requested endpoints, LCA and path; mirror bottom-up or use Auto.
Reconstruction has separate preorder/inorder and postorder/inorder rounds.

BST: choose Left/Right/Found at each comparison; insertion creates the new leaf
only after the correct path. Identify leaf/one-child/two-child deletion cases;
enter range bounds, watch pruning, then select exactly the returned rooms.
The final BST level demonstrates a sorted-insertion chain. Numeric answers use
the Input field and Submit. H gives a hint; Z undoes; F fits the tree.

AVL: identify invalid balance-factor badges; drag a subtree onto its pivot and
choose LL/RR; identify LR/RL and perform both primitive rotations separately;
predict the next insertion's rotation; follow subtree sizes to the k-th item,
then report inclusive rank and root size. Single/double repairs each have four
examples so every case appears twice. Wrong rotation choices add two alarm
steps and deduct 50 points. Partial repairs can be saved and resumed.

Heap Fortress: select active indices in either view, or type the index (two for
Swap). Build Heap constructs bottom-up; Heapify repairs downward and Sift Up
repairs upward. Extract Maximum retains the extracted value in the labelled
sorted suffix. Restore the active heap before extracting again. Sorted cells
are locked. The final door opens only after the new array is sorted ascending.
Undo, Replay, Reset, Hint and checkpoint/resume are supported throughout.

Endless starts with the specified seven-node AVL. Choose NONE, LL, RR, LR or RL
to predict the first rotation for the displayed INSERT/DELETE request. The
deadline is fixed per request; correct early answers wait for the next arrival.
Wrong answers and timeouts end the run and save one local leaderboard result.
Esc pauses, with Resume, Save and Exit, and Exit Run choices. Resume Endless
loads a saved run paused. Focus loss also pauses and requires Resume.

Sandbox: select a structure, type integer input, choose an operation with the
operation selector, then Run. Generate loads an example. Queries show their
result below the controls. BinaryTree reconstruction/equality accepts
`preorder / inorder` (or `postorder / inorder` for BUILD_POST_IN). Child insertion
uses `value unused parentId`; ADD_RIGHT chooses the right slot. General-tree
PATH/LCA use node IDs; ordered-tree LCA uses keys. Range operations take two
values; heap swap takes two zero-based indices. Hover nodes to see IDs/metadata.
Undo restores deep state, Replay reproduces accepted successful actions, and
Reset returns to the selected structure's initial empty state.

Space pauses/resumes event playback; Left/Right step through it; Z undoes in
Sandbox. Mouse wheel zooms trees; middle-button drag pans; F resets the view.
Settings controls playback speed and mute. No audio assets are required or
currently loaded. Endless gameplay time excludes animation and paused time.

## Backend tests

```bash
bash tests/run.sh
SANITIZE=1 ASAN_OPTIONS=detect_leaks=1 bash tests/run.sh
SANITIZE=1 ASAN_OPTIONS=detect_leaks=1 bash tests/run.sh --campaign-only
```

The focused campaign option checks all twenty levels without rerunning the broader
stress suites: solve, fail/retry, optimal score, replay, sequential unlocks,
progress reload, checkpoint/undo and hint caps. The default runner also checks
400 paired deterministic Endless rounds, all five prediction cases, deadlines,
pause/focus behavior, leaderboard ordering/top-10 persistence, exact save/resume,
invalid JSON/tree saves, Sandbox undo/replay and heap restoration. Test artifacts
use fresh directories under `build/endless-tests.*`; player saves are untouched.

For the existing graphics smoke test, explicitly select the verified version:

```bash
mkdir -p build
g++ -std=c++17 -I/usr/local/include tests/test.cpp -o build/sfml-test \
    -L/usr/local/lib -Wl,-rpath,/usr/local/lib \
    -lsfml-graphics -lsfml-window -lsfml-system
DISPLAY=:0 ./build/sfml-test --auto-close
```

## Specifications and structure

Read [AGENTS.md](AGENTS.md), [PRD](docs/prd.md),
[architecture](docs/architecture.md), [event contract](docs/event-contract.md)
and [development guide](docs/development-guide.md) before coding. Endless behavior
is defined by [the owner-approved rules](docs/endless-mode-rules.md).

`include/backend` and `src/backend` contain graphics-independent algorithms.
Middleware coordinates game rules and events; frontend handles SFML drawing
and input. `data/levels` contains all twenty campaign definitions. The UI
uses an available local DejaVu, Liberation or Windows Arial font; it does not
download assets at runtime. Player saves/leaderboard are in `data/saves/`.
The supplied official assignment PDF was read separately; it is not bundled.

## Delivery evidence

- [Project report (12-page PDF)](docs/i253017-i252125.pdf)
- [Printable report source](docs/project-report.html)
- [Executed checks and limitations](docs/test-results.txt)
- [Level authoring notes](docs/level-authoring.txt)
- [Tree benchmark CSV](docs/benchmarks/trees.csv) and [heap benchmark CSV](docs/benchmarks/heaps.csv)
- Actual captures are under `docs/captures/`.

Reproduce the model-only measurements with `bash tests/benchmark.sh`. They use
deterministic data, events disabled, -O2 and independent output checks. Timings
are one local CPU-clock run and may vary; assertions must remain enabled.
The backend flag is `setEventRecording(false)`; the benchmark asserts that event
batches remain empty while independent comparison/rotation/swap counters run.

The report includes all seven Section 10 experiments: BST/AVL heights; 10,000
searches with average comparisons and timing; average AVL rotations per insert
and delete and maximum per delete; 1,000 kth queries; bottom-up heap build;
Heap Sort on random/sorted/reverse/duplicate-heavy inputs; and comparison with
manual Insertion Sort, including time, comparisons, swaps/shifts and auxiliary
space. All sizes are 100, 1,000 and 10,000. CSVs retain the unrounded measurements.

## What to include in the submission

Submit the project folder with these contents, retaining its directory layout:

| Path | Purpose |
| --- | --- |
| `src/` | All implementation `.cpp` files, including `main.cpp` |
| `include/` | All project `.h` headers needed to compile the implementation |
| `tests/` | Unit/controller test `.cpp` files, runners and benchmark source |
| `data/levels/` | All twenty external level JSON definitions |
| `assets/` | Any supplied runtime fonts/images/audio and their notices |
| `run.sh`, `.vscode/` | Build/run script and development configurations |
| `docs/i253017-i252125.pdf` | Written report with complexity, results and screenshots |
| `docs/test-results.txt`, `docs/test-run-2026-11-22.txt` | Test report and actual latest run output |
| `docs/level-authoring.txt` | Level schema, editing example and verification instructions |
| `docs/benchmarks/`, `docs/captures/` | Raw measured results and actual screenshot evidence |
| `README.md`, remaining `docs/` | Setup, controls, specifications and report source |

"All source files" includes both implementation `.cpp` files and their `.h`
headers, including the test and benchmark source. An executable alone is not
source code. Include the Linux executable `build/vault-heist` too if the handout
asks for a runnable build; the review packager includes it. Do not include `.git`,
temporary build/test directories, editor caches or personal save/leaderboard files.

"Unit test suite ... along with a test report" means the runnable automated
checks **and** a document recording what was tested, the commands, actual outcomes
and limitations. This is supplied by `tests/backend/`, `tests/middleware/`,
`tests/run.sh` and the two test-report files above. Performance benchmarks are a
separate deliverable and do not replace correctness tests.

"Level data files and a short guide" means the JSON scenarios **and** instructions
explaining their fields and how to edit, load and validate a level. See
`data/levels/` and `docs/level-authoring.txt`. The current implementation supports
replacing scenarios within twenty fixed campaign slots. Extra slots/new objective
types require code changes; it does not yet support arbitrary no-code expansion.

The executable targets Ubuntu WSL/WSLg with the installed SFML 2.6.1 libraries.
A native Windows .exe has not been produced. Exhaustive manual GUI/performance
acceptance and optional audio testing remain unverified. Some nonmutating
Sandbox queries report results without full per-node animation. Campaign JSON
uses fixed reproducible arrays; its seed field does not randomize those arrays.

`tests/package-review.ps1` creates a fresh review ZIP with source, executable,
levels, report and tests, excluding player saves and Git history. Fill in the
actual contributor details and rename the ZIP to the course's required
`25I-XXXX_25I-XXXX_Section.zip` pattern before official submission.
