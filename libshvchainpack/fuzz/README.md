# libshv fuzz testing
libshv supports some limited fuzz testing.

## Building
Fuzz tests are built by default, when building with Clang. Make sure to supply `-fsanitize=address,undefined` or
`-fsanitize=memory` to the build flags to catch errors. For example:
```bash
CXXFLAGS=-fsanitize=address,undefined LDFLAGS=-fsanitize=address,undefined cmake ..
```

## Running
To run the fuzzer, simply run any of the built fuzz targets.
