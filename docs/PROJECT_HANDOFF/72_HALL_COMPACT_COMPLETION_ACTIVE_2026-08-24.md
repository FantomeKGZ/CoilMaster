# CoilMaster — active compact Hall completion checkpoint

Date: **2026-08-24**  
Source of truth: **`cmp-protocol-v1` only**.

This checkpoint supersedes stale completion-status paragraphs in `71_HALL_RAW_STREAM_MIGRATION_2026-08-24.md`.
For new-chat continuation, `73_NEXT_CHAT_TRANSFER_2026-08-24.md` is newer and should be read first.

## Active wire path

Calibration raw measurement:

```text
Uno A0
  -> CMP1|CAL_SAMPLE|BASELINE|raw|sequence|elapsed_ms|C|CRC
  -> CMP1|CAL_SAMPLE|RUN|raw|sequence|elapsed_ms|C|CRC
  -> ESP32 HallCalibrationRawCollector
```

Completion/correlation emitted by Uno:

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

Legacy receive fallback is intentionally retained on ESP32:

```text
CAL_RESULT
 -> HardwareControlClient::processCalibrationResult
 -> UartEventReceiver::takeHallCalibrationResult
 -> HallCalibrationCompletionAdapter::enrichLegacy
 -> same HallCalibrationRemoteResult shape
```

For both receive paths, baseline/min/max/runSamples/duration are owned by ESP32 `HallCalibrationRawCollector`. Completion frames carry only the Uno-owned transient correlation ID.

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

## Key commits

```text
7025769132fbb1e77c572e8bf81c9965c7342673  feat(hall): define compact calibration done protocol
29c19abd3abd54b9d3660adc5cc5fa398ef45d6a  feat(hall): parse compact calibration done frames
5a168dbeb1d16ad2b2dc36ea6b043271acd53c10  feat(hall): add compact completion adapter
b47bcf727e64a6288c5c7482feaefdeb00b185f4  feat(hall): build remote result from compact completion
fc7c6cd918a5f3938f2ea8b533e55ae656596c3e  test(hall): lock compact completion adapter ownership
8e66ce6b3b2114e18e1f66a6ce79917f4ca8e8fe  feat(hall): prepare receiver compact completion state
d35a6de9b378a48578274f4c7320d92b3be4b230  feat(hall): receive compact calibration completion
60d761e072ea1ddeb557153c4799d1634a43826a  feat(hall): implement Uno compact calibration done formatter
24b00f59e3f493012695b80d6901df52c6326fd2  test(hall): lock dual completion receive path
5d2b763f2e615d3444bdaed948e46f2eac22c0a9  feat(hall): switch Uno completion to compact frame
abb48f80f52125e9e12ff16ba49274208dc494cc  test(hall): enforce active compact completion TX
d77a24b3437831d0a236086055a193f233e1be7e  test(hall): align safety audit with compact completion
b07de01ee4f3b1216153036dd977fa48bc053c2f  docs(handoff): record compact completion CI findings
```

## Verified Uno build / size

Compact-completion Uno builds confirmed SUCCESS:

```text
32751151359  build-uno  checkout 5d2b763f2e615d3444bdaed948e46f2eac22c0a9  SUCCESS
32751199627  build-uno  checkout a928a51bc77c00407b146587aaf34c1e08a19998  SUCCESS
```

Exact measured memory on `a928a51bc77c00407b146587aaf34c1e08a19998`:

```text
RAM   1213 / 2048 = 59.2%   free 835 B
Flash 31640 / 32256 = 98.1% free 616 B
```

Previous verified comparison baseline:

```text
checkout 02d9cd7e3c0679ae77d645a550af4f933b355e76
RAM   1213 / 2048 = 59.2%   free 835 B
Flash 31648 / 32256 = 98.1% free 608 B
```

Compact completion saves **8 B Flash** and changes RAM by **0 B** versus that baseline.

## Verified host-tests — GREEN

The stale Hall safety assertion was fixed in:

```text
d77a24b3437831d0a236086055a193f233e1be7e
```

Fresh post-fix Actions supplied by the user are fully successful:

```text
32753340348  host-tests  checkout d77a24b3437831d0a236086055a193f233e1be7e  SUCCESS
32753408620  host-tests  checkout b07de01ee4f3b1216153036dd977fa48bc053c2f  SUCCESS
```

The latest run `32753408620` passed all listed gates, including:

```text
4/4 CTest protocol tests
Hall calibration safety contracts
Hall lost-apply reconciliation
Uno Hall parser ownership
Hall raw migration ownership/wire contracts
Hall history and SD reference contracts
release safety / job lifecycle / material writeoff contracts
```

Therefore `b07de01ee4f3b1216153036dd977fa48bc053c2f` is a verified **host-tests GREEN** checkpoint.

This does not imply hardware GREEN.

## Next step

1. Keep compact completion; it is now host-tests GREEN and saves 8 B Flash with unchanged RAM.
2. Recalculate the exact longest remaining Uno Hall response now that legacy long `CAL_RESULT` TX is gone.
3. Evaluate `HallCalibrationProtocol::MaxFrameLength = 96` only against actual longest active `CAL_APPLIED` / `CAL_STATE` response including CRC/newline.
4. If reducing the buffer, add exact regression coverage first/with the change.
5. Run a fresh `build-uno` and host-tests; compare exact Flash/RAM against `31640 / 1213`.
6. Keep ESP32 legacy `CAL_RESULT` receive fallback unless compatibility removal is a deliberate later decision.
7. Continue software optimization without intermediate hardware tests.
8. Only after optimization is complete run the single full hardware acceptance sequence documented in `73_NEXT_CHAT_TRANSFER_2026-08-24.md`.
