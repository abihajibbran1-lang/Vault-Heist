$ErrorActionPreference = 'Stop'
Set-Location (Join-Path $PSScriptRoot '..')
$culture = [System.Globalization.CultureInfo]::InvariantCulture
function Number($value, $digits = 3)
{
    return ([double]::Parse([string]$value, $culture)).ToString('F' + $digits, $culture)
}
function Row($cells)
{
    return '<tr><td>' + ($cells -join '</td><td>') + '</td></tr>'
}
function Table($headings, $rows)
{
    return '<table><thead><tr><th>' + ($headings -join '</th><th>') + '</th></tr></thead><tbody>' + ($rows -join '') + '</tbody></table>'
}
function Page($title, $content)
{
    return '<section class="page"><h1>' + $title + '</h1>' + $content + '</section>'
}
$existing = Get-Content docs/project-report.html -Raw -Encoding UTF8
$preserved = @{}
foreach ($match in [regex]::Matches($existing, '<section.*?</section>', 'Singleline'))
{
    $title = [regex]::Match($match.Value, '<h1>(.*?)</h1>').Groups[1].Value
    $preserved[$title] = [regex]::Replace($match.Value, '<div class="footer">.*?</div>', '', 'Singleline')
}
$trees = Import-Csv docs/benchmarks/trees.csv
$heaps = Import-Csv docs/benchmarks/heaps.csv
if ($trees.Count -ne 6 -or $heaps.Count -ne 12)
{
    throw 'Expected six tree and twelve heap benchmark cases.'
}
$pages = @()
$pages += Page 'Vault Heist: The Living Vault' @'
<p class="lead">Project report · revised 22 November 2026</p>
<p>C++17 · SFML 2.6.1 · GCC 15.2.0 · Ubuntu WSL/WSLg</p>
<p>This report presents the architecture, algorithmic complexity, all seven Section 10 benchmark experiments, test evidence and screenshots from the running game. The campaign contains four acts and twenty levels, supported by Sandbox and Endless AVL modes.</p>
<figure><img class="wide" src="captures/menu-smoke.png" alt="Actual SFML main menu"><figcaption>Figure 1. Main menu captured from the executable under WSLg.</figcaption></figure>
<h2>Submission details</h2><p>Student IDs: __________________ / __________________<br>Section: __________________<br>Contributions: complete with the actual contributors before submission.</p>
<p>The screenshots are real automated captures of controller-driven gameplay and event playback. They are not evidence of exhaustive manual mouse testing. Benchmarks were rerun on 22 November 2026; GUI captures were recorded on 20 November 2026.</p>
'@
$pages += $preserved['Architecture and ownership']
$pages += $preserved['Campaign: twenty working objectives']
$pages += $preserved['Algorithmic complexity']
$pages += Page 'Unit tests and integration evidence' @'
<h2>Reproducible commands</h2><pre>SANITIZE=1 ASAN_OPTIONS=detect_leaks=1 bash tests/run.sh
bash tests/benchmark.sh
bash run.sh --build-only</pre>
<p>The test suite is assertion-based C++ with manually managed reference buffers. Assertions remain enabled. Backend tests link no SFML; controller tests exercise middleware without opening a window. Sanitizers check invalid accesses, undefined behavior and leaks.</p>
<table><tr><th>Suite</th><th>Coverage</th></tr>
<tr><td>BinaryTree</td><td>Empty/single/skewed trees; recursive/iterative traversal; reconstruction; height/diameter; LCA/paths; mirror/equality/symmetry; IDs, copies and events.</td></tr>
<tr><td>BST and AVL</td><td>Duplicate rejection; search and all deletion cases; extremes; queries; LL/RR/LR/RL; height/size/balance invariants; deterministic mutation stress and rank/kth.</td></tr>
<tr><td>MaxHeap</td><td>Build, sifting, insert/extract, sorting, negatives/duplicates, active prefix/sorted suffix, copies, validation and event ordering.</td></tr>
<tr><td>Campaign</td><td>All twenty levels: solve/fail/retry, optimal score, stars, sequential unlock, replay without extra score, checkpoints, undo and hint cap.</td></tr>
<tr><td>Endless/Sandbox</td><td>400 paired deterministic rounds; five prediction outcomes; deadlines/pause/focus; leaderboard; save/resume; explicit AVL repair; undo/replay.</td></tr>
<tr><td>Persistence</td><td>Malformed saves; temporary validated restoration; stale campaign-progress recovery; save replacement and failed-source preservation.</td></tr></table>
<h2>Persistence design</h2><p>Checkpoints contain the model, accepted commands, counters, hint/selection state and unlock stars. A temporary controller replays and validates the saved state before replacing the live state. Normally a temporary file replaces the save; a verified copy fallback preserves previous bytes in a backup when replacement is denied.</p>
<p>Results and exact verification limits are in <b>docs/test-results.txt</b>. The current run log is <b>docs/test-run-2026-11-22.txt</b>. GUI captures demonstrate actual execution; exhaustive manual paths, native Windows packaging and large-view 60-FPS acceptance remain unverified.</p>
'@
$heightRows = @()
$searchRows = @()
$rotationRows = @()
$orderRows = @()
foreach ($t in $trees)
{
    $heightRows += Row @($t.n, $t.input, $t.bst_height, $t.avl_height)
    $searchRows += Row @("$($t.n) / $($t.input)", (Number $t.bst_comparisons_per_search 2), (Number $t.avl_comparisons_per_search 2), (Number ([double]$t.bst_search_seconds * 1000)), (Number ([double]$t.avl_search_seconds * 1000)), (Number ([double]$t.bst_seconds_per_search * 1000000)), (Number ([double]$t.avl_seconds_per_search * 1000000)))
    $rotationRows += Row @("$($t.n) / $($t.input)", $t.avl_insert_rotations, (Number $t.avl_rotations_per_insert 4), $t.avl_delete_rotations, (Number $t.avl_rotations_per_delete 4), $t.max_rotations_per_delete)
    $orderRows += Row @("$($t.n) / $($t.input)", (Number ([double]$t.bst_kth_seconds * 1000)), (Number ([double]$t.avl_kth_seconds * 1000)), $t.checksum)
}
$method = @'
<p>All seven required experiments use n = 100, 1,000 and 10,000. The benchmark links backend sources only, compiles with GCC -O2, and disables events through the <b>setEventRecording(false)</b> flag on every structure. Empty event batches are asserted; independent counters remain active. No GUI, animation or snapshot recording is timed.</p>
<p>Inputs use deterministic unsigned LCG state initially 3017. Tree cases insert distinct keys 0..n−1, sorted or shuffled. Both structures receive identical 10,000 successful random search keys. Timings use std::clock CPU seconds: tables convert to milliseconds or microseconds. These are single local measurements; they have no confidence intervals and small values are resolution/scheduling sensitive.</p>
'@
$pages += Page 'Section 10: height and search cost' ($method + '<h2>1. Final height: BST versus AVL</h2>' + (Table @('n','Insertion order','BST height','AVL height') $heightRows) + '<p>Leaf height is 1. Sorted insertion makes the BST a height-n chain, while AVL height remains logarithmic. Random insertion improves this BST sample but provides no worst-case balancing guarantee.</p><h2>2. Search cost: 10,000 queries per case</h2>' + (Table @('n / input','BST cmp/query','AVL cmp/query','BST total ms','AVL total ms','BST µs/query','AVL µs/query') $searchRows) + '<p>Comparison averages equal counter/10,000; average times equal total/10,000. Searches are successful in-range queries, not a mix of misses and hits. Counts expose the chain cost more reliably than tiny timing differences.</p>')
$pages += Page 'Section 10: rotations and order statistics' ('<h2>3. AVL insertion and deletion rotations</h2><p>Each case inserts n keys and then deletes all n keys in the same sorted/shuffled order. Counts are primitive rotations: LR/RL each count as two. Averages divide totals by n; maximum/delete counts every primitive repair caused by one deletion.</p>' + (Table @('n / input','Insert total','Avg/insert','Delete total','Avg/delete','Max/delete') $rotationRows) + '<p>Insertion needs at most two primitive rotations after an update. Deletion may rebalance several ancestors, so one deletion can use more rotations. Results depend on the deterministic operation order and do not establish an average-case theorem.</p><h2>4. 1,000 k-th-smallest queries</h2><p>The same 1,000 positions are queried on both trees. BST walks inorder with a manual stack; AVL uses maintained subtree sizes. The result checksum is compared before results are accepted.</p>' + (Table @('n / input','BST total ms','AVL total ms','Matching checksum') $orderRows) + '<p>BST scanning is O(n) worst case per query; the size-augmented AVL query follows one O(log n) path. The measurement includes no event or GUI work. Raw totals, averages and times for experiments 1–4 are retained in <b>docs/benchmarks/trees.csv</b>.</p>')
$buildRows = @()
$sortRows = @()
foreach ($h in $heaps)
{
    $buildRows += Row @($h.n, $h.input, (Number ([double]$h.build_seconds * 1000)), $h.build_comparisons, $h.build_swaps, (Number ([double]$h.build_comparisons / [double]$h.n)))
    $sortRows += Row @("$($h.n) / $($h.input)", (Number ([double]$h.heap_sort_seconds * 1000)), $h.heap_sort_comparisons, $h.heap_sort_swaps, (Number ([double]$h.insertion_sort_seconds * 1000)), $h.insertion_comparisons, $h.insertion_moves)
}
$pages += Page 'Section 10: bottom-up heap construction' ('<h2>5. Build time and linear construction</h2><p>Build Max Heap receives a fresh raw array. Random values span negative and positive keys; ordered inputs are ascending and descending; duplicate-heavy values lie in 0..7. Copy/allocation performed by buildMaxHeap is included in build time. Validation runs after timing.</p>' + (Table @('n','Input','Build ms','Comparisons','Swaps','Comparisons/n') $buildRows) + '<h2>Why bottom-up construction is O(n)</h2><p>Starting at the last internal node, sift down each parent. About n/2 nodes have height 0, n/4 have height 1, n/8 have height 2, and so on. Total sift work is bounded by n × Σ(h / 2<sup>h+1</sup>) = O(n). The comparisons/n column stays bounded in these cases, supporting that analysis; timings alone are not a proof.</p><p>The heap stores O(n) values. Iterative sifting uses O(1) auxiliary workspace beyond that owned storage. Random and both ordered cases satisfy the required comparison; duplicate-heavy cases are included for consistency with sorting.</p><p>Raw times, comparisons and swaps are in <b>docs/benchmarks/heaps.csv</b>. A displayed 0.000 ms means rounding below the displayed precision, not zero operations.</p>')
$pages += Page 'Section 10: Heap Sort and sorting comparison' ('<h2>6. Heap Sort: all four input distributions</h2><p>Each Heap Sort run starts with an unsorted copy and includes bottom-up construction plus all extractions. Heap comparisons and swaps are counted with events disabled. Every sorted output is checked element-by-element against an independent manual Insertion Sort.</p>' + (Table @('n / input','Heap ms','Heap cmp','Heap swaps','Insert ms','Insert cmp','Insert shifts') $sortRows) + '<h2>7. Comparison with Insertion Sort</h2><p>Both algorithms receive identical original values. Insertion Sort is the course-style comparison baseline: its counter records key comparisons and rightward shifts, not heap-style swaps. A heap swap exchanges two cells; an insertion shift copies one cell. Counts are labelled separately rather than treated as equivalent operations.</p><table><tr><th>Algorithm</th><th>Time</th><th>Auxiliary space</th><th>Stability</th></tr><tr><td>Heap Sort</td><td>O(n log n) worst case</td><td>O(1), beyond existing heap array</td><td>Not stable</td></tr><tr><td>Insertion Sort</td><td>O(n) best; O(n²) worst</td><td>O(1), beyond reference array</td><td>Stable for equal values</td></tr></table><p>Heap Sort performs O(n) construction followed by n logarithmic sifts. Insertion Sort benefits from already sorted input but needs quadratic shifts on reverse input. Sorting auxiliary space excludes the O(n) test copies and post-timing verification snapshot; it is an algorithmic bound, not measured process memory. Tiny single-run timings should not be interpreted as a precise ranking.</p>')
$pages += $preserved['AVL rotation evidence']
$pages += $preserved['Heap construction, extraction and final vault']
$pages += Page 'Delivery, level data and reproduction' @'
<h2>Run and submit</h2><pre>cd Vault-Heist
bash run.sh --build-only
bash run.sh
bash tests/benchmark.sh</pre>
<p>Use Ubuntu WSL/WSLg and the installed SFML 2.6.1 under /usr/local. Include all <b>src/**/*.cpp</b> and <b>include/**/*.h</b>, plus tests, run/build scripts, assets, external JSON levels, documentation and benchmark CSVs. The executable is Linux/WSL, not a native Windows .exe. Do not submit .git history or personal player saves.</p>
<h2>What the two supporting deliverables mean</h2><p><b>Unit test suite and test report:</b> executable automated checks of algorithms and edge cases, plus commands, actual results and limitations. This repository supplies tests/backend, tests/middleware, tests/run.sh and docs/test-results.txt. Benchmarks measure performance and do not replace correctness tests.</p>
<p><b>Level files and authoring guide:</b> external data defining puzzle scenarios, with instructions for editing/loading/validating them. The twenty JSON files are under data/levels; docs/level-authoring.txt explains fields, examples and supported objective types.</p>
<figure><img style="height:180px" src="captures/briefing-3-3.png" alt="AVL level briefing"><figcaption>Figure 10. Objective briefing shown before the player starts the level.</figcaption></figure>
<h2>Limitations and final submission steps</h2><p>Fill in real IDs, section and contribution details; rename the archive to the course pattern. Review manual GUI paths on the demonstration machine. Some nonmutating Sandbox queries lack full per-node playback. Large-view 60-FPS acceptance, optional sound assets and native Windows deployment are unverified.</p>
<p>Level fixtures are reproducible explicit arrays; seed metadata does not randomize them. JSON editing can replace a supported scenario within the twenty slots without recompilation. Adding extra slots or a new objective handler currently requires code changes; the general no-code expansion requirement is therefore only partially met.</p>
<p>Source requirements: supplied 12-page Project Deliverable 1 (1).pdf, its Section 10 benchmark table, repository specifications and owner-approved Endless rules. No test result, screenshot, benchmark measurement or authorship is fabricated.</p>
'@
if ($pages.Count -ne 12 -or ($pages | Where-Object { -not $_ }).Count -gt 0)
{
    throw 'Missing source report sections; expected twelve complete pages.'
}
$style = [regex]::Match($existing, '<style>.*?</style>', 'Singleline').Value
$style = $style.Replace('</style>', '.wide{height:235px}thead{display:table-header-group}h1{font-size:22pt}td{overflow-wrap:anywhere}</style>')
$body = ''
for ($i = 0; $i -lt $pages.Count; $i++)
{
    $footer = '<div class="footer">Vault Heist: The Living Vault · Revised 22 November 2026 · ' + ($i + 1) + ' / 12</div>'
    $body += $pages[$i].Replace('</section>', $footer + '</section>')
}
$output = '<!doctype html><html lang="en"><head><meta charset="utf-8"><title>Vault Heist project report</title>' + $style + '</head><body>' + $body + '</body></html>'
[System.IO.File]::WriteAllText((Join-Path (Get-Location) 'docs/project-report.html'), $output, [System.Text.UTF8Encoding]::new($false))
Write-Output 'Generated twelve report pages using measured CSV results and real captures.'
