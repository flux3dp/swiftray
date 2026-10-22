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

### 物件 attribute（投影元素上）

物件本身只帶「是什麼、放在哪裡」；怎麼雕（模式、點距、層距、實心）是圖層參數，見下一節。

| attribute | 型別／單位 | 用途 |
| --- | --- | --- |
| `data-stl="1"` | marker | 將元素標記為 3D 內雕物件；值本身不解析。 |
| `id` | string | 對應 binary map 的 key；必填且不可為空。 |
| `data-stl-kind` | `mesh`／`photo`／`point-cloud` | 省略或其他值時為 `mesh`。 |
| `data-stl-matrix` | 16 numbers | `THREE.Matrix4.elements` 的 column-major 順序。結果必須已轉成畫布座標（10 units = 1 mm，Y 軸方向也由前端處理）。省略時為 identity；數量不是 16 時警告並用 identity。 |
| `data-stl-photo-width`／`data-stl-photo-height` | mm | 只用於 photo：transform 前的局部平面尺寸，兩者都必須 > 0。 |
| `data-stl-point-cloud` | BSPC 版本號 | 前端與 .beam 的點雲 marker。Swiftray 不讀它；點雲的身分只看 `data-stl-kind="point-cloud"`。 |

投影元素的 `fill` 與其他 SVG paint 不影響 3D 物件的雕刻種類。

### 圖層 attribute（圖層 `<g>` 上）

`my_qsvg_handler_qt6.cpp` 在 `<g class="layer">` 上讀進 layer config（數值 attribute 缺少時退回
`loadSVG` 的 `defaultConfig`：`stlPointSpacing`、`stlLayerSpacing`、`stlMinLayerSpacing`、
`stlSolid`；`data-stl-mode` 沒有這個退路），`ToolpathExporter::resolveStlPlacement()` 在收集物件時
從物件所在圖層複製到 `StlPlacement`。同一圖層的所有 3D 物件共用這組參數。

| attribute | 型別／預設 | 用途 |
| --- | --- | --- |
| `data-stl-mode` | `dot`／`line`，省略 = `dot` | 只有明確的 `line` 會切層打線；其他值都是點模式。只影響 mesh。 |
| `data-stl-point-spacing` | mm，省略或 <= 0 = 0.03 | mesh 點模式每個 shell 的 blue-noise 最小點距，以及 photo 的取樣格距。photo 的取樣格數沒有上限（與 STL 點路徑一致），下限由 Beam Studio 的 0.01 mm 點距把關。 |
| `data-stl-layer-spacing` | mm，省略或 <= 0 = 0.04 | `kLine` 的固定（或 adaptive 時的最大）Z 層距；`kDotFill` 的 shell 間距。 |
| `data-stl-min-layer-spacing` | mm，省略或 0 = 關閉 | > 0 時 `kLine` 改用 adaptive planes，範圍為此值到 `data-stl-layer-spacing`。對點模式無效。 |
| `data-stl-solid` | `1`＝開，省略 = 關 | 只用於 mesh 點模式（Beam Studio 介面名稱為「加厚輪廓」）：表面加 2 層向內 shell（共 3 個 shell，`BlueNoiseParams::max_shell_count`）。較薄的物件 shell 會更少。 |

後端預設值 0.04／0.03 在 `toolpath-exporter.cpp`（`kDefaultStlLayerSpacingMm`、
`kDefaultStlPointSpacingMm`），與前端預設同步。

## 2. 雕刻種類

`ToolpathExporter::stlEngraveKind()` 決定種類：

| 物件 | 圖層設定 | 種類 |
| --- | --- | --- |
| photo | 任何 | `kDot` |
| point cloud | 任何 | `kDot` |
| mesh | `data-stl-mode` 非 `line`，`data-stl-solid` 未開 | `kDot`（表面打點） |
| mesh | `data-stl-mode` 非 `line`，`data-stl-solid="1"` | `kDotFill`（表面 + 2 層向內 shell） |
| mesh | `data-stl-mode="line"` | `kLine`（切層打線） |

沒有填充打線的種類；`fill` 與 `data-stl-solid` 對 line 模式都沒有作用。

### `kDot`／`kDotFill`（STL 打點）

```xml
<g class="layer" data-stl-mode="dot" data-stl-point-spacing="0.03"
   data-stl-layer-spacing="0.04" data-stl-solid="1">
  <title>Layer 1</title>
  <rect id="mesh-dot" data-stl="1" data-stl-kind="mesh"
        data-stl-matrix="<16 numbers>" />
</g>
```

- `stlObjects["mesh-dot"]`：必要，base64 STL。
- 後端在 transform 後的 STL 表面（`kDotFill` 還有向內 shell）做 deterministic blue-noise sampling。
- `data-stl-point-spacing` 是同一 shell 內的最小點距；`data-stl-layer-spacing` 只對 `kDotFill`
  有效，是 shell 間距。

### `kLine`（STL 切層打線）

```xml
<g class="layer" data-stl-mode="line" data-stl-layer-spacing="0.04"
   data-stl-min-layer-spacing="0.02">
  <title>Layer 1</title>
  <rect id="mesh-line" data-stl="1" data-stl-kind="mesh"
        data-stl-matrix="<16 numbers>" />
</g>
```

- `stlObjects["mesh-line"]`：必要，base64 STL。
- `data-stl-layer-spacing` 是固定層距；`data-stl-min-layer-spacing` > 0 時改用 adaptive planes，
  範圍為最小層距至 `data-stl-layer-spacing`。0 或省略時用固定 `planes()`。

### 平面點整圖（3D 內雕版）

```xml
<image id="photo-1" data-stl="1" data-stl-kind="photo"
       data-stl-matrix="<16 numbers>"
       data-stl-photo-width="20"
       data-stl-photo-height="15"
       href="data:image/png;base64,..." />
```

- 路徑預覽與實際工作拿到同一份 gcode：exporter 不為預覽產生 mock 打點（`shouldMockFastGradient` 對 Promark 不再有作用）。打點的呈現由 Beam Studio 的 `PathPreview/tmpParseGcode.ts` 依 `;CONFIG INNER_ENGRAVING=1` 處理。
- 圖片像素由 SVG `<image>` 提供；不需要 `stlObjects["photo-1"]`。
- `stlObjects` 的 binary STL 以 header 宣告的三角數為準，尾端多餘的 bytes 容忍並忽略（與 Three.js 一致，`stl::detectFormat`）；截斷的檔案與 `solid` 開頭但長度不符的檔案仍拒收，回報在 `failedStlObjects`，Beam Studio 會因此擋下工作。
- 後端依 transform 後的實體尺寸與圖層的 `data-stl-point-spacing` 建立取樣格，合成白底後
  二值化／dithering；黑點進入 3D `kDot` 路徑。
- 圖層的 `data-stl-mode`、`data-stl-solid` 與 `fill` 都不影響種類，固定為 `kDot`。

### 點雲

```xml
<rect id="cloud-1" data-stl="1" data-stl-kind="point-cloud"
      data-stl-matrix="<16 numbers>" />
```

- `pointCloudObjects["cloud-1"]`：必要，base64 BSPC。
- 點是前端已決定好的直接雕刻點；後端只套用 `data-stl-matrix`、Z 範圍、折射補償與排序。
  圖層的點距、層距、模式與實心設定都不會重新取樣或改變種類。
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
  "machine_z_bucket": 0.01,

  "inner_engraving": 1,
  "is_uv_light": 1,
  "jump_speed": 4000,
  "laser_on_delay": 150,
  "laser_off_delay": 150,
  "marking_delay": 0,
  "corner_delay": 0,
  "jump_delay_min": 1000,
  "jump_delay_max": 1000
}
```

| param | 型別／預設 | 用途 |
| --- | --- | --- |
| `refractive_index` | number，預設 1.0 | 材料折射率 `n`。固定套用 `Zm = H - (H - z) / n`，不做 XY 補償；`n = 1` 時 Z 不變；非 finite 或 <= 0 時視為 1。 |
| `material_height` | mm，預設 0 | 材料上表面相對平台的高度 `H`，也是 `material_max_z` 的預設值。內雕工作應明確提供。 |
| `material_min_z` | mm，預設 0 | 可雕刻的最小模型 Z（含邊界）。 |
| `material_max_z` | mm，預設 `material_height` | 可雕刻的最大模型 Z（含邊界）。 |
| `first_pulse_killer_enabled` | bool，預設 false | 產生 `;CONFIG FIRST_PULSE_KILLER_ENABLED=0/1`，由 BSL 控制器在每個 list 與每個 `T` 套用；停用或 `T0` 時寫 0 清除控制卡上的設定。 |
| `inner_engraving` | 0/1，預設 0 | 產生 `;CONFIG INNER_ENGRAVING=1`，讓 `MachineJob` 與 BSL 控制器用內雕的時間模型；沒有它的工作一律用 develop 的舊算法（見 `promark-job-timing` skill）。Beam Studio 只在 3D 文件送。 |
| `machine_z_bucket` | mm，預設 `kDefaultMachineZBucketMm`(0.01)，最小 `kMinMachineZBucketMm`(0.001) | 補償後 machine Z 的量化間距，只用於打點；打線切片不分桶。低於最小值會被夾住並警告。 |

沒有 `focal_length` 之類的鏡頭參數：內雕鏡頭在同一個 machine Z 雕出的是一個平面，所以折射
只做軸向（Z）補償，不做焦距、場曲或離軸修正。

`material_min_z`／`material_max_z` 判斷的是**折射補償前的模型 Z**。超出範圍的 line slice
整層不輸出；dot、photo 與 point-cloud 則丟棄超出範圍的點。因此原始 `Z < 0` 的內容即使
經 Basic 折射公式後得到可到達的正 machine Z，也不會打到底板。

打點（dot、dot+fill、photo、point-cloud）會依補償後的 machine Z 量化到 `machine_z_bucket`。
打線的每層切片不分桶，各自用自己的 machine Z（取到產生器的 0.0001mm 精度，
`kStlZPrecisionMm`），所以 bucket 大於層距時也不會把相鄰切片併成同一個 Z；同一個 Z 上的點與
切片仍由下往上一起輸出。一個 bucket 就是一次 Z 軸
移動，而 Promark 每次軸移動有約 50ms 的固定開銷（見 `promark-job-timing` skill），所以這個
值直接決定內雕工作的時間下限：實測樣本中 0.001mm bucket 產生 8857 次 Z move，占 695s 工作
中的約 606s。加大 bucket 是用深度解析度換時間；預設 0.01mm 讓 Z move 次數少約一個數量級，
效果與 0.001mm 接近。

其餘 timing params 會以同名的大寫 `;CONFIG` 寫入 G-code。`first_pulse_killer_enabled` 也走
同一條路徑，不經 controller setter。STL 低速打點的時間預估對單點完整成本套用
實測係數 `2.0`，`MachineJob` 與 BSL streaming progress 使用相同公式。

## 維護約束

- 測試用定義與實作只能由 `UnitTest` target 引用，不可加入 `swiftray_app_bundle`。
- Basic 軸向折射固定套用；不要加入 opt-in、模型切換、XY 補償或焦距參數。
- 雕刻參數（模式、點距、層距、最小層距、實心）只從圖層 `<g>` 讀；不要在投影元素上解析它們，
  也不要讓投影的 `fill` 影響種類。
- STL machine-Z bucket 預設 `kDefaultMachineZBucketMm`（0.01 mm），可由 `machine_z_bucket`
  覆寫，但**最小值 `kMinMachineZBucketMm`（0.001 mm）是硬性下限，不是偏好**：更細的 bucket
  （例如 0.0001 mm）會讓工作在執行期卡住。Z 軸一個 pulse 是 0.000625 mm，0.001 mm 已接近軸解析度。前後端都要夾住這個下限。
- convert 期間的長時間切片、取樣、排序與輸出必須持續回報進度並可取消。
- 取消後直接停止產生輸出，不追加 STL 清理、dotting 關閉或 Z 軸歸零命令。
- 不要在正式路徑加入切片／補償結果的 CSV、image 或其他 debug dump。
