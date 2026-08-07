1、移除測試用 code。也可以用一個 boolean flag 控制編譯時要不要包含。
2、假如 STL 模型真的很大，可以考慮只使用同一組 mesh（套用 transform 後直接覆蓋原值）。並由前端在算完之後傳一個 discard 的 websocket 指令回收
3、sliceMeshes，實際使用時，大概率還是會一個一個呼叫，這個函數可能可以移除




TBD
- resamplePolygon
    是否要轉成對應 DPI 的位置？
    ~~若否，需要微調間隔平均取樣？短線（起點/終點/結尾）？尾端以 0.5d 分別處理？~~ → 已定案，見下方 I 節

===========

# I. 切片行為變更（取代 TODO.md 的舊定案，共用檔案不動）

## I-1. resamplePolygon 改成「四捨五入 + 平均取樣」

**取代 TODO.md 5-(p) 的「尾端不足一個間隔直接拋棄」。**

新規則：`gap_count = round(L / d)`（至少 1），實際步距 `step = L / gap_count`。

| | 點數 | 說明 |
| --- | --- | --- |
| 開放輪廓 | `gap_count + 1` | 頭尾都取，不丟棄尾端 |
| 封閉輪廓 | `gap_count` | 不重複起點，接縫的間距跟其他間距一樣 |
| `L < d/2` | 至少 1 個 gap | 開放給頭尾 2 點、封閉給起點 1 點 |

實際步距會跟指定值差最多半個間隔，但**所有間距完全相同**，所以 5-(p) 原本記的「封閉輪廓接縫過曝」副作用消失了。

例：10.5mm / 1mm → 開放 12 點（步距 0.9545），封閉 11 點（步距 0.9545，含接縫）。

> ⚠️ 自我檢查裡的「間距均勻」只能用**直線**驗證：輪廓有轉角時，跨過轉角的兩個取樣點之間的**直線距離**本來就會小於弧長步距。

## I-2. 多個 STL 共用一條 Z ladder

**取代原本「一個物件打完再換下一個」的順序。**

一個圖層裡的多個 STL 若 Z 有重疊，必須一起處理，**完全依照 Z 由低到高**：

1. 每個物件各自 `stl::Slicer::prepare()`，取得它自己的 `planes()`
2. 把所有 `(z, 物件)` 合併排序 → 這就是共用的 Z ladder
3. 逐個 Z step：把落在同一高度（差距 < 最細層高的千分之一）的所有物件切片結果一起收集
4. 一次 `moveZ`，然後輸出這個高度的所有輪廓

因此 `stl-utils` 新增 **`stl::Slicer`**（prepare 一次，`sliceAt(z)` 可切任意 Z）。內部的 active list 是往上掃的，Z 遞增是快路徑；往回切也正確，只是會重掃。`sliceMesh()` 保留成 Slicer 的薄包裝。

Z 只在**全部物件都做完之後**復位一次，不再每個物件各自來回。

## I-3. 四種 STL 物件在每個 Z step 內依序處理

**補充 I-2，不修改它：Z ladder 仍然是全圖層唯一一條、嚴格遞增。**

依 B-2，STL 物件依「打線/打點 × 填充/非填充」分成四種，每種要走不同的 gcode 路徑。`outputLayerStlGcode()` 在**每一個 Z step 內部**依 **Mode::Line+fill → Mode::Line+non-fill → Mode::Dot+fill → Mode::Dot+non-fill** 的順序分派：

```
z0: line+fill, line, dot+fill, dot | z1: line+fill, line, dot+fill, dot | ...
```

⚠️ **不可以改成「一種打完再換下一種」** —— 那會讓 Z 退回低點重來，違反 B-5「由深到淺」（已雕刻的裂點會散射後續雷射）。四種是同一個 Z 的不同輸出路徑，不是四趟。

| 種類 | 函數 | 輸出路徑 |
| --- | --- | --- |
| 打線 + 填充 | `outputStlLineFillGcode` | 輪廓 → `layer_filled_polygons_` → `outputLayerFillGcode` |
| 打線 + 非填充 | `outputStlLineGcode` | 輪廓 → `layer_polygons_` → `outputLayerPathGcode` |
| 打點 + 填充 | `outputStlDotFillGcode` | 輪廓 → 掃描線（間隔＝點距）取得線段 → 每段重取樣成點 → `outputLayerPathGcode` + dotting time |
| 打點 + 非填充 | `outputStlDotGcode` | 輪廓重取樣成點 → `outputLayerPathGcode` + dotting time |

- 種類是切片前就決定的（`stlEngraveKind()`：物件的 `data-stl-fill` 優先，沒有就看圖層 type），每個 Z step 把切出來的輪廓丟進四個 bucket，再依序輸出。
- 每個 Z step 只 `moveZ` 一次，四種共用；Z 只在整個圖層做完後復位一次。
- 每一種輸出前會清空 `layer_polygons_` / `layer_filled_polygons_`，四種互不污染。
- dotting time 只在**實際改變時**才重下（全部都是打點的圖層只會設一次）：`kDotFill` / `kDot` 需要，`kLineFill` / `kLine` 需要關掉。
- ⚠️ `outputLayerFillGcode` 取消時會**帶著 `polygons_mutex_` 提早 return**，所以 kind 迴圈每一輪開頭都要檢查 `cancelled_`。

### 打點 + 填充的做法（選了掃描線，不是 raster）

`outputLayerFillGcode` 增加一個 `FillOverride` 參數：

- `override_scan`：強制掃描參數（間隔＝該物件的點距、角度 0、雙向、單次 hatch），所以掃描線是水平的、剛好一個點距一條
- `out_segments`：**不輸出任何 gcode**，把每段填充線段交回給呼叫端
- `quiet`：關掉每次呼叫的 qInfo（1500 層會刷爆 log）

`outputStlDotFillGcode` 拿回線段後，每段用**同一個點距**跑 `resamplePolygon`，得到的點丟進 `layer_polygons_` 走 `outputLayerPathGcode` + dotting time —— 跟打點+非填充共用同一套「一個點是什麼」的定義。因為掃描間隔是**物件自己的點距**，所以是一個物件跑一次掃描。

不走 raster（`setDpmm` + `outputLayerBitmapGcode`）的理由：`setDpmm` 會重配 layer bitmap 並重建 `global_transform_`（其他 STL 路徑都在用），而且要**每個 Z step 做一次**。

⚠️ 網格只有在掃描方向上是等距的：列與列剛好差一個點距，但同一列內的點距是 `弦長 / round(弦長 / 點距)`（I-1 的四捨五入規則）。弦長短於半個點距時仍會給頭尾兩點，所以切片邊緣會比內部稍密。

## I-4. Y 軸翻轉在後端做

前端送來的 `data-stl-matrix` 與佔位 rect 的 `y` 都是 **3D 場景座標**（見 TODO-frontend.md 文末），與畫布 / G-code 的 Y 差一個翻轉：

```
canvas_y = workarea_height - scene_y
```

`workarea_height` 取 `machine_work_area_mm_.height() * canvas_mm_ratio_`（畫布單位 0.1mm），**不可寫死**。

實作方式：`ToolpathExporter::stlCanvasMatrix()` 把這個翻轉**併進 placement 矩陣**再交給 `stl::Slicer::prepare()`，所以切片出來的輪廓已經是畫布座標，其餘 STL 程式碼完全不需要知道有兩套座標系。翻轉會讓行列式變負（鏡射），`stl::applyTransform` 明確允許 —— 繞向在切片後會正規化。

⚠️ 佔位 rect 的 `x/y/width/height` 也是場景座標，所以 `convertPath()` 在 contour / 紅光模式把佔位 rect 當投影輸出時，**同樣要套一次 Y 翻轉**（已實作），否則框線會上下顛倒。

## I-5. 被拆成 `x` / `x-filled` 的圖層，STL 要合成一條 Z ladder

swiftray 是**以圖層為單位**設定填充與否，但前端是 per-object（用 `fill` / `opacity` 表達）。所以一個混用填充的畫布圖層，在 `findLayer()`（`mysvg-functions.h`）會被拆成 `x`（Line）與 `x-filled`（Fill）兩層，`convertStack` 再用「名稱 + type」把它們配對回來（`toolpath-exporter.cpp`）。

原本配對只是為了讓 focus 的 Z 移動不要做兩次。**對 STL 來說代價大得多**：`layer_stl_placements_` 每次 `convertLayer` 都會清空，等於**整條 Z ladder 跑兩趟** —— 從底掃到頂、退回底、再穿過剛剛雕好的材料掃第二次。這直接違反 B-5，不只是慢。

作法（評估過三個方案，選了「只有 STL 走合併」）：

1. `convertStack` 用 `convertLayer(layer, stl_paired)` 告訴第一半「後面還有配對圖層」
2. 第一半不清空 `layer_stl_placements_`、也不呼叫 `outputLayerStlGcode()`；第二半才一次輸出，所以兩半的物件在同一條 ladder 上
3. `layer_stl_placements_` 存 `StlPlacementJob{placement, kind}`，**kind 在 `convertPath()` 收集時就解析好** —— 那時 `current_layer_` 還是物件自己的圖層，到輸出時已經換成配對的另一半了

非 STL 的路徑完全不動。

**為什麼不需要記住來源圖層**：配對的兩層是同一份 config 的複本（`my_qsvg_handler_qt6.cpp` 的 `layer_config_map_[title + "-filled"] = layer_config_map_[title]`），emitter 從 `current_layer_` 讀的 speed / power / fill 參數 / dottingTime / wobble / dpmm 全部相同，**唯一不同的是 `type()`，而它只被 `stlEngraveKind()` 用到** —— 那個已經在收集時算完了。

⚠️ 配對條件只比對名稱與 type、不比對參數。萬一哪天出現名字湊巧配對、參數卻不同的兩層，STL 會全部套用第二層的參數 —— 已加 `qWarning` 比對 speed / power。

其他兩個方案沒選的理由：
- **在 `convertLayer` 的 shape 迴圈後直接接第二層**：`setDpmm` / `global_transform_` / `element_cnt_` / `with_image_` / frequency / progress 全是 per-layer，輸出端又都讀 `current_layer_` 拿參數，等於要把現在的兩趟重新實作一遍，而且爆炸半徑涵蓋所有機種；2D 的收益幾乎是零（focus Z 已經提出去了）
- **要求前端強制以圖層為單位設定填充**：模型比較乾淨，但它不是後端的保護傘 —— 只要有任何檔案產生了拆分，後端還是會靜默地掃兩趟 Z

===========

# 算圖（STL → GCode）：查證後補充的事項

以下都是實際讀過對應程式碼後確認的，行號為當時的位置。

## ⚠️ A. 阻塞項：stlObjects 現在無法用 binary 傳進來

`SwiftrayServer::processBinaryMessage`（`swiftray-server.cpp:124`）的實作是
`QString text = QString::fromUtf8(message); processMessage(text);`
—— **整包 binary 被當成 UTF-8 解碼**，STL 的二進位內容一定會被破壞（非法 UTF-8 序列被換成 U+FFFD）。

前端 A-3 說「swiftray client 的 `action()` payload > 4096 且 `SWIFTRAY_SUPPORT_BINARY` 時走 binary」，走的正是這條路。所以：

- **v1 的可行做法：stlObjects 內的 mesh 用 base64 放在 JSON 裡**（膨脹 4/3，35MB 的模型 → 約 47MB 的 JSON 欄位）
- 真的要傳原始 binary，得改協定：例如「先送一個 JSON 宣告 id 與長度，再送 N 個 binary frame」，`processBinaryMessage` 要能分辨「binary 是 JSON 文字」還是「binary 是 mesh 資料」

→ **需要跟前端敲定**。在敲定前，後端先按 base64 實作。

## B. Parser（step 2）三個會讓佔位 rect 直接消失的地方

已確認：`MYSVG` 定義下 `createRectNode`（`my_qsvg_handler_qt6.cpp:4041`）把 `<rect>` 轉成 **QSvgPath**（`qpath.addRoundedRect`），不是 QSvgRect。所以佔位 rect 走的是一般 path 流程 —— 這同時證實 B-1「不攔截就會多雕一個矩形」。

要小心三件事：

1. ⚠️ **`if (bounds.isEmpty()) return nullptr;`（`:4025`）** —— 佔位 rect 的 width/height 若為 0（前端還沒同步 3D bbox、或模型退化成平面），整個節點會回傳 nullptr，STL 物件直接消失且沒有任何錯誤訊息。有 `data-stl` 時要放行。
2. ⚠️ **`processMySVGNode` 的 QSVG_PATH 分支有兩個 `return`**（`mysvg-functions.h` 白色填充、無有效 stroke）—— 佔位 rect 很可能兩者都中（前端不會給它可見的樣式）。**`data-stl` 的判斷必須放在這兩個 skip 之前。**
3. ⚠️ **global 汙染**：現有的 `g_pass` / `g_zstep` 只在 `createImageNode` 設定，之後不會清掉；因為只有 image 分支會讀，所以現在沒事。新增 `g_stl_*` 若照抄這個寫法，**下一個普通 rect / path 會沿用上一個 STL 的 id**。必須在每個 create*Node 進入時重設（或改成只在 rect 分支讀取並立刻清空）。

其餘連帶要改的：`PathShape::clone()` 要複製新欄位；`DocumentSerializer`（BVG 存檔）若要支援再處理，daemon 路徑用不到，先標 TODO。

## C. 算圖入口確認（step 3）

- FPM1 UV 是 Promark → `worker.cpp` 的 `is_promark` 分支 → `ToolpathExporter` + `GCodeGenerator`。**只改 ToolpathExporter 是對的。**
- `type == "hull"` 走 `ConvexHullExporter`（前端的 framing）；B-7 之後要加【材料外框】，先標 TODO。
- `type == "contour"` 會 `handleContour()` → `is_contour_ = true`（紅光預覽），這時**不該切片**，用投影即可 —— 正好對應 `useStlDetail`。
- ⚠️ `type == "preview"` 只要 `timeCost`，但它仍然會完整跑一次 `convertStack`。若 preview 用投影，工時估算會嚴重低估（少了 1500 層的 Z 移動與掃描）；若用真實切片，preview 會跟正式算圖一樣慢。**待決定**。

## D. ToolpathExporter（step 4）

1. ⚠️ **`outputLayerGcode()` 會除以零**：`total_element_cnt_` 是各類元素數量的總和（`toolpath-exporter.cpp:470-476`），接著直接拿它當分母。若某圖層只有 STL 佔位 rect，而佔位 rect 又被攔截（不進 `layer_polygons_`），`total_element_cnt_` 會是 0 → NaN / 除零。**加 STL 計數或先判斷 0。**
2. **切片順序**：`for 每個物件 { for 每層 { } }`，不是 `for 每層 { for 每個物件 } }` —— B-6 明確寫「多物件的層各自打完再換物件」。
3. ⚠️ **Z 的正負向沒有自明的答案**。現有 focus 的寫法是 `gen_->moveZ(-focus * focus_dir)`，而 `focus_dir` 來自圖層的 `data-focusRev` —— 也就是**連既有功能都需要一個反向旗標才能對**。內雕的 Z 打錯方向 = 整件報廢，建議：
   - 比照做一個反向設定（或沿用圖層的 focusRev）
   - 第一版先用小模型 + 少層數實機驗證方向後再放大
4. **Z 復位**：現有 focus 的做法是累加 `total_move` 最後一次移回。若中途 `cancelled_`，`convertStack` 直接 return，**不會復位** —— 內雕累積 150mm 的位移，不復位問題比 focus 大得多。要處理取消路徑。
5. `moveZ` 一律相對（`gcode-generator.h:159` 送 `M102` + `Z`），符合 B-10「Z 是相對座標」✓。精度 `move_precision_ = 10000`（Promark）→ 0.0001mm，優於需求的 0.001mm ✓。
6. ⚠️ **M102 是「Z 軸 io」**（`bsl_motion_controller.cpp:638`：`SetIo(0b10, 0b10)`），不是同步等待的移動指令。1500 層每層都要等 Z 到位，**要確認 Z 移動與 galvo 掃描之間是否需要顯式等待**，否則會在 Z 還在動的時候就開始掃描。
7. ⚠️ **產出的 gcode 量級是最大的實務風險**。以測試模型（71 萬面 / 高 150mm）0.1mm 層高估算，切片會產生數百萬個點；每個點一行 gcode → **數十 MB 的 QString**。而 `worker.cpp` 後續會做：
   - `server_->m_buffer = QString::fromStdString(gen.toString())`
   - `server_->gcode_list_ = server_->m_buffer.split("\n")` ← 產生數百萬元素的 QStringList
   - `MachineJob::calcTotalTime(gcode_list_)` ← 逐行解析
   - 最後整包透過 websocket 回傳
   這條路徑在內雕的量級下大概率會爆記憶體或慢到不可用。**需要及早量測，可能要改成串流輸出或分塊。**

## E. 參數來源：per-object vs per-layer

TODO.md 的 TBD 傾向放圖層，但第 6 點把層高/點距放在物件的 OptionsPanel。後端先做成
**「佔位 rect 的 attribute 優先，沒有就 fallback 到圖層設定」**，兩種前端決定都能吃，不用等結論。

## F. 其他

- 若某個佔位 rect 找不到對應的 stlObject → 丟棄（已定），但**一定要 log warning**，否則使用者只會看到「什麼都沒雕」而查不出原因。
- `ToolpathExporter::parseParam` 是現成的 config 注入點（`toolpath-exporter.cpp:18`），折射率（B-3）之後從這裡進來最自然。

## G. 需要前端配合的介面（後端已按這個實作）

### loadSVG payload

```jsonc
{
  "file": { "data": "<svg string>", "thumbnail": "..." },
  "stlObjects": {
    "<佔位 rect 的 id>": "<base64 的 STL binary>"
    // 或 { "data": "<base64>" }，兩種都吃
  }
}
```

回傳會多帶 `loadedStlObjects`（成功數量）與 `failedStlObjects`（`[{id, error}]`）。

### 佔位 rect 的 attribute

| attribute | 說明 |
| --- | --- |
| `data-stl` | 物件 id，對應 stlObjects 的 key（必要） |
| `data-stl-matrix` | 16 個數字，**column-major**（＝ `THREE.Matrix4.elements` / CSS `matrix3d()` 的順序），已含 mm → 0.1mm 的 ×10 |
| `data-stl-layer-height` | 層高 mm，<= 0 或省略則用預設 0.1 |
| `data-stl-point-spacing` | 點距 mm，<= 0 或省略則用預設 0.1 |
| `data-stl-mode` | `"dot"` / `"line"`（預設 line） |
| `data-stl-fill` | `"1"` / `"0"`，省略則跟隨圖層 type |

> 層高 / 點距採「物件優先、圖層 fallback」，所以 per-object 或 per-layer 的結論還沒定也不影響後端。

### convert params

- `stl_z_reversed`（bool）：翻轉內雕 Z 的方向。**實機驗證前務必先用小模型確認**（見 D-3）。

## H. 這一輪還沒做的（已在程式碼標 TODO）

1. ~~**填充模式**（B-2）~~ → 四種都實作完了，見 I-3。剩下的是實機驗證打點的指令語意（下面第 2 點）。
2. **打點模式的實際指令語意**：目前是「laser off 跳到點 → 原地改 power 觸發」＋事先 `setDottingTime`。需要跟控制器確認。
3. **進度**：STL 沒有計入 `element_cnt_`，每層只呼叫 `onProgressChanged(current_progress_)`（**這是必要的** —— 它會跑 `processEvents()`，取消訊號才收得到，否則 1500 層的工作根本無法取消）。
4. ~~**每層輪廓沒有排序**~~ → **刻意不做**：Promark 跳點很快，不值得付排序成本。TODO 留在程式碼裡但不實作。
5. **repeat > 1 會重複切片**（`convertLayer` 每個 repeat 跑一次）。應該用 (id, 層高) 當 key 快取。
6. **切片本身無法中斷**：`sliceMesh` 是一次性的阻塞呼叫，大模型會卡住數秒。
7. **ConvexHullExporter** 完全沒處理 STL。
8. **DocumentSerializer** 沒有存 `StlPlacement`（daemon 路徑用不到，GUI 存檔才需要）。