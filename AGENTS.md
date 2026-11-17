# AGENTS.md — Mandatory Instructions for Vault Heist Coding Agents
READ THIS ENTIRE FILE BEFORE TOUCHING CODE. Applies to OpenCode, Codex, Copilot, other IDE agents, and humans making code changes.
0. NON-NEGOTIABLE DIRECTIVE
YOU MUST FOLLOW THE COURSE CONSTRAINTS AND THE APPROVED PRD.

YOU MUST NOT INVENT, ADD, OR IMPLEMENT FUNCTIONALITY THAT IS NOT REQUIRED BY THE COURSE HANDOUT OR EXPLICITLY APPROVED BY THE PROJECT OWNER.

Do not assume a feature is desirable because a framework, tutorial, or other project usually includes it. Do not silently change game rules, scoring, levels, APIs or the repository architecture. If something is unclear or genuinely impossible under the constraints, stop and ask before changing the specification.

Read, in order:
1. docs/prd.md — required behavior and approved design.
2. docs/architecture.md — layer responsibilities, memory, filenames.
3. docs/event-contract.md — producer/consumer interface and playback rules.
4. docs/development-guide.md — implementation sequence and acceptance criteria.
5. The actual headers and source files relevant to the assigned task.
The official assignment PDF and instructor clarifications outrank all documents here. When conflicts occur, flag them; do not choose your own replacement.
1. Absolute C++ restrictions
- No STL containers, algorithms, adapters or convenience collections, including std::vector, std::list, std::map, std::set, std::stack, std::queue, std::priority_queue, std::array, std::sort, etc. std::string is explicitly allowed for text.
- No global variables. No enums. Do not use enum or enum class even for game states/events. Use std::string identifiers and fields or narrowly scoped class members.
- Implement all required trees, BST, AVL and heap algorithms from scratch; no ready-made tree/heap containers or disguised equivalents.
- Dynamic collections use manual arrays, new[]/`delete[]`, length and capacity, or explicitly owned node pointers. Correct copying, destruction and bounds are mandatory.
- Standard C++ headers such as <cmath> and <cstdlib> are allowed where appropriate. Do not import an external JSON/data-structure library without explicit approval.
- SFML types are permitted for GUI, input, drawing, animation and sound only. Never include SFML in backend headers/sources.
- Height convention is leaf = 1, empty = 0, consistently in code, events, snapshots, validators and tests.
- Node IDs are stable and not derived from their values. Maintain parent links and AVL heights/subtree sizes correctly if applicable.
- Required core algorithms must work and be testable without launching SFML.
- No unapproved platform rewrite, alternate framework, build system, network dependency or third-party game engine.
Scope of the no-STL rule: authored project source and test code. Internal implementation details of the SFML library and the C++ compiler are not part of your project code. If an SFML API forces an STL type into authored source, identify it and seek owner/instructor approval instead of silently introducing a prohibited type.
2. REQUIRED CODING STYLE
Match the project owner's sample C++ code while preserving correctness:
- Simple beginner-readable C++: explicit if/else, for, while, direct recursion and helpers.
- Separate .h and .cpp files for every major class. Use header guards.
- Opening braces on the next line for classes, structs, functions, loops and conditionals.
- Four-space indentation. Braces even for single-line control branches when writing new code.
- Descriptive short identifiers. Follow the existing style and published API, such as preOrder, left_child, removehighest, or the exact mandated operation names.
- Keep operations transparent for a viva: no metaprogramming, excessive templates, unexplained macros, clever one-liners or overengineered inheritance.
- Prefer directly understandable algorithms and comments explaining why a step is needed, not commentary after every line.
- Avoid global using namespace std in headers. In source files, follow existing conventions if appropriate.
- Use safe NULL/pointer checks and initialize all fields; do not replicate memory errors or bugs found in the owner's learning examples.
- Fix actual correctness bugs, even if a submitted style example contains them. Explain any deliberate departure from style.
- Preserve the exact handout API names when legal. For impossible C++ signatures/keywords, use the documented minimal adaptation (see architecture).
Example:
void MaxHeap::siftDown(int i)
{
    while (true)
    {
        int left_child = 2 * i + 1;
        int right_child = 2 * i + 2;
        int highest = i;

        if (left_child < heapSize && arr[left_child] > arr[highest])
        {
            highest = left_child;
        }

        if (right_child < heapSize && arr[right_child] > arr[highest])
        {
            highest = right_child;
        }

        if (highest == i)
        {
            break;
        }

        // Perform the required swap and record the actual event.
        i = highest;
    }
}
This snippet illustrates formatting/logic flow; it is not a complete algorithm implementation.
3. Layer boundaries — never break them
Area	You may implement	You must NOT implement there
include/backend, src/backend	Required data structures/algorithms, invariants, event emission during operations	SFML graphics, input handling, menus, scores, level UI
include/middleware, src/middleware	GameController, adapters, level rules, score/alarm, events, snapshots, undo/replay, saves	C++ tree/heap algorithm duplication, SFML drawing
include/frontend, src/frontend, src/main.cpp	SFML screens, input, layout, animations, HUD, sound polish	Reimplement BST/AVL/heap logic, edit backend nodes directly, invent game rules
tests	Focused tests of required algorithms/contracts	STL test helpers or silently weakened assertions
data/levels	Required JSON level definitions and seeds	New modes, extra levels, arbitrary scoring bonuses


All frontend operations go through GameController. Frontend renderers consume snapshots/events and never hold owning backend pointers. The backend emits its own dependency-free BackendEvent records from include/backend/OperationEvent.h; EventMapper.cpp converts them to middleware GameEvent. Never add a middleware include to backend code.
4. Exact feature scope
Implement the 4 required acts, 20 campaign levels, Sandbox, Endless, leaderboard, Settings, all mandated operations, validators, scoring, alarm, hints, animations, undo/replay, save/load, tests and benchmarks. The owner explicitly approved:
- Automatic, lively step-by-step animations that stay faithful to emitted events.
- A bright, bubbly, coherent original visual style: pink, coral/red, orange, yellow, restrained green; 1280×720 target; upper title; centered menu buttons; static background; rounded buttons and soft shadows.
- Small optional sound effects for existing interactions/animations, with a mute toggle. Sounds may not add mechanics or alter scoring.
Do not add multiplayer, online leaderboards, new modes, cutscenes beyond the handout, bonus systems, extra levels, inventory, cosmetics shops, settings unrelated to existing needs, or any other invented functionality. Do not import an unrelated UI framework because the current graphics look plain.

Follow the GUI behavior specified by the handout (hover badges, minimal instructions, keyboard shortcuts, necessary drag gestures, zoom/pan/fit) even though ordinary gameplay is mainly click/button based.
5. Before starting any coding task
1. Read the four documents listed above and the assigned task/slice.
2. Inspect the real repository and existing interfaces. Never assume placeholder files contain valid implementations.
3. State the exact required feature, affected files, expected inputs/outputs and test cases.
4. Confirm dependencies; if work requires changing a shared interface, ask the owner first and update the contract along with producer/consumer tests.
5. Keep changes confined to the task. Do not rewrite another person's file without reason and approval.
6. Preserve uncommitted work. Never reset/delete unrelated changes, never force-push, never edit protected main directly.
When there is a conflict between "better design" and the approved scope, approved scope wins. Stop and ask instead of improvising.
6. While implementing
- Implement one small, testable slice at a time.
- Keep backend and middleware independently compilable.
- For operations, record the actual comparison/visit/rotation/swap sequence as backend-native events. Let the adapter map these to middleware events; never fabricate animation by re-running logic in the frontend.
- Use deep copies for undo, serialization and event/visual buffers. Validate restored state.
- Avoid blocking sleeps in the event loop; animate with per-frame elapsed time. A paused animation must not block the window.
- Preserve stable node IDs through AVL rotations and value swaps.
- For heap events, update array and tree visuals from one authoritative heap snapshot.
- Use external JSON level data, not hard-coded levels.
- Add the new .cpp to the VS Code build task if necessary; avoid introducing game CMake files unless the owner explicitly requests them.
- Audio effects must be optional, quietly fail if unavailable, and never block gameplay.
7. Test and report before calling a task complete
Provide:
1. Exact file list changed.
2. Which specific assignment requirement each change satisfies.
3. Commands actually run, build/test results, and errors; never claim success without running the check.
4. Memory/edge cases checked (empty input, duplicates, deletion, rotation, undo, heap active size, etc.).
5. Any pending integration requirement or known limitation.
6. Confirmation that no STL containers, no enums, no global variables and no invented features were added.
If the environment has no compiler, SFML, tests or assets, say "not verified" and explain how the owner can verify it. Do not fabricate screenshots, benchmark numbers or passing tests.
8. Git collaboration rules
- Work in a feature branch, create a small PR, and request partner review before merging to main.
- Keep meaningful commit messages and avoid unrelated formatting changes.
- If repository/course rules require signed-off commits, use git commit -s; never invent another author's sign-off.
- Update docs and tests when interfaces change.
- Do not delete branches, rewrite history, change branch rules, or push to the owner's remote without explicit instruction.
9. Stop and ask (do not guess)
Stop when:
- A requested change would violate a stated constraint.
- A required behavior is missing from both the handout and the PRD.
- An event payload, API behavior, JSON schema or ownership rule is inconsistent across documents.
- You need an additional library, screen, gameplay mechanic or major folder restructuring.
- The spec appears impossible to satisfy under the language/compiler restrictions.
- A security/permission boundary or teammate's unpublished work would be affected.
The owner's approval must be explicit. Do not use "vibe coding" as permission to expand scope.
