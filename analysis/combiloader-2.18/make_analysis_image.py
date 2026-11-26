"""Rebuild the static PE input used by REA for this CombiLoader build."""

import argparse
from hashlib import sha256
from pathlib import Path
import struct

import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_EDI, UC_X86_REG_EIP

# The entry, stop address, sections and import-table offsets are specific
# to the executable identified by this digest.
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("source", type=Path)
parser.add_argument("output", type=Path)
parser.add_argument(
    "--bootstrap-frames",
    type=Path,
    help="compare the embedded loader against the captured 147 bootstrap frames",
)
parser.add_argument(
    "--starter-relay-template",
    type=Path,
    help="extract the 222-byte starter relay template from this executable",
)
args = parser.parse_args()
source = args.source.read_bytes()
if sha256(source).hexdigest() != "c1c0f2695ed217ddb4d9894ed34ad96ee7af8f428e28b0822809bb27b92c59aa":
    parser.error("source SHA-256 does not match the analyzed executable")

pe = pefile.PE(data=source)
base = pe.OPTIONAL_HEADER.ImageBase
emulator = Uc(UC_ARCH_X86, UC_MODE_32)
emulator.mem_map(base, pe.OPTIONAL_HEADER.SizeOfImage)
emulator.mem_write(base, pe.get_memory_mapped_image())
emulator.mem_map(0x10000000, 0x10000)
emulator.reg_write(UC_X86_REG_ESP, 0x10008000)
# Stop before the first instruction of import resolution. The app entry
# and Win32 APIs are outside the emulated path.
emulator.emu_start(0x7987b0, 0x798896, timeout=15_000_000, count=40_000_000)
if emulator.reg_read(UC_X86_REG_EIP) != 0x798896:
    raise RuntimeError("decompression did not reach the expected stop address")
end = emulator.reg_read(UC_X86_REG_EDI)
image = emulator.mem_read(base, pe.OPTIONAL_HEADER.SizeOfImage)
print(f'Decompressed range 0x401000–{end:#x}')

def u32(offset):
    return struct.unpack_from('<I', image, offset)[0]

def cstr(offset):
    return bytes(image[offset:image.index(0, offset)])

offset = 0x396000
imports = []
while dll_offset := u32(offset):
    dll = cstr(0x39a2a0 + dll_offset)
    iat = 0x1000 + u32(offset + 4)
    offset += 8
    names = []
    while (tag := image[offset]) != 0:
        offset += 1
        if tag != 1:
            raise ValueError((hex(offset), tag))
        name = cstr(offset)
        names.append(name)
        offset += len(name) + 1
    offset += 1
    imports.append((dll, iat, names))
    print(dll.decode(), hex(iat + base), len(names))

# Import descriptors and lookup tables for static analysis. The IAT stays at
# its original addresses, so disassembly retains references to the right slots.
import_rva = 0x400000
table = bytearray((len(imports) + 1) * 20)
for i, (dll, iat, names) in enumerate(imports):
    dll_rva = import_rva + len(table)
    table.extend(dll + b'\0')
    while len(table) % 4:
        table.append(0)
    ilt = import_rva + len(table)
    slots = len(table)
    table.extend(bytes((len(names) + 1) * 4))
    for j, name in enumerate(names):
        while len(table) % 2:
            table.append(0)
        name_rva = import_rva + len(table)
        table.extend(b'\0\0' + name + b'\0')
        struct.pack_into('<I', table, slots + 4 * j, name_rva)
        struct.pack_into('<I', image, iat + 4 * j, name_rva)
    struct.pack_into('<5I', table, 20 * i, ilt, 0, 0, dll_rva, iat)

# Reuse the original PE32 header and address space. Section names and import
# metadata describe this static analysis image, not the packed file on disk.
sections = [
    (b'.text', 0x1000, 0xeb000, 0x60000020, image[0x1000:0xec000]),
    (b'.data', 0xec000, 0x223000, 0xc0000040, image[0xec000:min(0x30f000, end - base)]),
    (b'.more', 0x30f000, 0x87000, 0xc0000040, image[0x30f000:min(0x396000, end - base)]),
    (b'.idata', import_rva, len(table), 0xc0000040, table),
]
sections = [section for section in sections if len(section[4])]
header = bytearray(image[:0x1000])
nt = pe.DOS_HEADER.e_lfanew
opt = nt + 24
struct.pack_into('<H', header, nt + 6, len(sections))
struct.pack_into('<I', header, opt + 16, 0x141a)
struct.pack_into('<I', header, opt + 20, 0x1000)
struct.pack_into('<I', header, opt + 24, 0xec000)
struct.pack_into('<I', header, opt + 56, (import_rva + len(table) + 0xfff) & ~0xfff)
header[opt + 96:opt + 224] = bytes(128)
struct.pack_into('<II', header, opt + 104, import_rva, 20 * (len(imports) + 1))
section_offset = opt + pe.FILE_HEADER.SizeOfOptionalHeader
output = header
for name, rva, virtual_size, flags, data in sections:
    aligned_size = (len(data) + 0x1ff) & ~0x1ff
    struct.pack_into('<8s6I2HI', output, section_offset, name, virtual_size, rva, aligned_size, len(output), 0, 0, 0, 0, flags)
    section_offset += 40
    output.extend(data)
    output.extend(bytes(aligned_size - len(data)))
def decode_embedded(start, size):
    # The transform takes a synthetic return address in scratch memory.
    scratch, stack, stop = 0x10002000, 0x10008000, 0x10001000

    def run_routine(address, *parameters):
        stack_values = struct.pack(f"<{len(parameters) + 1}I", stop, *parameters)
        emulator.mem_write(stack, stack_values)
        emulator.reg_write(UC_X86_REG_ESP, stack)
        emulator.emu_start(address, stop, count=100_000)
        if emulator.reg_read(UC_X86_REG_EIP) != stop:
            raise RuntimeError(f"routine at {address:#x} did not return")

    run_routine(0x401a54, 0x4ec6a4)
    decoded = bytearray()
    block_end = size - size % 16
    for offset in range(0, block_end, 16):
        run_routine(0x401d4c, start + offset, scratch)
        decoded.extend(emulator.mem_read(scratch, 16))
    decoded.extend(emulator.mem_read(start + block_end, size - block_end))
    return decoded


if args.bootstrap_frames:
    frames = args.bootstrap_frames.read_bytes()
    if (
        len(frames) != 2627
        or frames[:8] != bytes.fromhex("00 00 00 f4 00 00 00 12")
        or frames[2601:2603] != bytes.fromhex("02 02")
    ):
        parser.error("expected captured bootstrap frames in bench sidecar format")
    blocks = [frames[9 + 18 * i : 27 + 18 * i] for i in range(144)]
    if any(block[0] != 1 for block in blocks):
        parser.error("unexpected bootstrap data frame type")
    captured = b"".join(block[1:17] for block in blocks) + frames[2603:2605]

    decoded = decode_embedded(0x4e0bac, 2306)
    if decoded != captured:
        raise RuntimeError("decrypted bootstrap does not match the capture")
    print(f"Matched {len(decoded)} captured loader bytes, SHA-256 {sha256(decoded).hexdigest()}")

if args.starter_relay_template:
    template = decode_embedded(0x4e1918, 222)
    with args.starter_relay_template.open("xb") as template_file:
        template_file.write(template)
    print(f"Starter relay template SHA-256: {sha256(template).hexdigest()}")

with args.output.open("xb") as output_file:
    output_file.write(output)
print(f"Analysis image SHA-256: {sha256(output).hexdigest()}")
