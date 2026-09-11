# Crypto benchmarks

Runs PSA Crypto operations and reports elapsed time and peak stack usage for each one.

## Design

Three layers, so that adding an algorithm touches one folder and one line:

```
main.c        psa_crypto_init(), walks the registry, prints the summary
runner.c      threading, timing, stack measurement, reporting
suites.c      the registry: one array per src/suites/<family> folder
src/suites/<family>/
  *_test_data.c    the suite table: which algorithms, key sizes, curves
  *_test_logic.c   the PSA calls themselves
```

**A suite** is one algorithm at one key size (`aes_cbc`/`aes128`). It declares up
to three stages, and omits a stage that does not apply to it:

| Stage      | Contains                                        |
|------------|-------------------------------------------------|
| `keysetup` | generate, import or derive the key              |
| `single`   | the one-shot form of the operation              |
| `multi`    | the multipart form, fed in exactly two updates  |

Key setup gates the other two, so an algorithm the platform will not produce a
key for is reported once rather than again as a failed operation.

**An operation** is one row of output and one thread. `runner.c` creates the
thread, names it `alg/keydesc/stage/op`, takes a cycle timestamp either side of
the callback, and reads the thread's peak stack while it is still parked. All
operations share one stack (`CONFIG_CRYPTO_BENCHMARKS_OP_STACK_SIZE`, 64000 bytes);
they run one at a time, and `CONFIG_INIT_STACKS` re-poisons it between runs, so
reuse does not blur the figures. Operation callbacks never log — the thread's
time and stack have to belong to the PSA calls alone.

Two escape hatches keep unmeasured work out of the figures:

- `prepare` — setup the row is not measuring (a peer's key, the far side of an
  exchange). Runs in its own thread; neither timed nor recorded.
- `check` — verifies what the operations produced, on the runner thread after
  the stages finish.

`PSA_ERROR_NOT_SUPPORTED` counts as **skipped**, not failed: that is how an
algorithm a board's driver does not implement is reported.

## Output

`CONFIG_CRYPTO_BENCHMARKS_OUTPUT_TEXT` (default) gives one aligned line per operation:

```
[00:00:01.412,109] <inf> crypto_benchmarks: sha256/single/compute                                   12.480 us  (status 0, stack 1128 bytes)
[00:00:01.412,231] <inf> crypto_benchmarks: sha256/multi/compute                                    18.360 us  (status 0, stack 1160 bytes)
[00:00:01.498,044] <inf> crypto_benchmarks: aes_cbc/aes128/keysetup/generate                        91.720 us  (status 0, stack 1416 bytes)
[00:00:05.003,918] <inf> crypto_benchmarks: === summary: 352 operations, 6 skipped, 0 failed ===
```

Reading a row: the name is `algorithm/key/stage/operation`, then elapsed time in
microseconds, then the PSA status and the thread's peak
stack.

`CONFIG_CRYPTO_BENCHMARKS_OUTPUT_JSON` emits the same records as one document via
`printk()`, counts first so a truncated capture still yields them:

```json
{"summary":{"operations":352,"skipped":6,"failed":0,"dropped_records":0},
"operations":[
{"group":"psa_hash","alg":"sha256","keydesc":null,"stage":"single","op":"compute","status":0,"elapsed_us":12.480,"stack_used":1128}
,{"group":"psa_cipher","alg":"aes_cbc","keydesc":"aes128","stage":"single","op":"encrypt","status":0,"elapsed_us":27.640,"stack_used":1704}
]}
```

## Limitations

These figures are indicative, not authoritative, and will not be accurate in
every case. Each row is a single run with no warm-up or repetition, so it
carries no statistics; it times a whole callback rather than one primitive, so a
`multi` row spans setup, two updates and finish and a PAKE `exchange` row spans
a protocol run; and its stack figure is a fill-pattern watermark that includes
the callback's own frames, not only what PSA touched. The inputs are chosen so
algorithms can be compared with each other: one 64-byte message everywhere, 
multipart always split into exactly two updates, and shared salt and info
strings even where a real protocol would send something specific. 
Correctness is checked by self-consistency not against known-answer vectors, so a driver
that is consistently wrong still passes.
Treat the output as a guide to orders of magnitude and relative cost on one
board and one configuration, not as a benchmark result.

