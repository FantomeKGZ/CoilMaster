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

Runtime/Uno transition observed in current branch:

```text
8e66ce6b3b2114e18e1f66a6ce79917f4ca8e8fe  feat(hall): prepare receiver compact completion state
5b7e81bc65fa431ffd576028d3004e70aab8ee1d  feat(hall): prepare Uno compact calibration done formatter
60d761e072ea1ddeb557153c4799d1634a43826a  feat(hall): implement Uno compact calibration done formatter
24b00f59e3f493012695b80d6901df52c6326fd2  test(hall): lock dual completion receive path
abb48f80f52125e9e12ff16ba49274208dc494cc  test(hall): enforce active compact completion TX
```

## Verified / unverified status

User explicitly confirmed all statuses GREEN for checkpoint:

```text
2779a42d39659a2a0c976693571a0e7d93bd91ca
```

That confirmation does **not** automatically cover later compact-completion runtime/TX commits.

At the time this checkpoint was written, GitHub combined status for later HEAD returned no status entries. Therefore do not claim the current compact-completion HEAD GREEN until Actions/CI is explicitly verified or the user confirms it.

Last verified Uno size baseline before compact completion TX:

```text
checkout 02d9cd7e3c0679ae77d645a550af4f933b355e76
RAM   1213 / 2048 = 59.2%  free 835 B
Flash 31648 / 32256 = 98.1% free 608 B
```

The next required software gate is an Uno build/size measurement on the compact `CAL_DONE` descendant and host regression result. Do not request intermediate hardware tests.

## Next step

1. Verify host tests on current descendant.
2. Verify `build-uno` and record exact checkout + Flash/RAM.
3. Compare against `31648 Flash / 1213 RAM` baseline.
4. If compact completion saves Flash, keep it and evaluate whether `HallCalibrationProtocol::MaxFrameLength` can be reduced based on the actual longest remaining calibration response.
5. Keep ESP32 legacy `CAL_RESULT` receive fallback unless there is a deliberate compatibility-removal decision.
6. Only after optimization is complete run the single full hardware acceptance sequence.
