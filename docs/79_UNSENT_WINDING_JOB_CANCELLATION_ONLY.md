# Cancellation is allowed only before the first winding job transmission

## Problem

`UartEventReceiver::cancelPendingJob()` is a local ESP32 queue operation. It does not send a remote cancellation command to Arduino.

After the first job frame has been transmitted, Arduino may already have accepted the job even if the corresponding `JOB_ACK` frame was lost or delayed. Reporting that job as cancelled on ESP32 would therefore create a false safety state: the web or control layer could show cancellation while Arduino still owns the job.

## Rule

A queued winding job can be cancelled locally only while it is still unsent:

- `m_hasPendingJob == true`;
- no unconsumed delivery result exists;
- `m_waitingJobAck == false`;
- `m_jobSendAttempts == 0`.

Once at least one transmission attempt has occurred, `cancelPendingJob()` returns `false` and preserves the pending delivery state.

## Behaviour

Successful pre-send cancellation still publishes:

- `JobDeliveryResult::Cancelled`;
- the queued `jobId`;
- `sendAttempts = 0`;
- the validated cancellation detail.

After transmission, the caller must wait for one of the authoritative delivery outcomes:

- `Accepted`;
- `Rejected`;
- `TimedOut`.

## Safety boundary

This change does not implement remote stopping or emergency stopping of Arduino. Motor and SSR safety remain under Arduino control and the physical controls. A future remote cancellation protocol would require a separate CRC-protected command, explicit Arduino acknowledgement, and state-machine rules that prevent unsafe SSR activation.