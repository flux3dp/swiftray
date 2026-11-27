# BeamGo development and acceptance

Both repositories use branch `codex/beamgo-usb`. Starting commits:

- Beam Studio `origin/main`: `28457552edb4e96d69ab4489c4597ecaf0bc6fb9`
- Swiftray `origin/develop`: `bf70a8f15ab64682be1d7928f4af97726d0feadb`

## Build the isolated backend tests

The protocol/compiler test needs CMake, a C++17 compiler and Python 3, without Qt.
macOS uses CommonCrypto, Windows uses BCrypt, and Linux needs OpenSSL development libraries.

```sh
cmake -S tests/beamgo -B build-beamgo-tests
cmake --build build-beamgo-tests --config Debug
ctest --test-dir build-beamgo-tests -C Debug --output-on-failure
```

To also compile the real serial controller, scanner and shared GCodeGenerator integration test,
provide Qt 6.7.2 with Core, Gui, Network and SerialPort. For example, on macOS:

```sh
cmake -S tests/beamgo -B build-beamgo-tests -DCMAKE_PREFIX_PATH=/path/to/Qt/6.7.2/macos
cmake --build build-beamgo-tests --config Debug
ctest --test-dir build-beamgo-tests -C Debug --output-on-failure
```

The controller test uses a local socket, a pseudo-terminal and the `openssl` command, and runs on
macOS/Linux only. It never scans USB hardware. The driver injects 115200 baud because macOS
pseudo-terminals do not implement the custom-speed ioctl; production remains 921600 baud.
Windows runs the crypto/compiler and shared-generator tests; test physical serial IO separately.

## Full application on the test computer

1. Use the paired Beam Studio and Swiftray changes. Initialize Swiftray submodules and build the
   normal application with the repository's platform dependencies (see root `AGENTS.md`). The isolated
   test target does not produce the Swiftray daemon.
2. On macOS, pass your Qt path with `-DCMAKE_PREFIX_PATH=/path/to/Qt/6.7.2/macos` when configuring the
   root project. On Windows use the existing Visual Studio/vcpkg build with Qt 6.7.2. Deploy the Qt
   runtime, including SerialPort, with the normal bundle / `windeployqt` process.
3. In Beam Studio run `pnpm install --frozen-lockfile`, then `pnpm nx run app:dev` and, in a separate
   terminal, `pnpm nx run app:start`. Qt is unnecessary for Beam Studio itself.
4. Use the newly built daemon, not the bundled older backend. Beam Studio's
   `apps/app/src/node/bootstrap.ts` sets `BACKEND_ROOT` to `backend` in the app working directory
   during development (`apps/app/backend` when using the commands above), or to the packaged
   resources directory's `backend` folder. `helpers/swiftrayProcess.ts::getSwiftrayPaths` expects
   `Swiftray.app/Contents/MacOS/Swiftray` on macOS or `swiftray/Swiftray.exe` on Windows. Preserve the
   runtime libraries alongside the executable, back up the previous bundle before replacing it,
   and restart Beam Studio after deployment. Check the backend startup log for the actual path.

## Hardware acceptance (pending)

Follow the paired Beam Studio PRD R1–R8 and record OS, firmware, wattage and measured results.
The simulator is not evidence that hardware acceptance passed.

1. Add BeamGo via USB; verify 90 × 90 mm, USB icon, identity/firmware, and the three menu entries.
   Confirm a simultaneously connected Promark is listed and still uses its own controller.
2. Read calibration before changing it. Save one scale value, reconnect and read it back; restore it.
   Verify the full register length and unchanged trailing bytes. Toggle safety settings and confirm
   readback preserves all unrelated mask bits. A field other than 90 mm must reject start/framing.
3. Use a 20 mm square at x/y 35–55 mm, 15%, 100 mm/s. Measure dimensions and orientation. Test
   preview 5%, 15%, 30% from both the framing dialog and Monitor. Preview emits laser light; the UI
   warning and power setting must be visible. Stop before changing power or starting a job.
4. Run a sufficiently long job to test pause/resume and stop. Check that no lines repeat or disappear,
   progress reaches completion only after the completion report, and another job can start after stop.
5. Test the door/tilt events with the physical enclosure and normal operator precautions. Verify the
   error is shown, execution stops, and clearing the event never resumes automatically. Test unplug
   while idle and during work; reconnect must not restart the previous job.
6. Verify vector fill and a black/white image containing white gaps, both scan directions and several
   rows. The initial implementation uses line segments, not dot or bitset encoding. Confirm no marks
   cross the white gaps. Check the selected 10/20 W preset table and fresh-layer defaults.
7. Verify no camera, rotary, Z, pulse-width, frequency or inner-engraving controls are exposed for
   BeamGo. Recheck those features on their existing supported models.

Protocol fixtures in `test-vectors.json` come from the 2026-11-27 USB handoff. The protocol test checks
671 exact-frame/fragmentation/compiler cases plus a full-block hash. The controller simulator covers
read-only discovery, framing power, calibration preservation/restore, safety flags, lifecycle,
streaming, external work, invalid-upload clearing, project packets and ambiguous-timeout no-retry.
