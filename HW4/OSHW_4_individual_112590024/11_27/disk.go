// go run disk.go <初始磁頭位置>
package main

import (
	"fmt"
	"math/rand"
	"os"
	"sort"
	"strconv"
)

const (
	CYLINDERS    = 5000
	NUM_REQUESTS = 1000
)

func generateRequests() []int {
	reqs := make([]int, NUM_REQUESTS)
	for i := range reqs {
		reqs[i] = rand.Intn(CYLINDERS)
	}
	return reqs
}

func abs(x int) int {
	if x < 0 {
		return -x
	}
	return x
}

// ── FCFS ────────────────────────────────────────────
func fcfs(reqs []int, head int) int {
	movement := 0
	cur := head
	for _, r := range reqs {
		movement += abs(cur - r)
		cur = r
	}
	return movement
}

// ── SCAN ────────────────────────────────────────────
// 向高編號掃，碰到頂端再往低編號掃
func scan(reqs []int, head int) int {
	sorted := make([]int, len(reqs))
	copy(sorted, reqs)
	sort.Ints(sorted)

	movement := 0
	cur := head

	// 分成比 head 大和比 head 小的兩段
	var right, left []int
	for _, r := range sorted {
		if r >= cur {
			right = append(right, r)
		} else {
			left = append(left, r)
		}
	}

	// 先往右（高編號）掃
	for _, r := range right {
		movement += abs(cur - r)
		cur = r
	}
	// 再往左（低編號）掃（從最大跑回來）
	for i := len(left) - 1; i >= 0; i-- {
		movement += abs(cur - left[i])
		cur = left[i]
	}

	return movement
}

// ── C-SCAN ──────────────────────────────────────────
// 只往高編號掃，掃到底後直接跳回 0，繼續往高編號掃
func cscan(reqs []int, head int) int {
	sorted := make([]int, len(reqs))
	copy(sorted, reqs)
	sort.Ints(sorted)

	movement := 0
	cur := head

	var right, left []int
	for _, r := range sorted {
		if r >= cur {
			right = append(right, r)
		} else {
			left = append(left, r)
		}
	}

	// 先往右掃到底
	for _, r := range right {
		movement += abs(cur - r)
		cur = r
	}

	// 跳回 0（不計算這段為服務移動，但實際磁頭要移動）
	if len(left) > 0 {
		movement += cur // 移到最末端 4999... 簡化為移到 0
		cur = 0
		// 從 0 繼續往右掃剩下的
		for _, r := range left {
			movement += abs(cur - r)
			cur = r
		}
	}

	return movement
}

func main() {
	if len(os.Args) < 2 {
		fmt.Println("用法: go run disk_scheduling.go <初始磁頭位置>")
		fmt.Println("範例: go run disk_scheduling.go 2500")
		os.Exit(1)
	}

	head, err := strconv.Atoi(os.Args[1])
	if err != nil || head < 0 || head >= CYLINDERS {
		fmt.Printf("錯誤：磁頭位置必須介於 0 到 %d 之間\n", CYLINDERS-1)
		os.Exit(1)
	}

	reqs := generateRequests()

	fmt.Printf("Cylinders:        0 to %d\n", CYLINDERS-1)
	fmt.Printf("Total Requests:   %d\n", NUM_REQUESTS)
	fmt.Printf("Initial Head:     %d\n", head)
	fmt.Println("----------------------------------------")
	fmt.Printf("FCFS   Total Head Movement: %d\n", fcfs(reqs, head))
	fmt.Printf("SCAN   Total Head Movement: %d\n", scan(reqs, head))
	fmt.Printf("C-SCAN Total Head Movement: %d\n", cscan(reqs, head))
}
