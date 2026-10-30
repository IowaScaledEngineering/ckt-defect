# Message Token Modifiers and Audio Parsing Reference Specification

> **Target Firmware:** ESP32-S2 Defect Detector (`ckt-defect`)  
> **Source Files:** [`ckt-defect/messages.h`](file:///home/data/design/projects/ckt-defect/src/ckt-defect/messages.h), [`ckt-defect/messages.cpp`](file:///home/data/design/projects/ckt-defect/src/ckt-defect/messages.cpp), [`ckt-defect/parser.h`](file:///home/data/design/projects/ckt-defect/src/ckt-defect/parser.h), [`ckt-defect/parser.cpp`](file:///home/data/design/projects/ckt-defect/src/ckt-defect/parser.cpp)  
> **Test Harness:** [`src/test/messages/test_messages.cpp`](file:///home/data/design/projects/ckt-defect/src/test/messages/test_messages.cpp)  

---

## 1. System Architecture & Processing Pipeline

The defect detector message generation pipeline operates in two distinct, sequential stages:

```
                      +---------------------------------------+
                      | Raw Template String (MessageBundle)   |
                      | e.g. "milepost #milepost:4.1 #track:8"|
                      +---------------------------------------+
                                          |
                                          v
+-----------------------------------------------------------------------------------+
| Stage 1: transformMessage(...)                                                    |
| In: Template, DetectorConfiguration, DataBundle, trackNum, breakDigits            |
+-----------------------------------------------------------------------------------+
                 |                                                |
  [breakDigits = false] (Display Mode)             [breakDigits = true] (Spoken Mode)
                 |                                                |
                 v                                                v
  +-------------------------------+              +----------------------------------+
  | Formatted 20x4 Display String |              | Spoken Token Stream              |
  | - Exact column widths         |              | - Numbers broken into digits     |
  | - Leading/trailing padding    |              |   ("3 4 6 . 9", "1 2 4")         |
  | - Newlines ('\') preserved    |              | - Padding constraints ignored    |
  | - Stored in displayMessage    |              | - Lowercased & queued to parser  |
  +-------------------------------+              +----------------------------------+
                 |                                                |
                 v                                                v
  +-------------------------------+              +----------------------------------+
  | LCD Task (getDisplayMessage)  |              | FreeRTOS parserQueue             |
  +-------------------------------+              +----------------------------------+
                                                                  |
                                                                  v
+-----------------------------------------------------------------------------------+
| Stage 2: parseTask(...) [FreeRTOS Task]                                           |
| Tokenizes by whitespace: " \t\r\n"                                                |
+-----------------------------------------------------------------------------------+
    |                 |                 |                 |                 |
    v                 v                 v                 v                 v
[#tone / #tone=M,N] [#pause / #pause=M] [#count / #ord]   [Punctuation]   [Vocab Word]
  Synthesizes         Generates           Numeric or      '.' -> "point"  vocabGetWord()
  1 kHz Sine Tone     Digital Silence     Ordinal Word    '-' -> "minus"  (If missing ->
  in memory           in memory           (Auto-increments)               SFX "horn")
    |                 |                 |                 |                 |
    +-----------------+-----------------+-----------------+-----------------+
                                        |
                                        v
                         +------------------------------+
                         | FreeRTOS audioQueue          |
                         | -> I2S Audio DMA Output      |
                         +------------------------------+
```

### 1.1 The Dual-Mode Execution Model (`breakDigits`)

The `transformMessage()` function serves both the visual character display and the speech synthesis audio engine via the `breakDigits` flag:

1. **Display Mode (`breakDigits = false`)**:
   - Designed for the physical 20x4 character LCD display.
   - Enforces strict character-width formatting, leading/trailing space padding, and truncation.
   - Multi-digit numbers remain contiguous (e.g., `"124"`, `"45"`, `"346.9"`).
   - Single backslashes (`\`) are converted to newlines (`\n`) to demarcate LCD display rows.
   - Double backslashes (`\\`) escape into a literal backslash (`\`).

2. **Spoken Mode (`breakDigits = true`)**:
   - Designed for feeding the speech synthesizer in [`parser.cpp`](file:///home/data/design/projects/ckt-defect/src/ckt-defect/parser.cpp).
   - Splits numbers into individual whitespace-separated digits and tokens (e.g., `346.9` becomes `"3 4 6 . 9"` and `124` becomes `"1 2 4"`).
   - Bypasses integer width/padding constraints (`integerDigits = 0`) to prevent awkward pauses or alignment spaces in spoken output.
   - Leaves audio directives (`#tone`, `#pause`, `#count`, `#ord`) untouched so they can be parsed by `parseTask`.
   - The resulting string is converted to lowercase by [`DetectorStateMachine`](file:///home/data/design/projects/ckt-defect/src/ckt-defect/state-machine.cpp#L190) prior to parsing.

---

## 2. Stage 1: `transformMessage` Reference

### 2.1 Function Signature
```cpp
void transformMessage(
    const std::string& inputMessage, 
    std::string& outputMessage, 
    const DetectorConfiguration& cfg, 
    const DataBundle& data, 
    uint8_t trackNum, 
    bool breakDigits
);
```

### 2.2 Escape Characters & Control Sequences

| Sequence | Output in Display Mode (`breakDigits=false`) | Output in Spoken Mode (`breakDigits=true`) | Description |
| :--- | :--- | :--- | :--- |
| `\` | `\n` | `\n` | **Line break:** Delimits lines on the 20x4 LCD screen. |
| `\\` | `\` | `\` | **Literal backslash:** Escapes a backslash character. |

### 2.3 General Token Modifier Syntax

Tokens begin with `#` followed by alphanumeric characters:
```
#<baseToken>[:[±]N[.M]]
```
* **`:`** — Optional modifier delimiter.
* **`±`** — Optional alignment sign:
  * **Positive (or omitted)**: Right-aligned (space-padded on the left).
  * **Negative (`-`)**: Left-aligned (space-padded on the right).
* **`N`** — Field width or integer digits count.
* **`.M`** — Decimal precision (fractional digits count; used for `#milepost`).

---

### 2.4 Token Reference Guide

#### 2.4.1 `#milepost` — Milepost Location

* **Source Field:** `cfg.milepost` (stored as an integer in tenths of a mile, e.g., `3469` represents Milepost `346.9`).
* **Syntax:** `#milepost` or `#milepost[:[±]N[.M]]`
* **Default Format:** `N = 1`, `M = 1`
* **Parameters:**
  * `N`: Minimum width for the integer portion. If `N > 0`, padded on left with spaces. If `N < 0`, padded on right with spaces after the fractional portion.
  * `M`: Number of fractional decimal digits (default `1`).
* **Behavior:**
  * When `breakDigits = true`: Padding is suppressed; output is emitted as space-separated digits and decimal point: `"3 4 6 . 9"`.
  * When `breakDigits = false`: Output is formatted with the requested padding and decimal precision.

##### Examples (`cfg.milepost = 3469`)

| Token Template | Display Output (`breakDigits=false`) | Spoken Output (`breakDigits=true`) | Explanation |
| :--- | :--- | :--- | :--- |
| `#milepost` | `"346.9"` | `"3 4 6 . 9"` | Standard default (1 fractional digit) |
| `#milepost:3.1` | `"346.9"` | `"3 4 6 . 9"` | Minimum 3 integer digits (already 3: `346`) |
| `#milepost:4.1` | `" 346.9"` | `"3 4 6 . 9"` | Right-aligned, padded with 1 leading space |
| `#milepost:-4.1` | `"346.9 "` | `"3 4 6 . 9"` | Left-aligned, padded with 1 trailing space |
| `#milepost:6.1` | `"   346.9"` | `"3 4 6 . 9"` | Right-aligned, padded with 3 leading spaces |
| `#milepost:-6.1` | `"346.9   "` | `"3 4 6 . 9"` | Left-aligned, padded with 3 trailing spaces |

---

#### 2.4.2 String Replacement Tokens (`#track`, `#direction`, `#rail`)

All string replacement tokens are processed by `formatStringField()`:
* **Truncation:** If the source string length exceeds `|N|`, it is truncated to exactly `|N|` characters.
* **Right Alignment (`N > 0`):** Padded with leading spaces to width `N`.
* **Left Alignment (`N < 0`):** Padded with trailing spaces to width `|N|`.
* **No Modifier:** String is inserted verbatim with no truncation or padding.

##### 1. `#track[:[±]N]` — Track Identification
* **Source Field:** `cfg.trackName[trackNum]` (e.g., `"Main 1"`).
* **Examples (`cfg.trackName[0] = "Main 1"`):**
  * `#track` $\to$ `"Main 1"`
  * `#track:4` $\to$ `"Main"` (truncated to 4 characters)
  * `#track:8` $\to$ `"  Main 1"` (right-aligned in 8-column field)
  * `#track:-8` $\to$ `"Main 1  "` (left-aligned in 8-column field)

##### 2. `#direction[:[±]N]` — Direction of Travel
* **Source Field:** `data.direction` mapped against `cfg.direction1Name` and `cfg.direction2Name`:
  * `data.direction == 1` $\to$ `cfg.direction1Name` (e.g., `"Northbound"`, `"Eastbound"`)
  * `data.direction == 2` $\to$ `cfg.direction2Name` (e.g., `"Southbound"`, `"Westbound"`)
  * Any other value $\to$ `""` (empty string)
* **Examples (`direction1Name = "Northbound"`, `direction2Name = "Southbound"`):**
  * `#direction` (direction 1) $\to$ `"Northbound"`
  * `#direction:4` (direction 2) $\to$ `"Sout"` (truncated)
  * `#direction:12` (direction 1) $\to$ `"  Northbound"` (right-aligned in 12 columns)
  * `#direction:-12` (direction 2) $\to$ `"Southbound  "` (left-aligned in 12 columns)
  * `#direction` (direction 0 or invalid) $\to$ `""`

##### 3. `#rail[:[±]N]` — Rail Designation
* **Source Field:** `cfg.railName` and `data.rail`.
  * If `cfg.railName` contains a slash `/` (e.g., `"North/South"`, `"East/West"`, `"Left/Right"`):
    * If `data.rail % 2 != 0` (odd axle/sensor): Selects the substring **before** `/` (e.g., `"North"`).
    * If `data.rail % 2 == 0` (even axle/sensor): Selects the substring **after** `/` (e.g., `"South"`).
  * If no slash is present: Uses `cfg.railName` verbatim.
* **Examples (`cfg.railName = "North/South"`):**
  * `#rail` (with `data.rail = 1`) $\to$ `"North"`
  * `#rail` (with `data.rail = 2`) $\to$ `"South"`
  * `#rail:6` (with `data.rail = 1`) $\to$ `" North"` (right-aligned in 6 columns)
  * `#rail:-6` (with `data.rail = 2`) $\to$ `"South "` (left-aligned in 6 columns)

---

#### 2.4.3 Integer Numeric Tokens (`#axle`, `#axles`, `#speed`, `#temp`)

* **Tokens:**
  * `#axle` — Axle count where the defect was detected (`data.axleCount`).
  * `#axles` — Total axles counted for the entire train (`data.totalAxles`).
  * `#speed` — Train speed in miles per hour (`data.speed`).
  * `#temp` — Ambient temperature rounded to nearest integer (`TemperatureManager::getInstance()->getTemperature() + 0.5`).
* **Syntax:** `#token` or `#token[:[±]N]`
* **Default:** `N = 1`
* **Behavior:**
  * **Spoken Mode (`breakDigits = true`):** Numbers are converted to space-delimited individual digits (`"4 5"`, `"1 2 4"`). Padding constraints are ignored.
  * **Display Mode (`breakDigits = false`):**
    * `N > 0`: Right-aligned and left-padded with spaces to width `N`.
    * `N < 0`: Left-aligned and right-padded with spaces to width `|N|`.

##### Examples

| Token Template | Value | Display Output (`breakDigits=false`) | Spoken Output (`breakDigits=true`) |
| :--- | :--- | :--- | :--- |
| `#axle` | 4 | `"4"` | `"4"` |
| `#axle:3` | 4 | `"  4"` | `"4"` |
| `#axles` | 124 | `"124"` | `"1 2 4"` |
| `#axles:5` | 124 | `"  124"` | `"1 2 4"` |
| `#axles:-5` | 124 | `"124  "` | `"1 2 4"` |
| `#speed` | 45 | `"45"` | `"4 5"` |
| `#speed:3` | 45 | `" 45"` | `"4 5"` |
| `#speed:-3` | 45 | `"45 "` | `"4 5"` |
| `#temp` | 72 | `"72"` | `"7 2"` |
| `#temp:3` | 72 | `" 72"` | `"7 2"` |

---

#### 2.4.4 `#defectlist` — Defect Aggregation

* **Source Field:** `data.defects` (`std::vector<std::string>`).
* **Syntax:** `#defectlist` (no modifier accepted).
* **Behavior:**
  * Inserts every accumulated defect detail string from `data.defects`, separated by a single space.
  * Defects are generated during train passage when `defectCount <= cfg->maxDefects`.
  * Each detail message is pre-transformed via `transformMessage(msgs->defects[i].detailMsg, ..., breakDigits=true)` before being appended to `data.defects`.
  * If defect count exceeds `maxDefects`, the `excessAlarmsMsg` (`"excessive alarms"`) is appended.
* **Example Output:**
  ```
  first hot journal axle 1 2 north rail second dragging equipment near axle 8 4
  ```

---

## 3. Stage 2: `parseTask` Audio Directives Reference

`parseTask()` runs as a dedicated FreeRTOS task ([`parser.cpp`](file:///home/data/design/projects/ckt-defect/src/ckt-defect/parser.cpp)). It receives `ParserObject` instances from `parserQueue`, updates the LCD display message, and processes the spoken message string token by token (separated by whitespace `" \t\r\n"`).

### 3.1 Punctuation Pronunciation Mappings

When numbers are expanded with `breakDigits = true`, punctuation marks appear as isolated single-character tokens. `parseTask` automatically translates these to spoken words before vocabulary lookup:

| Character Token | Internal Translated Token | Spoken Word File Played | Use Case |
| :--- | :--- | :--- | :--- |
| `.` | `"point"` | `vocabGetWord("point")` | Decimal point in mileposts (`3 4 6 . 9` $\to$ "... three four six **point** nine") |
| `-` | `"minus"` | `vocabGetWord("minus")` | Negative temperatures (`- 5` $\to$ "**minus** five") |

---

### 3.2 Tone Generator: `#tone`

The `#tone` directive generates a pure 1,000 Hz sine wave tone in memory using the `ToneSound` synthesis class at 16,000 samples/sec.

* **Syntax:**
  * `#tone`
  * `#tone=M`
  * `#tone=M,N`
* **Parameters:**
  * **`M` (Duration):** Length of the tone in **deciseconds** (tenths of a second, 100 ms units).
    * *Default:* `10` (= 1.0 second).
    * *Sample count formula:* $\text{Samples} = 1600 \times M$.
  * **`N` (Attenuation Exponent):** Volume level reduction factor.
    * *Default:* `1` (50% amplitude / -6 dB).
    * *Volume formula:* $\text{Amplitude} = \frac{1}{2^N}$.

#### Attenuation Scaling Table

| Attenuation Exponent `N` | Mathematical Factor | Relative Amplitude | Relative Decibels (dB) | Intended Usage |
| :---: | :---: | :---: | :---: | :--- |
| `0` | $1 / 2^0$ | **100.0%** | 0 dB | High-priority defect alert tone (maximum volume) |
| `1` | $1 / 2^1$ | **50.0%** | -6 dB | Standard alert tone (default) |
| `2` | $1 / 2^2$ | **25.0%** | -12 dB | Background alert tone |
| `3` | $1 / 2^3$ | **12.5%** | -18 dB | Low-priority advisory tone |

#### Examples

| Directive | Tone Duration | Volume Amplitude | Description |
| :--- | :--- | :--- | :--- |
| `#tone` | 1.0 second (10 decisecs) | 50% ($N=1$) | Standard default defect alert tone |
| `#tone=5` | 0.5 second (5 decisecs) | 50% ($N=1$) | Short alert chirp |
| `#tone=20` | 2.0 seconds (20 decisecs) | 50% ($N=1$) | Extended warning tone |
| `#tone=10,0` | 1.0 second (10 decisecs) | 100% ($N=0$) | Full-volume defect alarm tone |
| `#tone=5,2` | 0.5 second (5 decisecs) | 25% ($N=2$) | Quiet introductory alert |

---

### 3.3 Silence / Pause Generator: `#pause`

The `#pause` directive creates digital silence using the `SilenceSound` class at 16,000 samples/sec. This is used to insert natural conversational pacing between phrases or sentences.

* **Syntax:**
  * `#pause`
  * `#pause=M`
* **Parameters:**
  * **`M` (Duration):** Length of silence in **deciseconds** (tenths of a second, 100 ms units).
    * *Default:* `5` (= 0.5 seconds).
    * *Sample count formula:* $\text{Samples} = 1600 \times M$.

#### Examples

| Directive | Pause Duration | Typical Application |
| :--- | :--- | :--- |
| `#pause` | 0.5 second (5 decisecs) | Standard inter-phrase pause |
| `#pause=1` | 0.1 second (1 decisec) | Brief pause between numbers or code letters |
| `#pause=10` | 1.0 second (10 decisecs) | Pause between sentence blocks |
| `#pause=20` | 2.0 seconds (20 decisecs) | Long pause before message repetition |

---

### 3.4 Dynamic Message Counters: `#count` and `#ord`

`parseTask` maintains an internal counter variable (`uint32_t count = 1`) that is reset to `1` at the beginning of each parsed message. `#count` and `#ord` share this counter.

#### 1. `#count` — Numeric Word Counter
* **Behavior:** Looks up the vocabulary word for the current number string: `vocabGetWord(std::to_string(count))`. After queuing the audio word, it automatically increments `count++`.
* **Example Progression:**
  * 1st encounter $\to$ speaks `"1"` (`"one"`), increments `count` to 2.
  * 2nd encounter $\to$ speaks `"2"` (`"two"`), increments `count` to 3.

#### 2. `#ord` — Ordinal Word Counter
* **Behavior:** Retrieves the ordinal English word for the current counter via `getOrdinalWord(count)` and plays it via `vocabGetWord(...)`. After queuing the audio word, it automatically increments `count++`.
* **Supported Range (1 through 9):**
  * `1` $\to$ `"first"`
  * `2` $\to$ `"second"`
  * `3` $\to$ `"third"`
  * `4` $\to$ `"fourth"`
  * `5` $\to$ `"fifth"`
  * `6` $\to$ `"sixth"`
  * `7` $\to$ `"seventh"`
  * `8` $\to$ `"eighth"`
  * `9` $\to$ `"ninth"`
  * `> 9` $\to$ `""` (no audio played)

#### Mixed Counter Example
```
Template: "#ord defect axle 1 2 #ord defect axle 4 5"
Audio Sequence: "first" -> "defect" -> "1" -> "2" -> "second" -> "defect" -> "4" -> "5"
```

---

### 3.5 Vocabulary Lookup & Missing Word Fallback

1. **Standard Words:**
   Any token that does not match `#tone`, `#pause`, `#count`, or `#ord` is passed to `vocabGetWord(token)`.
   * Words and digits (`"0"` through `"9"`) are stored in flash ROM or on the FAT filesystem of the microSD card as 16 kHz 16-bit mono PCM WAV clips.
2. **Missing Word Diagnostic Fallback (`sfx:horn`):**
   * If `vocabGetWord()` returns `nullptr` (the requested word does not exist in vocabulary ROM or SD):
   * `parseTask` automatically plays the special effect sound **`"horn"`** ([`PneumaticHornDouble3-16k.wav`](file:///home/data/design/projects/ckt-defect/src/ckt-defect/sfx/wav/PneumaticHornDouble3-16k.wav)).
   * This provides an immediate, loud auditory cue during bench testing that a message template contains a word omitted from the vocabulary build.

---

## 4. Test Suite Reference (`src/test/messages`)

The message transformation engine is verified by a standalone test suite in [`src/test/messages/test_messages.cpp`](file:///home/data/design/projects/ckt-defect/src/test/messages/test_messages.cpp).

### 4.1 Running the Unit Tests

```bash
cd /home/data/design/projects/ckt-defect/src/test/messages
./run
```
*(Runs with g++ using mock Arduino, FreeRTOS, and NVS headers).*

### 4.2 Unit Test Matrix

Below is the complete catalog of all 27 test cases and the features they verify:

| Test # | Input Template | Expected Output | `breakDigits` | Tested Feature / Edge Case |
| :---: | :--- | :--- | :---: | :--- |
| **1** | `#milepost` | `"3 4 6 . 9"` | `true` | Milepost spoken mode: splits digits and decimal point |
| **2** | `#milepost` | `"346.9"` | `false` | Milepost display mode: compact representation |
| **3** | `#milepost:3.1` | `"3 4 6 . 9"` | `true` | Spoken mode ignores width constraints |
| **4** | `#milepost:4.1` | `" 346.9"` | `false` | Right-aligned milepost, padded with 1 leading space |
| **5** | `#milepost:-4.1` | `"346.9 "` | `false` | Left-aligned milepost, padded with 1 trailing space |
| **6** | `#milepost:6.1` | `"   346.9"` | `false` | Right-aligned milepost, padded with 3 leading spaces |
| **7** | `#milepost:-6.1` | `"346.9   "` | `false` | Left-aligned milepost, padded with 3 trailing spaces |
| **8** | `#axle` | `"4"` | `false` | Single-digit axle number |
| **9** | `#axle:3` | `"  4"` | `false` | Right-aligned axle number in 3-column field |
| **10** | `#axles:-3` | `"1 2 4"` | `true` | Spoken total axles: negative width ignored, digits split |
| **11** | `#track` | `"Main 1"` | `false` | Unmodified track string |
| **12** | `#track:4` | `"Main"` | `false` | Track string truncation (6 chars truncated to 4) |
| **13** | `#track:8` | `"  Main 1"` | `false` | Right-aligned track name padded to 8 columns |
| **14** | `#track:-8` | `"Main 1  "` | `false` | Left-aligned track name padded to 8 columns |
| **15** | `#direction` | `"Northbound"` | `false` | Direction 1 lookup (`direction = 1`) |
| **16** | `#direction:4` | `"Sout"` | `false` | Direction 2 truncation (`direction = 2` truncated to 4) |
| **17** | `#direction:12` | `"  Northbound"` | `false` | Direction 1 right-aligned to 12 columns |
| **18** | `#direction:-12` | `"Southbound  "` | `false` | Direction 2 left-aligned to 12 columns |
| **19** | `#direction` | `""` | `false` | Undefined direction (`direction = 0`) returns empty |
| **20** | `#direction` | `""` | `false` | Out-of-bounds direction (`direction = 12`) returns empty |
| **21** | `#speed` | `"4 5"` | `true` | Spoken speed: splits digits (`45` $\to$ `"4 5"`) |
| **22** | `#temp` | `"72"` | `false` | Display temperature from `TemperatureManager` |
| **23** | `#defectlist` | `"HOT_BOX DRAG_EQ"` | `false` | Defect list concatenation with space separator |
| **24** | `milepost #milepost speed #speed:3 mph` | `milepost 346.9 speed  45 mph` | `false` | Compound sentence with mixed tokens and modifiers |
| **25** | `Axles:#axles` | `Axles:124` | `false` | Adjacent literal text and token |
| **26** | `Axles:#axles\Speed:#speed` | `Axles:124\nSpeed:45` | `false` | Single backslash converted to LCD newline |
| **27** | `Axles:\\#axles` | `Axles:\124` | `false` | Double backslash escaped to literal backslash |

---

## 5. End-to-End Detector Message Examples

### 5.1 Real-Time Defect Alert (Spoken & LCD Display)

* **Trigger:** Axle sensor detects a hot journal on axle 48 on Track 2 (South rail).
* **Alert Message Template (`msgs->defects[0].alertMsg`):**
  ```
  #tone #track hot journal Axle #axle #rail Rail
  ```
* **Alert Display Template (`msgs->defects[0].displayMsg`):**
  ```
  #track\Hot Journal\Axle #axle #rail Rail
  ```

#### Processing Walkthrough:
1. **Spoken Output (`breakDigits = true`):**
   * `#tone` $\to$ preserved verbatim as `#tone`.
   * `#track` $\to$ `"Track 2"`.
   * `#axle` $\to$ `"4 8"` (split digits).
   * `#rail` $\to$ `"South"`.
   * Resulting string: `#tone track 2 hot journal axle 4 8 south rail`
   * Audio Engine (`parseTask`) execution:
     * Plays 1.0 s 1 kHz tone at 50% amplitude.
     * Plays vocabulary clips: `track` $\to$ `2` $\to$ `hot` $\to$ `journal` $\to$ `axle` $\to$ `4` $\to$ `8` $\to$ `south` $\to$ `rail`.
2. **Display Output (`breakDigits = false`):**
   * Single backslashes become newlines.
   * Resulting string:
     ```
     Track 2
     Hot Journal
     Axle 48 South Rail
     ```

---

### 5.2 Clean Train Exit Summary

* **Trigger:** Train with 124 axles completes passage with no defects at 45 mph, Milepost 346.9, 72°F.
* **Exit Clean Message Template (`trackMessages.exitCleanMsg`):**
  ```
  Equipment Defect Detector milepost #milepost #track #direction no defects repeat no defects total axles #axles train speed #speed temperature #temp degrees detector out
  ```
* **Exit Display Template (`trackMessages.exitCleanDisplayMsg`):**
  ```
  #track\NO DEFECTS\AXLES:#axles:3  SPEED:#speed:3
  ```

#### Processing Walkthrough:
1. **Spoken Output (`breakDigits = true`):**
   ```
   equipment defect detector milepost 3 4 6 . 9 track 2 southbound no defects repeat no defects total axles 1 2 4 train speed 4 5 temperature 7 2 degrees detector out
   ```
   * Audio Engine (`parseTask`):
     * The `.` in `3 4 6 . 9` is translated to `"point"`.
     * Every digit (`3`, `4`, `6`, `9`, `1`, `2`, `4`, `4`, `5`, `7`, `2`) is looked up and spoken individually.
2. **Display Output (`breakDigits = false`):**
   ```
   Track 2
   NO DEFECTS
   AXLES:124  SPEED: 45
   ```
   *(Note how `#speed:3` left-pads with 1 space so `" 45"` occupies exactly 3 columns).*

---

### 5.3 Train Exit with Defect List Summary

* **Trigger:** Train exits after detecting Hot Journal on Axle 12 and Dragging Equipment near Axle 84.
* **Detail Message Template (`detailMsg`):**
  ```
  #ord hot journal Axle #axle #rail Rail
  ```
* **Exit Defect Message Template (`trackMessages.exitDefectMsg`):**
  ```
  Equipment Defect Detector milepost #milepost you have a defect #defectlist total axles #axles detector out
  ```

#### Processing Walkthrough:
1. `data.defects` stores each pre-transformed detail string:
   * Defect 1: `"first hot journal axle 1 2 north rail"`
   * Defect 2: `"second dragging equipment near axle 8 4"`
2. `#defectlist` expands into:
   ```
   first hot journal axle 1 2 north rail second dragging equipment near axle 8 4
   ```
3. Full Spoken Exit Message:
   ```
   equipment defect detector milepost 3 4 6 . 9 you have a defect first hot journal axle 1 2 north rail second dragging equipment near axle 8 4 total axles 1 2 4 detector out
   ```

---

## 6. Quick Reference Summary Table

| Directive / Token | Stage Processed | Syntax | Default | Mode Sensitivity | Purpose |
| :--- | :---: | :--- | :---: | :---: | :--- |
| **`\`** | Stage 1 | `\` | — | Both | Converted to newline (`\n`) for LCD display row separation. |
| **`\\`** | Stage 1 | `\\` | — | Both | Escaped to literal backslash (`\`). |
| **`#milepost`** | Stage 1 | `:[±]N[.M]` | `1.1` | Displays padded decimal; speaks space-separated digits and `.` | Inserts milepost location. |
| **`#track`** | Stage 1 | `:[±]N` | Verbatim | Both truncate if `len > \|N\|`; pads left (`N>0`) or right (`N<0`). | Inserts current track name. |
| **`#direction`** | Stage 1 | `:[±]N` | Verbatim | Both truncate/pad. Emits empty string if direction is unknown. | Inserts travel direction name. |
| **`#rail`** | Stage 1 | `:[±]N` | Verbatim | Evaluates odd/even rail index against `North/South` slash string. | Inserts defect rail side. |
| **`#axle`** | Stage 1 | `:[±]N` | `1` | Displays padded number; speaks individual digits (`breakDigits=true`). | Inserts defect axle index. |
| **`#axles`** | Stage 1 | `:[±]N` | `1` | Displays padded number; speaks individual digits (`breakDigits=true`). | Inserts total train axle count. |
| **`#speed`** | Stage 1 | `:[±]N` | `1` | Displays padded number; speaks individual digits (`breakDigits=true`). | Inserts train speed (mph). |
| **`#temp`** | Stage 1 | `:[±]N` | `1` | Displays padded number; speaks individual digits (`breakDigits=true`). | Inserts ambient temperature (°F/°C). |
| **`#defectlist`** | Stage 1 | `#defectlist`| — | Aggregates all detected defect detail strings. | Inserts exit summary defect catalog. |
| **`#tone`** | Stage 2 | `[=M[,N]]` | `10,1` | Decisecs duration $M$ (default 1.0s), attenuation $N$ ($1/2^N$, default 50%). | Generates synthetic 1 kHz alert tone. |
| **`#pause`** | Stage 2 | `[=M]` | `5` | Decisecs duration $M$ (default 0.5s). | Generates synthetic digital silence. |
| **`#count`** | Stage 2 | `#count` | `1` | Auto-increments after queuing audio word. | Speaks numeric counter (`"1"`, `"2"`, ...). |
| **`#ord`** | Stage 2 | `#ord` | `1` | Auto-increments after queuing audio word. Supports 1–9. | Speaks ordinal word (`"first"`, `"second"`, ...). |
| **`.`** | Stage 2 | `.` | — | Translated to `"point"`. | Pronounces decimal point in numbers. |
| **`-`** | Stage 2 | `-` | — | Translated to `"minus"`. | Pronounces negative sign in numbers. |
| **`<word>`** | Stage 2 | Any string | — | Checks flash/SD vocabulary; falls back to `"horn"` SFX if missing. | Plays recorded voice vocabulary WAV. |
