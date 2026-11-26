"""Build the C509-L loader as Intel HEX and optionally package its serial bootstrap frames."""

import argparse
from functools import reduce
from hashlib import sha256
from operator import xor
from pathlib import Path
import re
from shutil import copyfile
from subprocess import CalledProcessError, run
from tempfile import TemporaryDirectory

LOAD_ADDRESS = 0xF400
BLOCK_SIZE = 16
STACK_BASE = 0x3F
DATA_BASE = 0x30
BIT_BASE = 0x08


def read_hex(path: Path) -> dict[int, int]:
    memory = {}
    for line in path.read_text(encoding="ascii").splitlines():
        if not line.startswith(":"):
            raise ValueError("invalid Intel HEX record")
        row = bytes.fromhex(line[1:])
        if len(row) < 5 or row[0] != len(row) - 5 or sum(row) & 0xFF:
            raise ValueError("invalid Intel HEX length or checksum")
        length, high, low, record_type = row[:4]
        if record_type == 1 and length == high == low == 0:
            break
        if record_type != 0:
            raise ValueError(f"unsupported Intel HEX record type {record_type}")
        start = (high << 8) | low
        for address, byte in enumerate(row[4:-1], start):
            if address in memory:
                raise ValueError(f"overlapping Intel HEX data at {address:#06x}")
            memory[address] = byte
    else:
        raise ValueError("missing Intel HEX end-of-file record")

    return memory


def read_image(path: Path) -> bytes:
    memory = read_hex(path)
    if LOAD_ADDRESS not in memory:
        raise ValueError("loader entry is missing")
    if min(memory) < LOAD_ADDRESS or max(memory) >= 0x10000:
        raise ValueError("loader extends outside 0xF400–0xFFFF")
    # Fill linker gaps with 8051 NOPs for the contiguous bootstrap transfer.
    return bytes(memory.get(address, 0) for address in range(LOAD_ADDRESS, max(memory) + 1))


def check_link_map(path: Path) -> None:
    # IRAM 0x20 contains the original flags. 0x30–0x3F holds compiler
    # temporaries, 0x40–0x5F is the stack, and 0x60 onward is fixed I/O data.
    link_map = path.read_text()
    entries = {
        name: int(value, 16)
        for value, name in re.findall(
            r"\b([0-9A-Fa-f]{8})\s+([sl]_\w+)\b", link_map
        )
    }
    entry = re.search(r"^C:\s+([0-9A-Fa-f]{8})\s+_bootstrap_entry\b", link_map, re.M)
    if not entry or int(entry[1], 16) != LOAD_ADDRESS:
        raise ValueError("bootstrap_entry must be at 0xF400")
    if entries["s_CSEG"] != LOAD_ADDRESS or entries["s_BSEG"] != BIT_BASE:
        raise ValueError("unexpected placement of C code or bit variables")
    if BIT_BASE + entries["l_BSEG"] > (DATA_BASE - 0x20) * 8:
        raise ValueError("compiler bit variables overlap the IRAM data area")
    if (DATA_BASE + entries["l_DSEG"] > STACK_BASE + 1
            or entries["s_OSEG"] + entries["l_OSEG"] > STACK_BASE + 1):
        raise ValueError("compiler data may overlap the loader stack")
    if any(entries[name] for name in ("l_ISEG", "l_XSEG", "l_XISEG", "l_XINIT")):
        raise ValueError("unexpected compiler data outside direct IRAM")


def package(payload: bytes) -> bytes:
    address = LOAD_ADDRESS.to_bytes(2, "big")
    output = bytearray([0x00])

    def append_frame(frame: bytes) -> None:
        output.extend(frame)
        output.append(reduce(xor, frame, 0))

    append_frame(bytes([0, 0]) + address + bytes([0, 0, BLOCK_SIZE + 2]))
    complete = len(payload) // BLOCK_SIZE * BLOCK_SIZE
    for offset in range(0, complete, BLOCK_SIZE):
        append_frame(bytes([1]) + payload[offset:offset + BLOCK_SIZE])

    # The type-2 frame reuses the preceding 17-byte transmit buffer.
    # Only the type, count and actual data fields are replaced.
    tail = payload[complete:]
    previous = output[-(BLOCK_SIZE + 2):-1]
    append_frame(bytes([2, len(tail)]) + tail + previous[2 + len(tail):])
    append_frame(bytes([0, 1]) + address + bytes(3))
    return bytes(output)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path, help="destination for bootstrap-c.ihx")
    parser.add_argument("--frames", type=Path, help="also write the serial bootstrap frames")
    args = parser.parse_args()
    root = Path(__file__).resolve().parent

    try:
        with TemporaryDirectory(prefix="janvar-bootstrap-c-") as tmp:
            directory = Path(tmp)
            for name in ("bootstrap.c", "bootstrap.h"):
                copyfile(root / "c509" / name, directory / name)
            commands = [
                ["sdcc", "-mmcs51", "--model-small", "--opt-code-size", "-c", "bootstrap.c"],
                ["sdld", "-nmi", "-w", "-b", f"CSEG={LOAD_ADDRESS:#x}",
                 "-b", f"BSEG={BIT_BASE:#x}", "-b", f"DSEG={DATA_BASE:#x}",
                 "bootstrap-c.ihx", "bootstrap.rel"],
            ]
            for command in commands:
                run(command, cwd=directory, check=True)
            image = directory / "bootstrap-c.ihx"
            payload = read_image(image)
            check_link_map(directory / "bootstrap-c.map")
            copyfile(image, args.output)
            if args.frames is not None:
                frames = package(payload)
                args.frames.write_bytes(frames)
                print(f"Packaged {len(frames)} bytes, SHA-256 {sha256(frames).hexdigest()}")
    except (OSError, ValueError, CalledProcessError) as error:
        parser.error(str(error))
    print(f"Built C509-L image, {len(payload)} bytes from 0x{LOAD_ADDRESS:04X}")


if __name__ == "__main__":
    main()
