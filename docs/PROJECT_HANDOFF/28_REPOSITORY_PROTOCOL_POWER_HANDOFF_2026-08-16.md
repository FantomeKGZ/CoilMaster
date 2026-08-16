# CoilMaster handoff — repository, protocol and power checkpoint

Дата: **2026-08-16**  
Ветка: **`cmp-protocol-v1`**  
Оценка CoilMaster v1: **91%**

## Последний recovery checkpoint

Реализован read-only apply preflight после V2 inspection, staging, strict restore
plan и проверенной локальной rollback-копии. Он повторно сверяет current,
rollback и staged files с CRC32, создаёт только readiness metadata и сохраняет:

```text
restore_apply_enabled=0
working_data_changed=0
```

Рабочие файлы не открываются для записи. Reboot не продолжает и не применяет
операцию; surviving metadata отображается как `STALE`.

```text
9ad5a889e26e0da717577bdd06b23546b6951de7
f4f6b20bfc52a50e26af0673bf54be76f0eff8f7
228a99e40ace0a25cd7b0a1d6d65e3db7940d999
CMP Protocol Tests: SUCCESS (run 31889384628)
ESP32 Build: SUCCESS (run 31889384626)
```

Hardware-подтверждение apply preflight ещё требуется.

## Repository cleanup

По явному подтверждению пользователя удалены 37 файлов из 15 пустых или
устаревших directory trees. Production `Arduino/`, `Core/`, lowercase
`firmware/`, hardware docs, `Shared/Protocol/`, `Tests/Protocol/`,
`Tests/Web/` и весь handoff сохранены.

```text
11b4a79b583a79b83743024ff22110c93f817871  cleanup
48305822bac82c4657b11b3b62499f80d71d4573  cleanup handoff
CMP Protocol Tests: SUCCESS (runs 31924588724, 31924650741)
Arduino Uno Build: SUCCESS (run 31924588743)
```

Cleanup не меняет RAM/Flash прошивки.

## Shared/Protocol boundary

`Shared/Protocol/` — ранний бинарный CMP с start word `0xAA55`, binary header,
fixed buffers и CRC-CCITT `0x1021`. Он не входит в PlatformIO production builds
и используется только `Tests/Protocol/`.

Фактический Arduino ↔ ESP32 transport — ограниченные текстовые кадры
`CMP1|...`, реализованные независимо в:

```text
Arduino/CM_UartEventTransport.*
firmware/esp32/src/CM_UartEventReceiver.*
```

Обе production-стороны используют общий stateless header-only
`Shared/CMP1Text/CM_Cmp1Crc.h`: CRC16/MODBUS, initial `0xFFFF`, reflected
polynomial `0xA001`. Старое утверждение CRC-CCITT в
`docs/17_UART_EVENT_TRANSPORT.md` исправлено. Прямые host tests проверяют
контрольный вектор, production event payload и incremental update.

```text
de8ee6b5da6b68d0880884e75f04e39e79c6b66d
CMP Protocol Tests: SUCCESS (run 31928080265)
Arduino Uno Build: SUCCESS (run 31928080266)
ESP32 Build: SUCCESS (run 31928080285)
```

Binary Shared core по-прежнему нельзя подключать к Uno вместо CMP1: форматы и
CRC несовместимы, а его fixed buffers опасны при подтверждённом малом запасе
SRAM. Сосуществование форматов теперь явно документировано.

Run-event `ACK/NACK` также защищены тем же CRC16/MODBUS. ESP32 всегда добавляет
checksum; Arduino проверяет её перед изменением event queue/retry state и
временно принимает точный legacy reply без CRC для staged rollout.

```text
a695440cbcae2582c158d1f29ff68cac5a38ba95
CMP Protocol Tests: SUCCESS (run 31929625664)
Arduino Uno Build: SUCCESS (run 31929625657)
ESP32 Build: SUCCESS (run 31929625636)
```

## Power decision

Обнаруженный симптом: ESP32 запускает Wi-Fi от USB, но не запускает его от
внешнего блока. Это считается аппаратной проблемой питания до обратного
доказательства, а не firmware defect.

Из wiring reference: LM2596 OUT+ был предусмотрен на ESP32 `VIN/5V`, OUT− на
общий GND. Для стационарной схемы выбран предпочтительный вариант:

```text
качественный изолированный USB supply 5 V, 3–4 A
├─ отдельный USB cable → ESP32
└─ отдельный USB cable → Arduino Uno
ESP32 GND ───────────── Arduino GND
level shifter: LV=3.3 V, HV=5 V, common GND
```

Одна плата не должна питаться через другую. На одну плату нельзя одновременно
подавать external USB power и USB power от PC без изоляции VBUS. Для прошивки
используется только один источник либо data cable с физически отключённой линией
5 V. Возле ESP32 рекомендованы local bulk/decoupling capacitors; точные номиналы
и полярность проверять перед монтажом.

Power decision пока не hardware-confirmed. Перед окончательной укладкой проверить
на выводах ESP32:

```text
VIN/5V ≈ 5.0 V
3V3 ≈ 3.3 V и не ниже 3.0 V при старте Wi-Fi
Wi-Fi start with ESP32 alone
Wi-Fi start with modules connected one-by-one
```

Для controlled check добавлен read-only `GET /api/system/diagnostics`; mobile и
desktop Settings показывают причину прошлого reset, `BROWNOUT` и heap. Это
диагностирует зафиксированную просадку, но не измеряет напряжение.

```text
d2f9b7371481ccd762bfc24f46a71b1d3c8d6904
CMP Protocol Tests: SUCCESS (run 31929236228)
ESP32 Build: SUCCESS (run 31929236220)
RAM: 15.8% (51840 / 327680 bytes)
Flash: 42.5% (1337369 / 3145728 bytes)
```

## Точное продолжение

1. Прошить current ESP32 и полностью заменить microSD `/web`.
2. Проверить read-only apply preflight до `READY`, неизменность business data,
   reboot → `STALE` без auto-resume и explicit cleanup.
3. Проверить выбранное dual-USB питание и записать фактические напряжения.
4. После hardware confirmation перейти к operator-only transactional restore
   apply с per-file atomic replacement и немедленным rollback при любой ошибке.

Общий CRC рабочего CMP1 и документированная граница binary `Shared/Protocol/`
уже завершены; повторять этот блок не нужно.

## Latest protocol continuation

Ответы удалённого задания Arduino → ESP32 теперь также защищены
CRC-16/MODBUS. ESP32 объявляет capability `C` в валидном `JOB`; только после
такого согласования Arduino добавляет `C|CRC16` к `JOB_ACK/JOB_CANCEL_ACK`.
Без capability используется точный legacy reply, поэтому платы можно обновлять
в любом порядке. ESP32 проверяет protected reply до изменения job state.

```text
b288ec82ed18aae4a7610f745cfa170cfc58c897  protocol implementation
b3385fb1aab08fced8d27010ba62ca58b183d947  Uno SRAM optimization
CMP Protocol Tests: SUCCESS (runs 31930079088, 31930198758)
Arduino Uno Build: SUCCESS (run 31930198773)
ESP32 Build: SUCCESS (run 31930079023)
Uno RAM: 70.7% (1447 / 2048 bytes; 601 bytes remain)
Uno Flash: 77.3% (24924 / 32256 bytes)
ESP32 RAM: 15.8% (51840 / 327680 bytes)
ESP32 Flash: 42.5% (1337777 / 3145728 bytes)
```

Следующая аппаратная проверка протокола: создать и отменить service job, затем
создать ещё один и подтвердить, что до физического START двигатель и SSR не
активируются. После этого продолжать пункты power/preflight выше. Общая оценка
готовности остаётся **91%**.

## Safety — не менять

- никакого automatic physical START;
- никакого auto-resume после reboot;
- ESP32/Web не управляют SSR напрямую;
- `RUN_COMPLETED` не списывает провод;
- manual wire writeoff требует exact
  `spool_id + source_session_id + source_run_id`.
