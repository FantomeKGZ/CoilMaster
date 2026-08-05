# CoilMaster web monitoring dashboard

The ESP32 web root now serves the first persistent responsive CoilMaster dashboard instead of a temporary form page.

## Access

1. Connect a phone, tablet, or PC to the ESP32 access point.
2. SSID: `CoilMaster`
3. Password: `CoilMaster123`
4. Open `http://192.168.4.1/`.

## Implemented dashboard blocks

- machine state;
- completed full-program runs;
- last run identifier;
- microSD journal state;
- current remote job identifier;
- winding type: working or starting;
- active turns program;
- Arduino link freshness;
- remote job form;
- live status refresh through `GET /api/status`.

The layout adapts to desktop and mobile screens and already contains the target portal navigation entries:

- Home;
- Winding;
- Motors;
- Clients;
- History;
- Wire inventory;
- Settings.

Only the winding page is active at this stage. The other links are placeholders for later modules.

## Safety rule

Submitting a web job only transfers the winding program to Arduino Uno. It never enables the SSR. The operator must press keypad `A` or the external START button locally.

## Current progress limitation

The existing CMP event stream reports `RUN_STARTED` and `RUN_COMPLETED`. Therefore the dashboard can display job acceptance, run activity, completion count, and journal state.

Exact live values for:

- current coil;
- current turn count;
- target turn count;
- percentage;
- pause and coil-complete states;

require the next protocol extension: periodic Arduino telemetry. The dashboard is prepared to consume those fields once added.

## API status fields

`GET /api/status` currently returns:

- `job_id`;
- `session_id`;
- `job_status`;
- `machine_status`;
- `job_type`;
- `program`;
- `completed_runs`;
- `last_run_id`;
- `run_active`;
- `arduino_ack_pending`;
- `arduino_online`;
- `storage_ready`.
