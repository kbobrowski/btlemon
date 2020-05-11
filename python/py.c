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

static void c_callback(const uint8_t addr[6], const int8_t *rssi) {
  char addr_string[18];
  sprintf(addr_string, "%2.2X:%2.2X:%2.2X:%2.2X:%2.2X:%2.2X",
         addr[5], addr[4], addr[3],
         addr[2], addr[1], addr[0]);
  PyObject *arglist;
  PyObject *result;
  arglist = Py_BuildValue("(s,i)", addr_string, *rssi);
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