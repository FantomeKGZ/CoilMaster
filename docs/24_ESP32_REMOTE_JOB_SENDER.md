# ESP32 remote job sender

The ESP32 can now transmit one complete winding program to Arduino Uno and wait for `JOB_ACK`.

## Frame

```text
CMP1|JOB|job_id|session_id|type|coil_count|turns_csv|CRC16
```

Example:

```text
CMP1|JOB|1|1000|WORKING|4|140,100,80,40|CRC16
```

CRC is CRC-16/Modbus with initial value `0xFFFF` and polynomial `0xA001`.

## Delivery rules

- Only one outgoing job can be pending at a time.
- A valid job is retransmitted every 2000 ms until Arduino answers.
- `JOB_ACK|...|ACCEPTED|READY` completes delivery successfully.
- `JOB_ACK|...|REJECTED|...` completes delivery as rejected.
- Arduino never starts the SSR automatically after receiving a job.
- Physical `A` or the external START button is still required.

## Bench command

For the first hardware test, open the ESP32 USB Serial Monitor at 115200 baud and send:

```text
demo
```

ESP32 queues this program:

```text
140 / 100 / 80 / 40
```

Expected ESP32 output after Arduino accepts it:

```text
JOB queued id=1 turns=140,100,80,40
JOB_ACK id=1 result=ACCEPTED_READY
```

If Arduino is winding, paused, in manual mode, or the frame is invalid, the result is `REJECTED`.

This serial command is a temporary bench interface. The web endpoint will call the same `queueJob()` API in the next stage.
