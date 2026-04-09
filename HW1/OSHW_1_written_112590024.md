# 112590024 邱紹溏 資料庫作業 HW1

```
Chap.1
1.16: Direct memory access(DMA) is used for high-speed I/O devices in order to avoid increasing the CPU’s execution load.
a. How does the CPU interface with the device to coordinate the transfer?
b. How does the CPU know when the memory operations are complete?
c. The CPU is allowed to execute other programs while the DMA controller is transferring data. Does this process interfere with the execution of the user programs? If so, describe what forms of interference are caused.
```

## 1.16(a) 

CPU 透過 DMA 來協調：先把來源/目的位址、傳輸資料量（byte count）、方向、傳輸模式寫進 DMA 的暫存器，然後發出啟動命令。之後 DMA 就自己透過系統匯流排直接傳資料，CPU 不需要參與其中


## 1.16(b)

DMA 控制器傳完整個 block 後，會發送一次DMA completion interrupt給 CPU。CPU 收到中斷後進入中斷處理，確認傳輸完成並做後續處理。 



## 1.16(c)

會產生輕微干擾，cycle stealing mode（干擾小）跟 burst mode（干擾較大），DMA 控制器傳輸資料時，會暫時占用記憶體匯流排，導致 CPU 在這些匯流排週期內必須等待（stall）。 

```
2.19: What is the main advantage of the microkernel approach to system design? How do user programs and system services interact in a microkernel architecture? What are the disadvantages of using the microkernel approach?
```

## 2.19
優點：  
擴充性：當需要新功能時，不需要動到核心  
移植性：程式碼非常少，可以很好的移植  
可靠性與安全性：大多束縛無都在使用者端而非核心。如果某個服務崩潰，不會影響到其他服務。  

使用者程式與系統服務的互動方式：在微核心架構中，使用者程式與系統服務之間是透過訊息傳遞（Message Passing） 

  1. 使用者程式（Client）需要某項服務時，會向微核心發送一則訊息。  

  2. 微核心（扮演中介角色）接收到訊息後，將其轉發給對應的系統服務程式（Server）。  

  3. 系統服務程式處理完請求後，再透過微核心發送一則包含結果的訊息回傳給使用者程式。  

* **通訊機制：** 這種機制通常被稱為**程序間通訊（Interprocess Communication, IPC）**。微核心的主要工作就是提供這種通訊設施，並負責低階的記憶體管理與執行緒排程。  

缺點：
系統功能開銷：在單核心（Monolithic kernel）中，系統呼叫通常只需一次環境切換（從使用者模式到核心模式）。但在微核心中，完成一次服務請求通常涉及多次訊息傳遞與環境切換。 

通訊成本：在單核心（Monolithic kernel）中，系統呼叫通常只需一次環境切換（從使用者模式到核心模式）。但在微核心中，完成一次服務請求通常涉及多次訊息傳遞與環境切換，實作較為複雜。 

```
2.20: What are the advantages of using loadable kernel modules?
```

## 2.20
**核心精簡** ：減少核心大小，節省記憶體  
**動態功能擴展**：即時載入/卸載模組，無需重啟  
**靈活性高**：支援多種硬體與功能模組  
**穩定性強**：隔離故障，降低核心崩潰風險  
**易於維護**：模組化設計，簡化更新與除錯  
**支援即時系統**：適合需要動態載入功能的應用  

```
3.12: Describe the actions taken by a kernel to context-switch between processes.
```

## 3.12
* **保存狀態 (Save State)：** 將目前執行中程序 (P0​) 的所有硬體暫存器、程式計數器 (PC) 數值，保存到該程序的 **PCB (Process Control Block)** 中。 

* **更新 PCB：** 更新 P0​ 的狀態（例如從「執行中」改為「就緒」或「等待」）。  

* **選擇程序 (Schedule)：** 排程器從就緒佇列中挑選下一個要執行的程序 (P1​)。  

* **載入狀態 (Restore State)：** 從 P1​ 的 **PCB** 中取出先前保存的暫存器與 PC 數值，重新載入到 CPU 硬體中。

* **恢復執行：** CPU 根據新載入的 PC 指向的位置，開始執行 P1​。  


```
3.18: Give an example of a situation in which ordinary pipes are more suitable than named pipes and an example of a situation in which named pipes are more suitable than ordinary pipes. 
```

## 3.18
* **一般管道 (Ordinary)：**
  * **場景：** 父子程序通訊（如 Shell `|`）。
  * **特性：** 暫時性、單向、需有親緣關係。

* **具名管道 (Named)：**
  * **場景：** 獨立程序通訊（如 日誌服務）。
  * **特性：** 持久性（存在檔案系統）、雙向、不需親緣關係。

