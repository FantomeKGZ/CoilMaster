# Winding job acknowledgement detail

## Purpose

The ESP32 delivery result now preserves the optional detail field returned by the Arduino in a `JOB_ACK` frame.

Supported frames:

```text
CMP1|JOB_ACK|<job_id>|ACCEPTED
CMP1|JOB_ACK|<job_id>|ACCEPTED|<detail>
CMP1|JOB_ACK|<job_id>|REJECTED|<reason>
```

`REJECTED` requires a non-empty reason. Frames with additional fields after the optional detail are rejected as malformed.

## Delivery event

`JobDeliveryEvent` now contains:

```cpp
char detail[24];
```

The value is always null-terminated and is truncated to 23 characters when the remote detail is longer.

Default details are:

- `ACCEPTED` when the Arduino accepts a job without an explicit detail;
- `NO_ACK` when the five delivery attempts expire.

For a rejected job, the Arduino-provided reason is retained so the controller and web interface can show the actual refusal cause instead of only a generic `Rejected` state.

## Integrity rules

- The acknowledgement must reference the currently pending `job_id`.
- Only `ACCEPTED` and `REJECTED` are valid statuses.
- `REJECTED` must contain a reason.
- At most one optional detail field is accepted.
- A malformed acknowledgement does not clear the pending job and therefore cannot falsely complete delivery.
