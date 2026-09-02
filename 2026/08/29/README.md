# Go encoding/json vs encoding/json/v2

Go 1.27 ships `encoding/json/v2`. The original `encoding/json` package is
still there: it is now implemented on top of v2, and the pre-v2
implementation can be restored with `GOEXPERIMENT=nojsonv2`.

This directory compares three configurations on the same documents:

1. **legacy** — `encoding/json` built with `GOEXPERIMENT=nojsonv2`
2. **json** — `encoding/json` as of Go 1.27 (v1 API, v2 backend)
3. **jsonv2** — `encoding/json/v2`

```sh
./run.sh
```

Requires Go 1.27 (`GOTOOLCHAIN=go1.27.0` is set by `run.sh`). On Linux the
process is pinned to one core with `taskset`. Documents are the simdjson
corpus (`twitter.json`, `canada.json`, `citm_catalog.json`) plus a generated
array of 10,000 small structs.

Results in `results/` were collected on an Apple M4 Max and a dual Intel
Xeon Gold 6548N (big4). `plot.py` redraws the figures in the blog post:

```sh
python3 plot.py    # json-v1-v2.png, json-records.png
```
