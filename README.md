# ChainPack RPC

## RpcValue
Very similar to JSON/MessagePack, but each value can have meta information(s) attached. Meta info is of form `int_key->RpcValue`.

There are more types than JSON offers, like TimeStamp or Blob

### PackedValue
```
+----------+------------+
| TypeInfo | PackedData |
+----------+------------+
```
* **TypeInfo** - uint8_t
* **PackedData** - binary blob of arbitrary length interpreted according to TypeInfo

#### TypeInfo
```
128 10000000 META_TYPE_ID
129 10000001 META_TYPE_NAMESPACE_ID
130 10000010 FALSE
131 10000011 TRUE
132 10000100 Null
133 10000101 UInt
134 10000110 Int
135 10000111 Double
136 10001000 Bool
137 10001001 Blob
138 10001010 String
139 10001011 DateTime
140 10001100 List
141 10001101 Map
142 10001110 IMap
143 10001111 MetaIMap
196 11000100 Null_Array
197 11000101 UInt_Array
198 11000110 Int_Array
199 11000111 Double_Array
200 11001000 Bool_Array
201 11001001 Blob_Array
202 11001010 String_Array
203 11001011 DateTime_Array
204 11001100 List_Array
205 11001101 Map_Array
206 11001110 IMap_Array
207 11001111 MetaIMap_Array
255 11111111 TERM
```
#### Arrays
When `ARRAY_FLAG` (bit 6) is set in TypeInfo, than an array of this type is packed. 
```
+----------+-------------+--------+-----+--------+
| TypeInfo | UInt length | data_1 | ... | data_n |
+----------+-------------+--------+-----+--------+
example: Int[] = {1,2,3}:  11000111|00000011|00000010|00000100|00000110
```

##### TERM
Special type. Terminates packed List and Map elements
##### META_TYPE_ID
Optimization type, packed as UInt. Used to save 1 byte when META_TYPE_ID is added to value 
##### META_TYPE_NAMESPACE_ID
Optimization type, packed as UInt. Used to save 1 byte when META_TYPE_ID is added to value 
##### FALSE
No packed data after *TypeInfo*. Optimization type, Used to save 1 byte when bool value is packed 
##### TRUE
No packed data after *TypeInfo*. Optimization type, Used to save 1 byte when bool value is packed 
##### Null
No packed data after *TypeInfo*. 
##### UInt
Values 0-63 are packed directly in TypeInfo to save one byte.

LSB is less significant byte
```
   0 ... 127              |0|x|x|x|x|x|x|x|<-- LSB
  128 ... 16383 (2^14-1)  |1|0|x|x|x|x|x|x| |x|x|x|x|x|x|x|x|<-- LSB
2^14 ... 2097151 (2^21-1) |1|1|0|x|x|x|x|x| |x|x|x|x|x|x|x|x| |x|x|x|x|x|x|x|x|<-- LSB
                          |1|1|1|0|x|x|x|x| |x|x|x|x|x|x|x|x| |x|x|x|x|x|x|x|x| |x|x|x|x|x|x|x|x|<-- LSB
                          |1|1|1|1|n|n|n|n| |x|x|x|x|x|x|x|x| |x|x|x|x|x|x|x|x| |x|x|x|x|x|x|x|x| ... <-- LSB
                          n ==  0 -> 4 bytes (32 bit number)
                          n ==  1 -> 5 bytes
                          n == 13 -> 18 bytes
                          n == 14 -> for future (number of bytes will be specified in next byte)
                          n == 15 -> not used, to not be mixed with TERM
```
example:
```
UInt(3):     00000011
UInt(81):    10000111|01010001
UInt(59049): 10000111|11000000|11100110|10101001
```
##### Int
Values 0-63 are packed directly in TypeInfo like `64+n` to save one byte.

LSB is less significant byte
s is sign bit
```
   0 ... 63              |s|0|x|x|x|x|x|x|<-- LSB
  64 ... 2^13-1          |s|1|0|x|x|x|x|x| |x|x|x|x|x|x|x|x|<-- LSB
2^13 ... 2^20-1          |s|1|1|0|x|x|x|x| |x|x|x|x|x|x|x|x| |x|x|x|x|x|x|x|x|<-- LSB
                         |s|1|1|1|n|n|n|n| |x|x|x|x|x|x|x|x| |x|x|x|x|x|x|x|x| |x|x|x|x|x|x|x|x|<-- LSB
                          n ==  0 -> 3 bytes
                          n ==  1 -> 4 bytes
                          n == 13 -> 17 bytes
                          n == 14 -> for future (number of bytes will be specified in next byte)
                          n == 15 -> not used, to not be mixed with TERM
```
example:
```
Int(3):    01000011
Int(81):  10001000|01000000|01010001
Int(-81): 10001000|11000000|01010001
```
##### Double
64 bit blob
example:
```
0.0:       10001000|00000000|00000000|00000000|00000000|00000000|00000000|00000000|00000000
-40000.0:  10001000|00000000|00000000|00000000|00000000|00000000|10001000|11100011|11000000
```
##### Bool
one byte, 0 - false, 1 - true
##### Blob
```
+-------------+---------+
| UInt length | data    |
+-------------+---------+
example:
Blob("fpowf\u0000sapofkpsaokfsa"):  10001010|00010100|01100110|01110000|01101111|01110111|01100110|00000000|01110011|01100001|01110000|01101111|01100110|01101011|01110000|01110011|01100001|01101111|01101011|01100110|01110011|01100001
```
##### String
like Blob
##### DateTime
like Int, msecs since epoch
example:
```
2017-05-03T15:52:31.123:  10001100|11010110|11111001|11101001|10101011|10110110|00100110
```
##### List
```
+------------+-----+------------+------+
| RpcValue_1 | ... | RpcValue_n | TERM |
+------------+-----+------------+------+
example:
["a",123,true,[1,2,3],null]
10001100|10001010|00000001|01100001|10000110|01000000|01111011|10000011|10001100|01000001|01000010|01000011|11111111|10000100|11111111
```
##### Map
```
+------------+------------+-----+------------+------------+------+
| Blob key_1 | RpcValue_1 | ... | Blob key_n | RpcValue_n | TERM |
+------------+------------+-----+------------+------------+------+
example:
{"bar":2,"baz":3,"foo":1}
10001101|00000011|01100010|01100001|01110010|01000010|00000011|01100010|01100001|01111010|01000011|00000011|01100110|01101111|01101111|01000001|11111111
{"bar":2,"baz":3,"foo":[11,12,13]}
10001101|00000011|01100010|01100001|01110010|01000010|00000011|01100010|01100001|01111010|01000011|00000011|01100110|01101111|01101111|10001100|01001011|01001100|01001101|11111111|11111111
```
##### IMap
```
+------------+------------+-----+------------+------------+------+
| UInt key_1 | RpcValue_1 | ... | UInt key_n | RpcValue_n | TERM |
+------------+------------+-----+------------+------------+------+
example:
{1:"foo",2:"bar",333:15}
10001110|00000001|10001010|00000011|01100110|01101111|01101111|00000010|10001010|00000011|01100010|01100001|01110010|10000001|01001101|01001111|11111111
```
##### MetaIMap
like IMap, but used for meta data storage

### RpcValue with MetaData
MetaData are prepend before packed RpcValue in form of types `META_TYPE_ID`, `META_TYPE_NAMESPACE_ID` or `MetaIMap`

example:
```
<1:2,2:1,8:"foo",9:[1,2,3]>[17,18,19]
10000000|00000010|10000001|00000001|10001111|00000001|00000010|00000010|01000001|00001000|10001010|00000011|01100110|01101111|01101111|00001001|10001100|01000001|01000010|01000011|11111111|11111111|10001100|01010001|01010010|01010011|11111111
```
## RPC
ChainPack RPC is relaxed form of JSON RPC, for example `jsonrpc` key is not required

Each RPC message is coded as `IMap` with `MetaTypeNameSpaceId == 0 (Global)` and `MetaTypeId == 1 (ChainPackRpcMessage)`. Meta types are defined in `metatypes.h`

Defined ChainPackRpc message keys (`rpcmessage.h`):
```
struct Key {
	enum Enum {
		Id = 1,
		Method,
		Params,
		Result,
		Error,
		ErrorCode,
		ErrorMessage,
		MAX_KEY
	};
};
```
example:
```
RpcRequest Id=123, method="foo", params = {"a": 45, "b":  "bar", "c": [1,2,3]}
<1:1u>{1:123u,2:"foo",3:{"a":45,"b":"bar","c":[1,2,3]}}:  10000001|00000001|10001111|00000011|00000001|10000110|01111011|00000010|10001011|00000011|01100110|01101111|01101111|00000011|10001110|00000011|00000001|01100001|01101101|00000001|01100010|10001011|00000011|01100010|01100001|01110010|00000001|01100011|10001101|00000011|01000001|01000010|01000011

RpcResponse Id=123, result=42
<1:1u>{1:123u,4:42u}:  10000001|00000001|10001111|00000010|00000001|10000110|01111011|00000100|00101010

```

#### Examples
JSON `{"compact":true,"schema":0}` 27 bytes

MessagePack 18 bytes

ChainPack `|Map|2|7|'c'|'o'|'m'|'p'|'a'|'c'|'t'|True|6|'s'|'c'|'h'|'e'|'m'|'a'|TinyUInt(0)|` 19 bytes

