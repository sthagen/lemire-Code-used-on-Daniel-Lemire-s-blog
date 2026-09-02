#!/usr/bin/env bash
# Compare encoding/json (legacy implementation), encoding/json (Go 1.27
# default, v2 backend), and encoding/json/v2.
#
# Usage:
#   ./run.sh                 # writes results/<host>-{v1v2,legacy,env}.txt
#   COUNT=10 BENCHTIME=2s ./run.sh
set -euo pipefail

cd "$(dirname "$0")"
# Resolve Go 1.27 and invoke that binary directly. Setting GOEXPERIMENT on
# an older host `go` (which then switches toolchains) can panic.
export GOTOOLCHAIN="${GOTOOLCHAIN:-go1.27.0}"
GO="${GO:-$(GOTOOLCHAIN=$GOTOOLCHAIN go env GOROOT)/bin/go}"

COUNT="${COUNT:-8}"
BENCHTIME="${BENCHTIME:-1s}"
CPU="${CPU:-0}"
OUTDIR="${OUTDIR:-results}"
HOST="${HOST:-$(hostname -s 2>/dev/null || hostname)}"

mkdir -p testdata "$OUTDIR"

if [[ ! -f testdata/twitter.json ]]; then
  echo "downloading simdjson corpus into testdata/"
  base="https://raw.githubusercontent.com/simdjson/simdjson/master/jsonexamples"
  for f in twitter.json canada.json citm_catalog.json; do
    curl -fsSL -o "testdata/$f" "$base/$f"
  done
fi

PIN=
if command -v taskset >/dev/null 2>&1; then
  PIN="taskset -c $CPU"
fi

{
  echo "date:        $(date -u +%Y-%m-%dT%H:%M:%SZ)"
  echo "host:        $HOST"
  echo "uname:       $(uname -a)"
  echo "go:          $($GO version)"
  if [[ "$(uname -s)" == Darwin ]]; then
    echo "cpu:         $(sysctl -n machdep.cpu.brand_string)"
    echo "ncpu:        $(sysctl -n hw.ncpu)"
  elif command -v lscpu >/dev/null 2>&1; then
    echo "cpu:         $(lscpu | awk -F: '/Model name/ {gsub(/^ +/, "", $2); print $2; exit}')"
    echo "ncpu:        $(nproc)"
  fi
  echo "GOTOOLCHAIN: $GOTOOLCHAIN"
  echo "GOMAXPROCS:  1"
  echo "count:       $COUNT"
  echo "benchtime:   $BENCHTIME"
  echo "pin:         ${PIN:-(none)}"
} | tee "$OUTDIR/${HOST}-env.txt"

echo
echo "=== json (v1 API) vs jsonv2 (v2 API), Go 1.27 default backend ==="
$PIN env GOMAXPROCS=1 "$GO" test -count=1 .
$PIN env GOMAXPROCS=1 "$GO" test -bench=. -benchmem -count="$COUNT" -benchtime="$BENCHTIME" -timeout=30m | tee "$OUTDIR/${HOST}-v1v2.txt"

echo
echo "=== encoding/json with GOEXPERIMENT=nojsonv2 (original implementation) ==="
$PIN env GOMAXPROCS=1 GOEXPERIMENT=nojsonv2 "$GO" test -tags=legacyjson -count=1 .
$PIN env GOMAXPROCS=1 GOEXPERIMENT=nojsonv2 "$GO" test -tags=legacyjson -bench=. -benchmem -count="$COUNT" -benchtime="$BENCHTIME" -timeout=30m | tee "$OUTDIR/${HOST}-legacy.txt"

echo
echo "wrote $OUTDIR/${HOST}-{env,v1v2,legacy}.txt"
