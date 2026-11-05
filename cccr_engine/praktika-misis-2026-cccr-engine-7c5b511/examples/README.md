# Examples

Shared datasets live under [`data/`](data/):

```text
examples/
  data/
    base/       # XML registry + seed (пример.md)
    kug/        # CSV academic calendar
    schedule/   # XML schedule
  base.cpp      # driver → cccr_example_base
  kug.cpp       # driver → cccr_example_kug
  schedule.cpp  # driver → cccr_example_schedule
  common/       # shared RunExample helper
  cli/          # terminal recipes for the `cccr` CLI
```

C++ example binaries still take an optional data-directory override as `argv[1]`; by default they use the baked `CCCR_EXAMPLE_DATA_DIR` pointing at `examples/data/<name>/`.

For **command-line** usage (not linking the library), see [`cli/README.md`](cli/README.md) and [`cli/demo.sh`](cli/demo.sh).
