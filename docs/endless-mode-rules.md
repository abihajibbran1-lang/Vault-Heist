# Vault Heist — Endless Mode Rules

**Status:** Project-specific clarification for implementation  
**Applies to:** Endless AVL survival mode only  
**Read with:** `AGENTS.md`, `docs/prd.md`, `docs/architecture.md`, `docs/event-contract.md`  
**Repository destination:** `docs/endless-mode-rules.md`

> **Important source distinction:** The course handout requires an Endless **AVL survival mode** with a **stream of mixed insertions and deletions**, a score based on **survival time while the tree remains valid**, **difficulty increasing through a faster operation rate**, and a **local leaderboard**. Its save/load section also requires the current Endless structure and random seed to be saved. **The handout does not give exact timing intervals, interaction rules, failure triggers, key-generation parameters, or tie-breaking rules.** The concrete values below are **project implementation decisions**, supplied here to remove that ambiguity. Do not present them as verbatim instructor requirements or invent further mechanics.

## 1. Scope and objective

The player survives an endless stream of AVL insertion and deletion requests. For each request, the player predicts the **first AVL rotation case** required (`NONE`, `LL`, `RR`, `LR`, or `RL`). The backend then performs the real operation and displays its actual events and rotations. The objective is to make correct predictions before the next operation arrives, keeping the AVL valid for as long as possible.

This reuses the **rotation-prediction interaction already required by campaign level 3.4**, rather than introducing a separate puzzle system. There is **no final level, victory state, additional power-up, or separate Endless upgrade system**.

Endless is available from the Main Menu. It does not unlock or modify campaign levels.

## 2. Starting state

- Start a new run with a valid AVL tree built by inserting these keys, in order: `50, 30, 70, 20, 40, 60, 80`.
- Use the normal backend AVL insertion code to construct the tree, then validate it. The setup operations do **not** count toward the player's completed operations or survival score.
- A new run begins with `survivalSeconds = 0`, `completedOperations = 0`, and an internal random-generator state (`seed`).
- Generate the **first request immediately** when the run begins; its response window starts at that moment.
- For ordinary play, generate and store a new unsigned seed at run creation. Tests must be able to inject a fixed seed to reproduce an identical request sequence.
- The tree is never deliberately left in an invalid AVL state between completed requests.

## 3. What the player does each round

1. Display exactly **one pending request**, such as `INSERT 65` or `DELETE 30`, along with the current AVL tree and a countdown until the next request.
2. Display five choices: `NONE`, `LL`, `RR`, `LR`, `RL`.
3. The player selects one choice before the deadline. The selection is the submitted answer; no extra confirmation button is required.
4. The middleware determines the correct answer by running the same operation on a **deep copy** of the current AVL and examining the first real rotation case in its event sequence. If the operation needs no rotation, the answer is `NONE`.
5. If the prediction is correct, apply the operation to the **live** AVL using the real backend, validate it, animate its actual event sequence, and increment `completedOperations` by one.
6. If the prediction is wrong, end the run immediately under the failure rules below. Do not mutate the live tree for that failed prediction.
7. If an AVL deletion requires more than one rebalance/rotation, the player predicts **only the first rotation case**; the backend completes and animates all remaining rotations automatically.
8. After a successful answer and its animation, the screen waits until the scheduled arrival time of the next request. The player cannot submit another answer during that wait.

**Rotation naming:** Use the case at the first unbalanced ancestor encountered bottom-up. For a left-heavy pivot (`balanceFactor > 1`), `leftChild.balanceFactor >= 0` means `LL`; otherwise `LR`. For a right-heavy pivot (`balanceFactor < -1`), `rightChild.balanceFactor <= 0` means `RR`; otherwise `RL`. The equality cases matter for AVL deletion. `NONE` means no rebalancing rotation is needed.

A prediction is about the **required case**, not the number of primitive left/right rotation calls. For example, `LR` is one choice even though it consists of two primitive rotations.

## 4. Request generation

- Use the project's own **per-run deterministic pseudo-random generator**, not a global random state. A simple unsigned 32-bit linear congruential generator is sufficient; keep its state inside the Endless controller.
- Generate integer keys in the inclusive range **1 through 999**.
- **Never generate an invalid request**: an insertion key must not already exist; a deletion key must exist.
- If the tree has fewer than **31 nodes**, choose insertion with **60% probability** and deletion with **40% probability**. If the tree is empty, force insertion. If it has **31 nodes**, force deletion. This 31-node limit is an implementation choice to keep the visualization readable, not a course requirement.
- For insertion, retry or deterministically scan until an unused key is found. For deletion, choose uniformly from the keys currently in the AVL using a manually allocated inorder array.
- Consume random numbers in a consistent, documented order so the same saved seed/state and tree produce the same future requests.
- Generating or previewing a request must not mutate the live tree, change the score, or emit visible gameplay events.

**No STL containers, global variables, or enums** are allowed for the generator, request queue, or AVL implementation. There is only **one pending request**, so no multi-item queue is needed.

## 5. Escalation schedule

Difficulty increases **only** by shortening the interval between incoming requests. Do not add new operation types, penalties, faster animation, smaller keys, or extra hazards.

At the instant a request appears, compute its interval from the **active survival time**:

`intervalSeconds = max(3.0, 8.0 - floor(survivalSeconds / 30.0))`

| Active survival time when request appears | Time until next request / answer deadline |
|---|---:|
| 0 to less than 30 seconds | 8 seconds |
| 30 to less than 60 seconds | 7 seconds |
| 60 to less than 90 seconds | 6 seconds |
| 90 to less than 120 seconds | 5 seconds |
| 120 to less than 150 seconds | 4 seconds |
| 150 seconds onward | 3 seconds (minimum) |

- Each request has a **fixed deadline**: `deadline = survivalSecondsAtSpawn + intervalSeconds`.
- **Do not shorten a deadline already in progress** when the survival clock crosses a 30-second threshold. Recompute the interval for the *next* request.
- The next request appears at that deadline if the previous request was completed correctly. If the previous request remains unanswered, the run fails at that deadline instead.
- There is no backlog: never create a second pending request while the first is unresolved.
- If a correct answer is submitted early, keep the existing next-arrival time; do not spawn the next request immediately. This prevents fast clicking from increasing the stream rate beyond the specified schedule.
- At the exact deadline (`remainingSeconds <= 0`), **timeout takes precedence** over a simultaneous input submission.
- The countdown displays the remaining time, rounded for readability; game logic uses the unrounded value.

## 6. Clock, animation, and pause behavior

`survivalSeconds` measures **active gameplay time**, not total wall-clock time. It advances by real frame delta time only when the player is able to act or is waiting for the next scheduled request.

- **Pause the survival clock and deadline countdown** during backend event animations, paused menus, or a suspended run. Do not count animation playback toward the score.
- Once an animation finishes, resume the clock from the same value and remaining countdown.
- Animation speed settings and sound settings **must not affect difficulty or score**.
- The backend and middleware receive elapsed seconds from the application loop; the middleware does not depend on SFML clocks or SFML types.
- Space may pause/resume event playback as defined in the event contract; pausing playback also freezes Endless gameplay time. Esc opens the pause/exit menu and freezes time.
- Never use blocking sleeps for animations or countdowns.
- On application/window focus loss, freeze the Endless clock until gameplay resumes, so a backgrounded window does not cause an unfair timeout.

## 7. Failure and end-of-run rules

A run ends in **failure** only when one of these player-facing conditions occurs:

1. **Wrong prediction:** The chosen rotation case differs from the first required case (including an incorrect `NONE`). Fail immediately, without applying that request to the live AVL.
2. **Timeout:** The next scheduled arrival/deadline is reached before the pending request was answered correctly. Fail immediately; do not apply the unanswered request.

At a failure:

- Freeze the survival clock and stop accepting gameplay input.
- Let any **already-started** visual effect finish before showing the final result, but never start or apply another AVL operation.
- Show `Survived: <whole seconds>s`, `Operations completed: <count>`, and `Reason: Wrong prediction` or `Reason: Time expired`.
- Offer `Retry` and `Main Menu` and show the local leaderboard result.
- A run has **no stars** and does **not** use the campaign's alarm capacity, `1000 - 10 * steps` points formula, hint penalty, or rotation penalty. Its only primary score is survival time.
- A voluntary `Exit Run` is **not** a failure and does not submit a leaderboard score. It may save the run for resumption.

**Invariant errors are bugs, not player mistakes:** After every accepted operation, verify BST ordering, AVL balance (`abs(balanceFactor) <= 1`), correct heights (`leaf = 1`), correct subtree sizes, and valid node links. If validation fails, stop the run with a technical error and **do not submit a leaderboard entry**. Do not pretend a corrupted backend is normal gameplay or blame the player.

## 8. Score and local leaderboard

- Primary score: `survivalScore = floor(survivalSeconds)` (nonnegative whole seconds).
- Secondary tie-breaker: higher `completedOperations` ranks first when survival scores match.
- If both match, preserve the earlier stored record's position.
- Store the **top 10 completed failure-run records** in a local file. Each record needs only the whole-second score, completed-operation count, and run seed; a user account or online service is not required.
- Insert a record **once** when a legitimate failure is finalized. Retry starts a new run with fresh counters; it does not alter the previous result.
- Never insert leaderboard records for paused runs, voluntary exits, resumes, internal errors, previews, or replayed animations.
- Display scores as seconds (for example, `92s`) and operation counts; do not convert them to campaign points.

## 9. Save, exit, and resume

The handout calls for saving the current Endless structure and random seed. To support that without changing gameplay:

- Persist the current AVL keys/shape using the project's approved tree serialization; recompute AVL heights and subtree sizes on load and validate before resuming.
- Persist the random-generator state, `survivalSeconds`, `completedOperations`, pending request (if any), and the **remaining time** until the next arrival/deadline.
- Save only a **consistent operation boundary**: after a successful operation and animation, when paused, or when choosing `Save and Exit`. Do not save halfway through a structural mutation or visual tween.
- A loaded run resumes **paused**, preserving its current pending request and remaining countdown. Resume begins counting only when the player returns to gameplay.
- Save the local leaderboard separately from the active run. A failed run is finalized and must not be resumable.
- A missing or corrupt save must show a concise error and allow a fresh run; it must not crash or silently load an invalid AVL.
- Use local files under `data/saves/` (for example, `endless-save.json` and `endless-leaderboard.json`), with the repository's existing permitted JSON handling. Do not add a third-party JSON dependency without approval.

## 10. Minimal UI and event integration

Use the existing Endless screen and the same AVL renderer, controller, backend events, and animator as campaign level 3.4. Display:

- The current AVL tree and its actual insertion/deletion/rotation animations.
- The pending operation and key.
- Five rotation-prediction choices.
- A clear countdown/pressure indicator, active survival seconds, and completed-operation count.
- Brief feedback for correct answers and the final failure reason.

The countdown may be drawn as an alarm-style bar, but **it is not the campaign alarm-step system**. Avoid new screens or unrelated visual mechanics. Sounds and bubbly visual effects may use the approved shared assets/settings; they do not change timing.

During an active Endless run, **Undo, Hint, Reset, and full-level Replay are disabled** to avoid reversing or duplicating survival progress. They remain available in the other modes where required. `Retry` is available after failure. Use the established event contract for actual animation playback and event ordering.

## 11. State machine (implementation contract)

Use string state names or class-owned fields, **not enums**:

| State | Allowed behavior | Transition |
|---|---|---|
| `WAITING_FOR_ANSWER` | Show pending request; advance active clock; accept one prediction | Correct -> `ANIMATING`; wrong -> `FAILED`; deadline -> `FAILED` |
| `ANIMATING` | Apply accepted operation once; play real events; clock frozen | After animation and successful validation -> `WAITING_FOR_NEXT` |
| `WAITING_FOR_NEXT` | Advance active clock; show next-arrival countdown; ignore predictions | At scheduled time -> generate request -> `WAITING_FOR_ANSWER` |
| `PAUSED` | Freeze clock; save/exit/resume | Resume -> prior active state |
| `FAILED` | Freeze clock; show result; write leaderboard once | Retry -> fresh run; exit -> menu |
| `ERROR` | Freeze clock; report invariant/save error; no leaderboard | Exit or restart |

The initial state is `WAITING_FOR_ANSWER` with the first request already generated. Use explicit checks to ensure a deadline cannot generate a new request and fail the old request in the same update. Each accepted operation mutates the live AVL **exactly once**.

## 12. Acceptance tests Codex MUST run

1. New run starts with the specified seven-node valid AVL; score and completed operations are zero.
2. A fixed seed reproduces the same request sequence, including inserts, deletes, and values.
3. Generated insertions never duplicate keys; generated deletions always target an existing key; 31 nodes forces deletion.
4. Correct `NONE`, `LL`, `RR`, `LR`, and `RL` predictions are recognized using real backend operations. For multi-rotation deletion, only the first case is requested.
5. Wrong prediction fails without modifying the live tree or incrementing completed operations.
6. An unanswered request fails at its deadline. An answer at the exact deadline does not bypass timeout.
7. Correct early answers do not spawn the next request early; no overlapping pending requests occur.
8. Intervals are 8/7/6/5/4/3 seconds at the specified survival-time boundaries; an in-progress deadline never shrinks.
9. The survival clock does not advance during animations, pause, loss of focus, or while loading a saved run.
10. All successful insertions/deletions leave a valid AVL with correct heights and subtree sizes.
11. Failure writes exactly one leaderboard entry; voluntary exit and internal errors write none. Ties are ordered by completed operations, then earlier entry.
12. Save/resume restores the exact tree, PRNG state, counters, pending request, and remaining time without replaying an operation or double-scoring.
13. Endless uses no campaign stars, alarm-step scoring, or extra penalties. Other game modes remain unchanged.
14. All Endless code respects `AGENTS.md`: no STL containers, no global variables, no enums, and no SFML dependencies in the backend.

## 13. Change control for Codex

**These rules are the explicit Endless-mode clarification requested by the project owner.** Treat this file as authoritative for Endless behavior wherever `docs/prd.md` merely says “escalating stream” or “minimal extras.” The official handout and instructor clarifications still take precedence if an actual conflict is identified.

Do not change the numerical schedule, failure triggers, prediction interaction, score calculation, generator limits, or save semantics without approval. Do not expand Endless with unrelated mechanics. If an implementation obstacle requires a different rule, report the conflict instead of silently making up a replacement.
