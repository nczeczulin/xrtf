#include <Python.h>
#include <structmember.h>
#include <stdbool.h>

extern PyObject *xrtf_Error;

#define GROUP            0
#define CONTROL_SYMBOL   1
#define CONTROL_WORD     2
#define TEXT             3

static PyTypeObject GroupToken_Type;

static PyStructSequence_Field GroupToken_fields[] = {
    {"type", ""},
    {"start", ""},
    {NULL}
};

static PyStructSequence_Desc GroupToken_desc = {
    "xrtf.GroupToken", NULL, GroupToken_fields, 2
};

static PyTypeObject ControlSymbolToken_Type;

static PyStructSequence_Field ControlSymbolToken_fields[] = {
    {"type", ""},
    {"symbol", ""},
    {NULL}
};

static PyStructSequence_Desc ControlSymbolToken_desc = {
    "xrtf.ControlSymbolToken", NULL, ControlSymbolToken_fields, 2
};

static PyTypeObject ControlWordToken_Type;

static PyStructSequence_Field ControlWordToken_fields[] = {
    {"type", ""},
    {"keyword", ""},
    {"param", ""},
    {NULL}
};

static PyStructSequence_Desc ControlWordToken_desc = {
    "xrtf.ControlWordToken", NULL, ControlWordToken_fields, 3
};

static PyTypeObject TextToken_Type;

static PyStructSequence_Field TextToken_fields[] = {
    {"type", ""},
    {"text", ""},
    {NULL}
};

static PyStructSequence_Desc TextToken_desc = {
    "xrtf.TextToken", NULL, TextToken_fields, 2
};

typedef struct {
    PyObject_HEAD
    Py_buffer buffer;
    char *buf;
    char *buf_end;
} Tokenizer;

static int Tokenizer_init(Tokenizer *self, PyObject *args, PyObject *kwargs)
{
    if (self->buffer.obj)
        PyBuffer_Release(&self->buffer);

    if (!PyArg_ParseTuple(args, "y*:Tokenizer", &self->buffer))
        return -1;

    self->buf = self->buffer.buf;
    self->buf_end = self->buf + self->buffer.len;

    return 0;
}

static void Tokenizer_dealloc(Tokenizer *self)
{
    if (self->buffer.obj)
        PyBuffer_Release(&self->buffer);
    Py_TYPE(self)->tp_free(self);
}

static PyObject* text_token(Tokenizer *self, char *start)
{
    PyObject *type;
    PyObject *text;
    PyObject *result;

    if (!(result = PyStructSequence_New(&TextToken_Type)))
        return NULL;
    if (!(type = PyLong_FromLong(TEXT)))
        goto fail;
    if (!(text = PyBytes_FromStringAndSize(start, self->buf - start)))
        goto fail;

    PyStructSequence_SET_ITEM(result, 0, type);
    PyStructSequence_SET_ITEM(result, 1, text);
    return result;
fail:
    Py_DECREF(result);
    return NULL;
}

static PyObject* group_token(Tokenizer *self, bool start)
{
    PyObject *type;
    PyObject *result;

    if (!(result = PyStructSequence_New(&GroupToken_Type)))
        return NULL;
    if (!(type = PyLong_FromLong(GROUP)))
        goto fail;

    PyStructSequence_SET_ITEM(result, 0, type);
    if (start) {
        Py_INCREF(Py_True);
        PyStructSequence_SET_ITEM(result, 1, Py_True);
    }
    else {
        Py_INCREF(Py_False);
        PyStructSequence_SET_ITEM(result, 1, Py_False);
    }
    return result;

fail:
    Py_DECREF(result);
    return NULL;
}

static PyObject* control_token(Tokenizer *self)
{
    char ch;
    char keyword[33];
    char param[11];
    char *tmp;

    PyObject *type;
    PyObject *arg1;
    PyObject *arg2;
    PyObject *result;

    if (self->buf >= self->buf_end) {
        PyErr_SetString(xrtf_Error, "buffer overrun: data is corrupt");
        return NULL;
    }

    ch = *self->buf;

    /* control symbol? */
    if (!isalpha(ch)) {
        self->buf++;
        if (!(result = PyStructSequence_New(&ControlSymbolToken_Type)))
            return NULL;
        if (!(type = PyLong_FromLong(CONTROL_SYMBOL)))
            goto fail;
        if (!(arg1 = PyUnicode_FromFormat("%c", ch)))
            goto fail;

        PyStructSequence_SET_ITEM(result, 0, type);
        PyStructSequence_SET_ITEM(result, 1, arg1);
        return result;
    }
    else {
        /* control word */
        char *buf_stop = self->buf + Py_MIN(32, self->buf_end - self->buf);
        for (tmp = keyword; self->buf < buf_stop && isalpha(ch); ch = *++self->buf) {
            *tmp++ = ch;
        }
        if (self->buf >= buf_stop) {
            if (self->buf >= self->buf_end) {
                PyErr_SetString(xrtf_Error, "buffer overrun: data is corrupt");
                return NULL;
            }
            if (isalpha(ch)) {
                PyErr_SetString(xrtf_Error, "invalid keyword: too long");
                return NULL;
            }
        }
        *tmp = '\0';

        tmp = param;
        if (ch == '-') {
            *tmp++ = ch;
            if (++self->buf >= self->buf_end) {
                PyErr_SetString(xrtf_Error, "buffer overrun: data is corrupt");
                return NULL;
            }
        }

        buf_stop = self->buf + Py_MIN(10, self->buf_end - self->buf);
        for (ch = *self->buf; self->buf < buf_stop && isdigit(ch); ch = *++self->buf) {
            *tmp++ = ch;
        }
        if (self->buf >= buf_stop) {
            if (self->buf >= self->buf_end) {
                PyErr_SetString(xrtf_Error, "buffer overrun: data is corrupt");
                return NULL;
            }
            if (isdigit(ch)) {
                PyErr_SetString(xrtf_Error, "invalid parameter: too long");
                return NULL;
            }
        }
        *tmp = '\0';

        if (ch == ' ')
            self->buf++;

        if (!(result = PyStructSequence_New(&ControlWordToken_Type)))
            return NULL;
        if (!(type = PyLong_FromLong(CONTROL_WORD)))
            goto fail;
        if (!(arg1 = PyUnicode_FromString(keyword)))
            goto fail;
        if (*param) {
            char *endptr;
            if (!(arg2 = PyLong_FromString(param, &endptr, 10)))
                goto fail;
        }
        else {
            Py_INCREF(Py_None);
            arg2 = Py_None;
        }
        PyStructSequence_SET_ITEM(result, 0, type);
        PyStructSequence_SET_ITEM(result, 1, arg1);
        PyStructSequence_SET_ITEM(result, 2, arg2);
        return result;
    }
fail:
    Py_DECREF(result);
    return NULL;
}

static PyObject* Tokenizer_next(Tokenizer *self)
{
    char ch;
    int deferred = 0;

    if (!self->buffer.obj)
        return NULL;

    while (self->buf < self->buf_end) {
        switch (ch = *self->buf) {
        case '{':
            if (deferred)
                return text_token(self, self->buf - deferred);
            self->buf++;
            return group_token(self, true);
            break;
        case '}':
            if (deferred)
                return text_token(self, self->buf - deferred);
            self->buf++;
            return group_token(self, false);
            break;
        case '\\':
            if (deferred)
                return text_token(self, self->buf - deferred);
            self->buf++;
            return control_token(self);
            break;
        case '\r':
        case '\n':
            if (deferred)
                return text_token(self, self->buf - deferred);
            self->buf++;
            break;
        default:
            deferred++;
            self->buf++;
        }
    }

    if (deferred)
        return text_token(self, self->buf - deferred);

    return NULL;
}

PyDoc_STRVAR(Tokenizer_read_doc, "read");

PyObject* Tokenizer_read(Tokenizer *self, PyObject *arg)
{
    long size;
    PyObject *result;

    if (!self->buffer.obj) {
        PyErr_SetString(PyExc_ValueError, "invalid read operation");
        return NULL;
    }
    if ((size = PyLong_AsLong(arg)) == -1) {
        if (PyErr_Occurred())
            return NULL;
    }

    if (size < 1 || self->buf_end - self->buf < size) {
        PyErr_SetString(PyExc_ValueError, "invalid read size");
        return NULL;
    }
    if ((result = PyBytes_FromStringAndSize(self->buf, size)))
        self->buf += size;
    return result;
}

static PyMethodDef Tokenizer_methods[] = {
    {"read", (PyCFunction)Tokenizer_read, METH_O, Tokenizer_read_doc},
    {NULL, NULL, 0, NULL}
};

PyDoc_STRVAR(Tokenizer_doc, "The Tokenizer class");

PyTypeObject Tokenizer_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "xrtf.Tokenizer",
    .tp_doc = Tokenizer_doc,
    .tp_basicsize = sizeof(Tokenizer),
    .tp_dealloc = (destructor)Tokenizer_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_methods = Tokenizer_methods,
    .tp_iter = PyObject_SelfIter,
    .tp_iternext = (iternextfunc)Tokenizer_next,
    .tp_init = (initproc)Tokenizer_init,
    .tp_new = PyType_GenericNew
};

int xrtf_tokenizer_exec(PyObject *m)
{
    if (PyType_Ready(&Tokenizer_Type) < 0)
        goto fail;
    PyModule_AddObject(m, "Tokenizer", (PyObject *) &Tokenizer_Type);

    if (PyStructSequence_InitType2(&GroupToken_Type, &GroupToken_desc) == -1)
        goto fail;
    PyModule_AddObject(m, "GroupToken", (PyObject *) &GroupToken_Type);

    if (PyStructSequence_InitType2(&ControlSymbolToken_Type, &ControlSymbolToken_desc) == -1)
        goto fail;
    PyModule_AddObject(m, "ControlSymbolToken", (PyObject *) &ControlSymbolToken_Type);

    if (PyStructSequence_InitType2(&ControlWordToken_Type, &ControlWordToken_desc) == -1)
        goto fail;
    PyModule_AddObject(m, "ControlWordToken", (PyObject *) &ControlWordToken_Type);

    if (PyStructSequence_InitType2(&TextToken_Type, &TextToken_desc) == -1)
        goto fail;
    PyModule_AddObject(m, "TextToken", (PyObject *) &TextToken_Type);

    PyModule_AddIntMacro(m, GROUP);
    PyModule_AddIntMacro(m, CONTROL_SYMBOL);
    PyModule_AddIntMacro(m, CONTROL_WORD);
    PyModule_AddIntMacro(m, TEXT);

    return 0; /* success */

fail:
    Py_XDECREF(m);
    return -1;
}