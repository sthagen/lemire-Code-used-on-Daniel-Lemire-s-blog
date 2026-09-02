package jsonbench

import (
	"os"
	"path/filepath"
	"strconv"
	"testing"
)

// Record is a typical JSON object as produced by a web API: a handful of
// scalars plus a small string slice.
type Record struct {
	ID     int      `json:"id"`
	Name   string   `json:"name"`
	Email  string   `json:"email"`
	Active bool     `json:"active"`
	Score  float64  `json:"score"`
	Tags   []string `json:"tags"`
}

const nRecords = 10_000

var (
	docs = []struct {
		name string
		data []byte
	}{
		{"twitter", mustRead("twitter.json")},
		{"canada", mustRead("canada.json")},
		{"citm_catalog", mustRead("citm_catalog.json")},
	}

	records     = makeRecords(nRecords)
	recordsJSON []byte // filled by each test file's init via setRecordsJSON
)

func mustRead(name string) []byte {
	b, err := os.ReadFile(filepath.Join("testdata", name))
	if err != nil {
		panic(err)
	}
	return b
}

func makeRecords(n int) []Record {
	tags := []string{"go", "json", "bench"}
	out := make([]Record, n)
	for i := range out {
		out[i] = Record{
			ID:     i,
			Name:   "user-" + strconv.Itoa(i),
			Email:  "user-" + strconv.Itoa(i) + "@example.com",
			Active: i%2 == 0,
			Score:  float64(i) * 0.5,
			Tags:   tags,
		}
	}
	return out
}

func setRecordsJSON(b []byte) { recordsJSON = b }

type codec struct {
	name      string
	marshal   func(any) ([]byte, error)
	unmarshal func([]byte, any) error
}

var sink any

func benchUnmarshal(b *testing.B, c codec, data []byte) {
	b.Helper()
	b.SetBytes(int64(len(data)))
	b.ReportAllocs()
	for b.Loop() {
		var v any
		if err := c.unmarshal(data, &v); err != nil {
			b.Fatal(err)
		}
		sink = v
	}
}

func benchMarshal(b *testing.B, c codec, v any) {
	b.Helper()
	out, err := c.marshal(v)
	if err != nil {
		b.Fatal(err)
	}
	b.SetBytes(int64(len(out)))
	b.ReportAllocs()
	for b.Loop() {
		out, err = c.marshal(v)
		if err != nil {
			b.Fatal(err)
		}
		sink = out
	}
}

func benchUnmarshalRecords(b *testing.B, c codec) {
	b.Helper()
	data := recordsJSON
	b.SetBytes(int64(len(data)))
	b.ReportAllocs()
	for b.Loop() {
		var v []Record
		if err := c.unmarshal(data, &v); err != nil {
			b.Fatal(err)
		}
		sink = v
	}
}
