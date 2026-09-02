//go:build !legacyjson

package jsonbench

// Exploration of ways to speed up marshaling a []Record with encoding/json/v2,
// which is slower than the original encoding/json implementation on this shape.

import (
	"bytes"
	"encoding/json/jsontext"
	jsonv2 "encoding/json/v2"
	"strconv"
	"testing"
)

// FastRecord is Record with a hand-written v2 marshaler.
type FastRecord struct {
	ID     int      `json:"id"`
	Name   string   `json:"name"`
	Email  string   `json:"email"`
	Active bool     `json:"active"`
	Score  float64  `json:"score"`
	Tags   []string `json:"tags"`
}

func (r FastRecord) MarshalJSONTo(enc *jsontext.Encoder) error {
	b := enc.AvailableBuffer()
	var err error
	b = append(b, `{"id":`...)
	b = strconv.AppendInt(b, int64(r.ID), 10)
	b = append(b, `,"name":`...)
	if b, err = jsontext.AppendQuote(b, r.Name); err != nil {
		return err
	}
	b = append(b, `,"email":`...)
	if b, err = jsontext.AppendQuote(b, r.Email); err != nil {
		return err
	}
	b = append(b, `,"active":`...)
	b = strconv.AppendBool(b, r.Active)
	b = append(b, `,"score":`...)
	b = jsontext.AppendFloat(b, r.Score, 64)
	b = append(b, `,"tags":[`...)
	for i, t := range r.Tags {
		if i > 0 {
			b = append(b, ',')
		}
		if b, err = jsontext.AppendQuote(b, t); err != nil {
			return err
		}
	}
	b = append(b, `]}`...)
	return enc.WriteValue(b)
}

var fastRecords = func() []FastRecord {
	out := make([]FastRecord, len(records))
	for i, r := range records {
		out[i] = FastRecord(r)
	}
	return out
}()

// appendRecords is a hand-rolled encoder: the ceiling for this shape.
func appendRecords(dst []byte, rs []Record) ([]byte, error) {
	var err error
	dst = append(dst, '[')
	for i := range rs {
		if i > 0 {
			dst = append(dst, ',')
		}
		r := &rs[i]
		dst = append(dst, `{"id":`...)
		dst = strconv.AppendInt(dst, int64(r.ID), 10)
		dst = append(dst, `,"name":`...)
		if dst, err = jsontext.AppendQuote(dst, r.Name); err != nil {
			return dst, err
		}
		dst = append(dst, `,"email":`...)
		if dst, err = jsontext.AppendQuote(dst, r.Email); err != nil {
			return dst, err
		}
		dst = append(dst, `,"active":`...)
		dst = strconv.AppendBool(dst, r.Active)
		dst = append(dst, `,"score":`...)
		dst = jsontext.AppendFloat(dst, r.Score, 64)
		dst = append(dst, `,"tags":[`...)
		for j, t := range r.Tags {
			if j > 0 {
				dst = append(dst, ',')
			}
			if dst, err = jsontext.AppendQuote(dst, t); err != nil {
				return dst, err
			}
		}
		dst = append(dst, `]}`...)
	}
	return append(dst, ']'), nil
}


// RecordSlice marshals the whole array in one WriteValue call, so the encoder's
// per-token bookkeeping is paid once instead of per field.
type RecordSlice []Record

func (rs RecordSlice) MarshalJSONTo(enc *jsontext.Encoder) error {
	b, err := appendRecords(enc.AvailableBuffer(), rs)
	if err != nil {
		return err
	}
	return enc.WriteValue(b)
}

var (
	optUTF8 = jsontext.AllowInvalidUTF8(true)
	optDup  = jsontext.AllowDuplicateNames(true)
	optRaw  = jsontext.PreserveRawStrings(true)
)

// marshalers maps a variant name to a func returning the encoded bytes.
// Each closure owns whatever state it wants to reuse across iterations.
func marshalVariants() []struct {
	name string
	fn   func() ([]byte, error)
} {
	type variant = struct {
		name string
		fn   func() ([]byte, error)
	}
	var vs []variant

	vs = append(vs, variant{"v2", func() ([]byte, error) {
		return jsonv2.Marshal(records)
	}})
	vs = append(vs, variant{"v2+utf8", func() ([]byte, error) {
		return jsonv2.Marshal(records, optUTF8)
	}})
	vs = append(vs, variant{"v2+dup", func() ([]byte, error) {
		return jsonv2.Marshal(records, optDup)
	}})
	vs = append(vs, variant{"v2+utf8+dup", func() ([]byte, error) {
		return jsonv2.Marshal(records, optUTF8, optDup)
	}})

	{
		var buf bytes.Buffer
		vs = append(vs, variant{"v2-MarshalWrite-reusebuf", func() ([]byte, error) {
			buf.Reset()
			err := jsonv2.MarshalWrite(&buf, records)
			return buf.Bytes(), err
		}})
	}
	{
		var buf bytes.Buffer
		enc := jsontext.NewEncoder(&buf)
		vs = append(vs, variant{"v2-MarshalEncode-reuseenc", func() ([]byte, error) {
			buf.Reset()
			enc.Reset(&buf)
			err := jsonv2.MarshalEncode(enc, records)
			return buf.Bytes(), err
		}})
	}
	{
		var buf bytes.Buffer
		enc := jsontext.NewEncoder(&buf, optUTF8, optDup)
		vs = append(vs, variant{"v2-MarshalEncode-reuseenc+opts", func() ([]byte, error) {
			buf.Reset()
			enc.Reset(&buf, optUTF8, optDup)
			err := jsonv2.MarshalEncode(enc, records)
			return buf.Bytes(), err
		}})
	}

	vs = append(vs, variant{"v2-MarshalJSONTo", func() ([]byte, error) {
		return jsonv2.Marshal(fastRecords)
	}})
	{
		var buf bytes.Buffer
		enc := jsontext.NewEncoder(&buf)
		vs = append(vs, variant{"v2-MarshalJSONTo+reuseenc", func() ([]byte, error) {
			buf.Reset()
			enc.Reset(&buf)
			err := jsonv2.MarshalEncode(enc, fastRecords)
			return buf.Bytes(), err
		}})
	}
	{
		var buf []byte
		{
		var buf bytes.Buffer
		enc := jsontext.NewEncoder(&buf)
		rs := RecordSlice(records)
		vs = append(vs, variant{"v2-sliceMarshalJSONTo+reuseenc", func() ([]byte, error) {
			buf.Reset()
			enc.Reset(&buf)
			err := jsonv2.MarshalEncode(enc, rs)
			return buf.Bytes(), err
		}})
	}
	{
		var buf bytes.Buffer
		enc := jsontext.NewEncoder(&buf, optRaw)
		rs := RecordSlice(records)
		vs = append(vs, variant{"v2-sliceMarshalJSONTo+rawstrings", func() ([]byte, error) {
			buf.Reset()
			enc.Reset(&buf, optRaw)
			err := jsonv2.MarshalEncode(enc, rs)
			return buf.Bytes(), err
		}})
	}
	{
		var buf bytes.Buffer
		enc := jsontext.NewEncoder(&buf, optRaw)
		frs := fastRecords
		vs = append(vs, variant{"v2-MarshalJSONTo+rawstrings", func() ([]byte, error) {
			buf.Reset()
			enc.Reset(&buf, optRaw)
			err := jsonv2.MarshalEncode(enc, frs)
			return buf.Bytes(), err
		}})
	}
	vs = append(vs, variant{"handwritten-reusebuf", func() ([]byte, error) {
			var err error
			buf, err = appendRecords(buf[:0], records)
			return buf, err
		}})
	}
	vs = append(vs, variant{"handwritten", func() ([]byte, error) {
		return appendRecords(nil, records)
	}})
	return vs
}

func TestMarshalVariantsAgree(t *testing.T) {
	want := recordsJSON // produced by encoding/json v1 in json_test.go's init
	for _, v := range marshalVariants() {
		got, err := v.fn()
		if err != nil {
			t.Fatalf("%s: %v", v.name, err)
		}
		got = bytes.TrimSuffix(got, []byte("\n")) // jsontext.Encoder ends each top-level value with a newline
		if !bytes.Equal(got, want) {
			t.Errorf("%s: output differs from encoding/json (%d vs %d bytes)",
				v.name, len(got), len(want))
		}
	}
}

func BenchmarkMarshalRecordsTuned(b *testing.B) {
	for _, v := range marshalVariants() {
		b.Run(v.name, func(b *testing.B) {
			out, err := v.fn()
			if err != nil {
				b.Fatal(err)
			}
			b.SetBytes(int64(len(out)))
			b.ReportAllocs()
			for b.Loop() {
				out, err := v.fn()
				if err != nil {
					b.Fatal(err)
				}
				sink = out
			}
		})
	}
}

// --- unmarshal side ---------------------------------------------------------

func unmarshalVariants() []struct {
	name string
	fn   func() ([]Record, error)
} {
	type variant = struct {
		name string
		fn   func() ([]Record, error)
	}
	var vs []variant

	vs = append(vs, variant{"v2", func() ([]Record, error) {
		var v []Record
		return v, jsonv2.Unmarshal(recordsJSON, &v)
	}})
	{
		var v []Record // kept across calls: v2 reuses the backing array
		vs = append(vs, variant{"v2-reuseslice", func() ([]Record, error) {
			return v, jsonv2.Unmarshal(recordsJSON, &v)
		}})
	}
	{
		var v []Record
		dec := jsontext.NewDecoder(bytes.NewReader(nil))
		vs = append(vs, variant{"v2-reuseslice+reusedec", func() ([]Record, error) {
			dec.Reset(bytes.NewReader(recordsJSON))
			return v, jsonv2.UnmarshalDecode(dec, &v)
		}})
	}
	return vs
}

func TestUnmarshalVariantsAgree(t *testing.T) {
	for _, v := range unmarshalVariants() {
		got, err := v.fn()
		if err != nil {
			t.Fatalf("%s: %v", v.name, err)
		}
		if len(got) != nRecords {
			t.Fatalf("%s: got %d records", v.name, len(got))
		}
		out, err := jsonv2.Marshal(got)
		if err != nil {
			t.Fatal(err)
		}
		if !bytes.Equal(out, recordsJSON) {
			t.Errorf("%s: roundtrip differs", v.name)
		}
	}
}

func BenchmarkUnmarshalRecordsTuned(b *testing.B) {
	for _, v := range unmarshalVariants() {
		b.Run(v.name, func(b *testing.B) {
			b.SetBytes(int64(len(recordsJSON)))
			b.ReportAllocs()
			for b.Loop() {
				out, err := v.fn()
				if err != nil {
					b.Fatal(err)
				}
				sink = out
			}
		})
	}
}
