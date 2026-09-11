---
name: stl-geometry-processing
description: Swiftray STL 內雕的幾何處理規則，包含 STL/BSPC 解析、4x4 transform、切片、contour nesting、adaptive planes、deterministic blue-noise、inward shells 與點排序。修改 stl-utils 或 exporter 的 3D 規劃流程時使用。
---

# STL 幾何處理

本 skill 描述 STL 內雕後端的幾何不變量。daemon payload、SVG attributes、折射參數等外部
介面請讀 `stl-inner-engraving` skill；Promark 執行時間請讀 `promark-job-timing` skill。

主要實作：

- `src/toolpath_exporter/stl-utils.h`
- `src/toolpath_exporter/stl-utils.cpp`
- `src/toolpath_exporter/toolpath-exporter.cpp` 的 `outputLayerStlGcode()` 與 STL output helpers
- `src/shape/stl-placement.h`
- `tests/test_stl_utils.h`

## 座標與單位邊界

- 幾何方向為 X 向右、Y 向下、Z 向上；切片平面法向量是 Z。
- STL 通常以 mm 建模，但 `stl-utils` 不自行猜測或轉換單位。
- 前端提供的 column-major `QMatrix4x4` 必須已包含 STL mm 到畫布單位的轉換
  （目前 10 canvas units = 1 mm）與 Y 方向處理。
- transform 必須在 bounds、法向量、面積、welding tolerance、inset distance、sampling
  distance 與 slicing 之前套用。不要先在 local mesh 上計算再縮放結果。
- mirroring／負 determinant 合法；切片完成後會重新分類 contour winding 與 nesting。

`Slicer::prepare()` 在 identity transform 下會借用原始 `Mesh`，避免複製大型模型；呼叫端必須
確保 mesh 活得比 slicer 久。非 identity transform 才持有 transformed copy。

## STL 與 BSPC 解析

Binary STL 不能只用 `solid` prefix 判斷，因為合法 binary header 也可能以 `solid` 開頭。
binary 判斷以 little-endian face count 與精確大小關係為準：

```text
84 + faceCount * 50 == fileSize
```

不符合時才嘗試 ASCII STL。解析失敗必須回傳明確 error，不留下半套 mesh。

BSPC v1 必須同時滿足：magic `BSPC`、version 1、components 3、point count > 0、檔案大小
精確等於 `12 + count * 12`，且所有 little-endian float32 座標都是 finite number。

## 切片

`Slicer::prepare()` 會丟棄零面積 triangle、計算 transformed bounding box、建立依 minimum Z
排序的 triangle index，並由模型尺度推導 plane epsilon 與預設 welding tolerance。

- `planes()` 由 bottom 到 top 回傳遞增 Z。
- 第一個固定平面通常在底部上方半個 layer，避免擦過 bounding-box face；當 layer height 大於
  模型高度時，不可讓唯一平面落到模型之外。
- `adaptivePlanes()` 以 `SliceParams::layer_height` 為最大步長，以呼叫參數為最小步長；淺斜面
  根據 XY/Z 移動率縮小 Z step，垂直面維持最大步長。
- adaptive slicing 是否啟用由 exporter 的 engraving kind 與 placement 參數決定，不要在
  `Slicer` 內推測 UI 模式。

`sliceAt()` 對非遞減 Z 使用 forward sweep；若要求較低 Z，必須重設 active triangle sweep。
vertex 落在 plane epsilon 內時一致視為 plane 上方，coplanar triangle 直接略過，避免共享邊被
重複計算。

intersection endpoint 需在 tolerance 內 welding，並搜尋相鄰 spatial-hash cells。broken mesh
可以產生 open polyline，不應強制閉合。closed contours 完成後再分類：outer orientation `+1`
（CCW）、hole `-1`（CW），並設定直接 `parent_index` 與 nesting `depth`。

## 多物件 Z 順序與折射

內雕必須跨所有 STL／photo／point-cloud jobs 共用由深到淺、bottom-to-top 的 Z ladder，不能
逐物件各自完整輸出。較深層必須先雕刻，避免已產生的 crack points 散射後方光路。

material Z 範圍先在模型 Z 上過濾，再使用 Basic refraction 轉成 machine Z。所有輸出最後依
補償後 machine Z 強制量化到 `0.0001 mm` bucket。接近的 slicing planes 可以共用規劃 step，
但不可因此繞過 machine-Z bucket 或改變跨物件的深度順序。

## Blue-noise 與 inward shells

- sampling 一律在 transformed mesh 上進行。
- 使用固定 `kBlueNoiseSeed`、專案自己的 integer PRNG 與 deterministic tie-break；不要改用
  跨標準函式庫結果未定義的 random distribution。
- surface sampling 使用 stream 0；inward shell 使用 `shell_index + 1`，讓每層不同但可重現。
- spacing 是同一 shell 內的最小 3D 距離；`shell_spacing <= 0` 時沿用 spacing。
- inward shell 數量受 `max_shell_count` 限制；達到限制時保留合法結果並回報
  `shell_limit_reached`，不要無界生成。
- 空 mesh、非 finite／非正 spacing、無非退化表面必須失敗並提供 error。

多點輸出使用 `nearestPointOrder()` 的 exact greedy nearest-neighbour；k-d tree 只是效能實作，
等距時仍以原始 index 決定，確保相同輸入跨執行結果一致。不要退回大型點雲的 O(n²) scan。

## 進度與取消

所有可能掃過大量 triangle、candidate、plane 或 point 的階段都接受 `ProgressCallback`。回報值
在 `[0, 1]`，回傳 `false` 表示取消；內層算法應立即停止，並以 `cancelled` error 或空結果讓
呼叫端辨識。

callback 本身不負責 UI event loop；`ToolpathExporter` 在轉接 callback 時 pump Qt events 並
檢查 `cancelled_`。取消後直接停止產生輸出，不追加 STL-specific dotting cleanup 或 Z 歸零。

## 修改與驗證

依修改範圍驗證 binary/ASCII STL 判斷、malformed BSPC、identity/scale/mirror transform、固定與
adaptive planes、open/closed contour、hole/island nesting、跨物件 bottom-to-top 順序、固定 seed
的重現性、shell limit、nearest-neighbour tie，以及各主要迴圈的取消。

測試實作只能加入 `UnitTest` target，不可被 `swiftray_app_bundle`、production server route 或
正式 exporter 引用。不要把 slice、補償或 sampling 結果寫成 CSV/image debug dump。
