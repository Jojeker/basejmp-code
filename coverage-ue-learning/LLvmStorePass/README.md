# LLVM Global Var Counter Pass

This is an LLVM pass that instruments a module to print its accessed global variables.
It works as follows:

- For every global variable `GVAR`, two new global ones are created: `GVAR_RD` and `GVAR_WR`
- For all reading and writing accesses to these variables, the corresponding `GVAR_{RD,WR}` is
incremented
- In order to dump the global variable accesses at runtime, a function `DUMP_GLOB` is defined that
prints the access counts in the format `GVAR=[GVAR_RD:GVAR_WR]`
- Finally, as part of this `DUMP_GLOB` the counter is reset.

## How to build

Use the `Makefile` and `llvm-18` (latest atm)

```bash
make 
```
## How to use

In order to use it (at least with one `.cpp` file) you can do as follows:

```Makefile
	clang++ -emit-llvm -c target.cpp -o target.ll 
	opt -load-pass-plugin=./global_var_counters.so --passes="global_var_counters" -o instrumented.bc target.ll
	clang++ instrumented.bc -o instrumented_target $(LDFLAGS)
```

This should allow calling `DUMP_GLOB()` when defined as a `extern "C" void DUMP_GLOB()` in the
module.
