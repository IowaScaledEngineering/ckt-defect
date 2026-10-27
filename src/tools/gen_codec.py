#!/usr/bin/env python3
"""
gen_codec.py - Automated Codec Generator and NVM Coverage Verifier for ckt-defect.

Reads:
  - ckt-defect/configuration.h (Struct definition, limits, inline comment @annotations)
  - ckt-defect/configuration.cpp (preferences.put* calls for NVM coverage verification)

Generates:
  - tools/schemas/schema_v<VERSION>.yaml (Machine-readable schema definition)
  - ckt-defect/config_codec.h (C++ firmware bitstream serializer & static_assert bounds guards)
"""

import os
import re
import sys
import yaml

CONFIG_H_PATH = "ckt-defect/configuration.h"
CONFIG_CPP_PATH = "ckt-defect/configuration.cpp"
OUTPUT_H_PATH = "ckt-defect/config_codec.h"
SCHEMA_DIR = "tools/schemas"

def parse_configuration_h(path):
    with open(path, "r", encoding="utf-8") as f:
        content = f.read()

    # 1. Extract Schema Version
    m_ver = re.search(r"#define\s+CONFIG_SCHEMA_VERSION\s+(\d+)", content)
    if not m_ver:
        raise ValueError("Could not find #define CONFIG_SCHEMA_VERSION in configuration.h")
    schema_version = int(m_ver.group(1))

    # 2. Extract Numerical Limits
    limits = {}
    for m in re.finditer(r"#define\s+([A-Z0-9_]+_(?:MIN|MAX|STEP_[A-Z0-9]+))\s+(-?\d+)", content):
        limits[m.group(1)] = int(m.group(2))

    # 3. Extract Struct DetectorConfiguration
    m_struct = re.search(r"struct\s+DetectorConfiguration\s*\{([^}]+)\};", content, re.DOTALL)
    if not m_struct:
        raise ValueError("Could not find struct DetectorConfiguration in configuration.h")
    struct_body = m_struct.group(1)

    # 4. Parse Fields
    fields = []
    lines = struct_body.splitlines()
    current_category = "General"

    for line in lines:
        line_clean = line.strip()
        if not line_clean:
            continue

        # Detect category header comment: e.g. // Audio & Display or // Speed
        m_sec = re.match(r"^//\s*([A-Za-z0-9 &/-]+)$", line_clean)
        if m_sec:
            current_category = m_sec.group(1).strip()
            continue

        if line_clean.startswith("//"):
            continue

        # Check for field declaration with possible trailing comment
        # e.g.: uint8_t volumeStep; // @bits: 5
        # e.g.: uint8_t trackNameId[NUM_TRACKS]; // @bits: 3
        # e.g.: std::string trackName[NUM_TRACKS]; // Volatile value, not stored in NVM
        m_field = re.match(r"^([a-zA-Z0-9_<>\s:*]+)\s+([a-zA-Z0-9_]+)(?:\[([a-zA-Z0-9_]+)\])?\s*;\s*(?://\s*(.*))?$", line_clean)
        if not m_field:
            continue

        type_str = m_field.group(1).strip()
        name_str = m_field.group(2).strip()
        array_size_str = m_field.group(3)
        comment_str = m_field.group(4) or ""

        # Ignore volatile/non-NVM fields explicitly noted or runtime vectors
        if "Volatile value" in comment_str or "not stored in NVM" in comment_str or "vector" in type_str:
            continue

        # Parse annotations from comment: @key: value or @key
        annotations = {}
        for part in re.split(r",\s*(?=@)", comment_str):
            part = part.strip()
            if part.startswith("@"):
                if ":" in part:
                    k, v = part.split(":", 1)
                    annotations[k.strip()[1:]] = v.strip()
                else:
                    annotations[part[1:].strip()] = True

        field_info = {
            "name": name_str,
            "category": current_category,
            "cpp_type": type_str,
            "raw_comment": comment_str,
            "is_array": bool(array_size_str),
            "array_size": array_size_str,
        }

        # Extract unit metadata if present in comment
        m_unit = re.search(r"Units\s*=\s*([a-zA-Z0-9_]+)", comment_str, re.IGNORECASE)
        if m_unit:
            unit_str = m_unit.group(1).lower()
            field_info["unit"] = unit_str
            if unit_str == "decimiles":
                field_info["divisor"] = 10
                field_info["display_unit"] = "MP"
            elif unit_str == "deciseconds":
                field_info["multiplier"] = 100
                field_info["display_unit"] = "ms"
            elif unit_str == "deciscale":
                field_info["divisor"] = 10
                field_info["display_unit"] = "x"
            elif unit_str == "seconds":
                field_info["display_unit"] = "sec"
        elif "100us" in comment_str:
            field_info["unit"] = "100us"
            field_info["multiplier"] = 100
            field_info["display_unit"] = "us"

        # Determine encoding attributes
        if type_str == "bool":
            field_info["encoding"] = "bits"
            field_info["bits"] = 1
        elif annotations.get("type") and "varint" in annotations.get("type"):
            field_info["encoding"] = "varint"
            field_info["bits"] = 8  # Min 1 byte when 0-127
        elif "string" in annotations or annotations.get("type") == "string" or "vocab" in annotations:
            field_info["encoding"] = "string"
            m_s_bits = re.search(r"\d+", str(annotations.get("string", annotations.get("bits", 8))))
            field_info["bits"] = int(m_s_bits.group(0)) if m_s_bits else 8
        elif "bits" in annotations:
            field_info["encoding"] = "bits"
            m_bits = re.search(r"\d+", annotations["bits"])
            if not m_bits:
                raise ValueError(f"Invalid @bits annotation on {name_str}: {annotations['bits']}")
            field_info["bits"] = int(m_bits.group(0))
        else:
            # If not annotated and not marked volatile, report warning/error
            raise ValueError(f"Field '{name_str}' in DetectorConfiguration lacks packing annotation (@bits or @type: varint)")

        if "offset" in annotations:
            m_offset = re.search(r"-?\d+", str(annotations["offset"]))
            if m_offset:
                field_info["offset"] = int(m_offset.group(0))

        fields.append(field_info)

    return schema_version, limits, fields

def verify_nvm_coverage(cpp_path, fields):
    with open(cpp_path, "r", encoding="utf-8") as f:
        cpp_content = f.read()

    # Find saveConfiguration function
    m_save = re.search(r"void\s+saveConfiguration\s*\([^)]*\)\s*\{([^}]+(?:\}[^}]+)*?)\n\}", cpp_content)
    save_body = m_save.group(1) if m_save else cpp_content

    # Extract all cfg->fieldName in preferences.put* calls
    # Examples: preferences.putUChar("vol", cfg->volumeStep);
    #           preferences.putUChar(key.c_str(), cfg->trackNameId[i]);
    #           preferences.putString("vocab", cfg->vocabSelected.c_str());
    nvm_fields = set()
    for m in re.finditer(r"preferences\.put[A-Za-z]+\s*\(\s*[^,]+,\s*cfg->([a-zA-Z0-9_]+)", save_body):
        nvm_fields.add(m.group(1))

    packed_field_names = {f["name"] for f in fields}

    missing = nvm_fields - packed_field_names
    if missing:
        raise RuntimeError(
            f"[FATAL] NVM Coverage Verification Failed!\n"
            f"The following persistent fields in configuration.cpp (saveConfiguration) are missing from the packing schema:\n"
            f"  {sorted(list(missing))}\n"
            f"All persistent fields MUST be annotated and included in DetectorConfiguration serialization."
        )

    print(f"[OK] 100% NVM Coverage Verified: all {len(nvm_fields)} persistent settings are mapped to packing schema.")
    return nvm_fields

def generate_yaml_schema(schema_version, limits, fields, output_yaml_path):
    os.makedirs(os.path.dirname(output_yaml_path), exist_ok=True)

    expanded_fields = []
    total_bits = 6  # 6-bit schema version header
    for f in fields:
        if f["is_array"]:
            # NUM_TRACKS is 2
            size = 2 if f["array_size"] == "NUM_TRACKS" else int(f["array_size"])
            for idx in range(size):
                item = {
                    "name": f"{f['name']}_{idx}",
                    "member": f"{f['name']}[{idx}]",
                    "category": f.get("category", "General"),
                    "cpp_type": f["cpp_type"],
                    "encoding": f["encoding"],
                    "bits": f["bits"],
                }
                for k in ("offset", "unit", "display_unit", "multiplier", "divisor"):
                    if k in f:
                        item[k] = f[k]
                expanded_fields.append(item)
                total_bits += f["bits"]
        else:
            item = {
                "name": f["name"],
                "member": f["name"],
                "category": f.get("category", "General"),
                "cpp_type": f["cpp_type"],
                "encoding": f["encoding"],
                "bits": f["bits"],
            }
            for k in ("offset", "unit", "display_unit", "multiplier", "divisor"):
                if k in f:
                    item[k] = f[k]
            expanded_fields.append(item)
            total_bits += f["bits"]

    # CRC-16 adds 16 bits
    total_bits_with_crc = total_bits + 16
    char_count = (total_bits_with_crc + 4) // 5

    schema_data = {
        "schema_version": schema_version,
        "format": "Crockford Base32 with CRC-16-CCITT",
        "total_bits_min": total_bits_with_crc,
        "base32_length_min": char_count,
        "max_allowed_characters": 60,
        "limits": limits,
        "fields": expanded_fields
    }

    with open(output_yaml_path, "w", encoding="utf-8") as f:
        yaml.dump(schema_data, f, sort_keys=False, default_flow_style=False)

    print(f"[OK] Generated YAML schema: {output_yaml_path} ({total_bits_with_crc} bits, ~{char_count} Base32 chars)")
    return schema_data, expanded_fields

def generate_cpp_header(schema_version, limits, fields, expanded_fields, output_h_path):
    os.makedirs(os.path.dirname(output_h_path), exist_ok=True)

    lines = []
    lines.append("// ============================================================================")
    lines.append("// AUTO-GENERATED CONFIGURATION CODEC - DO NOT EDIT MANUALLY")
    lines.append(f"// Generated by tools/gen_codec.py from ckt-defect/configuration.h (Schema v{schema_version})")
    lines.append("// ============================================================================")
    lines.append("#pragma once")
    lines.append("")
    lines.append("#include <Arduino.h>")
    lines.append("#include <string>")
    lines.append("#include <vector>")
    lines.append("#include <cmath>")
    lines.append('#include "configuration.h"')
    lines.append("")

    # Static assertion bounds checks
    lines.append("// ----------------------------------------------------------------------------")
    lines.append("// Compile-Time Bounds Checks (Enforces UI dial max values fit in bit allocations)")
    lines.append("// ----------------------------------------------------------------------------")
    lines.append("static_assert(ENTRANCE_AXLES_MAX < (1 << 4), \"ENTRANCE_AXLES_MAX exceeds allocated 4 bits!\");")
    lines.append("static_assert(MIN_AXLES_MAX < (1 << 7), \"MIN_AXLES_MAX exceeds allocated 7 bits!\");")
    lines.append("static_assert(MIN_SPEED_MAX < (1 << 6), \"MIN_SPEED_MAX exceeds allocated 6 bits!\");")
    lines.append("static_assert(SPEED_SCALE_MAX < (1 << 10), \"SPEED_SCALE_MAX exceeds allocated 10 bits!\");")
    lines.append("static_assert(DETECTOR_TIMEOUT_MAX < (1 << 5), \"DETECTOR_TIMEOUT_MAX exceeds allocated 5 bits!\");")
    lines.append("static_assert(SUMMARY_DISP_TIME_MAX < (1 << 6), \"SUMMARY_DISP_TIME_MAX exceeds allocated 6 bits!\");")
    lines.append("static_assert(MSG_REPEAT_TIMEOUT_MAX < (1 << 6), \"MSG_REPEAT_TIMEOUT_MAX exceeds allocated 6 bits!\");")
    lines.append("static_assert((TEMPERATURE_MAX + 100) < (1 << 8), \"TEMPERATURE_MAX (+offset 100) exceeds allocated 8 bits!\");")
    lines.append("static_assert(MAX_DEFECTS_MAX < (1 << 4), \"MAX_DEFECTS_MAX exceeds allocated 4 bits!\");")
    lines.append("static_assert((PTT_DELAY_MAX / 100) < (1 << 5), \"PTT_DELAY_MAX exceeds allocated 5 bits!\");")
    lines.append("static_assert((MIN_AXLE_TIME_MAX / 100) < (1 << 8), \"MIN_AXLE_TIME_MAX exceeds allocated 8 bits!\");")
    lines.append("static_assert(MILEPOST_MAX < (1 << 14), \"MILEPOST_MAX exceeds allocated 14 bits!\");")
    lines.append("")

    # Calculate total minimum bits for LCD check
    total_bits = 6 + sum(f["bits"] for f in expanded_fields) + 16
    lines.append(f"// Total minimum bit footprint: {total_bits} bits")
    lines.append(f"static_assert({total_bits} <= 300, \"Configuration code exceeds 60-character (300-bit) LCD limit!\");")
    lines.append("")

    # Codec Classes and functions
    lines.append("""namespace config_codec {

// Crockford Base32 Alphabet (Case-insensitive, no I, L, O, U)
static const char CROCKFORD_CHARS[] = "0123456789ABCDEFGHJKMNPQRSTVWXYZ";

class BitWriter {
public:
    std::vector<uint8_t> buffer;
    uint8_t currentByte = 0;
    int bitCount = 0;

    void write(uint32_t value, int bits) {
        for (int i = bits - 1; i >= 0; --i) {
            currentByte = (currentByte << 1) | ((value >> i) & 1);
            bitCount++;
            if (bitCount == 8) {
                buffer.push_back(currentByte);
                currentByte = 0;
                bitCount = 0;
            }
        }
    }

    void writeVarint(uint32_t value) {
        while (value >= 0x80) {
            write((value & 0x7F) | 0x80, 8);
            value >>= 7;
        }
        write(value & 0x7F, 8);
    }

    void flush() {
        if (bitCount > 0) {
            buffer.push_back(currentByte << (8 - bitCount));
            currentByte = 0;
            bitCount = 0;
        }
    }
};

inline uint16_t calculateCRC16(const uint8_t* data, size_t length) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < length; ++i) {
        crc ^= (uint16_t)data[i] << 8;
        for (int j = 0; j < 8; ++j) {
            if (crc & 0x8000) {
                crc = ((crc << 1) ^ 0x1021) & 0xFFFF;
            } else {
                crc = (crc << 1) & 0xFFFF;
            }
        }
    }
    return crc;
}

inline std::string encodeBase32(const std::vector<uint8_t>& data) {
    std::string result;
    uint32_t buffer = 0;
    int bitsLeft = 0;

    for (uint8_t byte : data) {
        buffer = (buffer << 8) | byte;
        bitsLeft += 8;
        while (bitsLeft >= 5) {
            bitsLeft -= 5;
            result.push_back(CROCKFORD_CHARS[(buffer >> bitsLeft) & 0x1F]);
        }
    }
    if (bitsLeft > 0) {
        result.push_back(CROCKFORD_CHARS[(buffer << (5 - bitsLeft)) & 0x1F]);
    }
    return result;
}

} // namespace config_codec
""")

    # Top-level getConfigurationString function
    lines.append(f"inline std::string getConfigurationString(const DetectorConfiguration* cfg) {{")
    lines.append("    config_codec::BitWriter bw;")
    lines.append(f"    // 1. Schema Version Header ({schema_version})")
    lines.append(f"    bw.write({schema_version}, 6);")
    lines.append("")
    lines.append("    // 2. Serialized Configuration Fields")

    for f in expanded_fields:
        name = f["name"]
        member = f["member"]
        encoding = f["encoding"]
        bits = f["bits"]
        offset = f.get("offset", 0)

        if encoding == "varint":
            lines.append(f"    bw.writeVarint(cfg->{member});")
        elif encoding in ("string", "vocab"):
            lines.append(f"    bw.write(cfg->{member}.empty() ? 0 : 1, {bits});")
        elif offset != 0:
            lines.append(f"    bw.write((uint32_t)((int)std::round(cfg->{member}) + {offset}), {bits});")
        else:
            lines.append(f"    bw.write(cfg->{member}, {bits});")

    lines.append("")
    lines.append("    // 3. Flush Payload to Byte Boundary before CRC")
    lines.append("    bw.flush();")
    lines.append("")
    lines.append("    // 4. Calculate and Append 16-bit CRC-16-CCITT")
    lines.append("    uint16_t crc = config_codec::calculateCRC16(bw.buffer.data(), bw.buffer.size());")
    lines.append("    bw.write(crc >> 8, 8);")
    lines.append("    bw.write(crc & 0xFF, 8);")
    lines.append("    bw.flush();")
    lines.append("")
    lines.append("    // 5. Return Crockford Base32 String")
    lines.append("    return config_codec::encodeBase32(bw.buffer);")
    lines.append("}")
    lines.append("")

    with open(output_h_path, "w", encoding="utf-8") as f:
        f.write("\n".join(lines))

    print(f"[OK] Generated C++ firmware codec: {output_h_path}")

def main():
    print("=== Running Config Codec Generator & Coverage Verifier ===")
    schema_version, limits, fields = parse_configuration_h(CONFIG_H_PATH)
    verify_nvm_coverage(CONFIG_CPP_PATH, fields)

    schema_yaml_path = os.path.join(SCHEMA_DIR, f"schema_v{schema_version}.yaml")
    schema_data, expanded_fields = generate_yaml_schema(schema_version, limits, fields, schema_yaml_path)

    generate_cpp_header(schema_version, limits, fields, expanded_fields, OUTPUT_H_PATH)
    print("=== Generator Succeeded ===")

if __name__ == "__main__":
    main()
