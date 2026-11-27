"""Cross-language verification against the USB handoff, without Qt or a physical device.

cmake -S tests/beamgo -B /tmp/beamgo-tests
cmake --build /tmp/beamgo-tests
ctest --test-dir /tmp/beamgo-tests --output-on-failure
"""
import json
from pathlib import Path
import subprocess
import sys

vectors = json.loads((Path(__file__).parent / "test-vectors.json").read_text())
cases = []


def check(command, expected):
    cases.append((command, expected))


for vector in vectors["hostFrames"]:
    check(f'tx {vector["plaintextHex"]}', vector["frameHex"])
for job in vectors["jobs"].values():
    for vector in job.get("sequence", []):
        check(f'tx {vector["plaintextHex"]}', vector["frameHex"])
for vector in vectors["fileTransfer0x21"]["packets"]:
    check(f'tx {vector["plaintextHex"]}', vector["frameHex"])

for vector in vectors["capturedReplies"]:
    frame = vector["frameHex"]
    plain = vector["plaintextHex"]
    for split in range(len(frame) // 2 + 1):
        check(f'rx 0013{frame} {split + 2}', plain)
    bad = bytearray.fromhex(frame)
    bad[5] ^= 1
    check(f'rx {bad.hex()}{frame}{frame} 0', f'{plain},{plain}')

mark = vectors["jobs"]["markSquare20mmP15S100"]
check('mark-blocks ' + ''.join(mark["recordsHex"]),
      ','.join(vector["plaintextHex"] for vector in mark["sequence"][:-1]))
preview = vectors["jobs"]["previewSquare20mm"]
check('preview-blocks ' + ''.join(preview["recordsHex"]), preview["sequence"][0]["plaintextHex"])

square = 'M100\nM104\nM5\nG90\nW90\nM3S150\nF6000\nG1X35Y35\nG1X55Y35\nG1X55Y55\nG1X35Y55\nG1X35Y35\nM2\n'
check('gcode ' + square.encode().hex(), ''.join(mark['recordsHex']))
check('gcode ' + 'M100\n'.encode().hex(), '')
for power in (5, 15, 30):
    records = [f'120000{power * 10:04x}' if record.startswith('12') else record for record in preview['recordsHex']]
    for spindle_power in (0, 150, 1000):
        code = square.replace('M104', 'M103').replace('S150', f'S{spindle_power}')
        check(f'framing {code.encode().hex()} {power}', ''.join(records))
for code, error in [
    ('M101', 'UNSUPPORTED: M command'), ('A10', 'UNSUPPORTED: A'),
    ('Z0', 'UNSUPPORTED: Z'), ('M102', 'UNSUPPORTED: M command'),
    ('WS0WD0', 'UNSUPPORTED: WS'), ('G2X10Y10', 'UNSUPPORTED: G command'),
    ('M3S1001', 'Invalid power'), ('M3\nG1X91Y20', 'OUT_OF_BOUNDS'),
    ('M3\nG1X-1Y20', 'OUT_OF_BOUNDS'), ('M3\nG1X20Y20\nF120060S100X30', 'Invalid marking speed'),
    ('D1PC32\nD2WFF\nD3FE', 'Short raster data'),
    (';CONFIG INNER_ENGRAVING=1', 'UNSUPPORTED: inner engraving'),
]:
    check('gcode ' + code.encode().hex(), 'ERROR:' + error)
for power in (0, 101):
    check(f'framing {square.encode().hex()} {power}', 'ERROR:INVALID_PARAMS')

def compiled(code):
    result = subprocess.run([sys.argv[1]], input=f'gcode {code.encode().hex()}\n', text=True, capture_output=True, check=True)
    return bytes.fromhex(result.stdout.strip())

# Modal coordinates and per-row raster expansion. White pixels must be jump records.
relative = compiled('M3S150\nG1X10Y10\nG91\nG1X5Y-2\nM2')
assert relative[-10:-5] == bytes.fromhex('012aaa16c1')
raster = compiled('M4S0\nG1X10Y10\nD0RM\n;PIXEL SIZE 0.1\nD1PC4\nD2WB\nD3FE\nD4PL\nG1X10.4S150\nD5\nM2')
motion = [raster[i:i+5] for i in range(0, len(raster), 5) if raster[i] in (1, 2)]
assert [record[0] for record in motion] == [2, 1, 2, 1]

# A full block is 5004 plaintext bytes / 5015 framed bytes. Test the exact fixture hash.
full = "10010000" + "0212345678" * 1000
import hashlib
result = subprocess.run([sys.argv[1]], input=f'tx {full}\n', text=True, capture_output=True, check=True)
assert hashlib.sha256(bytes.fromhex(result.stdout.strip())).hexdigest() == vectors["jobs"]["fullBlock"]["frameSha256"]

result = subprocess.run([sys.argv[1]], input='\n'.join(command for command, _ in cases) + '\n',
                        text=True, capture_output=True, check=True)
outputs = result.stdout.splitlines()
assert len(outputs) == len(cases), (len(outputs), len(cases), result.stderr)
for output, (command, expected) in zip(outputs, cases):
    assert output == expected, (command[:100], output, expected)
print(f'PASS: {len(cases)} protocol cases and full-block fixture hash')
