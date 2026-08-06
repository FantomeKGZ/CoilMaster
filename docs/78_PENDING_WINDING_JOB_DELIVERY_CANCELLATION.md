# Pending winding job delivery cancellation

## Purpose

ESP32 can now stop retrying a winding job that has not yet received a final `JOB_ACK` from Arduino.

```cpp
bool cancelPendingJob(const char* detail = "CANCELLED");
```

A successful cancellation publishes a normal `JobDeliveryEvent` with:

- `result = JobDeliveryResult::Cancelled`;
- the original `jobId`;
- the number of transmissions already attempted;
- a diagnostic detail string.

The result must be consumed through `takeJobDelivery()` before another job can be queued.

## Safety boundary

This operation cancels only ESP32 delivery retries. It is not a remote motor stop and does not control the SSR.

If Arduino has already accepted or started a winding job, machine motion remains governed by Arduino and the physical controls. The operator must use the local machine controls for stopping or cancelling actual winding.

## State rules

Cancellation succeeds only while a job is pending and no earlier delivery result is waiting to be consumed. On success ESP32 clears the retry timer and pending-delivery state without sending any additional frame to Arduino.
