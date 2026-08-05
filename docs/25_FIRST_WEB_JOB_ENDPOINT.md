# First Web Job Endpoint

ESP32 now starts a local Wi-Fi access point and exposes the first CoilMaster web page for sending a winding program to Arduino Uno.

## Wi-Fi access point

- SSID: `CoilMaster`
- Password: `CoilMaster123`
- Default address: `http://192.168.4.1/`

The address is also printed to the ESP32 USB Serial Monitor at 115200 baud.

## Web form

The start page contains:

- winding type: working or starting;
- coil turns entered with `/`, comma, semicolon, or spaces;
- a button to send the program to Arduino;
- the latest job delivery state.

Example input:

```text
140/100/80/40
```

The web request only transfers the program. Arduino does not start the SSR automatically. The operator must still press `A` on the keypad or the external START button.

## HTTP API

### Create a job

```http
POST /api/jobs
Content-Type: application/x-www-form-urlencoded

type=working&turns=140/100/80/40
```

Possible responses:

- `202` — queued and waiting for Arduino acknowledgement;
- `400` — missing or invalid turns;
- `409` — another job is already being delivered.

### Read current delivery status

```http
GET /api/status
```

Example response:

```json
{
  "job_id": 1,
  "job_status": "ACCEPTED_READY",
  "arduino_ack_pending": false
}
```

Possible status values:

- `IDLE`;
- `WAITING_ARDUINO_ACK`;
- `ACCEPTED_READY`;
- `REJECTED`.

## Safety rule

ESP32 is allowed to send job data, but it never directly enables the SSR. Final permission to start winding remains local to Arduino Uno.
