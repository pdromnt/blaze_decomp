"""Repackage the committed original icon; optional regeneration needs Pillow.

Normal builds use the committed ICNS and require neither Python nor the PE.
Nearest-neighbor scaling preserves the original pixel artwork and opaque black.
"""
from hashlib import sha256
from io import BytesIO
from pathlib import Path
import struct
from PIL import Image

resources = Path(__file__).resolve().parents[1] / "resources"
original = (resources / "blaze.ico").read_bytes()
assert sha256(original).hexdigest() == "9d244a7ea82847fa23327dc3da024a0fc9727262df5c6d5c5c0773a80a2d976f"
image = Image.open(BytesIO(original)).convert("RGBA")
assert image.size == (64, 64)
chunks = []
for kind, size in [(b"icp4", 16), (b"icp5", 32), (b"icp6", 64),
                   (b"ic07", 128), (b"ic08", 256), (b"ic09", 512),
                   (b"ic10", 1024), (b"ic11", 32), (b"ic12", 64),
                   (b"ic13", 256), (b"ic14", 512)]:
    png = BytesIO()
    image.resize((size, size), Image.Resampling.NEAREST).save(png, format="PNG")
    data = png.getvalue()
    chunks.append(kind + struct.pack(">I", len(data) + 8) + data)
payload = b"".join(chunks)
target = resources / "blaze.icns"
target.write_bytes(b"icns" + struct.pack(">I", len(payload) + 8) + payload)
print(f"{target.name}: {sha256(target.read_bytes()).hexdigest()}")
