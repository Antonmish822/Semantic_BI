#!/usr/bin/env python3
"""Surface smoke for cccr_py against examples/data/base."""

from __future__ import annotations

import pathlib
import sys

import cccr_py as cccr


def main() -> int:
    data = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else ".")
    schema_text = (data / "schema.json").read_text(encoding="utf-8")
    xml = (data / "input.xml").read_text(encoding="utf-8")
    seed = (data / "seed.nt").read_text(encoding="utf-8")

    assert cccr.__version__, "missing __version__"

    ctx = cccr.Context(arena_size=2 * 1024 * 1024)
    schema = ctx.load_schema(schema_text, include_base=str(data))
    assert schema is not None, ctx.last_error
    source = ctx.load_xml(xml)
    assert source is not None, ctx.last_error
    knowledge = ctx.load_graph(seed, format="ntriples")
    assert knowledge is not None, ctx.last_error
    graph = ctx.run(schema, source, knowledge=knowledge)
    assert graph is not None, ctx.last_error

    rdfxml = graph.to_rdfxml()
    assert rdfxml is not None, ctx.last_error
    assert "rdf:RDF" in rdfxml or "RDF" in rdfxml, "missing RDF root"
    assert "lipers24" in rdfxml, "missing expected subject IRI fragment"

    json_out = knowledge.query(
        "SELECT ?o WHERE { <https://lipers24.ru/SPO> "
        "<https://lipers24.ru#shortName> ?o }",
        result_format="json",
    )
    assert json_out is not None, ctx.last_error
    assert json_out, "empty SPARQL result"
    assert "bindings" in json_out or "lipers24" in json_out

    source_b = ctx.load_xml(xml)
    assert source_b is not None, ctx.last_error
    merged_run = ctx.run_many(schema, [source, source_b], knowledge=knowledge)
    assert merged_run is not None, ctx.last_error
    merged_xml = merged_run.to_rdfxml()
    assert merged_xml is not None, ctx.last_error
    assert "lipers24" in merged_xml

    seed_b = ctx.load_graph(seed, format="ntriples")
    assert seed_b is not None, ctx.last_error
    assert knowledge.merge(seed_b), ctx.last_error

    print("cccr_py smoke ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
