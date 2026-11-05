# CCCR patches for raptor2 2.0.16

| Patch | Reason |
| --- | --- |
| `libxml2-2.11-checked.patch` (applied in-tree) | libxml2 ≥ 2.11 removed `xmlEntity.checked`; upstream master guards with `LIBXML_VERSION < 21100`. |

Source: https://github.com/dajobe/raptor (master `raptor_libxml.c`)
