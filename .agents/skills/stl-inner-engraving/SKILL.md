---
name: stl-inner-engraving
description: Swiftray daemon 的 STL／BSPC／3D 內雕輸入契約，包含 loadSVG payload、SVG attributes、物件對應、折射與 material Z。修改 STL parser、placement、convert API 或前後端資料格式時使用。
---

# STL 內雕輸入與轉換契約

修改 3D 內雕資料流時，以本 skill 為前後端介面的單一真實來源。實作應同時檢查
`src/server/worker.cpp`、`src/parser/my_qsvg_handler_qt6.cpp`、
`src/shape/stl-placement.h`、`src/toolpath_exporter/stl-utils.*` 與
`src/toolpath_exporter/toolpath-exporter.*`；契約或預設值變更時同步更新本檔案與
`tests/test_stl_utils.h`。

本 skill 聚焦外部輸入契約。修改切片、contour、blue-noise、inward shells 或點排序時讀
`stl-geometry-processing` skill；修改 Promark 時間預估、delay 或 first-pulse 行為時讀
`promark-job-timing` skill。

本文描述 daemon 的 `loadSVG` 與 `convert` 預期收到的 STL 內雕資料。折射補償固定使用
Basic 軸向模型，不提供開關或其他模型。

## 1. `loadSVG` payload

```jsonc
{
  "file": {
    "data": "<包含 3D 物件投影的 SVG 字串>",
    "thumbnail": "<選填>"
  },
  "rotaryMode": false,
  "defaultConfig": {},
  "stlObjects": {
    "mesh-dot": "<base64 STL binary 或 ASCII STL>",
    "mesh-line": { "data": "<base64 STL binary 或 ASCII STL>" }
  },
  "pointCloudObjects": {
    "cloud-1": "<base64 BSPC>",
    "cloud-2": { "data": "<base64 BSPC>" }
  }
}
```

- `stlObjects` 與 `pointCloudObjects` 的 key 必須等於 SVG 元素的 `id`。
- STL 與 BSPC 都必須先 base64 編碼，不能把 raw binary 直接放進 JSON。
- 平面點整圖的像素已在 SVG `<image>` 內，不需放進 `stlObjects`。
- 載入結果會回報 `loadedStlObjects`、`failedStlObjects`、
  `loadedPointCloudObjects` 與 `failedPointCloudObjects`。

### SVG 物件共用 attribute

| attribute | 型別／單位 | 用途 |
| --- | --- | --- |
| `data-stl="1"` | marker | 將元素標記為 3D 內雕物件；值本身不解析。 |
| `id` | string | 對應 binary map 的 key；必填且不可為空。 |
| `data-stl-kind` | `mesh`／`photo`／`point-cloud` | 省略時為 `mesh`。 |
| `data-stl-matrix` | 16 numbers | `THREE.Matrix4.elements` 的 column-major 順序。結果必須已轉成畫布座標（10 units = 1 mm，Y 軸方向也由前端處理）。省略時為 identity。 |
| `data-stl-mode` | `dot`／`line` | mesh 的加工模式；省略時為 `line`。photo 與 point-cloud 固定走 `kDot`。 |
| `data-stl-layer-height` | mm | `kLine` 的固定／最大層高；`kDotFill` 的 inward-shell 間距。省略或 <= 0 時後端預設 0.1 mm。 |
| `data-stl-min-layer-height` | mm | **adaptive planes 的 opt-in。** 只有未填充的 `kLine` 且值 > 0 時啟用；此值是最小層高，`data-stl-layer-height` 是最大層高。省略或 <= 0 時一律使用固定 `planes()`。 |
| `data-stl-point-spacing` | mm | mesh 點模式的 blue-noise 最小點距，以及 photo 的取樣點距。省略或 <= 0 時後端預設 0.1 mm。 |
| `fill` | SVG paint | 沿用一般 SVG 填充判斷。mesh 的 line/dot 加上 fill 會分別成為 `kLineFill`／`kDotFill`。 |

## 2. 各物件需要的資料

### `StlEngraveKind::kDot`（STL 表面打點）

```xml
<rect id="mesh-dot" data-stl="1" data-stl-kind="mesh"
      data-stl-mode="dot"
      data-stl-matrix="<16 numbers>"
      data-stl-point-spacing="0.10"
      fill="none" />
```

- `stlObjects["mesh-dot"]`：必要，base64 STL。
- 後端在 transform 後的 STL 表面做 deterministic blue-noise sampling。
- `data-stl-point-spacing` 有效；`data-stl-layer-height` 對未填充的 `kDot` 無效。
- 元素不可被判定為 filled，否則會成為 `kDotFill`。

### `StlEngraveKind::kLine`（STL 切層打線）

```xml
<rect id="mesh-line" data-stl="1" data-stl-kind="mesh"
      data-stl-mode="line"
      data-stl-matrix="<16 numbers>"
      data-stl-layer-height="0.10"
      data-stl-min-layer-height="0.05"
      fill="none" />
```

- `stlObjects["mesh-line"]`：必要，base64 STL。
- `data-stl-layer-height` 是固定層高；若同時提供正值的
  `data-stl-min-layer-height`，才會改用 adaptive planes，範圍為最小層高至最大層高。
- 省略 `data-stl-min-layer-height` 時不做 adaptive slicing。
- 元素不可被判定為 filled，否則會成為 `kLineFill`。

### 平面點整圖（3D 內雕版）

```xml
<image id="photo-1" data-stl="1" data-stl-kind="photo"
       data-stl-matrix="<16 numbers>"
       data-stl-photo-width="20"
       data-stl-photo-height="15"
       data-stl-point-spacing="0.10"
       href="data:image/png;base64,..." />
```

- 圖片像素由 SVG `<image>` 提供；不需要 `stlObjects["photo-1"]`。
- `data-stl-photo-width`、`data-stl-photo-height` 是 transform 前的局部平面尺寸，單位 mm，
  兩者都必須 > 0。
- 後端依 transform 後的實體尺寸與 `data-stl-point-spacing` 建立取樣格，合成白底後二值化／
  dithering；黑點進入 3D `kDot` 路徑。
- `data-stl-mode` 與 `fill` 不影響種類，固定為 `kDot`。

### 點雲

```xml
<rect id="cloud-1" data-stl="1" data-stl-kind="point-cloud"
      data-stl-matrix="<16 numbers>" fill="none" />
```

- `pointCloudObjects["cloud-1"]`：必要，base64 BSPC。
- 點是前端已決定好的直接雕刻點；後端只套用 `data-stl-matrix`、Z 範圍、折射補償與排序。
  `data-stl-point-spacing`、`data-stl-layer-height`、`data-stl-mode` 與 `fill` 都不會重新取樣或
  改變種類。
- BSPC v1 是 little-endian binary：

| offset | bytes | value |
| ---: | ---: | --- |
| 0 | 4 | ASCII `BSPC` |
| 4 | 1 | version = `1` |
| 5 | 1 | components = `3` |
| 6 | 2 | reserved |
| 8 | 4 | `uint32` point count |
| 12 | `count * 12` | 每點三個 `float32`：local `x, y, z`（mm） |

檔案大小必須剛好等於 `12 + count * 12`，點數必須 > 0，所有座標必須是 finite number。

## 3. `convert` params

以下是 STL 內雕直接使用的欄位；`workarea`、`travelSpeed`、`type`、`isPromark` 等既有
convert 共用欄位仍照原介面提供。

```jsonc
{
  "refractive_index": 1.52,
  "material_height": 30.0,
  "material_min_z": 0.0,
  "material_max_z": 30.0,

  "first_pulse_killer_enabled": true,

  "is_uv_light": false,
  "jump_speed": 4000,
  "laser_on_delay": 150,
  "laser_off_delay": 150,
  "marking_delay": 0,
  "corner_delay": 0,
  "jump_delay_min": 10000,
  "jump_delay_max": 10000
}
```

| param | 型別／預設 | 用途 |
| --- | --- | --- |
| `refractive_index` | number，預設 1.0 | 材料折射率 `n`。固定套用 `Zm = H - (H - z) / n`，不做 XY 補償；`n = 1` 時 Z 不變。 |
| `material_height` | mm，預設 0 | 材料上表面相對平台的高度 `H`，也是 `material_max_z` 的預設值。內雕工作應明確提供。 |
| `material_min_z` | mm，預設 0 | 可雕刻的最小模型 Z（含邊界）。 |
| `material_max_z` | mm，預設 `material_height` | 可雕刻的最大模型 Z（含邊界）。 |
| `first_pulse_killer_enabled` | bool，預設 false | 產生 `;CONFIG FIRST_PULSE_KILLER_ENABLED=0/1`，由 BSL 控制器在每個 list 套用。 |

`material_min_z`／`material_max_z` 判斷的是**折射補償前的模型 Z**。超出範圍的 line slice
整層不輸出；dot、photo 與 point-cloud 則丟棄超出範圍的點。因此原始 `Z < 0` 的內容即使
經 Basic 折射公式後得到可到達的正 machine Z，也不會打到底板。

所有 STL 輸出會依補償後的 machine Z 強制量化到 `0.0001 mm` bucket。

其餘 timing params 會以同名的大寫 `;CONFIG` 寫入 G-code。`first_pulse_killer_enabled` 也走
同一條路徑，不再由 controller setter 設定。STL 低速打點的時間預估對單點完整成本套用
實測係數 `2.0`，`MachineJob` 與 BSL streaming progress 使用相同公式。

## 維護約束

- 測試用定義與實作只能由 `UnitTest` target 引用，不可加入 `swiftray_app_bundle`。
- Basic 軸向折射固定套用；不要重新加入 opt-in、模型切換或 XY 補償。
- STL machine-Z bucket 固定為 `0.0001 mm`。
- convert 期間的長時間切片、取樣、排序與輸出必須持續回報進度並可取消。
- 取消後直接停止產生輸出，不追加 STL 清理、dotting 關閉或 Z 軸歸零命令。
- 不要在正式路徑加入切片／補償結果的 CSV、image 或其他 debug dump。
