# Content-aware galvo blocks

Status: specified, not implemented.
Applies to: HEXA II galvo heads (`LayerModule::GALVO_CO2`, `GALVO_MOPA`).

## The problem

A galvo layer is engraved from a lattice of gantry stops. The lattice is
anchored at the machine origin and steps by the block displacement, so it takes
no account of what is drawn on it: a motif that would fit inside one field is
cut in half whenever a seam happens to pass through it, and the two halves are
then stitched back together by whichever seam strategy is configured.

For a drawing made of separate pieces that is the wrong trade twice over. The
seam costs time -- interlocking teeth add jumps, a dot band adds a dot per site
-- and it buys nothing, because the pieces never needed to be divided in the
first place. Two shapes that come up constantly:

- decoration in the four corners of a plate, which is four independent pieces
  wherever the plate is larger than a field;
- an array of small motifs laid out on a pitch that has nothing to do with the
  block displacement, so the lattice cuts through the motifs rather than the
  gaps between them.

In both, every piece fits inside the field. What is wanted is one gantry stop
per piece (or per group of pieces that share a field), with the seam machinery
switched off entirely.

## What makes this cheap

Splitting is settled entirely by planning. `emitGalvoBlocks()`
(`src/toolpath_exporter/toolpath-exporter-fcode.cpp`) re-runs the whole layer's
geometry once per block and lets `GalvoListWriter::clip_rect_` decide what
actually lands. Geometry is never partitioned; a block is nothing but a
`(park, region, clip)` triple.

So this feature adds a second way to produce that list of triples. Emission,
seam blending, time estimation, list splitting and the debug image are all
untouched.

## Definitions

- **item** -- one indivisible piece of geometry, as the factories already hold
  it (below). An item is never divided between blocks in this mode.
- **cluster** -- a set of items whose union fits inside the usable field. One
  cluster becomes one block.
- **lattice mode** -- the current behaviour: cells of the block displacement,
  anchored at the machine origin.
- **content mode** -- the behaviour specified here.

## Step 1: items

Each factory already holds the geometry it would need to report per-item
bounds; each needs one accessor, returning bounds in the same mm frame
`get_bounds_mm()` uses.

| factory | member it reads | one item is | accessor to add |
|---|---|---|---|
| `LaserPathFactory` | `QVector<QPolygonF> polygons` (mm after `preprocess()`) | one polygon | `get_item_bounds_mm()` |
| `LaserPathFilledFactory` | `QList<FilledPath> filled_` | one filled outline group | `get_item_bounds_mm()` |
| `LaserRasterGalvoFactory` | `QVector<Entry>` (each with `bbox_px`) | one bitmap | `get_item_bounds_mm()` |

A bitmap is one item whole. Connected-component analysis *inside* a bitmap --
which would catch four corner decorations rasterised into a single image -- is
a pixel pass over the image and is deliberately out of scope here; see Phase 2.

`collectGalvoItems()` in `ToolpathExporterFcode` gathers the three lists for the
layer being converted.

## Step 2: clustering

Greedy seed-and-absorb, not hierarchical agglomeration: the result has to be
stable and easy to read off a debug image when it goes wrong.

```
usable = galvo_field_mm - galvo_content_margin * 2
sort items by (top, left)
while an unassigned item remains:
    seed the next unassigned item as a new cluster
    for each remaining unassigned item, in order:
        if the cluster's union with it still fits usable x usable:
            absorb it
```

### Disjointness

Two clusters whose bounding boxes overlap would engrave the same ground twice,
since each block's clip is its own cluster's bounds. After clustering:

```
while two clusters' bounds intersect:
    if their union fits usable x usable: merge them
    else: content mode is impossible for this layer -- fall back to lattice
```

A layer that fails this test has geometry genuinely interleaved across more than
a field, which is the case the lattice exists for.

## Step 3: solving for a park

A cluster's union `C` is reachable from park `P` when the lens covers it:

```
P.x in [C.right - half_field, C.left  + half_field]
P.y in [C.bottom - half_field, C.top  + half_field]
```

`P` must also sit on the lattice -- a whole multiple of `galvo_block.width()` in
X and of `galvo_block.height()` in Y, measured from the machine origin, because
the gantry can only stop on whole numbers of full steps -- and inside
`galvoHeadTravel()`.

Intersect the three, then take the lattice point nearest the centre of `C`, so
the work sits as close to the middle of the field as the lattice allows and
picks up the least distortion. An empty intersection means this cluster cannot
be done from one stop: fall back to lattice mode for the layer.

`parkForRegion(region)` should be shared with the single-block case in
`planGalvoBlocks()`, which solves the same problem with a free (unquantised)
park.

## Step 4: blocks

```
region = cluster bounds
clip   = region grown by kGalvoSeamEpsilonMm
park   = from step 3
lattice = false
```

`GalvoBlock` gains `bool lattice = true`. `col`, `row` and the four `has_*`
flags are lattice notions and stay at their defaults for a content block.

Emission order: serpentine over the parks, by lattice row then alternating
direction in X, the same shape `planGalvoBlocks()` already uses.

### Blending must be off

Content blocks share no seams, so the seam machinery has nothing to do and its
cost must not be paid. In `emitGalvoBlocks()`:

```cpp
blend.active = blocks.size() > 1 && block.lattice;
```

This is where the time is won. On a solid fill the interlocking strategies cost
5-9% of the run time in jumps and the dot band far more; a drawing that never
crosses a seam should pay none of it.

## Step 5: choosing a mode

`galvo_block_mode` is `auto` (default), `lattice` or `content`. Under `auto`,
fall back to lattice when any of these holds:

1. an item on its own is wider or taller than the usable field -- it has to be
   divided, and this mode never divides an item;
2. clustering cannot produce disjoint clusters (step 2);
3. any cluster has no reachable lattice park (step 3);
4. content mode produces no fewer blocks than lattice mode would. Dense content
   gains nothing from clustering, and the lattice's cells tile the plane, which
   is the safer thing to be doing when there is no gap to exploit.

Rule 1 makes the mode all-or-nothing per layer. Mixing -- a big item on the
lattice while small ones take their own stops -- needs the two kinds of block to
agree on who owns the ground they share, and is Phase 2.

## Parameters

Both belong in the `is_galvo_machine_` block of `parseParam()`, alongside
`galvo_field` and `galvo_block`, and in the dev panel's splitting group.

| key | type | default | meaning |
|---|---|---|---|
| `galvo_block_mode` | `"auto"` \| `"lattice"` \| `"content"` | `"auto"` | which planner runs |
| `galvo_content_margin` | number, mm | 2 | field edge left unused when testing whether a cluster fits |

No new geometry comes from the frontend: the XY step is `galvo_block` and the
field is `galvo_field`, both already sent.

## Where the code goes

```
factories/laser-path.{h,cpp}          + get_item_bounds_mm()
factories/laser-path-filled.{h,cpp}   + get_item_bounds_mm()
factories/laser-raster-galvo.{h,cpp}  + get_item_bounds_mm()

toolpath-exporter-fcode.h
  GalvoBlock                          + bool lattice
  Config                              + galvo_block_mode, galvo_content_margin
                                      + collectGalvoItems()
                                      + planGalvoContentBlocks()
                                      + parkForRegion()

toolpath-exporter-fcode.cpp
  parseParam()                        read the two new keys
  collectGalvoItems()                 per-item bounds from the three factories
  planGalvoContentBlocks(items)       steps 2-4
  parkForRegion(region)               step 3, shared with planGalvoBlocks()
  convertGalvoLaserLayer()            choose a planner (step 5)
  convertGalvoDepthBitmaps()          the same choice; depth benefits equally
  emitGalvoBlocks()                   blend.active gated on block.lattice
```

`writeGalvoDebugImage()` needs no change: it draws each block's park, the field
it reaches and the region it owns, which reads correctly for a cluster.

## Verification

- **Nothing is engraved twice.** Cluster bounds are disjoint by construction;
  check it on the emitted file the way the seam work does, by collecting marked
  spans per scan line and looking for overlap.
- **Nothing is lost.** The marked length and extent must match what lattice mode
  produces for the same drawing.
- **Blending really is off.** No block may emit more segments than its geometry
  has; compare the segment count against a `galvo_dot_blend_overlap: 0` run.
- **Parks are on the lattice.** Every park is a whole multiple of the step on
  each axis, as `check_offsets.py` already asserts for lattice mode.
- **The cases this exists for.** Four corner decorations on a bed-sized plate
  should plan four blocks where the lattice plans dozens; an array of motifs on
  a pitch coprime with the step should plan one block per motif group with no
  motif divided.

## Phase 2

- Connected components inside a single bitmap, so a rasterised drawing gets the
  same treatment as a vector one.
- Mixed planning: oversized items on the lattice, the rest on their own stops,
  with the two kinds of block agreeing on ownership where they meet.
- Minimising gantry travel across the parks rather than walking them in
  serpentine order.
