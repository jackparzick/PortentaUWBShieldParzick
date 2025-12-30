# Portenta UWB Shield Library — Fork / Research Notes

This repository is a **personal fork** of the Arduino/Truesense Portenta UWB Shield library.

I **did not create the original library**. Credit for the upstream implementation belongs to the original authors and maintainers (see “Upstream / Credit” below). This fork exists to document what I’m learning, run experiments safely, and make targeted improvements for my specific hardware/workflow.

---

## Upstream / Credit

- Original project: Arduino Portenta UWB Shield library (by Truesense Srl / Arduino ecosystem)
- This fork is **not affiliated with** or endorsed by Arduino or Truesense.
- Upstream licensing and notices apply—see `LICENSE` (and any upstream notice files) included in this repository.

---

## What this fork is for

This fork focuses on practical engineering goals like:

- **Clear documentation** of how the shield behaves (hardware signals, session lifecycles, expected outputs, failure modes)
- **Better diagnostics** (logging, structured status checks, reproducible test steps)
- **Wrapper/API ergonomics** (small helpers that reduce “magic numbers” in sketches)
- **Robustness work** (bounded stop/start behavior, safer callbacks, multi-peer/session hygiene)
- **Feature exploration** where supported (ranging modes, session configuration, vendor TLVs, and other capabilities exposed by the underlying stack)

> Important: some capabilities are implemented in **precompiled vendor components** shipped with the original library distribution. This fork aims to *use and expose* what’s already there—without guessing beyond evidence.

---

## What this fork is NOT

- Not a rewrite of the underlying UWB firmware/stack
- Not guaranteed to be stable for everyone’s use case
- Not an “official” replacement for upstream
- Not a promise that every experimental feature will work on every board/firmware revision

If you need a stable baseline, use upstream first.

---

## Hardware / Requirements

- Arduino **Portenta UWB Shield** (ASX00074 / “Stella UWB Shield” / DCU150/SR150-based module)
- A compatible **Portenta host board** (this repo may assume a specific Portenta variant in examples—check the docs/examples notes)
- Arduino IDE (or CLI) with the appropriate board package installed

---

## Installation

### Option A — Arduino IDE (local library folder)
1. Download/clone this repo into your Arduino libraries directory:
   - macOS/Linux: `~/Documents/Arduino/libraries/`
   - Windows: `Documents\Arduino\libraries\`
2. Restart the Arduino IDE.
3. Verify it appears under **File → Examples**.

### Option B — Git workflow
- Keep this as a git clone and symlink/copy into the Arduino libraries directory.
- Use branches for experiments and keep `main` close to a “known good” state.

---

## Getting Started

1. Start with the **baseline ranging examples** first (distance must work before you chase advanced behaviors).
2. Confirm:
   - you can start/stop a session repeatedly,
   - you receive consistent ranging notifications,
   - your serial logs are readable and stable.

If something fails early, treat it as a “bring-up” issue (power/transport/session init) rather than a feature issue.

---

## Repository layout (typical)

- `src/`  
  Arduino-facing wrapper, session helpers, notification dispatch, utilities
- `examples/`  
  Demonstrations (baseline + any diagnostic/experimental sketches)
- `docs/`  
  Living notes (bring-up checklists, test plans, constraints, findings)
- `tools/` (optional)  
  Scripts used for analysis (symbol listings, disassembly notes, etc.)

---

## Development workflow (how I’m working)

- Changes are made in **small, testable steps**
- Experiments should live behind:
  - a dedicated example sketch, and/or
  - compile-time flags, and/or
  - a clearly documented “how to test / how to revert”
- When behavior depends on a vendor binary or firmware:
  - I document the evidence (strings/disassembly/logs) before changing wrapper code

---

## Reporting issues / debugging help

If you open an issue (even for yourself), include:
- host board + shield revision (if known)
- Arduino IDE + board package versions
- full serial log from boot → failure
- which example sketch was used
- what you expected vs what happened

---

## License / Redistribution note

Upstream is MIT-licensed (see `LICENSE`).  
Some builds may rely on **precompiled vendor libraries** distributed with the original project/package. If you redistribute this fork publicly, double-check:
- what binary components (if any) are included,
- and whether upstream’s distribution terms apply to those pieces.

---

## Status

Active work-in-progress fork.  
Goal is to keep the basics solid (ranging + session lifecycle), while improving documentation, diagnostics, and safe feature exposure over time.

