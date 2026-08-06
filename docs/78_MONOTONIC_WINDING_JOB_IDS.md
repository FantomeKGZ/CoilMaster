# Monotonic winding job IDs

## Purpose

The ESP32 UART delivery layer now rejects a winding job when its `job_id` is not greater than the last job ID queued during the current ESP32 runtime.

This prevents an old delayed `JOB_ACK` frame from being mistaken for the acknowledgement of a newly queued job that reused the same identifier.

## Queue rule

`UartEventReceiver::queueJob()` accepts a job only when all existing validation rules pass and:

```text
new_job_id > last_queued_job_id
```

The first valid job after ESP32 startup may use any non-zero `job_id`.

After a job is accepted into the local delivery queue, its ID becomes the new lower bound even when delivery later ends as:

- `Accepted`
- `Rejected`
- `TimedOut`
- `Cancelled`

A failed call to `queueJob()` does not change the stored last ID.

## Safety effect

This is a message-correlation guard only. It does not start the motor, activate the SSR, or replace the physical START/A confirmation required by the Arduino for each coil.

## Restart behavior

The monotonic counter is held in RAM and resets when the ESP32 restarts. The higher-level job creator should still generate unique IDs for each ESP32 session.
