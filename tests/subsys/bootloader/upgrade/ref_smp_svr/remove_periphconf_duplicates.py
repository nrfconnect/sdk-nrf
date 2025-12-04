#!/usr/bin/env python3
"""Remove duplicate periphconf entries where regptr and value are identical and print to stdout"""

import ctypes as c
import sys


class PeriphconfEntry(c.LittleEndianStructure):
    _pack_ = 1
    _fields_ = [("regptr", c.c_uint32), ("value", c.c_uint32)]


def main():
    data = open(sys.argv[1], 'rb').read()
    num_entries = len(data) // c.sizeof(PeriphconfEntry)
    entries = (PeriphconfEntry * num_entries).from_buffer_copy(data)

    seen = set()
    unique_entries = []
    for entry in entries:
        if entry.regptr == 0xFFFFFFFF: # Don't remove the padding
            unique_entries.append(entry)
        else:
            key = (entry.regptr, entry.value)
            if key not in seen:
                seen.add(key)
                unique_entries.append(entry)

    result = (PeriphconfEntry * len(unique_entries))(*unique_entries)
    sys.stdout.buffer.write(bytes(result))


if __name__ == "__main__":
    main()
