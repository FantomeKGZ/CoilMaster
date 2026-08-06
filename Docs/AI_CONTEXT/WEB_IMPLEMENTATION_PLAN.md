# CoilMaster OS — Web Implementation Plan

Status: `PLANNED`

Priority: `P1`

Purpose: define the required web-interface and handbook structure stored on the ESP32 microSD card.

## 1. General model

The ESP32 runs one HTTP server and serves the web interface from the microSD card.

The system contains two logical web applications:

1. Main CoilMaster interface.
2. CoilMaster handbook.

The handbook is a separate logical application and directory, but it remains part of the same CoilMaster web system and uses the same ESP32 HTTP server.

## 2. Required interface variants

Both the main site and the handbook must have two separately stored interface variants:

- desktop / PC version;
- mobile version.

The desktop and mobile variants may share data, API endpoints, images and common libraries, but their entry pages and interface assets must be separately addressable.

## 3. Mandatory navigation rule

The selected interface type must be preserved when the user opens the handbook.

Required behaviour:

- a user working in the desktop main interface must open the desktop handbook directory;
- a user working in the mobile main interface must open the mobile handbook directory;
- returning from the handbook must return to the corresponding main-interface variant;
- navigation must not unexpectedly switch between mobile and desktop layouts.

Examples:

```text
Main desktop interface
/web/desktop/
        |
        +-- Handbook --> /handbook/desktop/

Main mobile interface
/web/mobile/
        |
        +-- Handbook --> /handbook/mobile/
```

## 4. Proposed microSD directory structure

```text
SD/
├── web/
│   ├── desktop/
│   │   ├── index.html
│   │   ├── css/
│   │   ├── js/
│   │   └── assets/
│   │
│   ├── mobile/
│   │   ├── index.html
│   │   ├── css/
│   │   ├── js/
│   │   └── assets/
│   │
│   └── shared/
│       ├── js/
│       ├── css/
│       └── assets/
│
├── handbook/
│   ├── desktop/
│   │   ├── index.html
│   │   ├── css/
│   │   ├── js/
│   │   └── assets/
│   │
│   ├── mobile/
│   │   ├── index.html
│   │   ├── css/
│   │   ├── js/
│   │   └── assets/
│   │
│   ├── shared/
│   │   ├── articles/
│   │   ├── tables/
│   │   ├── images/
│   │   ├── schemas/
│   │   └── data/
│   │
│   └── manifest.json
│
├── database/
├── config/
├── logs/
├── backup/
└── updates/
```

The final names may change only after an explicit architecture decision. The requirement for separate mobile and desktop handbook entry directories must remain.

## 5. Routing requirements

The ESP32 HTTP layer must provide explicit routes for both variants.

Minimum planned routes:

```text
/                         -> interface selection or controlled redirect
/web/desktop/             -> desktop main interface
/web/mobile/              -> mobile main interface
/handbook/desktop/        -> desktop handbook
/handbook/mobile/         -> mobile handbook
/api/...                  -> shared application API
```

Possible aliases:

```text
/desktop/                 -> /web/desktop/
/mobile/                  -> /web/mobile/
/desktop/handbook/        -> /handbook/desktop/
/mobile/handbook/         -> /handbook/mobile/
```

The final public URL scheme must be approved before implementation.

## 6. Interface selection

The initial interface variant may be selected using one or more of the following mechanisms:

1. Explicit user selection.
2. Saved preference in browser storage.
3. Device-width or user-agent suggestion.
4. URL selected directly by the user.

Automatic detection must be treated as a suggestion, not as an irreversible choice. The user must be able to switch manually between mobile and desktop variants.

When switching variant:

- main desktop -> main mobile;
- handbook desktop -> handbook mobile;
- main mobile -> main desktop;
- handbook mobile -> handbook desktop.

The current logical section should be preserved where possible.

## 7. Shared and separate content

The desktop and mobile handbook interfaces should not duplicate the actual handbook knowledge base unnecessarily.

Recommended model:

- separate desktop and mobile presentation layers;
- common article, table, image and calculation data;
- common API;
- common versioned handbook manifest;
- separate CSS and layout scripts where needed.

This allows the same handbook content to be displayed differently on a phone and a PC without maintaining two independent copies of every article.

## 8. Main-site functions

Planned main interface functions:

- system status;
- winding state and progress;
- motor database;
- job history;
- settings;
- logs;
- diagnostics;
- backup and restore;
- wire inventory;
- winding recalculation tools;
- transition to the corresponding handbook version.

The web interface must not directly control physical hardware. Commands must pass through approved ESP32 application services and CMP to Arduino UNO.

## 9. Handbook functions

Planned handbook content:

- electric motor winding formulas;
- winding diagrams;
- wire tables;
- material properties;
- repair procedures;
- diagnostic instructions;
- aluminium-to-copper recalculation guidance;
- stock-aware wire selection;
- searchable articles;
- links to related motor records and calculation tools.

The handbook should remain usable when Arduino UNO is unavailable, provided that ESP32 and microSD are operating.

## 10. Implementation tasks

### P1 — Approve URL and directory structure

Status: `PLANNED`

- approve desktop and mobile directory names;
- approve public routes;
- define fallback and 404 behaviour;
- define default entry route;
- define manual interface switch behaviour.

### P1 — Implement microSD static-file server

Status: `PLANNED`

- initialize microSD before web services;
- map URL paths to approved SD directories;
- send correct MIME types;
- support index files;
- reject unsafe path traversal;
- provide controlled error pages;
- log missing and unreadable assets.

### P1 — Create desktop main interface

Status: `PLANNED`

- create `/web/desktop/index.html`;
- desktop navigation;
- desktop status and data views;
- link to `/handbook/desktop/`.

### P1 — Create mobile main interface

Status: `PLANNED`

- create `/web/mobile/index.html`;
- touch-friendly navigation;
- compact status and data views;
- link to `/handbook/mobile/`.

### P1 — Create desktop handbook interface

Status: `PLANNED`

- create `/handbook/desktop/index.html`;
- desktop search and article navigation;
- return link to `/web/desktop/`.

### P1 — Create mobile handbook interface

Status: `PLANNED`

- create `/handbook/mobile/index.html`;
- touch-friendly search and article navigation;
- return link to `/web/mobile/`.

### P1 — Create shared handbook data model

Status: `PLANNED`

- define article identifiers;
- define categories and tags;
- define table and image references;
- define content versioning;
- define search index format;
- avoid unnecessary duplication between desktop and mobile content.

### P1 — Implement variant-preserving navigation

Status: `PLANNED`

Acceptance criteria:

- desktop main site opens desktop handbook;
- mobile main site opens mobile handbook;
- desktop handbook returns to desktop main site;
- mobile handbook returns to mobile main site;
- manual variant switching is available;
- no accidental layout change during normal navigation.

### P2 — Add responsive fallback

Status: `PLANNED`

Each version should remain readable if opened on an unexpected screen size, but this does not replace the required separate mobile and desktop entry directories.

### P2 — Add offline-safe handbook behaviour

Status: `PLANNED`

- handbook assets served locally from microSD;
- no mandatory external CDN;
- no internet dependency for core articles;
- clear error handling if a local resource is missing.

## 11. Validation checklist

The feature is not complete until all checks pass:

- desktop main interface loads from microSD;
- mobile main interface loads from microSD;
- desktop handbook loads from its own directory;
- mobile handbook loads from its own directory;
- navigation preserves the selected variant;
- shared handbook content renders correctly in both variants;
- all core resources work without internet access;
- invalid paths cannot escape approved SD directories;
- ESP32 remains responsive while serving files;
- web access does not bypass application services or CMP;
- documentation, implementation log and session log are updated.

## 12. Current implementation status

As of 2026-08-07:

- requirement documented: `YES`;
- directory structure approved: `NO`;
- ESP32 HTTP server implemented: `NO`;
- static files served from microSD: `NO`;
- desktop main site implemented: `NO`;
- mobile main site implemented: `NO`;
- desktop handbook implemented: `NO`;
- mobile handbook implemented: `NO`;
- variant-preserving navigation implemented: `NO`.
