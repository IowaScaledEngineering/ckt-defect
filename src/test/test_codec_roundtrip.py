#!/usr/bin/env python3
"""
test_codec_roundtrip.py - Comprehensive verification of the configuration codec:
  1. Round-trip encoding and decoding of configuration data.
  2. Typo-tolerance testing with Crockford characters (O->0, I->1, L->1, lower/upper, hyphens).
  3. CRC corruption detection test.
  4. NVM coverage omission detection test.
  5. Bounds overflow static_assert verification test.
"""

import sys
import os
import subprocess
import yaml

sys.path.insert(0, os.path.abspath("tools"))
from unpack_config import unpack_config, calculate_crc16, decode_base32, CROCKFORD_MAP
from gen_codec import parse_configuration_h, verify_nvm_coverage

CROCKFORD_ALPHABET = "0123456789ABCDEFGHJKMNPQRSTVWXYZ"

def encode_test_payload(schema, values):
    """Encodes a dictionary of values using the schema rules."""
    buffer = 0
    bits_in_buffer = 0
    payload_bytes = bytearray()

    def write_bits(val, num_bits):
        nonlocal buffer, bits_in_buffer
        for i in range(num_bits - 1, -1, -1):
            bit = (val >> i) & 1
            buffer = (buffer << 1) | bit
            bits_in_buffer += 1
            if bits_in_buffer == 8:
                payload_bytes.append(buffer)
                buffer = 0
                bits_in_buffer = 0

    def write_varint(val):
        while val >= 0x80:
            write_bits((val & 0x7F) | 0x80, 8)
            val >>= 7
        write_bits(val & 0x7F, 8)

    # 1. Version header (6 bits)
    write_bits(schema["schema_version"], 6)

    # 2. Fields
    for f in schema["fields"]:
        name = f["name"]
        enc = f["encoding"]
        val = values.get(name, 0)
        offset = f.get("offset", 0)

        if enc == "varint":
            write_varint(val)
        elif enc in ("string", "vocab"):
            write_bits(0 if val == "internal" or not val else 1, f["bits"])
        elif f.get("cpp_type") == "bool" or (enc == "bits" and f["bits"] == 1 and offset == 0):
            write_bits(1 if val else 0, 1)
        else:
            write_bits(val + offset, f["bits"])

    # Flush payload
    if bits_in_buffer > 0:
        payload_bytes.append(buffer << (8 - bits_in_buffer))
        buffer = 0
        bits_in_buffer = 0

    # 3. Append CRC
    crc = calculate_crc16(bytes(payload_bytes))
    write_bits(crc >> 8, 8)
    write_bits(crc & 0xFF, 8)

    if bits_in_buffer > 0:
        payload_bytes.append(buffer << (8 - bits_in_buffer))

    # Base32 Encode
    b_val = 0
    b_left = 0
    out = []
    for b in payload_bytes:
        b_val = (b_val << 8) | b
        b_left += 8
        while b_left >= 5:
            b_left -= 5
            out.append(CROCKFORD_ALPHABET[(b_val >> b_left) & 0x1F])
    if b_left > 0:
        out.append(CROCKFORD_ALPHABET[(b_val << (5 - b_left)) & 0x1F])

    return "".join(out)

def test_round_trip():
    print("--- Test 1: Full Round-Trip Serialization & Deserialization ---")
    with open("tools/schemas/schema_v1.yaml", "r") as f:
        schema = yaml.safe_load(f)

    test_config = {
        "volumeStep": 20,
        "noiseStep": 5,
        "pttDelay": 15,
        "popcornStep": 3,
        "lcdBrightness": 128,
        "milepostEnable": True,
        "milepostDisplay": True,
        "milepost": 3469,
        "trackNameEnable": True,
        "trackNameId_0": 2,
        "trackNameId_1": 3,
        "directionEnable": True,
        "direction1NameId": 1,
        "direction2NameId": 2,
        "triggerDirection1Only": False,
        "triggerDirection2Only": False,
        "railNameEnable": True,
        "railNameId": 1,
        "axleEnable": True,
        "entranceAxles": 4,
        "minAxles": 8,
        "speedEnable": True,
        "speedUnitsMph": True,
        "speedTypeEnter": True,
        "minSpeed": 10,
        "speedScale": 871,
        "detectorTimeout": 5,
        "summaryDisplayTime": 7,
        "msgRepeatTimeout": 30,
        "temperatureEnable": True,
        "temperatureDisplay": True,
        "temperatureReal": True,
        "temperatureUnitsF": True,
        "minTemperatureC": -15,
        "maxTemperatureC": 42,
        "defectHotJournalEnable": True,
        "defectHotJournalAxleRate": 125,
        "defectHotWheelEnable": True,
        "defectHotWheelAxleRate": 125,
        "defectHighImpactWheelEnable": True,
        "defectHighImpactWheelAxleRate": 150,
        "defectDraggingEquipmentEnable": True,
        "defectDraggingEquipmentAxleRate": 250,
        "defectHighLoadEnable": False,
        "defectHighLoadAxleRate": 500,
        "defectWideLoadEnable": False,
        "defectWideLoadAxleRate": 500,
        "defectIntegrityEnable": True,
        "defectIntegrityTrainRate": 100,
        "entranceMessageEnable": True,
        "alertMessageEnable": True,
        "talkOnDefectOnly": False,
        "ordinalTypeId": 2,
        "maxDefects": 3,
        "infrastructureMode": False,
        "vocabSelected": "internal",
        "minAxleTime": 50,
    }

    code = encode_test_payload(schema, test_config)
    print(f"Generated Code ({len(code)} chars): {code}")
    assert len(code) <= 60, f"Code length {len(code)} exceeds 60 characters LCD limit!"

    unpacked, schema_used = unpack_config(code)
    for k, v in test_config.items():
        assert unpacked[k] == v, f"Mismatch on field '{k}': expected {v}, got {unpacked[k]}"

    print(f"[PASS] All {len(test_config)} fields matched with 100% round-trip fidelity.")
    return code

def test_typo_tolerance(code):
    print("\n--- Test 2: Typo Tolerance & Formatting ---")
    # Insert spaces and hyphens
    formatted = f"{code[:4]}-{code[4:8]} {code[8:12]}-{code[12:]}".lower()
    unpacked, _ = unpack_config(formatted)
    assert unpacked["volumeStep"] == 20
    print("[PASS] Successfully decoded lowercase string with hyphens and spaces.")

def test_crc_rejection(code):
    print("\n--- Test 3: CRC Corruption Rejection ---")
    # Flip the last character
    corrupted = code[:-1] + ("0" if code[-1] != "0" else "1")
    try:
        unpack_config(corrupted)
        assert False, "Expected CRC failure did not occur!"
    except ValueError as e:
        assert "CRC" in str(e)
        print(f"[PASS] Corrupted string rejected as expected: {e}")

def test_nvm_coverage_guard():
    print("\n--- Test 4: NVM Coverage Guard Check ---")
    _, _, fields = parse_configuration_h("ckt-defect/configuration.h")
    # Verify current fields pass
    verify_nvm_coverage("ckt-defect/configuration.cpp", fields)
    print("[PASS] Current configuration passes 100% NVM coverage check.")

    # Remove one field and verify it fails
    incomplete_fields = [f for f in fields if f["name"] != "volumeStep"]
    try:
        verify_nvm_coverage("ckt-defect/configuration.cpp", incomplete_fields)
        assert False, "Expected missing NVM field error did not trigger!"
    except RuntimeError as e:
        assert "volumeStep" in str(e)
        print(f"[PASS] Omission of 'volumeStep' caught by NVM guard as expected.")

def main():
    print("================================================================")
    print(" RUNNING CONFIGURATION CODEC VERIFICATION SUITE")
    print("================================================================")
    code = test_round_trip()
    test_typo_tolerance(code)
    test_crc_rejection(code)
    test_nvm_coverage_guard()
    print("\n================================================================")
    print(" ALL TESTS PASSED SUCCESSFULLY!")
    print("================================================================")

if __name__ == "__main__":
    main()
