# ENG-001 — CMP Core

## Dependency Matrix

Version: 1.0

Status: Approved

---

# Purpose

This document defines the dependency order for all CMP modules.

Implementation must follow this order.

No cyclic dependencies are allowed.

---

## Level 0

CMP_Defines

No dependencies.

---

CMP_Result

Depends on:

CMP_Defines

---

CMP_Flags

Depends on:

CMP_Defines

---

CMP_Command

Depends on:

CMP_Defines

---

## Level 1

CMP_Header

Depends on

CMP_Defines

CMP_Flags

---

CMP_Packet

Depends on

CMP_Header

CMP_Defines

---

CMP_CRC

Depends on

CMP_Defines

---

## Level 2

CMP_Buffer

Depends on

CMP_Defines

---

## Level 3

CMP_Parser

Depends on

CMP_Buffer

CMP_Packet

CMP_CRC

CMP_Result

---

## Level 4

CMP_Protocol

Depends on

CMP_Buffer

CMP_Parser

CMP_Packet

CMP_Command

CMP_Result

---

## Level 5

CMP_Dispatcher

Depends on

CMP_Protocol

CMP_Command

CMP_Result

---

# Dependency Graph

CMP_Defines

↓

CMP_Result
CMP_Flags
CMP_Command

↓

CMP_Header

↓

CMP_Packet

↓

CMP_CRC

↓

CMP_Buffer

↓

CMP_Parser

↓

CMP_Protocol

↓

CMP_Dispatcher

---

# Rules

No module may include a module located above itself.

No cyclic include chains are allowed.

Only public headers may be included by higher layers.

---

END
