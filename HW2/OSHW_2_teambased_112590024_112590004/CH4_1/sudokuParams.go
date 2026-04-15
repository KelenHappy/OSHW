package main

import "fmt"

// 傳給每個 goroutine 的參數
type SudokuParams struct {
	grid      [9][9]int
	startRow  int
	startCol  int
	checkType string // "row", "col", "box"
}

// 有效的數獨
var validGrid = [9][9]int{
	{5, 3, 4, 6, 7, 8, 9, 1, 2},
	{6, 7, 2, 1, 9, 5, 3, 4, 8},
	{1, 9, 8, 3, 4, 2, 5, 6, 7},
	{8, 5, 9, 7, 6, 1, 4, 2, 3},
	{4, 2, 6, 8, 5, 3, 7, 9, 1},
	{7, 1, 3, 9, 2, 4, 8, 5, 6},
	{9, 6, 1, 5, 3, 7, 2, 8, 4},
	{2, 8, 7, 4, 1, 9, 6, 3, 5},
	{3, 4, 5, 2, 8, 6, 1, 7, 9},
}

// 無效的數獨（第一行有兩個 5）
var invalidGrid = [9][9]int{
	{5, 3, 4, 6, 7, 8, 9, 1, 5}, // 重複的 5
	{6, 7, 2, 1, 9, 5, 3, 4, 8},
	{1, 9, 8, 3, 4, 2, 5, 6, 7},
	{8, 5, 9, 7, 6, 1, 4, 2, 3},
	{4, 2, 6, 8, 5, 3, 7, 9, 1},
	{7, 1, 3, 9, 2, 4, 8, 5, 6},
	{9, 6, 1, 5, 3, 7, 2, 8, 4},
	{2, 8, 7, 4, 1, 9, 6, 3, 5},
	{3, 4, 5, 2, 8, 6, 1, 7, 9},
}

// 檢查某一行
func checkRow(params SudokuParams, ch chan bool) {
	seen := make(map[int]bool)
	for col := 0; col < 9; col++ {
		num := params.grid[params.startRow][col]
		if num < 1 || num > 9 || seen[num] {
			ch <- false
			return
		}
		seen[num] = true
	}
	ch <- true
}

// 檢查某一列
func checkCol(params SudokuParams, ch chan bool) {
	seen := make(map[int]bool)
	for row := 0; row < 9; row++ {
		num := params.grid[row][params.startCol]
		if num < 1 || num > 9 || seen[num] {
			ch <- false
			return
		}
		seen[num] = true
	}
	ch <- true
}

// 檢查 3x3 方格
func checkBox(params SudokuParams, ch chan bool) {
	seen := make(map[int]bool)
	for row := params.startRow; row < params.startRow+3; row++ {
		for col := params.startCol; col < params.startCol+3; col++ {
			num := params.grid[row][col]
			if num < 1 || num > 9 || seen[num] {
				ch <- false
				return
			}
			seen[num] = true
		}
	}
	ch <- true
}

func validate(grid [9][9]int) bool {
	ch := make(chan bool, 27) // 9行 + 9列 + 9方格 = 27個goroutine

	// 檢查 9 行
	for i := 0; i < 9; i++ {
		params := SudokuParams{grid: grid, startRow: i, checkType: "row"}
		go checkRow(params, ch)
	}

	// 檢查 9 列
	for i := 0; i < 9; i++ {
		params := SudokuParams{grid: grid, startCol: i, checkType: "col"}
		go checkCol(params, ch)
	}

	// 檢查 9 個 3x3 方格
	for row := 0; row < 9; row += 3 {
		for col := 0; col < 9; col += 3 {
			params := SudokuParams{grid: grid, startRow: row, startCol: col, checkType: "box"}
			go checkBox(params, ch)
		}
	}

	// 收集所有結果
	valid := true
	for i := 0; i < 27; i++ {
		if !<-ch {
			valid = false
		}
	}
	return valid
}

func main() {
	for _, grid := range []struct {
		name string
		grid [9][9]int
	}{
		{"Valid Grid", validGrid},
		{"Invalid Grid", invalidGrid},
	} {
		fmt.Println(grid.name)
		for row := 0; row < 9; row++ {
			fmt.Println(grid.grid[row])
		}
		if validate(grid.grid) {
			fmt.Println(grid.name + ": Sudoku solution is VALID")
		} else {
			fmt.Println(grid.name + ": Sudoku solution is INVALID")
		}
	}
}