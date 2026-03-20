// Chap.2 - 2.24*
// Build:
//   go build -o FileCopy ./
//
// Run:
//   ./FileCopy
//
// Trace syscalls on Linux:
//   strace ./FileCopy
package main

import (
	"bufio"
	"errors"
	"fmt"
	"io"
	"os"
	"path/filepath"
	"strings"
	"syscall"
)

func fatalf(format string, args ...any) {
	fmt.Fprintf(os.Stderr, "Error: "+format+"\n", args...)
	os.Exit(1)
}

func promptLine(r *bufio.Reader, prompt string) string {
	fmt.Print(prompt)
	line, err := r.ReadString('\n')
	if err != nil {
		if errors.Is(err, io.EOF) {
			// EOF but maybe we got partial data
			line = strings.TrimRight(line, "\r\n")
			if line == "" {
				fatalf("unexpected end of input")
			}
			return line
		}
		fatalf("failed to read input: %v", err)
	}
	line = strings.TrimRight(line, "\r\n")
	if line == "" {
		fatalf("empty path is not allowed")
	}
	return line
}

func isSameFileByStat(a, b os.FileInfo) bool {
	ast, okA := a.Sys().(*syscall.Stat_t)
	bst, okB := b.Sys().(*syscall.Stat_t)
	if !okA || !okB || ast == nil || bst == nil {
		return false
	}
	return ast.Dev == bst.Dev && ast.Ino == bst.Ino
}

func writeAll(f *os.File, buf []byte) {
	for len(buf) > 0 {
		n, err := f.Write(buf)
		if err != nil {
			// If interrupted, retry.
			if errors.Is(err, syscall.EINTR) {
				continue
			}
			fatalf("write failed: %v", err)
		}
		if n == 0 {
			fatalf("write returned 0 (unexpected)")
		}
		buf = buf[n:]
	}
}

func main() {
	reader := bufio.NewReader(os.Stdin)

	srcPath := promptLine(reader, "Enter source file name: ")
	dstPath := promptLine(reader, "Enter destination file name: ")

	// Normalize for a friendlier same-path check. (Still do inode check later.)
	srcClean := filepath.Clean(srcPath)
	dstClean := filepath.Clean(dstPath)
	if srcClean == dstClean {
		fatalf("source and destination paths are identical")
	}

	// Stat source: must exist and be a regular file.
	srcInfo, err := os.Stat(srcPath)
	if err != nil {
		if os.IsNotExist(err) {
			fatalf("source file does not exist: %s", srcPath)
		}
		fatalf("stat(source) failed: %v", err)
	}
	if !srcInfo.Mode().IsRegular() {
		fatalf("source is not a regular file: %s", srcPath)
	}

	// Open source.
	s, err := os.Open(srcPath)
	if err != nil {
		fatalf("open(source) failed: %v", err)
	}
	defer func() {
		if err := s.Close(); err != nil {
			fatalf("close(source) failed: %v", err)
		}
	}()

	// Open destination (create/truncate).
	// Start with 0600 then chmod to match source perms after copy.
	d, err := os.OpenFile(dstPath, os.O_WRONLY|os.O_CREATE|os.O_TRUNC, 0o600)
	if err != nil {
		fatalf("open(destination) failed: %v", err)
	}
	defer func() {
		if err := d.Close(); err != nil {
			fatalf("close(destination) failed: %v", err)
		}
	}()

	// Extra safety: ensure dest isn't the same underlying file (hard link case).
	dstInfo, err := d.Stat()
	if err == nil && isSameFileByStat(srcInfo, dstInfo) {
		fatalf("source and destination refer to the same file (hard link)")
	}

	// Copy loop using explicit Read/Write.
	buf := make([]byte, 64*1024)
	for {
		nr, rerr := s.Read(buf)
		if nr > 0 {
			writeAll(d, buf[:nr])
		}
		if rerr != nil {
			if errors.Is(rerr, syscall.EINTR) {
				continue
			}
			if errors.Is(rerr, io.EOF) {
				break
			}
			fatalf("read failed: %v", rerr)
		}
	}

	// Flush destination to disk.
	if err := d.Sync(); err != nil {
		fatalf("fsync(destination) failed: %v", err)
	}

	// Match permissions to source (mode bits only).
	if err := os.Chmod(dstPath, srcInfo.Mode().Perm()); err != nil {
		fatalf("chmod(destination) failed: %v", err)
	}

	fmt.Printf("Copied '%s' -> '%s'\n", srcPath, dstPath)
}