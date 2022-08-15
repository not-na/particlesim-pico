#!/bin/env python3

import sys
import re
import subprocess

# 2MB Flash
FLASH_SIZE = 2*1024*1024
# 264KB SRAM, though only 256KB available for non-stack uses
RAM_SIZE = 256*1024

# We accept exactly one argument: the filename of the final .elf to check
if len(sys.argv) != 2:
    print(f"ERROR: Expected exactly one argument, but got {len(sys.argv)-1}!")
    sys.exit(2)

elf_fname = sys.argv[1]
if not elf_fname.endswith(".elf"):
    print(f"ERROR: filename should end with .elf! Got '{elf_fname}'")
    sys.exit(2)

# Get output, split by lines and remove the header line
out = subprocess.run(
    ["size", "-B", "-d", elf_fname],
    capture_output=True,
    check=True,
).stdout.decode("ascii").splitlines(False)[1:]

if len(out) != 1:
    print("Unexpected number of output lines")
    sys.exit(2)

# Extraction patterns based on https://github.com/platformio/platformio-core/blob/bedbae6311c7067ed1fa7c29664eb8d96bd96e38/platformio/builder/tools/pioupload.py#L152
pattern_prog = re.compile(r"^(\d+)\s+(\d+)\s+\d+\s")
pattern_data = re.compile(r"^\d+\s+(\d+)\s+(\d+)\s+\d+")

line = out[-1]  # We only care about the last line
line = line.strip()


def extract_usage(l, pattern, name):
    match = pattern.search(l)
    if not match:
        print(f"Couldn't extract {name} usage!")
        return 0
    return sum(int(val) for val in match.groups())


# Extract usages
flash = extract_usage(line, pattern_prog, "flash")
ram = extract_usage(line, pattern_data, "RAM")


def print_usage(name, used, total):
    perc = (used/total)
    blockcnt = 50
    blocks = "="*min(int(blockcnt*perc+0.5), blockcnt)
    print(f"{name.ljust(10, ' ')}: {used: 9}B of {total: 9}B ({perc*100:6.2f}%) {total-used: 9}B free [{blocks}{' '*(blockcnt-len(blocks))}]")

    if used > total:
        print(f"ERROR: Used more than 100% of available {name}!")


# Print out everything
print("Memory usage:")
print_usage("Flash", flash, FLASH_SIZE)
print_usage("RAM", ram, RAM_SIZE)

if flash > FLASH_SIZE or ram > RAM_SIZE:
    print(f"Aborting due to memory overrun")
    sys.exit(2)
