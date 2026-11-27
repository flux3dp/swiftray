# BeamGo USB backend

| Field | Value |
|---|---|
| Status | Initial implementation complete; full application build and hardware acceptance pending |
| Author | Codex |
| Created | 2026-11-27 |
| Target product | Swiftray daemon / Beam Studio desktop |
| Owner area | USB transport, BeamGo compiler, discovery and device actions |
| Related code | `src/server/swiftray-server.{h,cpp}`; `src/periph/motion_controller/`; `src/toolpath_exporter/generators/gcode-generator.h` |
| External repos | `flux3dp/beam-studio` |
| Companion PRD | `../beam-studio/docs/prd/beamgo-usb.md` (requirements R1–R8 and WebSocket contract) |

## Summary

Implement the HY-LASER USB protocol for BeamGo without applying BSL IO/calibration operations to it. Consume the existing Promark G-code dialect at the device boundary, including M100 as a successful no-op.

## Background & current state

`src/server/swiftray-server.cpp` creates a single BSL Machine and casts its controller for Promark actions. `src/periph/motion_controller/motion_controller.cpp` reads serial input as newline-delimited text. `src/toolpath_exporter/toolpath-exporter.cpp::convertStack` emits `disableRotary()` on ordinary planar jobs; `gcode-generator.h::disableRotary` emits M100. `dirty-area-outline-generator.h::toString` also emits M100 for non-rotary framing. `bsl_motion_controller.cpp::handleGcode` uses M100 to disable rotary IO, so rejecting it would reject normal shared output.

## Goals & non-goals

Implement codec, record compiler, asynchronous single-thread serial transport, discovery, per-device actions and machine-stored settings. Reuse SVG geometry conversion. No OTA, Bluetooth, camera, rotary/Z, wobble or inner engraving on BeamGo. Initial images use line segments, not hardware dot/bitset encoding.

## Scenarios

Discover a supported CDC device, read identity/calibration/safety, accept a job or framing request, stream/execute/control it, and expose state/progress/errors over the existing WebSocket envelope. Project-file upload is separate from job execution.

## Acceptance criteria

The companion PRD's R1–R8 are the end-to-end criteria. Hardware-free checks must additionally reproduce every host frame and job fixture; split/corrupt RX frames must never complete the wrong command. M100 must complete without USB output; unsupported motion and malformed data must fail before emission. Framing 5/15/30% must produce record 0x12 values 50/150/300. Invalid framing power returns INVALID_PARAMS without starting.

## Must not change

- `src/server/worker.cpp` — `/parser/convert` callers: existing Promark G-code generation keeps its BSL origin and rotary settings; BeamGo uses a zero origin and no rotary independently of an attached Promark.

- `src/server/swiftray-server.{h,cpp}` — Beam Studio `/devices` caller: preserve existing Promark discovery fields and add BeamGo entries.
- `src/server/swiftray-server.{h,cpp}` — Beam Studio `/devices/BSL` callers: preserve all Promark lifecycle, atomic start/lens, framing, error and button behavior.
- `src/server/swiftray-server.{h,cpp}` — `/parser`, `/segment`, `/ws/sr/system` callers: preserve routing, worker ownership, callbacks, progress/chunks and disconnect handling.
- `src/server/CMakeLists.txt` / `src/periph/motion_controller/CMakeLists.txt` — root platform builds: retain all existing sources and link requirements; new protocol tests must also build independently of the desktop GUI.
- Shared G-code generator and BSL controller are references, not targets for M100 changes. Extend this list before touching additional shared files.

## Design

Use a pure C++ protocol/compiler layer, an event-driven Qt serial transport and device-specific command dispatch. All QSerialPort IO stays on its owning Qt thread. Parse/validate jobs before USB upload. Keep each device's buffers, pending command and operation state separate. Reuse the server's request/callback envelopes and existing SVG conversion; isolate BeamGo binary protocol behavior from BSL's executor/DLL path.

Never retry ambiguous uploads or starts. Mandatory read failures make the device unavailable; optional capability reads may report unsupported. Abort on safety errors; require fresh reads before starting/resuming/framing. Pause is host-maintained, completion comes from 49 45. Calibration is read/modify/write/readback with full-byte backup; preserve unknown trailing bytes and all unedited values.

## Data contracts

See the companion PRD. `startFraming` accepts optional integer `framingPower` (1–100, default 15) and width 90. Validate before device IO. USB uses fixed directional AES-128-CBC keys, IV 0x01 ×16, PKCS#7, asymmetric TX/RX framing and CRC-16/CCITT-FALSE. Constants/fixtures come from the 2026-11-27 handoff.

## Performance/packaging

One pending command per device, at most 1000 records/block, 3000-record initial start and 10000-record outstanding window. Priority stop/pause between frames. No nested event loops or blocking waits in the server thread. Protocol tests must be runnable without Qt; full daemon compile/transport validation requires Qt6 SerialPort/WebSockets.

## Rollout (PR plan)

Protocol/compiler → serial/scanner/actions → frontend integration → simulator/regression/build → hardware acceptance.

## Open decisions

1. ~~M100~~ — 2026-11-27 user approved no-op in the BeamGo controller.
2. Provisionally stop on door/tilt; confirm hardware behavior before release.
3. Field must match 90 mm, without automatic correction. Laser-type field is not authoritative.
4. ~~Crypto implementation~~ — 2026-11-27: native CommonCrypto on macOS and BCrypt on Windows; OpenSSL only for standalone tests on other platforms. No vendored AES implementation. Cross-platform fixtures verify directional keys, CBC, padding and CRC; Windows build verification remains pending.

## Risks & follow-ups

Hardware has not validated fill/images, real dimensions/direction, cable removal during work or door-switch effects. Simulator success does not close these cases. The BeamGo library, shared GCodeGenerator test and serial-controller test driver compile with isolated Qt 6.7.2 under `/tmp/beamgo-qt`; Qt was not installed system-wide. The complete Swiftray application and Windows BCrypt path still require a build on the test computer. See `tests/beamgo/README.md`.

## Test plan

Exact handoff vectors; RX fragmentation/noise/corruption; G-code modal behavior and unsupported commands; streaming/cancellation/timeouts; calibration byte preservation; safety masks; discovery and WS routing; Promark regression; hardware R1–R8 in the companion PRD.


## Implementation verification (2026-11-27)

Branch `codex/beamgo-usb` starts at `origin/develop` commit `bf70a8f15ab64682be1d7928f4af97726d0feadb`.
The isolated CMake build compiles codec/compiler, transport/controller/manager and the shared GCodeGenerator test with Qt 6.7.2. CTest protocol/compiler (671 cases plus full-block hash), generator integration and real Qt serial-controller simulator all pass. The simulator includes delayed physical idle after stop ACK and rejects a simultaneous restart until idle. No hardware was contacted. Full Swiftray application build, Windows BCrypt verification and R1–R8 hardware acceptance are pending; use `tests/beamgo/README.md` on the test computer.
