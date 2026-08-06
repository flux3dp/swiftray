1、移除測試用 code。也可以用一個 boolean flag 控制編譯時要不要包含。
2、假如 STL 模型真的很大，可以考慮只使用同一組 mesh（套用 transform 後直接覆蓋原值）。並由前端在算完之後傳一個 discard 的 websocket 指令回收
3、sliceMeshes，實際使用時，大概率還是會一個一個呼叫，這個函數可能可以移除




TBD
- resamplePolygon
    是否要轉成對應 DPI 的位置？
    若否，需要微調間隔平均取樣？短線（起點/終點/結尾）？尾端以 0.5d 分別處理？

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

1. **填充模式**（B-2）：目前打線/打點都只輸出輪廓，遇到 filled 會 warning 並退回輪廓。打線+填充要接 `outputLayerFillGcode` 的掃描線；打點+填充要把輪廓 rasterize 成指定 DPI 再走 `outputLayerBitmapGcode`。
2. **打點模式的實際指令語意**：目前是「laser off 跳到點 → 原地改 power 觸發」＋事先 `setDottingTime`。需要跟控制器確認。
3. **進度**：STL 沒有計入 `element_cnt_`，每層只呼叫 `onProgressChanged(current_progress_)`（**這是必要的** —— 它會跑 `processEvents()`，取消訊號才收得到，否則 1500 層的工作根本無法取消）。
4. **每層輪廓沒有排序**，目前照 chaining 順序輸出，跳點時間沒有最佳化。
5. **repeat > 1 會重複切片**（`convertLayer` 每個 repeat 跑一次）。應該用 (id, 層高) 當 key 快取。
6. **切片本身無法中斷**：`sliceMesh` 是一次性的阻塞呼叫，大模型會卡住數秒。
7. **ConvexHullExporter** 完全沒處理 STL。
8. **DocumentSerializer** 沒有存 `StlPlacement`（daemon 路徑用不到，GUI 存檔才需要）。