# Packed galvo dots (opcode 0x20)

Status: implemented.
Applies to: HEXA II galvo heads (`LayerModule::GALVO_CO2`, `GALVO_MOPA`).
Settles: `HX2_GALVO_PROTOCOL.md` §16-D1, whose 0x20 row still reads "參數待定".

## What this is

A compression of the dot stream, and nothing else. The decoder restores every
site to the same `JUMP_ABS` + `LASER_ON` pair swiftray would have written
uncompressed, calling the same SDK entry points in the same order.

That single sentence settles most of the design:

- the §14 time model does not change, because the machine does the same work;
- every downstream quantity -- list length, the card's replay recording, the
  per-dot delay the card charges -- is still counted on the *expanded* stream,
  so compression must declare that count rather than hide it;
- correctness has an exact definition. The expansion of a packed run must be
  byte-identical to the uncompressed emission of the same geometry, which is a
  diff, not a judgement.

What it does not buy is engraving time. At 2000 DPI the pitch is 0.0127 mm,
which lands at the floor of the variable delay window, so the card charges
roughly `kJumpDelayMinUs` = 200 µs per dot however the file states it. A
100 x 100 mm field at full density is 62 M dots, i.e. about 3.4 hours in jump
delay alone. Reducing that needs a different marking mode on the card and is
not this.

What it does buy, on that same field: 2.0 GB of records becomes 7.9 MB.

## Why opcode 0x20 and not command byte 16

The gantry already has a compressed raster path: fcode command byte 16, with
sub-commands 1-5 (`FCodeGenerator::set_line_pixels` / `fill_32_pixels` /
`set_fill_end` / `set_print_line_status`, read back in
`task_loader_v2.cpp` under `kCustomCmdFastRaster`). It cannot be reused here.

§19.1 defines a galvo block as a *contiguous run of command byte 23*, ending at
the first command that is not byte 23, where the player inserts a sync barrier.
A command-16 record inside a galvo layer would therefore close the block,
barrier, and reopen -- once per scan line. The packed run has to be a byte-23
record, which means a galvo opcode, which is 0x20.

## Record layout

A packed run is an ordinary galvo record, so a parser that knows nothing about
0x20 still skips it correctly by `4 + 8 * paramCount` bytes:

```
byte   23                      command byte
u16    0x20                    opcode (LE)
u8     paramCount              7 + slot_count
f64[7] header                  see below
u8[slot_count * 8] mask        site bitmap, MSB first, zero-padded
```

### Header

| # | field | type | meaning |
|---|---|---|---|
| 0 | `origin_x` | f64 mm | field-local position of site 0 |
| 1 | `origin_y` | f64 mm | field-local position of site 0 |
| 2 | `step_x` | f64 mm | added to x per site index |
| 3 | `step_y` | f64 mm | added to y per site index |
| 4 | `site_count` | f64, integral | N: how many bits of the mask are meaningful |
| 5 | `dot_count` | f64, integral | popcount of those N bits |
| 6 | `dwell_us` | f64 µs | the `LASER_ON` parameter for every dot in this run |

Site `i` sits at `(origin_x + i * step_x, origin_y + i * step_y)`, in the same
field-local frame `GalvoListWriter::toField()` produces.

`(step_x, step_y)` carries the axis, the direction and the pitch in one place:
an X run has `step_y = 0`, a Y run has `step_x = 0`, a backwards run has a
negative component, and the pitch is the magnitude. X and Y pitch need not
agree, because they are independent numbers rather than one resolution -- which
is what lets `pixel_per_mm_x` differ from `pixel_per_mm`, and lets the seam
band use `galvo_dot_blend_pitch` instead of the image's own pitch. A diagonal
run is representable at no extra cost; nothing emits one today.

`dwell_us` is in the record rather than taken from the list prologue because
`dotting_time_us` is not part of the prologue: it is swiftray-side state that
chooses dotting over marking, and the dwell only ever reaches the file as
`LASER_ON`'s parameter.

### Mask

A byte string: site `i` is byte `i >> 3`, bit `7 - (i & 7)`. A set bit is a dot.

MSB first within each byte, which is `fg_iterate_x`'s convention -- it fills
from bit 31 downwards, and the gantry path's `M132W%08X` shows that order
directly. Here it also means the bytes on disk run in site order, with no
endianness question to get wrong in a hand decoder: byte `k` of a parameter slot
holds sites `8k .. 8k+7`, in that order.

The mask occupies `ceil(site_count / 8)` bytes, zero-padded to a whole number of
8-byte parameter slots. Every bit from `site_count` to the end of the padding
MUST be zero.

### Canonical form

A conforming encoder MUST satisfy all of these, and a decoder MAY reject a
record that does not:

1. `site_count >= 1`, `dot_count >= 1`.
2. Bit `0` and bit `site_count - 1` are both set. The run begins and ends on a
   dot, so `origin` is the first dot's position and there are no trailing zeros.
3. `(step_x, step_y) != (0, 0)`.
4. `dot_count` equals the popcount of bits `0 .. site_count - 1`.
5. No gap of zeros longer than `floor(kJumpDelayLimitMm / |step|)` sites. This
   is what makes the time closed form exact -- see below -- and it also
   compresses better: a gap of G zeros costs G bits, while a fresh record costs
   60 bytes, so splitting pays from about G = 480 upwards, which at 2000 DPI is
   well inside the limit of 787.
6. `paramCount <= 255`, so `slot_count <= 248` and `site_count <= 15872`.
   See "Limits on one record" below: this is never the binding constraint.

### Worked example

A Y-direction run at 2000 DPI (pitch 0.0127 mm) starting at field-local
(10.0, -5.0), 70 sites, with sites 0-2 and 67-69 marked:

```
17                                 command byte 23
20 00                              opcode 0x20
09                                 paramCount = 7 + 2 slots
f64  10.0        origin_x
f64  -5.0        origin_y
f64  0.0         step_x
f64  0.0127      step_y
f64  70.0        site_count
f64  6.0         dot_count
f64  120.0       dwell_us
E0 00 00 00 00 00 00 00            slot 0: byte 0 = sites 0,1,2 (bits 7,6,5)
1C 00 00 00 00 00 00 00            slot 1: byte 8 = sites 67,68,69 (bits 4,3,2)
```

Total 76 bytes, expanding to 12 records (192 bytes) -- and to 251,968 bytes if
the run were a full 7874-site line.

## Expansion

```
for i in 0 .. site_count-1:
    if mask bit i is clear: continue
    p = (origin_x + i*step_x, origin_y + i*step_y)
    if emitted_valid and p == emitted: pass        # no JUMP_ABS, see below
    else: emit JUMP_ABS(p); emitted = p; emitted_valid = true
    emit LASER_ON(dwell_us)
```

Three things in there are easy to lose and are therefore MUST:

- **A zero-length jump emits nothing.** `GalvoListWriter::jumpTo()` returns
  early when the distance is zero, so the uncompressed stream has no
  `JUMP_ABS` there. The expansion has to agree or the diff fails.
- **`emitted` carries across records.** The first jump of a run is measured from
  wherever the previous record left the beam -- usually the far end of the
  previous scan line. That distance feeds both `jump_delay_ms()` and the time
  estimate, so a decoder must not treat each record as a fresh start.
- **Field bounds are still checked.** `toField()` counts out-of-field points
  while converting; expansion must count them the same way rather than
  silently passing coordinates through.

Both sides MUST handle the mask as bytes. It is a bit pattern occupying
parameter slots, not a number: loading a slot into a floating-point register
risks quieting a signalling NaN on an x87 path, and nothing is gained by it. So
the writer gets its own entry point rather than passing the mask through
`write_galvo_command`'s `initializer_list<double>`:

```cpp
void write_galvo_packed_dots(const double header[7],
                             const uint8_t* mask, size_t slots);
```

The bytes it produces are indistinguishable from a uniform record, and a parser
that does not know 0x20 still skips it by `4 + 8 * paramCount`; but neither side
ever treats a slot as a float.

## Cost in records

```
expanded_records = 2 * dot_count - (entry jump is zero-length ? 1 : 0)
```

O(1) from the header, and the constant is 2. The splitter uses `2 * dot_count`,
which overstates by at most one record.

## Cost in time

Two computations, for two purposes.

**Emitted estimate: accumulate per dot while walking the mask.** The encoder
walks the mask anyway to build it, so this is free, and -- more importantly --
it sums in the same order as the uncompressed path, so `SET_END_OF_LIST` comes
out bit-identical and the golden-vector diff stays exact. Closed forms do not
have that property.

**Splitter lookahead: a closed form, O(1).** With `s = |step|`, `v =
jump_speed_mm_s`, `N = site_count`, `D = dot_count`, and `d0` the entry jump:

```
jump distance inside the run = s * (N - 1)
```

because the gaps between consecutive dots telescope to `last - first`, which
canonical form fixes at `N - 1` whatever the bit pattern is. Then

```
traverse_ms = (d0 + s*(N-1)) / v * 1000
delay_ms    = jump_delay_ms(d0)
            + ((D-1)*kJumpDelayMinUs
               + (kJumpDelayMaxUs - kJumpDelayMinUs) / kJumpDelayLimitMm * s*(N-1))
              / 1000
dwell_ms    = D * dwell_us / 1000
```

`delay_ms` is exact rather than approximate because `jump_delay_ms()` is affine
in distance below `kJumpDelayLimitMm`, and canonical rule 5 guarantees every
internal jump is below it. `d0` is the one jump that may saturate, so it is
charged separately through the real function.

## Limits on one record

Two ceilings, and neither binds.

**The player's payload buffer is 30040 bytes** (`BSL_CMD_DATA_SIZE`, read in
`TaskLoaderV2::handleGalvoListCmd()`, which throws "payload_len exceeds buffer
size" above it). A galvo record's payload cannot reach that: `paramCount` is a
`uint8`, so the largest record any opcode can state is `255 * 8` = 2040 bytes of
payload, 2044 on the wire. The buffer has 15x the headroom.

**`paramCount <= 255` is therefore the real ceiling**, at 248 mask slots and
15872 sites. Measured against the widest sweep the hardware can make:

| | sites | mask | paramCount | record |
|---|---|---|---|---|
| 110 mm field, 2000 DPI | 8662 | 1083 B | 143 | **1148 B** |
| 60 mm field, 2000 DPI | 4725 | 591 B | 81 | 652 B |
| the `uint8` cap | 15872 | 1984 B | 255 | 2044 B |

So a full-width traverse of the largest field at 2000 DPI fits in one record
with 112 parameters to spare, and one record spans the full 110 mm up to
**3665 DPI**.

### The header and the mask stay in one record

They do not need splitting into two commands, and splitting them would cost
more than it buys:

- there is nothing to buy. The sizing above fits with 44% of the parameter
  space unused, at a DPI 1.8x beyond the target;
- a mask continuation record makes the decoder stateful between records -- it
  would have to hold a pending header -- and that state has to survive a list
  boundary, a `splitIfFull()`, and the §19.1 block rule that any non-byte-23
  command ends the block. A header and its mask that cannot be separated cannot
  be separated by accident either;
- the arity check (below) is one `case` for a self-contained record, against a
  pair of opcodes with an ordering rule for a split one.

The list-capacity cut is a separate matter and happens first in practice: at
full density a 110 mm line is 8662 dots, which expands to 17324 records against
a list that holds 10000, so such a line is two lists however the file states it.

## Encoding

One pass, one fixed buffer, no post-hoc cutting. The encoder stages into a
248-slot (1984-byte) zero-initialised array and closes the record whenever a
limit is reached, so a mask is never built larger than it will be written and
the splitter never has to find the k-th set bit.

```
rec = none
for each site i along the line:
    if site i is not a dot: continue          # nothing to do; the bit is already 0

    if rec is open and any of:
          dots(rec) == budget(rec)                        # list capacity
          i - origin_index(rec) >= 15872                  # paramCount ceiling
          i - last_dot_index(rec) > max_zero_gap          # canonical rule 5
       then close(rec)

    if rec is none:
        if remainingListCapacity() < 2: endList(); beginList()
        rec = open(origin = site i, budget = remainingListCapacity() / 2)

    set bit (i - origin_index(rec)) in rec
    dots(rec) += 1
    last_dot_index(rec) = i
    accumulate the time of this dot's jump and dwell

close(rec) if open
```

where `close(rec)`:

```
site_count = last_dot_index - origin_index + 1       # no trailing zeros, ever
write the record
list_command_count_ += 2 * dots                      # the expanded cost
if list_command_count_ reaches the limit: endList(); beginList()
```

Four consequences worth stating:

- **Trailing zeros cannot occur.** The record closes at a dot, so canonical rule
  2 holds by construction rather than by a trim pass.
- **The budget is fixed when the record opens**, at `remainingListCapacity() /
  2`. It cannot move underneath the record, because nothing else writes to the
  list while a run is being staged.
- **No prefix popcount anywhere.** Counting as we go replaces it; this is the
  main reason to encode streaming rather than cut afterwards.
- **`max_commands_per_list_ == 0` means no limit** (`galvo_max_list_commands`
  defaults to 0), and the budget is then effectively infinite, leaving the
  15872-site ceiling as the only cut.

`max_zero_gap` is `min(floor(kJumpDelayLimitMm / |step|), 480)`: the first term
is canonical rule 5, the second is where a fresh 60-byte header becomes cheaper
than the zeros it replaces. At 2000 DPI that is 480; the limit term only takes
over below about 165 DPI.

## Player-side changes

`TaskLoaderV2::handleGalvoListCmd()` needs two additions, both O(1).

**The arity table must admit 0x20.** The switch is exhaustive with
`default: throw "Unknown opcode"`, and a fixed `expected` per opcode. 0x20's
arity is variable, so it needs its own case: accept `param_count >= 8`, then
after reading the payload check `slot_count == ceil(ceil(site_count / 8) / 8)`
and `dot_count` against the mask's popcount. A record that fails either is
malformed and should throw like any other arity failure.

> While in that switch: **0x21 `AXIS_MOVE` and 0x22 `SET_IO` are not in it
> either**, so they currently hit `default` and throw, although §4.4 defines
> them. That is a pre-existing gap, unrelated to this document, but it lives in
> the same `case` list and should be fixed in the same visit.

**Progress accounting must include 0x20.** `galvo_block_distance_` only
accumulates for opcodes 1-4, so a block made of packed records would report
zero distance and the progress estimate would stall. From the header alone:

```
entry = hypot(origin - (galvo_x_, galvo_y_))         # 0 if position unknown
galvo_block_distance_ += entry + |step| * (site_count - 1)
galvo_x_, galvo_y_ = origin + (site_count - 1) * step
```

which is the same telescoping identity the time estimate uses, so it is exact
rather than an approximation of the expanded stream.

## Encoder rules

The two places that produce dots today (`LaserRasterGalvoFactory`) both already
walk one unidirectional line at a fixed pitch, which is exactly a run:

| producer | run axis | pitch | dwell |
|---|---|---|---|
| `emitLine()`, `entry.dots` -- dithered image | X, or Y when `transposed` | `1/pixel_per_mm_x`, or `1/pixel_per_mm` | layer `dottingTime()` |
| `emitBandDots()` -- hybrid seam band | X per row | `dot_process.pitch_mm`, falling back to the image's | `dot_process.pulse_on_time_us`, falling back to the swept equivalent |

Both skip sites for threshold, ownership and -- for the band -- the core
exclusion. Those skips are the zeros in the mask; nothing else about them
changes.

Two rules the encoder must respect that the per-dot path got for free:

- **Clip per site, inside the writer.** `dotAt()` makes the same
  `clip_rect_.contains()` test the one-dot path makes, and a site the block does
  not own simply leaves its bit clear. That is equivalent to clipping the index
  range and needs no index arithmetic: a clipped end shortens the run, a clipped
  middle becomes a gap, and a long enough gap breaks the record. What must never
  be used here is `clipSegment()` -- its floating-point parameter would slide
  the origin by a fraction of a pitch and burn every site in the wrong place.
- **Split on long zero gaps**, per canonical rule 5.

There is no flag. A galvo layer always packs its dots, so there is one path to
get right rather than two that have to agree. `moveTo()`'s one-dot branch stays
as what a packed record expands back into -- and for a caller that dots a single
point -- but nothing in a galvo file reaches it.

## Where the code goes

```
generators/fcode-generator.{h,cpp}   + write_galvo_packed_dots()
generators/galvo-list.h              + beginDotRun() / dotAt() / endDotRun()
                                     + remainingListCapacity()
generators/galvo-list.cpp              the staging buffer, the limits, timing
factories/laser-raster-galvo.cpp       emitLine() dot branch: stream site indices
                                       emitBandDots(): the same, per row
```

The streaming API is three calls:

```cpp
void beginDotRun(const QPointF& base, const QPointF& step, double dwell_us);
void dotAt(int site);   // site index from base, strictly increasing
void endDotRun();
```

`base` is site 0 of the *line*; each record's `origin` is `base + first_site *
step`, which the writer works out when it opens one. The factory therefore
contributes only "this site is dark" -- every limit, every record boundary,
every list boundary and the whole time estimate stay in the writer, where the
list state already is.

That also makes the two producers a small change: `emitLine()`'s dot branch and
`emitBandDots()` both already walk sites in order and `continue` past the ones
they skip, so each loses its `move()` call and gains a `dotAt(i)`.

## Verification

The harness lives in `~/.claude/projects/-Users-software-dev-swiftray/harness/`:
`check_fcode.py` decodes and validates packed records, `packed.py` drives the
three cases below. Measured on this implementation:

| case | runs | dots | packed | expanded | ratio |
|---|---|---|---|---|---|
| 200 x 200 dithered, 254 DPI | 200 | 40000 | 18.4 KB | 1.28 MB | 70x |
| the same under a 1000-record list cap | 277 | 40000 | 23.0 KB | 1.28 MB | 56x |
| hybrid seam band, 150 mm square over 4 blocks | 3186 | 290000 | 275 KB | 9.28 MB | 34x |
| one full-width sweep, 2000-DPI pitch | 20 | 160520 | 21.4 KB | 5.14 MB | **240x** |

The ratio rises with the sites per record, because the 60-byte header is the
whole of the overhead: a 200-site row at 254 DPI spends 60 bytes on 25 bytes of
mask, while an 8026-site row at a 2000-DPI pitch spends the same 60 on 1004. The
last row is the case this exists for, and it also confirms the sizing: 8026
sites is **one record of 133 parameters**, against the 255 available.

What the checks assert:

- **Canonical form**, on every record: `site_count` and `dot_count` integral and
  positive, first and last bit set, non-zero step, `dot_count` equal to the
  popcount, the slot count equal to `ceil(ceil(sites / 8) / 8)`, every padding
  bit clear, and a positive dwell (`packed_issues()`).
- **The lattice.** Expanding every run of a solid bitmap must land one dot on
  each cell of the grid the bitmap asks for, with none twice and none off the
  lattice by more than 1e-6 of a pitch. This is what proves origin, step,
  direction and mask all agree, and it is phase-free -- it indexes from the
  minimum, because a forward row and a reversed one reach the same nominal
  position by different arithmetic and so differ in the last bits.
- **No bare dots.** A galvo file must contain no `LASER_ON` record at all;
  every dot is inside a packed run.
- **The list cap still means something.** Summing each list's cost as
  `2 * dot_count` per packed record, no list may exceed its cap -- and a full
  list comes out at exactly `cap + 2`, because `splitIfFull()` splits on
  reaching the cap and `endList()` then writes `DISABLE_LASER` +
  `SET_END_OF_LIST`. The uncompressed path does the same thing: binarised
  stripes under a cap of 1000 also produce 1002-record lists. That control was
  run, and it is the point -- the cap has to mean the same counted expanded.
- **§19 as a whole.** `check_fcode.check()` passes on all four files: prologue,
  epilogue, one `SET_END_OF_LIST` per list, an absolute first move, and every
  point inside the field (for a packed record, its first and last site).

A note on exactness: a record states positions as `origin + i * step`, while the
blend and threshold tests sample the bitmap's own per-pixel formula,
`(bbox + at) / pixel_per_mm`. The two agree to within rounding, not bit for bit,
so the record's formula is the normative one. Ownership is unaffected, because
every block tests with the same per-pixel formula and therefore still agrees
with its neighbours about who owns a site.

Still to do, and what §16-D1 asks for: **golden vectors** -- a table of
`(header, mask) -> expected record sequence` run against both this expander and
the player's. The cases worth tabulating are the ones the rules exist for: a
zero-length entry jump, a run whose first dot is the previous run's last point,
a mask ending mid-word, a single-dot run, a run cut by the list cap, and a run
cut by a long zero gap.

## Open

- **The card's real list limit.** `MAX_BUFFER_LIST_SIZE = 10000` is the vendor
  sample's streaming batch size, not a capacity (§20.5), and the sample has no
  capacity check at all -- the only flow control is `lcs_load_list()`'s return.
  The split count above assumes 10000. Measuring the true limit changes how
  often runs are cut, and nothing else in this document.
- **The card-side replay buffer.** `BSLListManager` records every expanded call
  in `open_list_` for USB-drop recovery, at roughly 48 bytes each. That is
  480 KB per 5000-dot list, which is fine, but it scales with the expanded
  count and not with the file, so it is another reason the expanded count has to
  stay bounded.
- **Naming.** 0x20 is reserved in §4.4 as `RASTER_ROW`, which reads like a
  second raster path rather than an encoding. `PACKED_DOTS` says what it is.
  The row still says "參數待定", so renaming it is free today.
