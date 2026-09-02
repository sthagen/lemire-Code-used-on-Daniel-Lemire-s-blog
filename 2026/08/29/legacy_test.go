//go:build legacyjson

package jsonbench

import (
	"encoding/json"
	"testing"
)

func init() {
	b, err := json.Marshal(records)
	if err != nil {
		panic(err)
	}
	setRecordsJSON(b)
}

var codecLegacy = codec{
	name:      "legacy",
	marshal:   json.Marshal,
	unmarshal: json.Unmarshal,
}

func TestRoundtrip(t *testing.T) {
	c := codecLegacy
	for _, doc := range docs {
		t.Run(c.name+"/"+doc.name, func(t *testing.T) {
			var v any
			if err := c.unmarshal(doc.data, &v); err != nil {
				t.Fatalf("unmarshal: %v", err)
			}
			out, err := c.marshal(v)
			if err != nil {
				t.Fatalf("marshal: %v", err)
			}
			if len(out) == 0 {
				t.Fatal("empty output")
			}
		})
	}
	t.Run(c.name+"/records", func(t *testing.T) {
		var v []Record
		if err := c.unmarshal(recordsJSON, &v); err != nil {
			t.Fatalf("unmarshal: %v", err)
		}
		if len(v) != nRecords {
			t.Fatalf("got %d records, want %d", len(v), nRecords)
		}
	})
}

func BenchmarkUnmarshalAny(b *testing.B) {
	c := codecLegacy
	for _, doc := range docs {
		b.Run(c.name+"/"+doc.name, func(b *testing.B) {
			benchUnmarshal(b, c, doc.data)
		})
	}
}

func BenchmarkMarshalAny(b *testing.B) {
	c := codecLegacy
	for _, doc := range docs {
		var v any
		if err := c.unmarshal(doc.data, &v); err != nil {
			b.Fatal(err)
		}
		b.Run(c.name+"/"+doc.name, func(b *testing.B) {
			benchMarshal(b, c, v)
		})
	}
}

func BenchmarkUnmarshalRecords(b *testing.B) {
	b.Run(codecLegacy.name, func(b *testing.B) {
		benchUnmarshalRecords(b, codecLegacy)
	})
}

func BenchmarkMarshalRecords(b *testing.B) {
	b.Run(codecLegacy.name, func(b *testing.B) {
		benchMarshal(b, codecLegacy, records)
	})
}
