#include <Python.h>
#include "btlemon.h"

#if PY_MAJOR_VERSION < 3
#error "only Python 3 supported"
#endif

static PyObject *py_callback = NULL;

static PyObject *set_callback(PyObject *self, PyObject *args) {
  PyObject *result = NULL;
  PyObject *temp;

  if (PyArg_ParseTuple(args, "O:set_callback", &temp)) {
    if (!PyCallable_Check(temp)) {
      PyErr_SetString(PyExc_TypeError, "callback must be callable");
      return NULL;
    }
    Py_XINCREF(temp);
    Py_XDECREF(py_callback);
    py_callback = temp;
    Py_INCREF(Py_None);
    result = Py_None;
  }
  return result;
}

static void c_callback(const uint8_t addr[6], const int8_t *rssi, const uint8_t *data, uint8_t data_len) {
  char addr_string[18];
  int i;
  PyObject *arglist;
  PyObject *pydata;
  PyObject *result;
  sprintf(addr_string, "%2.2X:%2.2X:%2.2X:%2.2X:%2.2X:%2.2X",
         addr[5], addr[4], addr[3],
         addr[2], addr[1], addr[0]);
  arglist = PyTuple_New(3);
  pydata = PyTuple_New(data_len);
  PyObject *pyaddr = Py_BuildValue("s", addr_string);
  PyObject *pyrssi = Py_BuildValue("i", *rssi);
  PyTuple_SetItem(arglist, 0, pyaddr);
  PyTuple_SetItem(arglist, 1, pyrssi);
  for (i = 0; i < data_len; i++) {
    PyObject *d = Py_BuildValue("i", data[i]);
    PyTuple_SetItem(pydata, i, d);
  }
  PyTuple_SetItem(arglist, 2, pydata);
  result = PyObject_CallObject(py_callback, arglist);
  Py_DECREF(arglist);
  if (result == NULL) {
    printf("ERROR: python callback failed\n");
    btlemon_stop();
  }
  Py_XDECREF(result);
}

static PyObject *run(PyObject *self, PyObject *args) {
  if (py_callback) {
    btlemon_set_callback(c_callback);
  }
  if (btlemon_run() < 0) {
    PyErr_SetString(PyExc_RuntimeError, "Error in btlemon execution, check output");
    return NULL;
  };
  Py_RETURN_NONE;
}

static PyMethodDef PybtlemonMethods[] = {
    {"run", run, METH_VARARGS, "Start btlemon"},
    {"set_callback", set_callback, METH_VARARGS, "Set callback"},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef pybtlemonmodule = {
    PyModuleDef_HEAD_INIT,
    "pybtlemon",
    NULL,
    -1,
    PybtlemonMethods
};

PyMODINIT_FUNC
PyInit_pybtlemon(void) {
  return PyModule_Create(&pybtlemonmodule);
}