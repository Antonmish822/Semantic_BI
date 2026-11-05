#!/usr/bin/env bash
# Terminal demo for the cccr CLI using examples/data.
# Usage:
#   CCCR_CLI=/path/to/cccr_cli ./demo.sh
#   CCCR_ENGINE_ROOT=/path/to/cccr-engine ./demo.sh

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
EXAMPLES_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
ROOT="$(cd "${CCCR_ENGINE_ROOT:-${EXAMPLES_DIR}/..}" && pwd)"
DATA="${EXAMPLES_DIR}/data"
OUT="${TMPDIR:-/tmp}/cccr_cli_demo"
mkdir -p "${OUT}"

resolve_cli() {
  if [[ -n "${CCCR_CLI:-}" && -x "${CCCR_CLI}" ]]; then
    echo "${CCCR_CLI}"
    return
  fi
  if command -v cccr_cli >/dev/null 2>&1; then
    command -v cccr_cli
    return
  fi
  if command -v cccr >/dev/null 2>&1; then
    command -v cccr
    return
  fi
  local candidate
  for candidate in \
    "${ROOT}/bin/cli/debug-linux-x86_64/cccr_cli" \
    "${ROOT}/bin/cli/release-linux-x86_64/cccr_cli" \
    "${ROOT}/bin/cli/relwithdebinfo-linux-x86_64/cccr_cli"; do
    if [[ -x "${candidate}" ]]; then
      echo "${candidate}"
      return
    fi
  done
  echo "error: set CCCR_CLI to the cccr_cli binary" >&2
  exit 1
}

CLI="$(resolve_cli)"
echo "Using CLI: ${CLI}"
echo "Data dir:  ${DATA}"
echo "Output:    ${OUT}"
echo

echo "=== help / version ==="
"${CLI}" --version
"${CLI}" run --help | head -n 20
echo

echo "=== run: base (XML + seed) ==="
"${CLI}" run \
  -s "${DATA}/base/schema.json" \
  -i "${DATA}/base/input.xml" \
  --seed "${DATA}/base/seed.nt" \
  -o "${OUT}/base.rdf"
wc -c "${OUT}/base.rdf"
echo

echo "=== run: kug (CSV) ==="
"${CLI}" run \
  -s "${DATA}/kug/schema.json" \
  -i "${DATA}/kug/input.csv" \
  --input-format csv \
  -o "${OUT}/kug.rdf"
wc -c "${OUT}/kug.rdf"
echo

echo "=== run: schedule (XML) ==="
"${CLI}" run \
  -s "${DATA}/schedule/schema.json" \
  -i "${DATA}/schedule/input.xml" \
  -o "${OUT}/schedule.rdf"
wc -c "${OUT}/schedule.rdf"
echo

echo "=== query: N-Triples seed ==="
"${CLI}" query \
  -g "${DATA}/base/seed.nt" \
  -q 'SELECT ?s ?o WHERE { ?s <https://lipers24.ru#shortName> ?o }' \
  --format json \
  -o "${OUT}/seed_query.json"
head -c 240 "${OUT}/seed_query.json"
echo
echo

echo "=== query: RDF/XML from run (by extension) ==="
"${CLI}" query \
  -g "${OUT}/base.rdf" \
  -q 'SELECT ?s ?p ?o WHERE { ?s ?p ?o } LIMIT 3' \
  --format json \
  -o "${OUT}/base_query.json"
head -c 240 "${OUT}/base_query.json"
echo
echo

echo "=== query: turtle ==="
printf '%s\n' \
  '@prefix ex: <https://example.org#> .' \
  'ex:a ex:p "hello" .' \
  > "${OUT}/tiny.ttl"
"${CLI}" query \
  -g "${OUT}/tiny.ttl" \
  -q 'SELECT ?s ?p ?o WHERE { ?s ?p ?o }' \
  --format json
echo

echo "=== query: mixed nt + rdfxml ==="
"${CLI}" query \
  -g "${DATA}/base/seed.nt" "${OUT}/base.rdf" \
  -q 'SELECT ?s ?p ?o WHERE { ?s ?p ?o } LIMIT 5' \
  --format json
echo

echo "Demo finished. Artifacts in ${OUT}"
