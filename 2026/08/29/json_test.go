//go:build !legacyjson

package jsonbench

import (
	jsonv1 "encoding/json"
	jsonv2 "encoding/json/v2"
	"testing"
)

func init() {
	b, err := jsonv1.Marshal(records)
	if err != nil {
		panic(err)
	}
	setRecordsJSON(b)
}

func v2Marshal(v any) ([]byte, error) { return jsonv2.Marshal(v) }

func v2Unmarshal(data []byte, v any) error { return jsonv2.Unmarshal(data, v) }

var (
	codecV1 = codec{
		name:      "json",
		marshal:   jsonv1.Marshal,
		unmarshal: jsonv1.Unmarshal,
	}
	codecV2 = codec{
		name:      "jsonv2",
		marshal:   v2Marshal,
		unmarshal: v2Unmarshal,
	}
	codecs = []codec{codecV1, codecV2}
)

func TestRoundtrip(t *testing.T) {
	for _, c := range codecs {
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
			out, err := c.marshal(v)
			if err != nil {
				t.Fatalf("marshal: %v", err)
			}
			if len(out) == 0 {
				t.Fatal("empty output")
			}
		})
	}
}

func BenchmarkUnmarshalAny(b *testing.B) {
	for _, c := range codecs {
		for _, doc := range docs {
			b.Run(c.name+"/"+doc.name, func(b *testing.B) {
				benchUnmarshal(b, c, doc.data)
			})
		}
	}
}

func BenchmarkMarshalAny(b *testing.B) {
	for _, c := range codecs {
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
}

func BenchmarkUnmarshalRecords(b *testing.B) {
	for _, c := range codecs {
		b.Run(c.name, func(b *testing.B) {
			benchUnmarshalRecords(b, c)
		})
	}
}

func BenchmarkMarshalRecords(b *testing.B) {
	for _, c := range codecs {
		b.Run(c.name, func(b *testing.B) {
			benchMarshal(b, c, records)
		})
	}
}
