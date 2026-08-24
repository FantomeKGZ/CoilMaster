# CoilMaster — active compact Hall completion checkpoint

Date: **2026-08-24**  
Source of truth: **`cmp-protocol-v1` only**.

This checkpoint supersedes the stale completion-status paragraphs in `71_HALL_RAW_STREAM_MIGRATION_2026-08-24.md`.

## Active wire path

Calibration raw measurement is now:

```text
Uno A0
  -> CMP1|CAL_SAMPLE|BASELINE|raw|sequence|elapsed_ms|C|CRC
  -> CMP1|CAL_SAMPLE|RUN|raw|sequence|elapsed_ms|C|CRC
  -> ESP32 HallCalibrationRawCollector
```

Completion/correlation emitted by Uno is now compact:

```text
CMP1|CAL_DONE|measurement_id|C|CRC
```

`Arduino/CM_HallCalibrationProtocol.cpp::formatResult()` delegates to `formatDone()`.
Uno no longer emits the legacy long `CAL_RESULT` carrier.

## ESP32 backward compatibility

`UartEventReceiver::poll()` accepts `CMP1|CAL_DONE|` before the generic hardware-control parser.

Compact path:

```text
CAL_DONE
 -> HallCalibrationDoneProtocol::parseDone
 -> HallCalibrationCompletionAdapter::buildFromDone
 -> HallCalibrationRemoteResult
 -> existing Web/analyzer flow
```

Legacy fallback is intentionally retained on ESP32:

```text
CAL_RESULT
 -> HardwareControlClient::processCalibrationResult
 -> UartEventReceiver::takeHallCalibrationResult
 -> HallCalibrationCompletionAdapter::enrichLegacy
 -> same HallCalibrationRemoteResult shape
```

For both completion paths, baseline/min/max/runSamples/duration are owned by ESP32 `HallCalibrationRawCollector`. Completion frames carry only the Uno-owned transient correlation ID.

## Safety ownership unchanged

Arduino Uno remains authoritative for:

- physical START;
- SSR ownership/control;
- local calibration confirmation;
- calibration peer timeout / fail-closed abort;
- normal winding Hall turn detection/counting;
- final Hall settings EEPROM;
- exact proposal measurement-id gate;
- local confirmation before EEPROM apply.

`CAL_SAMPLE` and `CAL_DONE` have no actuator semantics.

## Commits in this checkpoint

Prepared protocol/adapter:

```text
7025769132fbb1e77c572e8bf81c9965c7342673  feat(hall): define compact calibration done protocol
29c19abd3abd54b9d3660adc5cc5fa398ef45d6a  feat(hall): parse compact calibration done frames
5a168dbeb1d16ad2b2dc36ea6b043271acd53c10  feat(hall): add compact completion adapter
b47bcf727e64a6288c5c7482feaefdeb00b185f4  feat(hall): build remote result from compact completion
fc7c6cd918a5f3938f2ea8b533e55ae656596c3e  test(hall): lock compact completion adapter ownership
```

Runtime/Uno transition:

```text
8e66ce6b3b2114e18e1f66a6ce79917f4ca8e8fe  feat(hall): prepare receiver compact completion state
d35a6de9b378a48578274f4c7320d92b3be4b230  feat(hall): receive compact calibration completion
5b7e81bc65fa431ffd576028d3004e70aab8ee1d  feat(hall): prepare Uno compact calibration done formatter
60d761e072ea1ddeb557153c4799d1634a43826a  feat(hall): implement Uno compact calibration done formatter
24b00f59e3f493012695b80d6901df52c6326fd2  test(hall): lock dual completion receive path
5d2b763f2e615d3444bdaed948e46f2eac22c0a9  feat(hall): switch Uno completion to compact frame
a928a51bc77c00407b146587aaf34c1e08a19998  docs: mark compact completion active
abb48f80f52125e9e12ff16ba49274208dc494cc  test(hall): enforce active compact completion TX
50762d47380fba37dd61d0b38e54972fd7315bb5  docs(handoff): record active compact Hall completion
d77a24b3437831d0a236086055a193f233e1be7e  test(hall): align safety audit with compact completion
```

## Verified Uno build / size

The user supplied the relevant Actions runs. Two compact-completion Uno builds are confirmed SUCCESS:

```text
32751151359  Arduino Uno Build  checkout 5d2b763f2e615d3444bdaed948e46f2eac22c0a9  SUCCESS
32751199627  Arduino Uno Build  checkout a928a51bc77c00407b146587aaf34c1e08a19998  SUCCESS
```

Exact measured memory on `a928a51bc77c00407b146587aaf34c1e08a19998`:

```text
RAM   1213 / 2048 = 59.2%   free 835 B
Flash 31640 / 32256 = 98.1%  free 616 B
```

Previous verified comparison baseline:

```text
checkout 02d9cd7e3c0679ae77d645a550af4f933b355e76
RAM   1213 / 2048 = 59.2%   free 835 B
Flash 31648 / 32256 = 98.1%  free 608 B
```

Compact completion therefore saves **8 B Flash** and changes RAM by **0 B** versus that baseline.

## Host-test investigation

The supplied CMP Protocol Test runs after the compact-completion transition were FAIL:

```text
32750962381  checkout d35a6de9...  FAILURE
32750987680  checkout 18253e24...  FAILURE
32751151376  checkout 5d2b763f...  FAILURE
32751194078  checkout 24b00f59...  FAILURE
32751199602  checkout a928a51b...  FAILURE
32751327822  checkout abb48f80...  FAILURE
32751482421  checkout 50762d47...  FAILURE
```

The final run `32751482421` was inspected step-by-step. Configure/build/CTest and every listed audit passed except exactly one:

```text
Audit Hall calibration safety contracts  FAILURE
```

Exact stale assertion:

```text
Arduino Hall calibration protocol: missing CMP1|CAL_RESULT|INVALID|0|0|0|0|0|RISING|0|0|%lu|C
```

This was an obsolete audit expectation after the intentional Uno TX migration to `CAL_DONE`; it was not a runtime/compiler/raw-migration failure. In the same run:

```text
4/4 CTest protocol tests                          PASS
Hall lost-apply reconciliation                    PASS
Uno Hall parser ownership                         PASS
Hall raw migration ownership/wire contracts       PASS
Hall history and SD reference navigation          PASS
```

Commit `d77a24b3437831d0a236086055a193f233e1be7e` updates only that stale safety audit to require:

- `formatResult()` delegates to `formatDone()`;
- Uno protocol no longer contains legacy `CMP1|CAL_RESULT|` TX;
- compact formatter contains `CMP1|CAL_DONE|%lu|C` + CRC;
- compact completion formatter has no START/SSR actuator semantics.

All existing local `#`, physical START, peer-timeout, proposal identity, EEPROM ordering, lost-apply reconciliation and fail-closed assertions remain in the safety audit.

## Verified / unverified status

User explicitly confirmed all statuses GREEN for checkpoint:

```text
2779a42d39659a2a0c976693571a0e7d93bd91ca
```

The later compact-completion Uno build is now verified SUCCESS with the size above. The host-test failures through `32751482421` are explained by the stale audit and have been corrected in `d77a24b...`, but **do not call the new descendant GREEN until a fresh post-fix Actions run succeeds or the user explicitly confirms it**.

## Next step

1. Verify fresh CMP Protocol Tests after `d77a24b...`.
2. Once host tests are GREEN, keep compact completion: it saves 8 B Flash and RAM is unchanged.
3. Evaluate `HallCalibrationProtocol::MaxFrameLength` only against the actual longest remaining calibration response; do not shrink it based on `CAL_DONE` alone because `CAL_APPLIED`/state frames may be longer.
4. Keep ESP32 legacy `CAL_RESULT` receive fallback unless there is a deliberate compatibility-removal decision.
5. Continue software optimization without intermediate hardware tests.
6. Only after optimization is complete run the single full hardware acceptance sequence.
