# libshv fuzz testing
libshv supports some limited fuzz testing.

## Building
Fuzz tests are built by default, when building with Clang.

**Important:** *Make sure to set the `WITH_SANITIZER` option to `ON`. Fuzzing doesn't do anything if sanitizers aren't enabled.*

Alternatively, you can supply `-fsanitize=address,undefined` or `-fsanitize=memory` to the build flags manually. For example:
```bash
CXXFLAGS=-fsanitize=address,undefined LDFLAGS=-fsanitize=address,undefined cmake ..
```

## Running
To run the fuzzer, simply run any of the built fuzz targets.
