# SD Card External Configuration Parser Architecture & Implementation Plan

> **Target Firmware:** ESP32-S2 Defect Detector (`ckt-defect`)  
> **Target File:** `config.txt` on FAT/FAT32 formatted MicroSD card  
> **Status:** Architectural Plan & Design Specification (Aligned & Finalized)  

---

## 1. Executive Summary & Core Requirements

The objective is to implement a robust, fail-safe configuration file parser that reads runtime customizations from an optional `config.txt` file located in the root directory of the MicroSD card. This enables end-users to customize voice templates, LCD defect alerts, track/direction names, and defect rates without recompiling firmware or using the physical LCD button menu.

### Core Behavioral Constraints:
1. **Presence Detection:** If `config.txt` is present on the mounted SD card, `cfg.externalConfigPresent` is set to `true`.
2. **Fail-Fast & Freeze on First Error:** Any syntax error, unknown key, invalid data type, out-of-range integer, or illegal character must immediately stop parsing. The LCD screen displays the error description and line number, freezing in place with a single **`REBOOT`** button on Button 4 (col 16) that triggers an ESP32 hardware reset via `restartWithDelay()`.
3. **Success Indication:** If `config.txt` parses completely with no errors, a brief (2-second) `"External Config Loaded"` banner is displayed on the LCD before proceeding.
4. **Case Handling:** Keys are strictly case-insensitive (`entrancemsg` == `entranceMsg`). String values preserve their case verbatim.
5. **Message Override & Menu Hiding:**
   - If any message key is defined (1 or more), built-in defaults are skipped for all messages. Unspecified messages remain empty (`""`), which intentionally silences any broadcasts that operators do not wish to play.
   - The entire `Defects` menu tree is hidden.
   - All `Include*` menus under `Messages` are hidden.
   - `Entrance Message`, `Live Defect Msgs`, and `Talk Defect Only` menus are hidden.
6. **Selective Hardware Setting Overrides:**
   - Overriding `trackName1` (or alias `trackNameA`) hides "Track A Name".
   - Overriding `trackName2` (or alias `trackNameB`) hides "Track B Name".
   - Overriding `direction1Name` or `direction2Name` hides "Direction 1 Name" or "Direction 2 Name".
   - Overriding `railName` hides "Rail Names".
   - Overriding `vocabSelected` hides "Vocab Select".
   - Overriding `integrityTrainRate` hides "Random Integrity Rate".
   - Functions like `updateTrackNames()`, `updateDirectionNames()`, and `updateRailNames()` that regenerate strings from NVM ID tables must NOT overwrite settings loaded from `config.txt`.
7. **NVM Write Protection:** Variables loaded from `config.txt` (such as `vocabSelected`) must **never** be saved to NVM by `saveConfiguration()`.

---

## 2. Configuration File Format Specification (`config.txt`)

### 2.1 File Syntax & Grammar

The configuration file uses standard INI/Key-Value formatting:
* **Encoding:** Plain ASCII or UTF-8 without BOM. Lines delimited by CRLF (`\r\n`) or LF (`\n`).
* **Comments:** Any text following a `#` or `;` character is treated as a comment and ignored. Blank lines and whitespace-only lines are ignored.
* **Key-Value Separator:** An equals sign (`=`). Leading and trailing whitespace around both key and value are trimmed.
* **Case Sensitivity:**
  * Keys are case-insensitive (`trackname1` == `TrackName1` == `TRACKNAME1`).
  * Values preserve case exactly (e.g. `trackName1 = Main 1` preserves capital `M`).
* **Character Set & Token Policy:**
  * String values must only contain alphanumeric characters, spaces, and accepted formatting/punctuation: `[a-zA-Z0-9 _\-:\.\/\\=,#\?!]`. Any character outside this set halts parsing with a syntax error.
  * Directives starting with `#` (e.g. `#milepost`, `#track`, `#speed`) are **not** validated for token name validity during configuration file parsing. Downstream message transformers (`transformMessage`) and runtime audio sequencers (`parseTask`) handle token parsing and sound the pneumatic horn alert (`sfx:horn`) if an invalid or unsupported token is encountered.

---

### 2.2 Global Configuration Keys

| Config Key | Data Type | Target Destination | Validation & Bounds | Menu Hidden |
| :--- | :---: | :--- | :--- | :--- |
| `trackName1`<br/>*(alias: `trackNameA`)* | String | `cfg.trackName[0]` | Allowed string characters; max 16 chars; preserves case | Track A Name |
| `trackName2`<br/>*(alias: `trackNameB`)* | String | `cfg.trackName[1]` | Allowed string characters; max 16 chars; preserves case | Track B Name |
| `direction1Name` | String | `cfg.direction1Name` | Allowed string characters; max 16 chars; preserves case | Direction 1 Name |
| `direction2Name` | String | `cfg.direction2Name` | Allowed string characters; max 16 chars; preserves case | Direction 2 Name |
| `railName` | String | `cfg.railName` | Format `"Side1/Side2"` (e.g. `"North/South"`) | Rail Names |
| `vocabSelected` | String | `cfg.vocabSelected` | Alphanumeric string; preserves case. *(Directory existence is not checked during config parse; validated later during runtime vocab loading)* | Vocab Select |
| `integrityTrainRate` | Integer | `trackMessages.integrityTrainRate` | Integer in range `1` to `999,999` trains (matches 6-digit menu limits) | Random Integrity Rate |

---

### 2.3 Message Keys

All message keys accept template strings formatted according to the Message Token Specification:

| Config Key | Target Destination in `MessageBundle` | Description & Context |
| :--- | :--- | :--- |
| `entranceMsg` | `trackMessages.entranceMsg` | Spoken phrase when train first detected |
| `exitCleanMsg` | `trackMessages.exitCleanMsg` | Spoken exit summary when no defects detected |
| `exitCleanDisplayMsg` | `trackMessages.exitCleanDisplayMsg` | 20x4 LCD text for clean exit summary (using `\`) |
| `exitDefectMsg` | `trackMessages.exitDefectMsg` | Spoken exit summary when defects detected |
| `exitDefectDisplayMsg`| `trackMessages.exitDefectDisplayMsg` | 20x4 LCD text for defect exit summary |
| `integrityMsg` | `trackMessages.integrityMsg` | Spoken phrase for simulated integrity failure |
| `integrityDisplayMsg` | `trackMessages.integrityDisplayMsg` | 20x4 LCD text for integrity failure |
| `minAxleMsg` | `trackMessages.minAxleMsg` | Spoken phrase if train departs below min axle count |
| `minAxleDisplayMsg` | `trackMessages.minAxleDisplayMsg` | 20x4 LCD text for minimum axle timeout |
| `tooSlowMsg` | `trackMessages.tooSlowMsg` | Spoken phrase if train speed below min speed |
| `tooSlowDisplayMsg` | `trackMessages.tooSlowDisplayMsg` | 20x4 LCD text for speed below threshold |
| `detectorBlockedMsg` | `trackMessages.detectorBlockedMsg` | Spoken phrase if axle stopped in front of sensor |
| `detectorBlockedDisplayMsg` | `trackMessages.detectorBlockedDisplayMsg` | 20x4 LCD text for blocked detector condition |
| `excessAlarmsMsg` | `trackMessages.excessAlarmsMsg` | Phrase played when defect count exceeds max |

---

### 2.4 Defect Message Grouping (`[defect]`)

Because a detector installation may define multiple custom defects (e.g. Hot Journal, Hot Wheel, Dragging Equipment, High Load), defect messages are grouped into repeated entries added to `trackMessages.defects`.

#### Grouping Syntax: INI-Style Section Header (`[defect]`)

Each `[defect]` section begins a new `DefectMessage` instance:
```ini
[defect]
alertMsg   = #tone #track hot journal Axle #axle #rail Rail
displayMsg = #track\Hot Journal\Axle #axle #rail Rail
detailMsg  = #ord hot journal Axle #axle #rail Rail
axleRate   = 125

[defect]
alertMsg   = #tone #track dragging equipment near Axle #axle
displayMsg = #track\Dragging Equipment\Axle #axle
detailMsg  = #ord dragging equipment near Axle #axle
axleRate   = 250
```

#### Defect Keys within a `[defect]` Group:
1. `alertMsg` (String): Real-time spoken phrase played immediately when sensor triggers.
2. `displayMsg` (String): Real-time 3-line message shown on LCD screen when sensor triggers (`\` represents newline).
3. `detailMsg` (String): Spoken detail phrase aggregated into `#defectlist` during exit broadcast.
4. `axleRate` (Integer): Defect frequency rate (average 1 in $N$ axles; valid range $1 \dots 999,999$).

> [!IMPORTANT]
> **Strict 4-Key Completeness Requirement:** Every `[defect]` block must define all four keys (`alertMsg`, `displayMsg`, `detailMsg`, `axleRate`). If any key is omitted when the next `[defect]` header or end-of-file is encountered, the parser halts immediately with a syntax error specifying the line number. This fail-fast policy ensures no detector operates with an incomplete defect specification.

---

### 2.5 Complete `config.txt` Example

```ini
# ==============================================================================
# Iowa Scaled Engineering Defect Detector Custom Configuration
# ==============================================================================

# Hardware & Location Settings
trackName1         = Main 1
trackName2         = Siding
direction1Name     = Eastbound
direction2Name     = Westbound
railName           = North/South
vocabSelected      = default
integrityTrainRate = 200

# Broadcast Messages
entranceMsg               = Equipment Defect Detector milepost #milepost #track #direction
exitCleanMsg              = Equipment Defect Detector milepost #milepost #track #direction no defects repeat no defects total axles #axles train speed #speed temperature #temp degrees detector out
exitCleanDisplayMsg       = #track\NO DEFECTS\AXLES:#axles:3  SPEED:#speed:3
exitDefectMsg             = Equipment Defect Detector milepost #milepost you have a defect #defectlist total axles #axles detector out
exitDefectDisplayMsg      = #track\DEFECT DETECTED!\AXLES:#axles:3  SPEED:#speed:3
integrityMsg              = Equipment Defect Detector integrity failure
integrityDisplayMsg       = #track\INTEGRITY FAILURE\
minAxleMsg                = Equipment Defect Detector integrity failure total axles #axles
minAxleDisplayMsg         = #track\INTEGRITY FAILURE\AXLES:#axles:3
tooSlowMsg                = Equipment Defect Detector train 2 slow
tooSlowDisplayMsg         = #track\TRAIN TOO SLOW\
detectorBlockedMsg        = Equipment Defect Detector detector blocked
detectorBlockedDisplayMsg = #track\DETECTOR BLOCKED\
excessAlarmsMsg           = excessive alarms

# Defect 1: Hot Box / Journal
[defect]
alertMsg   = #tone #track hot journal Axle #axle #rail Rail
displayMsg = #track\Hot Journal\Axle #axle #rail Rail
detailMsg  = #ord hot journal Axle #axle #rail Rail
axleRate   = 150

# Defect 2: Dragging Equipment Detector (DED)
[defect]
alertMsg   = #tone #track dragging equipment near Axle #axle
displayMsg = #track\Dragging Equip\Near Axle #axle
detailMsg  = #ord dragging equipment near Axle #axle
axleRate   = 300
```

---

## 3. Architecture & Code Placement Proposal

### 3.1 New Source Files

We propose creating a dedicated parser module rather than embedding hundreds of lines of file-parsing code into `ckt-defect.ino`:

1. **`ckt-defect/config-file.h`**:
   - Header declaring `ConfigFileResult` error tracking structure.
   - Function declaration: `bool loadExternalConfigFile(const char* filepath, DetectorConfiguration& cfg, MessageBundle& msgs, DisplayLcd* lcd);`.
2. **`ckt-defect/config-file.cpp`**:
   - Implements FatFs file reader (`f_open`, custom `readLine`, `f_close`).
   - Implements key/value splitting, whitespace trimming, and case-insensitive comparison.
   - Implements strict validators:
     - `validateInteger(const std::string& val, int32_t min, int32_t max, int32_t& outVal)`
     - `validateString(const std::string& val, bool isDisplay)`
   - Manages state machine for parsing global keys vs. `[defect]` blocks.
   - Implements the LCD error display and freezing reboot loop on first failure.

### 3.2 Benefits of a Dedicated Module
* **Clean Separation of Concerns:** Keeps `ckt-defect.ino` focused on task scheduling, FreeRTOS queues, and hardware interrupts.
* **Unit Testability:** The parsing and validation functions can be compiled directly into the standalone host test harness (`src/test/`) using mock files or in-memory string streams.
* **Maintainability:** Adding future configuration keys (e.g. speed thresholds, volumes, timeouts) only touches `config-file.cpp`.

---

## 4. Detailed Code Changes Across Existing Files

### 4.1 `ckt-defect/configuration.h`

In `DetectorConfiguration`, add tracking booleans inside the non-NVM tracking section:

```cpp
	// Config Tracking
	bool                     externalConfigPresent;               // Volatile value, not stored in NVM
	bool                     externalMessagesPresent;             // Volatile value, not stored in NVM
	bool                     externalTrackNamePresent[NUM_TRACKS];// Volatile value, not stored in NVM
	bool                     externalDirection1NamePresent;       // Volatile value, not stored in NVM
	bool                     externalDirection2NamePresent;       // Volatile value, not stored in NVM
	bool                     externalRailNamePresent;             // Volatile value, not stored in NVM
	bool                     externalVocabSelectedPresent;        // Volatile value, not stored in NVM
	bool                     externalIntegrityTrainRatePresent;   // Volatile value, not stored in NVM
```
> [!NOTE]
> All tracking variables are explicitly marked `// Volatile value, not stored in NVM`. The pre-compiler codec generator (`tools/gen_codec.py`) will automatically ignore them, preserving 100% NVM schema compatibility.

---

### 4.2 `ckt-defect/configuration.cpp`

1. **NVM Save Protection (`saveConfiguration`):**
   ```cpp
   // Only save vocab to NVM if it was NOT loaded from config.txt
   if (!cfg->externalVocabSelectedPresent)
   {
       std::string storedVocab = preferences.getString("vocab", (cfg->vocabSelected + "_").c_str()).c_str();
       if(cfg->vocabSelected != storedVocab)
           preferences.putString("vocab", cfg->vocabSelected.c_str());
   }
   ```
2. **Selective Name Updates:**
   ```cpp
   void updateTrackNames(DetectorConfiguration* cfg)
   {
       for(uint32_t i = 0; i < NUM_TRACKS; i++)
       {
           if (!cfg->externalTrackNamePresent[i])
           {
               cfg->trackName[i] = trackNames[cfg->trackNameId[i]];
           }
       }
   }

   void updateDirectionNames(DetectorConfiguration* cfg)
   {
       if (!cfg->externalDirection1NamePresent)
           cfg->direction1Name = cfg->directionEnable ? directionNames[cfg->direction1NameId] : "";
       if (!cfg->externalDirection2NamePresent)
           cfg->direction2Name = cfg->directionEnable ? directionNames[cfg->direction2NameId] : "";
   }

   void updateRailNames(DetectorConfiguration* cfg)
   {
       if (!cfg->externalRailNamePresent)
           cfg->railName = railNames[cfg->railNameId];
   }
   ```

---

### 4.3 `ckt-defect/messages.cpp` & `messages.h`

In `updateMessages()`:
```cpp
void updateMessages(MessageBundle& trackMessages, const DetectorConfiguration& cfg)
{
	// If messages were loaded from external config.txt, skip default initialization
	if (!cfg.externalMessagesPresent)
	{
		setDefaultMessages(trackMessages, cfg);
	}
}
```

---

### 4.4 `ckt-defect/menu-factory.cpp`

In `updateAllMenuVisibility(const DetectorConfiguration& cfg)`:
```cpp
void updateAllMenuVisibility(const DetectorConfiguration& cfg)
{
    // ... existing defect rate visibility logic ...

    // External Message Overrides: Hide message and defect editing menus
    if (cfg.externalMessagesPresent)
    {
        if (menuDefects) menuDefects->hide();
        if (menuEntranceMessage) menuEntranceMessage->hide();
        if (menuAlertMessage) menuAlertMessage->hide();
        if (menuTalkDefectOnly) menuTalkDefectOnly->hide();

        if (menuMilepostEn) menuMilepostEn->hide();
        if (menuTrackNameEn) menuTrackNameEn->hide();
        if (menuAxleEn) menuAxleEn->hide();
        if (menuSpeedEn) menuSpeedEn->hide();
        if (menuDirectionEn) menuDirectionEn->hide();
        if (menuRailNameEn) menuRailNameEn->hide();
        if (menuTemperatureEn) menuTemperatureEn->hide();
    }

    // Hardware Names Overrides
    if (cfg.externalTrackNamePresent[0] && menuTrackNameA) menuTrackNameA->hide();
    if (cfg.externalTrackNamePresent[1] && menuTrackNameB) menuTrackNameB->hide();
    if (cfg.externalDirection1NamePresent && menuDirectionName1) menuDirectionName1->hide();
    if (cfg.externalDirection2NamePresent && menuDirectionName2) menuDirectionName2->hide();
    if (cfg.externalRailNamePresent && menuRailName) menuRailName->hide();
    if (cfg.externalVocabSelectedPresent && menuVocabSelect) menuVocabSelect->hide();
}
```

---

### 4.5 `ckt-defect/ckt-defect.ino` (Startup Flow Reordering)

In `setup()`, the execution sequence must be restructured:
1. `loadConfiguration(&cfg)` loads persistent baseline from NVM.
2. Initialize LCD & display splash screen.
3. Mount SD Card via SPI/FatFs.
4. Check for `config.txt`:
   - If present: call `loadExternalConfigFile("config.txt", cfg, trackMessages, lcd)`.
   - If parser encounters error:
     - Formats error message and line number to LCD Rows 0–2.
     - Displays `REBOOT` on Row 3 (col 14–19).
     - Loops indefinitely servicing watchdog and button polling; on Button 4 press, triggers `restartWithDelay()`.
   - If parsing succeeds:
     - Sets `cfg.externalConfigPresent = true`.
     - Displays `"External Config Loaded"` on LCD for 2.0 seconds.
5. Call `updateTrackNames(&cfg)`, `updateDirectionNames(&cfg)`, `updateRailNames(&cfg)` (safely updates only un-overridden names).
6. Call `updateMessages(trackMessages, cfg)` (skips if `cfg.externalMessagesPresent == true`).
7. Construct menus: `auto menus = createAppMenu(cfg, lcd, data, trackMessages)`. Menus are initialized with correct visibility flags!
8. Load vocabulary (SD or internal ROM).
9. Start `menuManager` and main operational loop.

---

## 5. UI Specifications: Error Screen & Reboot Button

When an error occurs during parsing, the system freezes on an error screen:

```
+--------------------+  <- 20 columns wide
|Config Error: Line 7|  Row 0: Error category and line number
|Unknown Key:        |  Row 1: Specific error diagnosis
|volume_gain         |  Row 2: Context token or offending value
|              REBOOT|  Row 3: Button 4 (col 14-19) triggers reboot
+--------------------+
 [Btn 1] [Btn 2] [Btn 3] [Btn 4]
```

### Reboot Button Handler:
```cpp
void freezeOnError(DisplayLcd* lcd, int lineNum, const std::string& errTitle, const std::string& errDetail)
{
    lcd->clear();
    lcd->gotoxy(0, 0);
    lcd->print(std::format("Config Error: Line {}", lineNum));
    lcd->gotoxy(0, 1);
    lcd->print(errTitle.substr(0, 20));
    lcd->gotoxy(0, 2);
    lcd->print(errDetail.substr(0, 20));
    lcd->gotoxy(14, 3);
    lcd->print("REBOOT");

    while (true)
    {
        esp_task_wdt_reset();
        lcd->readKeys();
        DisplayEvent ev;
        if (lcd->getEvent(&ev) && ev.type == DisplayEventType::KEY_PRESS)
        {
            if (ev.keyNum == 4) // Button 4: REBOOT
            {
                restartWithDelay(500, lcd);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
```

---

## 6. Resolved Architectural Design Decisions

All seven design questions and edge cases have been resolved with engineering decisions:

### 1. Defect Group Syntax: Standard `[defect]` Section Headers
* **Decision:** Use standard INI section headers `[defect]` (case-insensitive).
* **Rationale:** Clean, unambiguous, intuitive for end-users, and eliminates delimiter confusion. Encountering `[defect]` begins assembly of a new `DefectMessage` object in `trackMessages.defects`.

### 2. Incomplete Defect Blocks: Strict Fail-Fast Validation
* **Decision:** Require all 4 keys (`alertMsg`, `displayMsg`, `detailMsg`, `axleRate`) for every `[defect]` section.
* **Rationale:** Fail-fast validation prevents detectors from operating with broken or half-configured defect alerts. If any key is missing when the next `[defect]` header or EOF is reached, the parser immediately halts with a descriptive syntax error and line number.

### 3. All-or-Nothing Message Defaults: Intentional Selective Silencing
* **Decision:** Confirmed intentional. If at least one message is defined in `config.txt`, default initialization (`setDefaultMessages()`) is skipped for all messages. Unprovided messages remain empty (`""`).
* **Rationale:** This allows operators to intentionally silence specific announcements (e.g. entrance phrase, exit phrase, or defect alerts) without having to specify dummy or blank audio files.

### 4. String Character Set & Token Validation Policy
* **Decision:** Validate string characters against allowed set `[a-zA-Z0-9 _\-:\.\/\\=,#\?!]`. Do **not** validate `#` token directives during config parsing.
* **Rationale:** Downstream message transformation (`transformMessage`) and audio processing (`parseTask`) already parse and validate tokens at runtime, sounding the pneumatic horn (`sfx:horn`) if an invalid token is used. Keeping token validation in the runtime engine avoids duplicating token grammar and keeps the config parser lightweight.

### 5. `vocabSelected` Validation: Deferred to Vocab Engine
* **Decision:** Do **not** check directory existence on the SD card during configuration file parsing.
* **Rationale:** String format validation is performed during config reading. Vocabulary directory existence is already verified by the existing vocabulary loading subsystem later in startup; duplicating this check in the config parser is unnecessary and couples the parser to directory layouts.

### 6. Track Name Key Aliases: Support Both Canonical and Aliased Forms
* **Decision:** Support `trackName1` / `trackName2` as canonical keys, and accept `trackNameA` / `trackNameB` as case-insensitive aliases.
* **Rationale:** Eliminates confusion between engineering key names (1/2) and user-facing LCD menu labels ("Track A Name" / "Track B Name").

### 7. Integer Bounds Alignment: Range-Checked Against Menus
* **Decision:** Validate integers strictly against their data type and corresponding menu bounds:
  * `axleRate`: $1 \dots 999,999$ axles.
  * `integrityTrainRate`: $1 \dots 999,999$ trains (matching 6-digit thumbwheel menu range).
* **Rationale:** Prevents runtime integer overflows, division by zero, and invalid configuration states.

---

## 7. Phased Implementation Plan

| Phase | Tasks & Deliverables | Verification Gates |
| :---: | :--- | :--- |
| **Phase 1** | **Tracking Fields & Data Structures:**<br/>• Add tracking booleans to `DetectorConfiguration` in `configuration.h`<br/>• Update `configuration.cpp` (`saveConfiguration`, `update*Names`)<br/>• Update `messages.cpp` (`updateMessages`) | Run `python3 tools/gen_codec.py` to confirm 100% NVM schema preservation. |
| **Phase 2** | **Standalone Parser Module (`config-file.h/cpp`):**<br/>• Implement file reader, line tokenizer, key-value splitter, case-insensitive comparison<br/>• Implement data validation and range checkers (`axleRate` 1..999,999; `integrityTrainRate` 1..999,999)<br/>• Implement strict 4-key `[defect]` grouping logic<br/>• Support `trackNameA`/`trackNameB` aliases<br/>• Write unit test suite (`src/test/config/test_config.cpp`) with mock files | Run standalone unit test suite on PC; verify 100% test pass on valid/invalid configs. |
| **Phase 3** | **Menu Visibility Logic (`menu-factory.cpp`):**<br/>• Update `updateAllMenuVisibility()` to hide menus based on tracking flags | Verify menu trees build cleanly and hide appropriate nodes. |
| **Phase 4** | **System Integration (`ckt-defect.ino`):**<br/>• Reorder startup sequence in `setup()` (mount SD earlier)<br/>• Implement `freezeOnError()` on 20x4 LCD with `REBOOT` button<br/>• Implement 2-second success banner | Compile full firmware using `make compile`; verify flash/RAM budgets. |
| **Phase 5** | **End-to-End Bench Verification:**<br/>• Test valid `config.txt` loading and success banner<br/>• Test syntax errors, invalid values, and reboot button behavior<br/>• Verify NVM preferences are not overwritten | Complete functional verification. |
