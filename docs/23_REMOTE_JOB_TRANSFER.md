# Remote winding job transfer (ESP32 -> Arduino Uno)

This stage adds reception of a complete winding program by Arduino Uno over the existing CMP UART link.

## Frame

```text
CMP1|JOB|job_id|session_id|type|coil_count|turns_csv|CRC16
```

Example:

```text
CMP1|JOB|105|8001|WORKING|4|140,100,80,40|A1B2
```

Fields:

- `job_id`: identifier assigned by ESP32;
- `session_id`: optional non-zero session identifier, or `0` so Arduino allocates one;
- `type`: `WORKING` or `STARTING`;
- `coil_count`: from 1 to 10;
- `turns_csv`: exactly `coil_count` positive values, each not greater than 9999;
- `CRC16`: CRC-16/Modbus over everything before the final separator.

## Arduino acceptance rules

Arduino accepts a remote program only when the state machine is not winding, paused, or in manual mode and the complete job passes validation.

After acceptance the program is placed in `READY`. The SSR remains off. Physical start still requires keypad `A` or the external START button.

Arduino response:

```text
CMP1|JOB_ACK|105|ACCEPTED|READY
```

or:

```text
CMP1|JOB_ACK|105|REJECTED|BUSY_OR_INVALID
```

## Safety

Receiving a job never starts the motor and never turns the SSR on. ESP32 supplies data only; Arduino remains the final safety controller.

## Current scope

Implemented now:

- frame reception on Arduino A2 through the LLC;
- CRC and bounds validation;
- working/starting winding type;
- loading into the existing `StateMachine`;
- explicit acceptance or rejection response.

Next stage:

- ESP32 job sender with retry and `JOB_ACK` processing;
- web/API endpoint for creating the job;
- display of the remote source on LCD and web monitor.
