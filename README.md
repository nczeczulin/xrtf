# xrtf

### [MS-OXRTFCP]: Rich Text Format (RTF) Compression Algorithm

Implemented in C for speed.

```
>>> import xrtf
>>>
>>> data = b'{\\rtf1 WXYZWXYZWXYZWXYZWXYZ}'
>>>
>>> t = xrtf.compress(b'{\\rtf1 WXYZWXYZWXYZWXYZWXYZ}')
>>> t
b'\x1a\x00\x00\x00\x1c\x00\x00\x00LZFu\xe2\xd4KQA\x00\x04 WXYZ\rn}\x01\x0e\xb0'
>>>
>>> xrtf.parse_header(t)
xrtf.CompressedRtfHeader(comp_size=26, raw_size=28, comp_type=1967544908, crc=1363924194)
>>>
>>> xrtf.decompress(t)
b'{\\rtf1 WXYZWXYZWXYZWXYZWXYZ}'
>>>
>>>
>>> t = xrtf.compress(b'{\\rtf1 WXYZWXYZWXYZWXYZWXYZ}', comp_type=xrtf.UNCOMPRESSED)
>>> t
b'(\x00\x00\x00\x1c\x00\x00\x00MELA\x00\x00\x00\x00{\\rtf1 WXYZWXYZWXYZWXYZWXYZ}'
>>>
>>> xrtf.parse_header(t)
xrtf.CompressedRtfHeader(comp_size=40, raw_size=28, comp_type=1095517517, crc=0)
>>>
>>> xrtf.decompress(t)
b'{\\rtf1 WXYZWXYZWXYZWXYZWXYZ}'
```