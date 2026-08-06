1、移除測試用 code。也可以用一個 boolean flag 控制編譯時要不要包含。
2、假如 STL 模型真的很大，可以考慮只使用同一組 mesh（套用 transform 後直接覆蓋原值）。並由前端在算完之後傳一個 discard 的 websocket 指令回收
3、sliceMeshes，實際使用時，大概率還是會一個一個呼叫，這個函數可能可以移除




TBD
- resamplePolygon
    是否要轉成對應 DPI 的位置？
    若否，需要微調間隔平均取樣？短線（起點/終點/結尾）？尾端以 0.5d 分別處理？