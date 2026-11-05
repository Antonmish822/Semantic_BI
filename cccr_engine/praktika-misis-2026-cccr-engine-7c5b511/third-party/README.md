# Third-party dependencies

Vendored source releases. CMake pulls them in via
[`cmake/Dependencies.cmake`](../cmake/Dependencies.cmake).

| Library  | Version | Used when                    | Upstream |
| -------- | ------- | ---------------------------- | -------- |
| glaze    | 7.9.1   | always (core)                | https://github.com/stephenberry/glaze/releases/tag/v7.9.1 |
| raptor2  | 2.0.16  | always (core, Redland)       | https://download.librdf.org/source/raptor2-2.0.16.tar.gz |
| rasqal   | 0.9.33  | always (core, Redland)       | https://download.librdf.org/source/rasqal-0.9.33.tar.gz |
| redland  | 1.0.17  | always (core, Redland/`librdf`) | https://download.librdf.org/source/redland-1.0.17.tar.gz |
| doctest  | 2.5.3   | `CCCR_BUILD_TESTS`           | https://github.com/doctest/doctest/releases/tag/v2.5.3 |
| argparse | 3.2     | `CCCR_BUILD_CLI`             | https://github.com/p-ranav/argparse/releases/tag/v3.2 |
| pybind11 | 3.0.4   | `CCCR_BUILD_BINDINGS_PYTHON` | https://github.com/pybind/pybind11/releases/tag/v3.0.4 |

**SaxonC-HE** is not vendored here; it is located or downloaded by
[`cmake/FindOrFetchSaxonCHE.cmake`](../cmake/FindOrFetchSaxonCHE.cmake)
into `${CCCR_CACHE_DIR}/saxonc-he` (default: `<project>/.cache/saxonc-he`).

## Persistent cache

SaxonC-HE downloads that should survive deleting `build/` go under
`${CCCR_CACHE_DIR}` (default `<project>/.cache`, gitignored). Override with
`-DCCCR_CACHE_DIR=...`.

## Redland (raptor2 / rasqal / librdf)

Resolved by [`cmake/BuildRedland.cmake`](../cmake/BuildRedland.cmake):

1. System packages via pkg-config (`raptor2`, `rasqal`, `redland`) when
   `CCCR_REDLAND_USE_SYSTEM=ON` (default)
2. Else ExternalProject from these trees → install prefix under the CMake build dir

Imported targets: `Redland::raptor2`, `Redland::rasqal`, `Redland::redland`,
`cccr::redland` (umbrella). `CCCR_REDLAND_VENDORED` is set when using (2).

**Platform:** Linux (and WSL). Windows is not supported for this stack.

**System packages (Debian/Ubuntu):** `libraptor2-dev`, `librasqal3-dev`,
`librdf0-dev`.

**Host packages for vendored build:** `libxml2-dev` (pkg-config `libxml-2.0`),
`pkg-config`, `make`, plus usual build tools. Recommended: `libpcre3-dev` for
rasqal. Optional: `libcurl` / ICU (picked up by raptor configure when present).

**Local patch:** `raptor2` includes an in-tree libxml2 ≥ 2.11 compatibility
fix for `xmlEntity.checked` (see `raptor2/cccr-patches/`).
