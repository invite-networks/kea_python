#include "keamodule.h"

using namespace std;
using namespace isc::hooks;
using namespace isc::dhcp;

extern "C" {

static PyObject *
Option_getBytes(OptionObject *self, PyObject *args) {
    try {
        vector<uint8_t> value = self->ptr->toBinary();
        return (PyBytes_FromStringAndSize((const char *) &value[0], value.size()));
    } catch (const exception &e) {
        PyErr_SetString(PyExc_TypeError, e.what());
        return (0);
    }
}

static PyObject *
Option_setBytes(OptionObject *self, PyObject *args) {
    PyObject *data;

    if (!PyArg_ParseTuple(args, "S", &data)) {
        return (0);
    }

    try {
        char *buff;
        Py_ssize_t len;

        PyBytes_AsStringAndSize(data, &buff, &len);
        self->ptr->setData(&buff[0], &buff[len]);
        Py_INCREF(self);
        return ((PyObject *)self);
    } catch (const exception &e) {
        PyErr_SetString(PyExc_TypeError, e.what());
        return (0);
    }
}

static PyObject *
Option_getString(OptionObject *self, PyObject *args) {
    try {
        vector<uint8_t> value = self->ptr->toBinary();
        return (PyUnicode_FromStringAndSize((const char *) &value[0], value.size()));
    } catch (const exception &e) {
        PyErr_SetString(PyExc_TypeError, e.what());
        return (0);
    }
}

static PyObject *
Option_setString(OptionObject *self, PyObject *args) {
    char *data;
    Py_ssize_t len;

    if (!PyArg_ParseTuple(args, "s#", &data, &len)) {
        return (0);
    }

    try {
        self->ptr->setData(&data[0], &data[len]);
        Py_INCREF(self);
        return ((PyObject *)self);
    } catch (const exception &e) {
        PyErr_SetString(PyExc_TypeError, e.what());
        return (0);
    }
}

static PyObject *
Option_getUint32(OptionObject *self, PyObject *args) {
    try {
        return (PyLong_FromLong(self->ptr->getUint32()));
    } catch (const exception &e) {
        PyErr_SetString(PyExc_TypeError, e.what());
        return (0);
    }
}

static PyObject *
Option_setUint32(OptionObject *self, PyObject *args) {
    long value;

    if (!PyArg_ParseTuple(args, "i", &value)) {
        return (0);
    }

    try {
        self->ptr->setUint32(value);
        Py_INCREF(self);
        return ((PyObject *)self);
    } catch (const exception &e) {
        PyErr_SetString(PyExc_TypeError, e.what());
        return (0);
    }
}

static PyMethodDef Option_methods[] = {
    {"getBytes", (PyCFunction) Option_getBytes, METH_NOARGS,
     "Returns option data."},
    {"setBytes", (PyCFunction) Option_setBytes, METH_VARARGS,
     "Sets content to data."},
    {"getString", (PyCFunction) Option_getString, METH_NOARGS,
     "Returns option data from UTF-8 decoded string."},
    {"setString", (PyCFunction) Option_setString, METH_VARARGS,
     "Sets content to UTF-8 encoded string."},
    {"getUint32", (PyCFunction) Option_getUint32, METH_NOARGS,
     "Returns content of first double word."},
    {"setUint32", (PyCFunction) Option_setUint32, METH_VARARGS,
     "Sets content to single uint32 value."},
    {0}  // Sentinel
};

static PyObject *
Option_use_count(OptionObject *self, void *closure) {
    return (PyLong_FromLong(self->ptr.use_count()));
}

static PyGetSetDef Option_getsetters[] = {
    {(char *)"use_count", (getter) Option_use_count, (setter) 0, (char *)"shared_ptr use count", 0},
    {0}  // Sentinel
};

static void
Option_dealloc(OptionObject *self) {
    self->ptr.reset();
    Py_TYPE(self)->tp_free((PyObject *) self);
}

static int
Option_init(OptionObject *self, PyObject *args, PyObject *kwds) {
    long type;

    if (kwds != 0) {
        PyErr_SetString(PyExc_TypeError, "keyword arguments are not supported");
        return (0);
    }
    if (!PyArg_ParseTuple(args, "i", &type)) {
        return (-1);
    }
    self->ptr.reset(new Option(Option::Universe::V4, (uint16_t)type));

    return (0);
}

static PyObject *
Option_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    OptionObject *self;
    self = (OptionObject *) type->tp_alloc(type, 0);
    if (self) {
        self->ptr.reset();
    }
    return ((PyObject *) self);
}

static PyTypeObject OptionType = {
    PyObject_HEAD_INIT(0)
    "kea.Option",                               // tp_name
    sizeof(OptionObject),                       // tp_basicsize
    0,                                          // tp_itemsize
    (destructor) Option_dealloc,                // tp_dealloc
    0,                                          // tp_vectorcall_offset
    0,                                          // tp_getattr
    0,                                          // tp_setattr
    0,                                          // tp_as_async
    0,                                          // tp_repr
    0,                                          // tp_as_number
    0,                                          // tp_as_sequence
    0,                                          // tp_as_mapping
    0,                                          // tp_hash
    0,                                          // tp_call
    0,                                          // tp_str
    0,                                          // tp_getattro
    0,                                          // tp_setattro
    0,                                          // tp_as_buffer
    Py_TPFLAGS_DEFAULT,                         // tp_flags
    "Kea server Option",                        // tp_doc
    0,                                          // tp_traverse
    0,                                          // tp_clear
    0,                                          // tp_richcompare
    0,                                          // tp_weaklistoffset
    0,                                          // tp_iter
    0,                                          // tp_iternext
    Option_methods,                             // tp_methods
    0,                                          // tp_members
    Option_getsetters,                          // tp_getset
    0,                                          // tp_base
    0,                                          // tp_dict
    0,                                          // tp_descr_get
    0,                                          // tp_descr_set
    0,                                          // tp_dictoffset
    (initproc) Option_init,                     // tp_init
    PyType_GenericAlloc,                        // tp_alloc
    Option_new                                  // tp_new
};

int
Option_Check(PyObject *object) {
    return (Py_TYPE(object) == &OptionType);
}

PyObject *
Option_from_handle(OptionPtr &ptr) {
    OptionObject *self = PyObject_New(OptionObject, &OptionType);
    if (self) {
        memset(&self->ptr, 0 , sizeof(self->ptr));
        self->ptr = ptr;
    }
    return (PyObject *)self;
}

int
Option_define() {
    if (PyType_Ready(&OptionType) < 0) {
        return (1);
    }
    Py_INCREF(&OptionType);
    if (PyModule_AddObject(kea_module, "Option", (PyObject *) &OptionType) < 0) {
        Py_DECREF(&OptionType);
        return (1);
    }

    return (0);
}

}