# Engineering Session Notes: MicroSD External Configuration Implementation

**Date:** October 30 – November 8, 2026  
**Project:** `ckt-defect` (Talking Defect Detector)  
**Topic:** MicroSD External Configuration File Parser (`0:config.txt`)

---

## 1. Executive Summary

Implemented an SD card configuration file reader that allows overriding messages, defect rates, track/direction/rail names, and vocabulary selection at boot from `0:config.txt`.

* **Fail-Fast Error Handling:** Halts on the first syntax/type/bounds error. Freezes on the 20x4 LCD with line number and diagnostic message, providing a Button 4 `REBOOT` action.
* **Success Banner:** Displays a 2.0-second `"External Config Loaded"` banner when parsed successfully.
* **Non-Overwriting / Volatile Protection:** Volatile presence flags prevent NVM overwrites (`saveConfiguration()` will not persist SD overrides to NVM flash), and prevent ID-based name functions from overwriting loaded strings.
* **All-or-Nothing Message Defaults:** Defining $\ge 1$ message skips default message generation, leaving unprovided messages silent/empty.
* **Dynamic Menu Visibility:** Automatically hides menus associated with any setting overridden by `config.txt`.

---

## 2. File Modification & Creation Inventory

### A. New Files Created
* [`src/ckt-defect/config-file.h`](file:///home/data/design/projects/ckt-defect/src/ckt-defect/config-file.h): Public interface for parser (`ConfigFileParser`), result struct (`ConfigFileResult`), stream parsing helpers, and embedded target hooks (`loadExternalConfigFile`, `freezeOnError`).
* [`src/ckt-defect/config-file.cpp`](file:///home/data/design/projects/ckt-defect/src/ckt-defect/config-file.cpp): Complete parser engine:
  * Case-insensitive key and `[defect]` section matching.
  * Inline comment stripping (`;`), line-comment stripping (`#`, `;`), whitespace trimming.
  * Canonical keys and aliases (`trackName1`/`trackName2` and `trackNameA`/`trackNameB`).
  * Strict validation: character filter (`[a-zA-Z0-9 _\-:\.\/\\=,#\?!]`), 16-char string limits for names, integer bounds (`axleRate` 1..999,999; `integrityTrainRate` 1..999,999).
  * Strict 4-key defect state machine (`alertMsg`, `displayMsg`, `detailMsg`, `axleRate`).
  * 20x4 LCD `freezeOnError()` with watchdog servicing and `REBOOT` key handler.
  * `loadExternalConfigFile()` with 2-second splash screen.
* [`src/test/config/test_config.cpp`](file:///home/data/design/projects/ckt-defect/src/test/config/test_config.cpp): 73 comprehensive assertions covering valid parsing, aliases, bounds, comments, and error conditions.
* [`src/test/config/run`](file:///home/data/design/projects/ckt-defect/src/test/config/run): Test execution script for host-native compilation with `g++ -std=gnu++2b`.
* [`doc/external-config/external_configuration_plan.md`](file:///home/data/design/projects/ckt-defect/doc/external-config/external_configuration_plan.md): Architectural specification document.
* [`doc/external-config/generate_pdf.py`](file:///home/data/design/projects/ckt-defect/doc/external-config/generate_pdf.py): ReportLab PDF generation script.
* [`doc/external-config/external_configuration_plan.pdf`](file:///home/data/design/projects/ckt-defect/doc/external-config/external_configuration_plan.pdf): 3-page publication-grade reference PDF.

### B. Modified Existing Files
* [`src/ckt-defect/configuration.h`](file:///home/data/design/projects/ckt-defect/src/ckt-defect/configuration.h):
  * Added volatile flags to `DetectorConfiguration`: `externalConfigPresent`, `externalMessagesPresent`, `externalTrackNamePresent[NUM_TRACKS]`, `externalDirection1NamePresent`, `externalDirection2NamePresent`, `externalRailNamePresent`, `externalVocabSelectedPresent`, `externalIntegrityTrainRatePresent` (marked `// Volatile value, not stored in NVM`).
* [`src/ckt-defect/configuration.cpp`](file:///home/data/design/projects/ckt-defect/src/ckt-defect/configuration.cpp):
  * Initialized volatile flags to `false` in `loadConfiguration()`.
  * Guarded `saveConfiguration()` to prevent saving `vocabSelected` to NVM when overridden from `config.txt`.
  * Guarded `updateTrackNames()`, `updateDirectionNames()`, and `updateRailNames()` to preserve strings loaded from `config.txt`.
* [`src/ckt-defect/messages.h`](file:///home/data/design/projects/ckt-defect/src/ckt-defect/messages.h):
  * Verified struct definitions; kept primitive fields plain without redundant member initializers.
* [`src/ckt-defect/messages.cpp`](file:///home/data/design/projects/ckt-defect/src/ckt-defect/messages.cpp):
  * Updated `updateMessages()` to initialize `integrityTrainRate` from `cfg.defectIntegrityTrainRate` when not externally set.
  * Skipped `setDefaultMessages()` when `cfg.externalMessagesPresent` is true.
* [`src/ckt-defect/common.h`](file:///home/data/design/projects/ckt-defect/src/ckt-defect/common.h):
  * Forward-declared `class Display;`.
  * Declared `void restartWithDelay(uint32_t delay_ms, Display* disp = nullptr);`.
* [`src/ckt-defect/common.cpp`](file:///home/data/design/projects/ckt-defect/src/ckt-defect/common.cpp):
  * Implemented `restartWithDelay()` inside `#if defined(ESP_PLATFORM) || defined(ARDUINO) // Excluded from native host unit tests (e.g. test_messages)`.
* [`src/ckt-defect/menu-custom.h`](file:///home/data/design/projects/ckt-defect/src/ckt-defect/menu-custom.h) & [`src/ckt-defect/menu-custom.cpp`](file:///home/data/design/projects/ckt-defect/src/ckt-defect/menu-custom.cpp):
  * Removed local declaration and implementation of `restartWithDelay()`.
* [`src/ckt-defect/menu-factory.cpp`](file:///home/data/design/projects/ckt-defect/src/ckt-defect/menu-factory.cpp):
  * Promoted menu pointers to file scope.
  * Updated `updateAllMenuVisibility()` to hide menus overridden by `config.txt`.
* [`src/ckt-defect/state-machine.cpp`](file:///home/data/design/projects/ckt-defect/src/ckt-defect/state-machine.cpp):
  * Guarded `PROBABILITY_MAX` rate divisions using ternary `(rate == 0 ? 1 : rate)` for both integrity train rate and axle defect rates.
  * Prevents division-by-zero panics and avoids C++ template deduction conflicts between `unsigned int` and Xtensa's `uint32_t` (`unsigned long`).
* [`src/ckt-defect/ckt-defect.ino`](file:///home/data/design/projects/ckt-defect/src/ckt-defect/ckt-defect.ino):
  * Mounted FatFs SD filesystem early in `loop()`/`setup()` flow.
  * Calls `loadExternalConfigFile("config.txt", cfg, trackMessages, lcd)`.
  * Removed obsolete legacy string parsing functions and commented-out config block.
  * Cleaned up redundant secondary calls to `update*Names()`.

---

## 3. Key Design Decisions & Bug Fixes Resolved

1. **`restartWithDelay` Relocation (Option 1):**
   * *Issue:* `config-file.cpp` runs at early boot and needed `restartWithDelay()`, forcing an unnatural `#include "menu-custom.h"`.
   * *Resolution:* Moved `restartWithDelay` to `common.h` / `common.cpp` with a forward declaration `class Display;`. Wrapped implementation in `#if defined(ESP_PLATFORM) || defined(ARDUINO)` so native Linux unit tests (`test_messages`) compile without FreeRTOS/ESP-IDF mocks.
2. **Xtensa Toolchain Template Deduction Bug:**
   * *Issue:* `std::max(1U, msgs->integrityTrainRate)` failed on `arduino-cli compile` because Xtensa GCC defines `uint32_t` as `unsigned long`, conflicting with `1U` (`unsigned int`).
   * *Resolution:* Changed to ternary `(msgs->integrityTrainRate == 0 ? 1 : msgs->integrityTrainRate)` and `(msgs->defects[i].axleRate == 0 ? 1 : msgs->defects[i].axleRate)`.
3. **`integrityTrainRate` 6-Digit Limit:**
   * *Issue:* Initial documentation incorrectly stated `integrityTrainRate` was limited to `1..1,000`.
   * *Resolution:* Checked `menu-factory.cpp` and found `menuRandomIntegrityRate` is a 6-digit thumbwheel (`6, 0, true`). Corrected the range in `config-file.cpp`, unit tests, specification markdown, and PDF to `1 .. 999,999`.
4. **Redundant Startup Calls Cleaned:**
   * *Issue:* `updateTrackNames`, `updateDirectionNames`, `updateRailNames` were called again after `loadExternalConfigFile`.
   * *Resolution:* Because names are already loaded from NVM earlier, and `loadExternalConfigFile` populates strings directly, the second call was a no-op and was removed.

---

## 4. Verification & Testing Reference

### Running Host-Native Unit Tests
From repo root:
```bash
# 1. Config file parser tests (73 assertions)
cd src/test/config && ./run

# 2. Message transformation tests (27 assertions)
cd src/test/messages && ./run

# 3. NVM Codec coverage and roundtrip tests
cd src && python3 tools/gen_codec.py --check
cd src && python3 test/test_codec_roundtrip.py
```

### Arduino CLI Firmware Build
From `src/`:
```bash
arduino-cli compile -b esp32:esp32:esp32s2:CDCOnBoot=cdc,UploadMode=cdc,PSRAM=enabled \
  --build-path ./build \
  --build-property upload.maximum_size=4128768 \
  --build-property compiler.cpp.extra_flags='-DGIT_REV="b78fe9" -DVERSION_STRING="v0.2.3+"' \
  --build-property compiler.optimization_flags=-O2 \
  --libraries ckt-defect/menu/ ckt-defect \
  --warnings all
```

---

## 5. Next Steps for Next Session

1. **Bench Testing with MicroSD Card:**
   * Test with valid `config.txt` to verify the 2-second `"External Config Loaded"` banner.
   * Test syntax errors, missing `=`, illegal characters, and out-of-range rates to verify the 20x4 LCD error screen and Button 4 `REBOOT` action.
   * Verify that overridden menus (`Defects`, `Include*`, `Track Names`, `Vocab Select`, etc.) are hidden.
2. **Upcoming Workspace Items from `notes.txt`:**
   * Detect SD card removal and reboot.
   * Train length calculation.
