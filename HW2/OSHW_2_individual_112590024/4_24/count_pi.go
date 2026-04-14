/*
 * Monte Carlo method to estimate π
 *
 * Compile: go build count_pi.go
 * Run:     ./count_pi
 * Or:      go run count_pi.go
 */

package main

import (
	"fmt"
	"math/rand"
	"sync"
)

const totalPoints = 1000000 // total number of random points

var circlePoints int64 // global variable: number of points inside the circle

func generatePoints(wg *sync.WaitGroup) {
	defer wg.Done()

	var count int64
	for i := 0; i < totalPoints; i++ {
		x := rand.Float64()*2 - 1 // random x in [-1, 1]
		y := rand.Float64()*2 - 1 // random y in [-1, 1]

		if x*x+y*y <= 1 { // check if point is inside the circle
			count++
		}
	}

	circlePoints = count
}

func main() {
	var wg sync.WaitGroup

	wg.Add(1)
	go generatePoints(&wg) // launch child goroutine
	wg.Wait()              // wait for child goroutine to finish

	pi := 4 * float64(circlePoints) / float64(totalPoints)
	fmt.Printf("Number of points: %d\n", totalPoints)
	fmt.Printf("Points in circle: %d\n", circlePoints)
	fmt.Printf("Estimated π = %f\n", pi)
}
