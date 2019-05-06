#include <Python.h>

int xrtf_compression_exec(PyObject *m);
int xrtf_tokenizer_exec(PyObject* m);

PyObject *xrtf_Error;

static int xrtf_exec(PyObject *m)
{
    if (xrtf_Error == NULL) {
        xrtf_Error = PyErr_NewException("xrtf.Error", NULL, NULL);
        if (xrtf_Error == NULL)
            goto fail;
    }
    PyModule_AddObject(m, "Error", xrtf_Error);

    return 0; /* success */

fail:
    Py_XDECREF(m);
    return -1;
}

PyDoc_STRVAR(xrtf_doc, "The xrtf module");

static PyModuleDef_Slot xrtf_slots[] = {
    {Py_mod_exec, xrtf_exec},
    {Py_mod_exec, xrtf_compression_exec},
    {Py_mod_exec, xrtf_tokenizer_exec},
    {0, NULL}
};

static PyModuleDef xrtf_def = {
    PyModuleDef_HEAD_INIT,
    "_xrtf",
    xrtf_doc,
    0,    /* m_size */
    NULL, /* m_methods */
    xrtf_slots,
    NULL, /* m_traverse */
    NULL, /* m_clear */
    NULL, /* m_free */
};

PyMODINIT_FUNC PyInit__xrtf() { return PyModuleDef_Init(&xrtf_def); }
