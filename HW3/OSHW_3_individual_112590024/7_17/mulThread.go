// go run mulThread.go
package main

import (
	"fmt"
	"math"
	"math/rand"
	"sync"
)

const (
	NUM_THREADS = 4       // 執行緒數量
	NUM_POINTS  = 100000 // 每個執行緒產生的隨機點數量
)

// 共享全域變數，記錄落在圓內的點數
var circlePoints int

// mutex lock，保護共享變數避免 race condition
var mutex sync.Mutex

// 每個 goroutine 執行的函式
// 產生隨機點，判斷是否落在圓內，並更新共享變數
func generatePoints(wg *sync.WaitGroup) {
	defer wg.Done()

	localCount := 0

	for i := 0; i < NUM_POINTS; i++ {
		// 產生 -1.0 ~ 1.0 的隨機座標
		x := rand.Float64()*2 - 1
		y := rand.Float64()*2 - 1

		// 判斷是否落在單位圓內 (x^2 + y^2 <= 1)
		if math.Sqrt(x*x+y*y) <= 1 {
			localCount++
		}
	}

	// 更新共享變數前加鎖，避免 race condition
	mutex.Lock()
	circlePoints += localCount
	mutex.Unlock()
}

func main() {
	var wg sync.WaitGroup

	// 建立多個 goroutine，每個各自產生隨機點
	for i := 0; i < NUM_THREADS; i++ {
		wg.Add(1)
		go generatePoints(&wg)
	}

	// 等待所有 goroutine 完成
	wg.Wait()

	// 計算 π 估計值
	// π ≈ 4 * (圓內點數 / 總點數)
	totalPoints := NUM_THREADS * NUM_POINTS
	pi := 4 * float64(circlePoints) / float64(totalPoints)

	fmt.Printf("Total Points : %d\n", totalPoints)
	fmt.Printf("Circle Points: %d\n", circlePoints)
	fmt.Printf("Estimated Pi : %.6f\n", pi)
}