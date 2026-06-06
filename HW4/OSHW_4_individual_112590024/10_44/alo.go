// go run alo.go <frame 數量>
package main

import (
	"fmt"
	"math/rand"
	"os"
	"strconv"
)

const PAGE_REF_LEN = 20

// 產生隨機 page reference string（頁碼 0~9）
func generatePageRefString() []int {
	refs := make([]int, PAGE_REF_LEN)
	for i := range refs {
		refs[i] = rand.Intn(10)
	}
	return refs
}

// FIFO 頁面置換
func fifo(pages []int, frames int) int {
	memory := make([]int, 0, frames)
	faults := 0

	for _, page := range pages {
		hit := false
		for _, p := range memory {
			if p == page {
				hit = true
				break
			}
		}
		if !hit {
			faults++
			if len(memory) < frames {
				memory = append(memory, page)
			} else {
				// 移除最先進來的（index 0），加到最後
				memory = append(memory[1:], page)
			}
		}
	}
	return faults
}

// LRU 頁面置換
func lru(pages []int, frames int) int {
	memory := make([]int, 0, frames)
	faults := 0

	for _, page := range pages {
		hit := false
		hitIdx := -1
		for i, p := range memory {
			if p == page {
				hit = true
				hitIdx = i
				break
			}
		}
		if hit {
			// 移到最尾端（最近使用）
			memory = append(memory[:hitIdx], memory[hitIdx+1:]...)
			memory = append(memory, page)
		} else {
			faults++
			if len(memory) < frames {
				memory = append(memory, page)
			} else {
				// 移除最久未使用（index 0）
				memory = append(memory[1:], page)
			}
		}
	}
	return faults
}

// OPT 最佳頁面置換
func opt(pages []int, frames int) int {
	memory := make([]int, 0, frames)
	faults := 0

	for i, page := range pages {
		hit := false
		for _, p := range memory {
			if p == page {
				hit = true
				break
			}
		}
		if !hit {
			faults++
			if len(memory) < frames {
				memory = append(memory, page)
			} else {
				// 找出 memory 中未來最晚會被使用的 page
				evictIdx := 0
				farthest := -1
				for j, p := range memory {
					nextUse := len(pages) // 假設不會再用到
					for k := i + 1; k < len(pages); k++ {
						if pages[k] == p {
							nextUse = k
							break
						}
					}
					if nextUse > farthest {
						farthest = nextUse
						evictIdx = j
					}
				}
				memory[evictIdx] = page
			}
		}
	}
	return faults
}

func main() {
	if len(os.Args) < 2 {
		fmt.Println("用法: go run page_replacement.go <frame 數量>")
		fmt.Println("範例: go run page_replacement.go 3")
		os.Exit(1)
	}

	frames, err := strconv.Atoi(os.Args[1])
	if err != nil || frames <= 0 {
		fmt.Println("錯誤：frame 數量必須是正整數")
		os.Exit(1)
	}

	refs := generatePageRefString()

	fmt.Printf("Page Reference String (%d pages): ", PAGE_REF_LEN)
	for i, p := range refs {
		if i > 0 {
			fmt.Print(", ")
		}
		fmt.Print(p)
	}
	fmt.Println()
	fmt.Printf("Number of Frames: %d\n", frames)
	fmt.Println("----------------------------------------")
	fmt.Printf("FIFO  Page Faults: %d\n", fifo(refs, frames))
	fmt.Printf("LRU   Page Faults: %d\n", lru(refs, frames))
	fmt.Printf("OPT   Page Faults: %d\n", opt(refs, frames))
}
