#include <Python.h>
#include <stdbool.h>
#include <stdint.h>

extern PyObject *xrtf_Error;

#define COMPRESSED 0x75465a4c
#define UNCOMPRESSED 0x414c454d

typedef struct HDR {
    uint32_t comp_size;
    uint32_t raw_size;
    uint32_t comp_type;
    uint32_t crc;
} HDR;

#define HEADER_LENGTH 16

#define DICT_INIT "{\\rtf1\\ansi\\mac\\deff0\\deftab720{\\fonttbl;}" \
                  "{\\f0\\fnil \\froman \\fswiss \\fmodern \\fscript " \
                  "\\fdecor MS Sans SerifSymbolArialTimes New RomanCourier" \
                  "{\\colortbl\\red0\\green0\\blue0\r\n\\par \\pard\\plain\\" \
                  "f0\\fs20\\b\\i\\u\\tab\\tx"

static PyTypeObject CompressedRtfHeader_Type;

static PyStructSequence_Field CompressedRtfHeader_fields[] = {
    {"comp_size", "Compressed size of the data + 12"},
    {"raw_size", "Uncompressed size of the data"},
    {"comp_type", "Type of compression"},
    {"crc", "Computed crc for the compressed content"},
    {NULL}
};

static PyStructSequence_Desc CompressedRtfHeader_desc = {
    "xrtf.CompressedRtfHeader", NULL, CompressedRtfHeader_fields, 4
};

static const uint32_t crc_table[] = {
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
    0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
    0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
    0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172, 0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
    0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
    0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924, 0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
    0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
    0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
    0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
    0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
    0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
    0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
    0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
    0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a, 0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
    0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
    0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
    0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
    0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236, 0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
    0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
    0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
    0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
    0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
    0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
    0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
    0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94, 0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

static uint32_t uint32_from_bytes_le(unsigned char *buf)
{
    return (uint32_t)buf[0] | (uint32_t)buf[1] << 8 | (uint32_t)buf[2] << 16 | (uint32_t)buf[3] << 24;
}

static void uint32_to_bytes_le(uint32_t n, unsigned char *buf)
{
    buf[0] = n & 0xff;
    buf[1] = (n >> 8) & 0xff;
    buf[2] = (n >> 16) & 0xff;
    buf[3] = (n >> 24) & 0xff;
}

static uint32_t update_crc(uint32_t crc, unsigned char c)
{
    return crc_table[(crc ^ c) & 0xff] ^ (crc >> 8);
}

static uint32_t calculate_crc(unsigned char *buf, ssize_t len)
{
    uint32_t t = 0;
    for (ssize_t i = 0; i < len; i++)
        t = update_crc(t, buf[i]);
    return t;
}

static bool parse_header(unsigned char *buf, ssize_t len, HDR *hdr)
{
    if (len < HEADER_LENGTH) {
        PyErr_SetString(xrtf_Error, "not enough data");
        return false;
    }
    hdr->comp_size = uint32_from_bytes_le(buf);
    hdr->raw_size = uint32_from_bytes_le(buf + 4);
    hdr->comp_type = uint32_from_bytes_le(buf + 8);
    switch (hdr->comp_type) {
    case COMPRESSED:
    case UNCOMPRESSED:
        break;
    default:
        PyErr_SetString(xrtf_Error, "invalid compression type");
        return false;
    }
    hdr->crc = uint32_from_bytes_le(buf + 12);

    return true;
}

static void set_header(
    unsigned char *buf, uint32_t comp_size, uint32_t raw_size, uint32_t comp_type, uint32_t crc)
{
    uint32_to_bytes_le(comp_size, buf);
    uint32_to_bytes_le(raw_size, buf + 4);
    uint32_to_bytes_le(comp_type, buf + 8);
    uint32_to_bytes_le(crc, buf + 12);
}

typedef struct CSTATE {
    unsigned char *src;
    ssize_t src_left;

    unsigned char dict[4096];
    int dict_write_offset;
    int dict_end_offset;

    int best_match_offset;
    int best_match_len;
} CSTATE;

static void add_byte_to_dict(CSTATE *state, unsigned char c) {

    state->dict[state->dict_write_offset] = c;
    if (state->dict_end_offset < 4096)
        state->dict_end_offset++;
    state->dict_write_offset = (state->dict_write_offset + 1) & 0xfff;
}

static void try_match(CSTATE *state, int match_offset)
{
    int max_len = (int)Py_MIN(17, state->src_left);
    int dict_offset = match_offset;
    int match_len = 0;
    int input_offset = 0;

    while (state->dict[dict_offset] == state->src[input_offset] && match_len < max_len) {
        match_len++;
        if (match_len > state->best_match_len)
            add_byte_to_dict(state, state->src[input_offset]);

        input_offset++;
        dict_offset = (dict_offset + 1) & 0xfff;
    }

    if (match_len > state->best_match_len) {
        state->best_match_offset = match_offset;
        state->best_match_len = match_len;
    }
}

static int find_longest_match(CSTATE *state)
{
    int final_offset = state->dict_write_offset & 0xfff;
    int match_offset = 0;

    if (state->dict_end_offset == 4096)
        match_offset = (state->dict_write_offset + 1) & 0xfff;
    state->best_match_len = 0;

    do {
        try_match(state, match_offset);
        match_offset = (match_offset + 1) & 0xfff;

    } while (match_offset != final_offset && state->best_match_len < 17);
    if (state->best_match_len == 0)
        add_byte_to_dict(state, *state->src);

    return state->best_match_len;
}

PyDoc_STRVAR(xrtf_compress_doc,
    "compress(data, comp_type=COMPRESSED)\n"
    "\n"
    "Compress bytes data.");

PyObject* xrtf_compress(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *keywords[] = { "", "comp_type", NULL };
    Py_buffer buffer = { NULL, NULL };
    uint32_t comp_type = COMPRESSED;

    unsigned char *dst;
    ssize_t dst_len;
    ssize_t dst_offset = HEADER_LENGTH;
    unsigned char token_buf[16];
    int token_offset;
    uint32_t control;
    uint32_t control_bit;
    uint32_t crc = 0;

    CSTATE state = {
        .dict = DICT_INIT,
        .dict_write_offset = sizeof(DICT_INIT) - 1,
        .dict_end_offset = sizeof(DICT_INIT) - 1
    };

    PyObject *result = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "y*|k:compress", keywords, &buffer, &comp_type))
        return NULL;

    switch (comp_type) {
    case COMPRESSED:
    case UNCOMPRESSED:
        break;
    default:
        PyErr_SetString(PyExc_ValueError, "invalid compression type");
        goto exit;
    }
    if (comp_type == UNCOMPRESSED) {
        if (buffer.len > PY_SSIZE_T_MAX - HEADER_LENGTH) {
            PyErr_SetString(PyExc_ValueError, "input too big");
            goto exit;
        }
        if (!(result = PyBytes_FromStringAndSize(NULL, buffer.len + HEADER_LENGTH)))
            goto exit;
        dst = (unsigned char *)PyBytes_AS_STRING(result);
        set_header(dst, (uint32_t)buffer.len + 12, (uint32_t)buffer.len, UNCOMPRESSED, 0);
        memcpy(dst + HEADER_LENGTH, buffer.buf, buffer.len);
        goto exit;
    }

    if (buffer.len > UINT32_MAX) {  /* cannot set header raw_size */
        PyErr_SetString(PyExc_ValueError, "input too big");
        goto exit;
    }

    ssize_t alloc_size;
    if (buffer.len > 1024 * 100)
        alloc_size = buffer.len / 4;
    else if (buffer.len > 1024 * 50)
        alloc_size = buffer.len / 3;
    else
        alloc_size = buffer.len / 2;
    dst_len = alloc_size + HEADER_LENGTH;
    if (!(result = PyBytes_FromStringAndSize(NULL, dst_len)))
        goto exit;
    dst = (unsigned char *)PyBytes_AS_STRING(result);
    state.src = buffer.buf;
    state.src_left = buffer.len;
    bool done = false;

    Py_BEGIN_ALLOW_THREADS
        while (!done) {
            control = 0;
            control_bit = 1;
            token_offset = 0;

            /* ensure 1 max sized run + end marker free */
            if (dst_len - dst_offset < 21) {
                if (PY_SSIZE_T_MAX - dst_offset < 8192) {
                    Py_BLOCK_THREADS
                        Py_CLEAR(result);
                    PyErr_SetString(PyExc_ValueError, "input too big");
                    Py_UNBLOCK_THREADS
                        break;
                }
                dst_len += 8192;
                Py_BLOCK_THREADS
                    int ret = _PyBytes_Resize(&result, dst_len);
                if (!ret)
                    dst = (unsigned char *)PyBytes_AS_STRING(result);
                Py_UNBLOCK_THREADS
                    if (ret == -1)
                        break;
            }

            for (; control_bit <= 0x80 && state.src_left; control_bit <<= 1) {
                if (find_longest_match(&state) < 2) {
                    token_buf[token_offset++] = *state.src++;
                    state.src_left--;
                }
                else {
                    token_buf[token_offset] = (state.best_match_offset >> 4) & 0xff;
                    token_buf[token_offset + 1] = ((state.best_match_offset << 4) & 0xf0) |
                        ((state.best_match_len - 2) & 0xf);
                    control |= control_bit;
                    token_offset += 2;
                    state.src += state.best_match_len;
                    state.src_left -= state.best_match_len;
                }
            }

            if (!state.src_left && control_bit <= 0x80) {
                /* step 8 - add end marker */
                control |= control_bit;
                token_buf[token_offset++] = (state.dict_write_offset >> 4) & 0xff;
                token_buf[token_offset++] = ((state.dict_write_offset << 4) & 0xf0);
                done = true;
            }

            /* write current run */
            dst[dst_offset++] = control;
            memcpy(dst + dst_offset, &token_buf, token_offset);
            dst_offset += token_offset;
        }
    if (result) {  /* can be null if memory resize failed */
        crc = calculate_crc(dst + HEADER_LENGTH, dst_offset - HEADER_LENGTH);
        set_header(dst, (uint32_t)dst_offset - 4, (uint32_t)buffer.len, COMPRESSED, crc);
    }
    Py_END_ALLOW_THREADS

        if (result)
            _PyBytes_Resize(&result, dst_offset);

exit:
    if (buffer.obj)
        PyBuffer_Release(&buffer);

    return result;
}

PyDoc_STRVAR(xrtf_decompress_doc,
    "decompress(data)\n"
    "\n"
    "Decompress bytes data.");

PyObject *xrtf_decompress(PyObject *self, PyObject *arg)
{
    HDR hdr;
    Py_buffer buffer = { NULL, NULL };
    unsigned char *src;
    ssize_t src_len;
    ssize_t src_left;
    unsigned char *dst;
    ssize_t dst_left;
    uint32_t crc = 0;
    uint32_t control = 0;
    uint32_t reference;
    uint32_t token_cnt = 0;
    unsigned char dict[4096] = DICT_INIT;
    int dict_write_offset = sizeof(DICT_INIT) - 1;
    int dict_read_offset;
    int dict_read_length;
    PyObject* result = NULL;

    if (PyObject_GetBuffer(arg, &buffer, PyBUF_SIMPLE) == -1)
        return NULL;

    src = buffer.buf;
    src_len = buffer.len;

    if (!parse_header(src, src_len, &hdr))
        goto exit;

    if (hdr.comp_type == UNCOMPRESSED) {
        result = PyBytes_FromStringAndSize((char *)src + HEADER_LENGTH, src_len - HEADER_LENGTH);
        goto exit;
    }

#if UINT32_MAX == SIZE_MAX
    if (hdr.raw_size > PY_SSIZE_T_MAX) {
        PyErr_SetString(PyExc_OverflowError, "buffer overflow: input too big");
        goto exit;
    }
#endif
    if (!(result = PyBytes_FromStringAndSize(NULL, hdr.raw_size))) {
        goto exit;
    }
    dst = (unsigned char *)PyBytes_AS_STRING(result);
    Py_BEGIN_ALLOW_THREADS
        dst_left = hdr.raw_size;
    src += HEADER_LENGTH;
    src_left = src_len - HEADER_LENGTH;
    while (true) {
        if ((token_cnt++ & 7) == 0) {
            if (--src_left < 0)
                break;
            crc = update_crc(crc, *src);
            control = *src++;
        }
        else {
            control >>= 1;
        }

        if ((control & 1) == 0) {
            if (--src_left < 0)
                break;
            if (--dst_left < 0)
                break;
            crc = update_crc(crc, *src);
            dict[dict_write_offset] = *dst++ = *src++;
            dict_write_offset = (dict_write_offset + 1) & 0xfff;
            continue;
        }

        src_left -= 2;
        if (src_left < 0)
            break;

        reference = (uint32_t)* src << 8 | (uint32_t) * (src + 1);
        dict_read_offset = (reference >> 4) & 0xfff; /* 12 bits */
        dict_read_length = (reference & 0xf) + 2;    /* 4 bits */
        crc = update_crc(crc, *src++);
        crc = update_crc(crc, *src++);

        if (dict_read_offset == dict_write_offset) { /* we are done */
            while (src_left) {
                crc = update_crc(crc, *src++);
                src_left--;
            }
            break;
        }
        dst_left -= dict_read_length;
        if (dst_left < 0)
            break;

        while (dict_read_length--) {
            *dst++ = dict[dict_read_offset];
            dict[dict_write_offset] = dict[dict_read_offset];
            dict_read_offset = (dict_read_offset + 1) & 0xfff;
            dict_write_offset = (dict_write_offset + 1) & 0xfff;
        }
    }
    Py_END_ALLOW_THREADS

        if (src_left < 0) {
            Py_CLEAR(result);
            PyErr_SetString(xrtf_Error, "buffer overrun: data is corrupt");
        }
        else if (dst_left < 0) {
            Py_CLEAR(result);
            PyErr_SetString(xrtf_Error, "buffer overflow: data is corrupt");
        }
        else if (hdr.crc != crc) {
            Py_CLEAR(result);
            PyErr_SetString(xrtf_Error, "invalid crc: data is corrupt");
        }

exit:
    if (buffer.obj)
        PyBuffer_Release(&buffer);

    return result;
}

PyDoc_STRVAR(xrtf_parse_header_doc,
    "parse_header(data)\n"
    "\n"
    "Parse data header from bytes.");

PyObject* xrtf_parse_header(PyObject *self, PyObject *arg)
{
    HDR hdr;
    Py_buffer buffer = { NULL, NULL };
    PyObject *result = NULL;

    if (PyObject_GetBuffer(arg, &buffer, PyBUF_SIMPLE) == -1)
        return NULL;

    if (!parse_header(buffer.buf, buffer.len, &hdr))
        goto exit;

    if (!(result = PyStructSequence_New(&CompressedRtfHeader_Type)))
        goto exit;

    PyStructSequence_SET_ITEM(result, 0, PyLong_FromUnsignedLong(hdr.comp_size));
    PyStructSequence_SET_ITEM(result, 1, PyLong_FromUnsignedLong(hdr.raw_size));
    PyStructSequence_SET_ITEM(result, 2, PyLong_FromUnsignedLong(hdr.comp_type));
    PyStructSequence_SET_ITEM(result, 3, PyLong_FromUnsignedLong(hdr.crc));

exit:
    if (buffer.obj)
        PyBuffer_Release(&buffer);

    return result;
}

static PyMethodDef xrtf_compression_functions[] = {
    {"compress", (PyCFunction)xrtf_compress, METH_VARARGS | METH_KEYWORDS, xrtf_compress_doc},
    {"decompress", (PyCFunction)xrtf_decompress, METH_O, xrtf_decompress_doc},
    {"parse_header", (PyCFunction)xrtf_parse_header, METH_O, xrtf_parse_header_doc},
    {NULL, NULL, 0, NULL}
};

int xrtf_compression_exec(PyObject *m)
{
    PyModule_AddFunctions(m, xrtf_compression_functions);

    if (PyStructSequence_InitType2(&CompressedRtfHeader_Type, &CompressedRtfHeader_desc) == -1)
        goto fail;

    PyModule_AddIntMacro(m, COMPRESSED);
    PyModule_AddIntMacro(m, UNCOMPRESSED);
    return 0; /* success */

fail:
    Py_XDECREF(m);
    return -1;
}