# SHV RPC
* [Documentation](https://silicon-heaven.github.io/shv-doc/)
## Build

```sh
mkdir build
cd build
cmake ..
cmake --build .
```
with CLI examples:
```
cmake -DWITH_CLI_EXAMPLES ..
```
with GUI samples:
```
cmake -DWITH_GUI_EXAMPLES ..
```
## Samples
### Minimal SHV Broker
```
./minimalshvbroker --config $SRC_DIR/examples/cli/minimalshvbroker/config/shvbroker.conf
```
### Minimal SHV Client
to connect to minimalshvbroker
```
./minimalshvclient 
```
to connect to other shv broker
```
./minimalshvclient --host HOST:port
```
to debug RPC trafic
```
./minimalshvclient -v rpcmsg
```
to print help
```
./minimalshvclient --help
```
## Tools
### ShvSpy
https://github.com/silicon-heaven/shvspy

Gui tool to inspect shv tree and to call shv methods. Prtable binaries an be downloaded from CI [Actions](https://github.com/silicon-heaven/shvspy/actions).
