"""Extract canonical ROP/motion/icon resources; never load or execute the PE.

0x420060 copies these 65536 bytes unchanged. No runtime PE dependency.
"""
import hashlib
from pathlib import Path
import struct
import sys

data = Path(sys.argv[1]).read_bytes()
if hashlib.sha256(data).hexdigest() != "02332ea9c4746ce6255f044097c4b095f44c539f265927a3ca76ca01737fa74d":
    raise SystemExit("Resource extraction requires the canonical executable")
pe = struct.unpack_from("<I", data, 0x3c)[0]
count = struct.unpack_from("<H", data, pe + 6)[0]
optional_size = struct.unpack_from("<H", data, pe + 20)[0]
for i in range(count):
    section = pe + 24 + optional_size + i * 40
    _, rva, size, offset = struct.unpack_from("<IIII", data, section + 8)
    if rva <= 0x58560 and 0x68560 <= rva + size:
        start = offset + 0x58560 - rva
        result = data[start:start + 65536]
        if len(result) != 65536:
            raise SystemExit("Truncated WINGLROP resource")
        target = Path(sys.argv[2])
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(result)
        break
else:
    raise SystemExit("WINGLROP resource is outside file-backed sections")

# 320 signed little-endian integer sine samples shared by the original motion
# routines (the cosine view starts 64 samples later). Preserve exact rounding.
for i in range(count):
    section = pe + 24 + optional_size + i * 40
    _, rva, size, offset = struct.unpack_from("<IIII", data, section + 8)
    if rva <= 0x3b7f8 and 0x3bcf8 <= rva + size:
        start = offset + 0x3b7f8 - rva
        Path(sys.argv[2]).with_name("trig.bin").write_bytes(data[start:start + 1280])
        break
else:
    raise SystemExit("Motion table is outside file-backed sections")

# Repackage RT_GROUP_ICON + its referenced RT_ICON payloads as an ICO. Walk
# the PE resource directory rather than relying on a particular resource RVA.
def file_offset(address, length):
    for i in range(count):
        section = pe + 24 + optional_size + i * 40
        _, rva, size, offset = struct.unpack_from("<IIII", data, section + 8)
        if rva <= address and address + length <= rva + size:
            result = offset + address - rva
            if result + length <= len(data):
                return result
    raise SystemExit("Resource is outside file-backed sections")


optional = pe + 24
if struct.unpack_from("<H", data, optional)[0] != 0x10b:
    raise SystemExit("Expected canonical PE32 resource layout")
resource_rva, resource_size = struct.unpack_from("<II", data, optional + 96 + 2 * 8)
resource_base = file_offset(resource_rva, resource_size)


def entries(relative):
    named, ids = struct.unpack_from("<HH", data, resource_base + relative + 12)
    return [struct.unpack_from("<II", data, resource_base + relative + 16 + i * 8)
            for i in range(named + ids)]


def resource(kind, identifier):
    directory = 0
    for key in (kind, identifier):
        matches = [value for name, value in entries(directory) if name == key]
        if len(matches) != 1 or not matches[0] & 0x80000000:
            raise SystemExit("Missing or ambiguous icon resource")
        directory = matches[0] & 0x7fffffff
    # The canonical executable has one Japanese-language variant (1041).
    variants = entries(directory)
    if len(variants) != 1 or variants[0][0] != 1041 or variants[0][1] & 0x80000000:
        raise SystemExit("Unexpected icon resource languages")
    address, size = struct.unpack_from("<II", data, resource_base + variants[0][1])
    offset = file_offset(address, size)
    return data[offset:offset + size]


group = resource(14, 1)
reserved, kind, images = struct.unpack_from("<HHH", group)
if reserved or kind != 1 or len(group) != 6 + images * 14:
    raise SystemExit("Invalid icon group")
directory, payloads, offset = [], [], 6 + images * 16
for i in range(images):
    entry = group[6 + i * 14:6 + (i + 1) * 14]
    size, identifier = struct.unpack_from("<IH", entry, 8)
    payload = resource(3, identifier)
    if len(payload) != size:
        raise SystemExit("Icon payload size mismatch")
    directory.append(entry[:12] + struct.pack("<I", offset))
    payloads.append(payload)
    offset += size
Path(sys.argv[2]).with_name("blaze.ico").write_bytes(
    group[:6] + b"".join(directory) + b"".join(payloads))
