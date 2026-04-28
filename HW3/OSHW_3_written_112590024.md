# 112590024 邱紹溏 資料庫作業 HW3

```
 7.8: The Linux kernel has a policy that a  process cannot hold a spinlock while  attempting to acquire a semaphore. Explain  why this policy is in place.
```

## 7.8

因為持有 Spinlock 的行程「絕對不能休眠」，而嘗試獲取 Semaphore 失敗時會強迫「進入休眠」。

若帶著 Spinlock 進入休眠，會導致其他等待 Spinlock 的程式在 CPU 上不斷空轉，最後造成系統 Deadlock。

***

```
8.20: In a real computer system, neither the resources available nor the demands of processes for resources are consistent over long periods (months). Resources break or are replaced, new processes come and go, and new resources are bought and added to the system.

If deadlock is controlled by the banker’s algorithm, which of the following changes can be made safely (without introducing the possibility of deadlock), and under what circumstances?
 (a) Increase Available (new resources added).

 (b) Decrease Available (resource permanently  removed from system).

 (c) Increase Max for one process (the process  needs or wants more resources than allowed).

 (d) Decrease Max for one process (the process  decides that it does not need that many  resources).

 (e) Increase the number of processes.

 (f) Decrease the number of processes.


```

## 8.20

### (a)


### (b)

### (c)


### (d)


### (e)

### (f)

