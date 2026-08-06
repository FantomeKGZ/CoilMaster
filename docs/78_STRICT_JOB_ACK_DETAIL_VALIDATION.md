# Strict winding job acknowledgement detail validation

ESP32 now validates the optional detail field of `CMP1|JOB_ACK` frames before completing job delivery.

## Accepted format

A detail token may contain only:

- uppercase ASCII letters `A-Z`;
- digits `0-9`;
- underscore `_`;
- hyphen `-`.

The maximum length is `JobDeliveryEvent::MaxDetailLength` (23 characters).

Examples:

- `ACCEPTED`
- `BUSY`
- `INVALID_PROGRAM`
- `SESSION-MISMATCH`

## Rejected format

The frame is ignored and the pending job remains active when the detail:

- exceeds 23 characters;
- contains whitespace;
- contains lowercase letters;
- contains separators or control characters;
- is empty for a `REJECTED` acknowledgement.

This prevents silent truncation and ambiguous operator messages. A malformed acknowledgement cannot overwrite the pending job result or stop the bounded retry process.

The public UART frame layout and retry policy are unchanged.