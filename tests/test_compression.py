import pytest

import xrtf


simple_compressed = b"-\x00\x00\x00+\x00\x00\x00LZFu\xf1\xc5\xc7\xa7\x03\x00\n" \
                      b"\x00rcpg125B2\n\xf3 hel\t\x00 bw\x05\xb0ld}\n\x80\x0f\xa0"
simple_uncompressed = b"{\\rtf1\\ansi\\ansicpg1252\\pard hello world}\r\n"

crossing_compressed = b"\x1a\x00\x00\x00\x1c\x00\x00\x00LZFu\xe2\xd4KQA\x00" \
                      b"\x04 WXYZ\rn}\x01\x0e\xb0"
crossing_uncompressed = b"{\\rtf1 WXYZWXYZWXYZWXYZWXYZ}"

test_ids = ['simple', 'crossing']

test_compress_data = [
    (simple_uncompressed, simple_compressed),
    (crossing_uncompressed, crossing_compressed)
]

test_decompress_data = [
    (simple_compressed, simple_uncompressed),
    (crossing_compressed, crossing_uncompressed)
]


@pytest.mark.parametrize("test_input, expected", test_compress_data, ids=test_ids)
def test_compress(test_input, expected):
    assert xrtf.compress(test_input) == expected

@pytest.mark.parametrize("test_input, expected", test_decompress_data, ids=test_ids)
def test_decompress(test_input, expected):
    assert xrtf.decompress(test_input) == expected
