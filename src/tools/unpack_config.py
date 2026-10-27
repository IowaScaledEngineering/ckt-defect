#!/usr/bin/env python3
"""
unpack_config.py - Offline Decoder CLI for ckt-defect configuration codes.

Usage:
  python3 tools/unpack_config.py <CODE_STRING>
  python3 tools/unpack_config.py <CODE_STRING> --json
"""

import sys
import os
import argparse
import json
import yaml

CROCKFORD_MAP = {c: i for i, c in enumerate("0123456789ABCDEFGHJKMNPQRSTVWXYZ")}
# Normalizations for human error tolerance
CROCKFORD_MAP['O'] = 0
CROCKFORD_MAP['I'] = 1
CROCKFORD_MAP['L'] = 1

def decode_base32(code_str: str) -> bytes:
    clean = code_str.upper().replace("-", "").replace(" ", "")
    buffer = 0
    bits_left = 0
    out = bytearray()

    for ch in clean:
        if ch not in CROCKFORD_MAP:
            raise ValueError(f"Invalid character '{ch}' in Crockford Base32 string.")
        buffer = (buffer << 5) | CROCKFORD_MAP[ch]
        bits_left += 5
        while bits_left >= 8:
            bits_left -= 8
            out.append((buffer >> bits_left) & 0xFF)

    return bytes(out)

def calculate_crc16(payload: bytes) -> int:
    crc = 0xFFFF
    for b in payload:
        crc ^= (b << 8)
        for _ in range(8):
            if crc & 0x8000:
                crc = ((crc << 1) ^ 0x1021) & 0xFFFF
            else:
                crc = (crc << 1) & 0xFFFF
    return crc

class BitReader:
    def __init__(self, data: bytes):
        self.data = data
        self.byte_idx = 0
        self.bit_offset = 0

    def read_bits(self, bits: int) -> int:
        val = 0
        for _ in range(bits):
            if self.byte_idx >= len(self.data):
                raise ValueError("Unexpected end of bitstream.")
            bit = (self.data[self.byte_idx] >> (7 - self.bit_offset)) & 1
            val = (val << 1) | bit
            self.bit_offset += 1
            if self.bit_offset == 8:
                self.bit_offset = 0
                self.byte_idx += 1
        return val

    def read_varint(self) -> int:
        result = 0
        shift = 0
        while True:
            byte = self.read_bits(8)
            result |= (byte & 0x7F) << shift
            if not (byte & 0x80):
                break
            shift += 7
            if shift > 35:
                raise ValueError("Varint overflow.")
        return result

def unpack_config(code_str: str, schema_dir="tools/schemas"):
    raw_bytes = decode_base32(code_str)
    if len(raw_bytes) < 3:
        raise ValueError(f"Code string is too short ({len(raw_bytes)} bytes).")

    payload = raw_bytes[:-2]
    expected_crc = (raw_bytes[-2] << 8) | raw_bytes[-1]
    actual_crc = calculate_crc16(payload)

    if actual_crc != expected_crc:
        raise ValueError(f"CRC verification failed! (Calculated: 0x{actual_crc:04X}, Expected: 0x{expected_crc:04X})")

    reader = BitReader(payload)
    schema_version = reader.read_bits(6)

    schema_file = os.path.join(schema_dir, f"schema_v{schema_version}.yaml")
    if not os.path.isfile(schema_file):
        raise FileNotFoundError(f"Schema definition '{schema_file}' for version {schema_version} not found.")

    with open(schema_file, "r", encoding="utf-8") as f:
        schema = yaml.safe_load(f)

    config_data = {
        "_schema_version": schema_version,
        "_crc16": f"0x{expected_crc:04X}",
        "_raw_payload_length_bytes": len(payload),
    }

    fields = schema.get("fields", [])
    for field in fields:
        name = field["name"]
        encoding = field["encoding"]
        cpp_type = field.get("cpp_type", "")
        offset = field.get("offset", 0)

        if encoding == "varint":
            val = reader.read_varint()
        elif encoding in ("string", "vocab"):
            v_code = reader.read_bits(field.get("bits", 8))
            val = "internal" if v_code == 0 else "custom"
        elif cpp_type == "bool" or (encoding == "bits" and field.get("bits") == 1 and offset == 0):
            val = bool(reader.read_bits(1))
        else:
            raw_val = reader.read_bits(field["bits"])
            val = raw_val - offset

        config_data[name] = val

    return config_data, schema

def format_output_table(config: dict, schema: dict) -> str:
    lines = []
    lines.append("=" * 65)
    lines.append(f" DEFECT DETECTOR CONFIGURATION (Schema v{config.get('_schema_version')})")
    lines.append(f" CRC: {config.get('_crc16')} (Valid) | Payload: {config.get('_raw_payload_length_bytes')} bytes")
    lines.append("=" * 65)

    # Dynamically group fields by the category defined in the schema YAML
    categories = {}
    for f in schema.get("fields", []):
        cat = f.get("category", "General Settings")
        categories.setdefault(cat, []).append(f)

    for cat_name, cat_fields in categories.items():
        lines.append(f"\n--- {cat_name} ---")
        for f in cat_fields:
            name = f["name"]
            val = config.get(name)
            if val is None:
                continue

            # Format dynamically using YAML metadata (zero hardcoded field names!)
            mult = f.get("multiplier", 1)
            div = f.get("divisor", 1)
            display_unit = f.get("display_unit")
            encoding = f.get("encoding")

            if encoding in ("string", "vocab"):
                display_val = '"" (internal / default)' if val in (0, "internal") else "custom"
            elif mult > 1 and display_unit:
                display_val = f"{val} ({val * mult} {display_unit})"
            elif div > 1 and display_unit:
                display_val = f"{val} ({val / div:.1f} {display_unit})"
            elif display_unit:
                display_val = f"{val} {display_unit}"
            elif "temperature" in name.lower() and f.get("offset") == 100:
                f_deg = (val * 1.8) + 32.0
                display_val = f"{val:.0f}°C / {f_deg:.0f}°F"
            else:
                display_val = str(val)

            lines.append(f"  {name:<32}: {display_val}")

    lines.append("=" * 65)
    return "\n".join(lines)

def main():
    parser = argparse.ArgumentParser(description="Decode ckt-defect Configuration String.")
    parser.add_argument("code", help="Alphanumeric Crockford Base32 configuration string")
    parser.add_argument("--json", action="store_true", help="Output JSON instead of table")
    parser.add_argument("--schema-dir", default="tools/schemas", help="Directory containing schema YAML files")
    args = parser.parse_args()

    try:
        data, schema = unpack_config(args.code, args.schema_dir)
        if args.json:
            print(json.dumps(data, indent=2))
        else:
            print(format_output_table(data, schema))
    except Exception as e:
        print(f"[ERROR] {e}", file=sys.stderr)
        sys.exit(1)

if __name__ == "__main__":
    main()
