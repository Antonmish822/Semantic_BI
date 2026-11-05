# CCCR CLI — terminal examples

These recipes use the shared datasets in [`../data/`](../data/). They do **not** link `cccr::cli` as a library; they run the `cccr` executable.

Build the CLI (from the CMake build tree), then set `CCCR_CLI` to the binary if it is not on `PATH`:

```bash
# Debug (Ninja / linux-gcc-debug preset), adjust config/arch as needed:
export CCCR_CLI=/path/to/cccr-engine/bin/cli/debug-linux-x86_64/cccr_cli

# Or add that directory to PATH and use `cccr_cli` / rename to `cccr`.
```

Below, `$DATA` is `examples/data` relative to the `cccr-engine/` source root.

```bash
ROOT=/path/to/cccr-engine
DATA="$ROOT/examples/data"
CLI="${CCCR_CLI:-cccr_cli}"
```

Quick check:

```bash
"$CLI" --help
"$CLI" --version
"$CLI" run --help
"$CLI" query --help
```

---

## `run` — schema + input → RDF/XML

### Base (XML + N-Triples seed)

```bash
"$CLI" run \
  -s "$DATA/base/schema.json" \
  -i "$DATA/base/input.xml" \
  --seed "$DATA/base/seed.nt" \
  -o /tmp/base.rdf

# stdout instead of -o:
"$CLI" run \
  -s "$DATA/base/schema.json" \
  -i "$DATA/base/input.xml" \
  --seed "$DATA/base/seed.nt" | head
```

Force XML (same as auto for `.xml`):

```bash
"$CLI" run \
  -s "$DATA/base/schema.json" \
  -i "$DATA/base/input.xml" \
  --input-format xml \
  --seed "$DATA/base/seed.nt" \
  -o /tmp/base.rdf
```

Multiple inputs (merged graph) and multiple seeds:

```bash
"$CLI" run \
  -s "$DATA/base/schema.json" \
  -i "$DATA/base/input.xml" "$DATA/base/input.xml" \
  --seed "$DATA/base/seed.nt" "$DATA/base/seed.nt" \
  -o /tmp/base_multi.rdf
```

### KUG (CSV)

```bash
"$CLI" run \
  -s "$DATA/kug/schema.json" \
  -i "$DATA/kug/input.csv" \
  -o /tmp/kug.rdf

"$CLI" run \
  -s "$DATA/kug/schema.json" \
  -i "$DATA/kug/input.csv" \
  --input-format csv \
  -o /tmp/kug.rdf
```

### Schedule (XML, no seed)

```bash
"$CLI" run \
  -s "$DATA/schedule/schema.json" \
  -i "$DATA/schedule/input.xml" \
  -o /tmp/schedule.rdf
```

---

## `query` — SPARQL over RDF files

Graph syntax is inferred from the extension (`.nt`, `.ttl`, `.rdf` / `.owl`), or forced with `--graph-format {ntriples,turtle,rdfxml}`.

Result wire format: `--format {json,xml,html}` (default `json`).

### N-Triples seed

```bash
"$CLI" query \
  -g "$DATA/base/seed.nt" \
  -q 'SELECT ?s ?p ?o WHERE { ?s ?p ?o }' \
  --format json
```

Query text from a file (any existing path; schedule ships an `.rq`):

```bash
"$CLI" query \
  -g "$DATA/base/seed.nt" \
  -q "$DATA/schedule/список-учителей-расписание.rq" \
  --format json
```

### RDF/XML produced by `run`

```bash
"$CLI" run \
  -s "$DATA/base/schema.json" \
  -i "$DATA/base/input.xml" \
  --seed "$DATA/base/seed.nt" \
  -o /tmp/base.rdf

"$CLI" query \
  -g /tmp/base.rdf \
  -q 'SELECT ?s ?p ?o WHERE { ?s ?p ?o } LIMIT 5' \
  --format json

"$CLI" query \
  -g /tmp/base.rdf \
  --graph-format rdfxml \
  -q 'SELECT ?s ?p ?o WHERE { ?s ?p ?o } LIMIT 5' \
  -o /tmp/base_query.json
```

### Turtle

```bash
printf '%s\n' \
  '@prefix ex: <https://example.org#> .' \
  'ex:a ex:p "hello" .' \
  > /tmp/tiny.ttl

"$CLI" query -g /tmp/tiny.ttl -q 'SELECT ?s ?p ?o WHERE { ?s ?p ?o }'
```

### Mixed graphs (per-file inference)

```bash
"$CLI" query \
  -g "$DATA/base/seed.nt" /tmp/base.rdf \
  -q 'SELECT ?s ?p ?o WHERE { ?s ?p ?o } LIMIT 5' \
  --format json
```

---

## Runnable demo

From this directory (or pass `CCCR_CLI` / `CCCR_ENGINE_ROOT`):

```bash
./demo.sh
```

The script resolves `examples/data`, runs the recipes above, and writes under `/tmp/cccr_cli_demo/`.
