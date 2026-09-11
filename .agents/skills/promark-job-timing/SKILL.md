---
name: promark-job-timing
description: Swiftray Promark／BSL 的工作時間模型與 per-job CONFIG 協定。修改 MachineJob 時間預估、BSL list streaming/progress、galvo delay、Z/A 軸移動、dotting 或 first-pulse 行為時使用。
---

# Promark 工作時間與 Job Config

Promark 同一份 G-code 有兩套時間計算，兩者必須使用相同規則：

- `MachineJob::calcTotalTime()` 在送出前計算整份工作的總時間，作為進度分母。
- `BSLMotionController::estimated_time_` 在 streaming 時逐 list 累加，作為執行進度與
  double-buffer list 切換依據。

共用公式與狀態放在 `src/promark_timing.h`。修改計時行為時，優先擴充這個共用層，不要在
兩個 consumer 各寫一份近似公式。

## 相關程式碼

- `src/promark_timing.h`：計時公式、`Config`、`MarkSequence`、CONFIG parser。
- `src/constants.h`：實際傳給 LCS 的軸參數與 Promark 預設 delay。
- `src/executor/machine_job/machine_job.cpp`：整份 G-code 的預估。
- `src/periph/motion_controller/bsl_motion_controller.cpp`：實際 LCS 指令、per-list 預估與
  streaming progress。
- `src/toolpath_exporter/toolpath-exporter.cpp`：從 convert params 產生 `;CONFIG`。
- `src/toolpath_exporter/generators/gcode-generator.h`：Promark 的 `P`、`B`、`T` 指令輸出。

STL convert params 與輸入契約請讀 `stl-inner-engraving` skill；幾何演算法請讀
`stl-geometry-processing` skill。

## Per-job CONFIG 資料流

`ToolpathExporter::parseParam()` 應先輸出 `;CONFIG RESET`，再為本次工作提供的參數輸出
`;CONFIG KEY=VALUE`。

`PromarkTiming::Config` 管理下列會影響計時的 key：

- `JUMP_SPEED`
- `LASER_ON_DELAY`
- `LASER_OFF_DELAY`
- `MARKING_DELAY`
- `CORNER_DELAY`
- `JUMP_DELAY_MIN`
- `JUMP_DELAY_MAX`
- `FIRST_PULSE_KILLER_ENABLED`

`UV` 會影響 controller 的 laser type，但不屬於計時參數，由
`BSLMotionController::handleGcode()` 個別處理。未知 key 不應默默改變 `Config`。

`RESET` 必須讓每份工作的 timing `Config` 回到 compiled defaults，避免上一份工作的參數洩漏
到下一份。它不重設 controller 的 laser type；需要指定 UV/MOPA 的工作仍應明確提供 `UV`。
新增會改變實際工作時間的 CONFIG key 時，必須同時處理 exporter、共用 `Config`、整體預估
與 controller 實際設定。

## 單位與軸移動

- XY、Z 與距離使用 mm。
- G-code `F` 是 mm/min；BSL 與 `PromarkTiming::markTimeMs()` 使用 mm/s。
- LCS laser/scanner/jump delays 與 G-code `T` 使用 µs。
- 所有 `PromarkTiming` 回傳值與 `estimated_time_` 使用 ms。
- Z/A 實際移動先依 `*_PULSE_PER_MM` 量化為整數 pulse；controller 與 estimator 應使用同一
  組 `run speed`、`start speed`、`acceleration time`。

`axisMoveTimeMs()` 不能退回單純的距離除速度。短距離 Z move 通常到不了 run speed，必須用
三角速度曲線；足夠長的 move 才使用含 cruise 的梯形曲線。更改
`lcs_set_axis_move()` 參數時，同步更新 `src/constants.h`，不要只校正 estimator。

## G-code 動作分類

- Jump：laser off 或 `S0` 的 XY move。時間為 travel 加依距離插值的 jump delay。
- Mark：laser on、`T0` 的 XY move。第一段加 laser-on delay，連續段之間加 corner delay，
  sequence 結束時加 laser-off 與 marking delay。
- Dot：`T > 0` 且 power 開啟。先 jump 到點，再計入獨立 laser pulse 的完整 delay。
- Z/A 軸移動、jump、dot 與 `M2`／job 結束都會結束當前 `MarkSequence`。

Dot polygon 的第一點可能是只有 `F...S...`、沒有 XY 位移的 power-only command；它仍是一個
實際 dot，兩套 estimator 與 controller 都不可因 `distance == 0` 而忽略。

`Config::dotTimeMs()` 包含 laser-on、laser-off/marking、dotting time，以及啟用
first-pulse killer 時的 `dotting_time + 5µs`。目前 STL dot 的實測係數是 `2.0`；除非有新的
硬體量測依據，不要單獨修改或在其他檔案再乘一次。

## List 邊界

- `estimated_time_` 會參與 `MAX_BUFFER_LIST_TIME` 判斷，公式誤差也會改變 list 切換時機。
- `startList()` 必須重新套用 list-level laser/scanner delays、pulse、speed、power 與
  first-pulse killer。
- `T` 常在 `M3` 前出現，因此 `M3` 不可重設 `dotting_time_`；新 list 也要重新套用它。
- list swap 不代表 mark sequence 在物理上自然結束；只有實際動作語意要求時才結束 sequence。

## 修改與驗證

計時相關修改至少檢查以下案例：純 jump、連續 mark、mark 後 jump、一般 dot、零距離第一個
dot、Z move、A move、CONFIG RESET，以及跨 list 邊界的 dotting。比較
`MachineJob::calcTotalTime()` 與 controller 對相同 G-code 的累計邏輯，不要只驗證其中一套。

若變更的是硬體實測常數，記錄機型、laser type、參數、樣本工作與量測結果；不把單次測試的
補償值擴大成所有 Promark 工作的通則。
