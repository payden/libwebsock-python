#include <Python.h>
#include <stdio.h>
#include <stdlib.h>

#include <websock/websock.h>

typedef struct {
  PyObject_HEAD
  libwebsock_client_state *state;
} libwebsock_ClientStateObject;

typedef struct {
  PyObject_HEAD
  libwebsock_message *msg;
} libwebsock_MessageObject;

static PyTypeObject libwebsock_ClientStateType = {
  PyObject_HEAD_INIT(NULL)
  0,
  "libwebsock.ClientState",
  sizeof(libwebsock_ClientStateObject),
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  Py_TPFLAGS_DEFAULT,
  "libwebsock objects",
};

static libwebsock_context *ws_ctx = NULL;
static PyObject *onopen_callback = NULL;
static PyObject *onclose_callback = NULL;
static PyObject *onmessage_callback = NULL;

static PyObject *libwebsock_onopen(PyObject *self, PyObject *args);
static PyObject *libwebsock_onclose(PyObject *self, PyObject *args);
static PyObject *libwebsock_onmessage(PyObject *self, PyObject *args);
static PyObject *libwebsock_run(PyObject *self, PyObject *args);

static PyMethodDef LibwebsockMethods[] = {
  {"onopen", libwebsock_onopen, METH_VARARGS, "Set onopen callback"},
  {"onclose", libwebsock_onclose, METH_VARARGS, "Set onclose callback"},
  {"onmessage", libwebsock_onmessage, METH_VARARGS, "Set onmessage callback"},
  {"run", libwebsock_run, METH_VARARGS, "Run WebSocket server"},
  {NULL, NULL, 0, NULL}
};

static int ws_onopen(libwebsock_client_state *state)
{
  PyObject *stateObject;
  PyObject *arglist;

  libwebsock_ClientStateObject *internalStateObject;
  //generate python client state object and call python callable
  if(onopen_callback) {
    stateObject = PyObject_CallObject((PyObject *)&libwebsock_ClientStateType, NULL);
    internalStateObject = (libwebsock_ClientStateObject *)stateObject;
    internalStateObject->state = state;
    arglist = Py_BuildValue("(O)", stateObject);
    PyObject_CallObject(onopen_callback, arglist);
    Py_DECREF(arglist);
  }
  return 0;
}

static int ws_onclose(libwebsock_client_state *state)
{
  //generate python client state object and call python callable
  return 0;
}

static int ws_onmessage(libwebsock_client_state *state, libwebsock_message *msg)
{
  //generate python objects and call python callable

  PyObject *stateObject;
  PyObject *arglist;

  libwebsock_ClientStateObject *internalStateObject;

  if(onmessage_callback) {
    stateObject = PyObject_CallObject((PyObject *)&libwebsock_ClientStateType, NULL);
    internalStateObject = (libwebsock_ClientStateObject *)stateObject;
    internalStateObject->state = state;
    arglist = Py_BuildValue("(Os)", stateObject, msg->payload);
    PyObject_CallObject(onmessage_callback, arglist);
    Py_DECREF(arglist);
  }

  return 0;
}

PyMODINIT_FUNC initlibwebsock(void)
{
  PyObject *m;
  libwebsock_ClientStateType.tp_new = PyType_GenericNew;
  if(PyType_Ready(&libwebsock_ClientStateType) < 0) {
    return;
  }
  ws_ctx = libwebsock_init();
  m = Py_InitModule("libwebsock", LibwebsockMethods);
  Py_INCREF(&libwebsock_ClientStateType);
  PyModule_AddObject(m, "ClientState", (PyObject *)&libwebsock_ClientStateType);
}

static PyObject *libwebsock_run(PyObject *self, PyObject *args)
{
  PyObject *result = NULL;
  char *port;

  if(!PyArg_ParseTuple(args, "s", &port)) {
    return NULL;
  }
  libwebsock_bind(ws_ctx, "0.0.0.0", port);
  ws_ctx->onopen = ws_onopen;
  ws_ctx->onclose = ws_onclose;
  ws_ctx->onmessage = ws_onmessage;
  libwebsock_wait(ws_ctx); //Run loop
  Py_INCREF(Py_None);
  return Py_None;
}



static PyObject *libwebsock_onopen(PyObject *self, PyObject *args)
{
  PyObject *result = NULL;
  PyObject *temp;

  if(PyArg_ParseTuple(args, "O:onopen", &temp)) {
    if(!PyCallable_Check(temp)) {
      PyErr_SetString(PyExc_TypeError, "parameter must be callable");
      return NULL;
    }
    Py_XINCREF(temp);
    Py_XDECREF(onopen_callback);
    onopen_callback = temp;
    Py_INCREF(Py_None);
    result = Py_None;
  }
  return result;
}

static PyObject *libwebsock_onclose(PyObject *self, PyObject *args)
{
  PyObject *result = NULL;
  PyObject *temp;

  if(PyArg_ParseTuple(args, "O:onclose", &temp)) {
    if(!PyCallable_Check(temp)) {
      PyErr_SetString(PyExc_TypeError, "parameter must be callable");
      return NULL;
    }
    Py_XINCREF(temp);
    Py_XDECREF(onclose_callback);
    onclose_callback = temp;
    Py_INCREF(Py_None);
    result = Py_None;
  }
  return result;
}

static PyObject *libwebsock_onmessage(PyObject *self, PyObject *args)
{
  PyObject *result = NULL;
  PyObject *temp;
  
  if(PyArg_ParseTuple(args, "O:onmessage", &temp)) {
    if(!PyCallable_Check(temp)) {
      PyErr_SetString(PyExc_TypeError, "parameter must be callable");
      return NULL;
    }
    Py_XINCREF(temp);
    Py_XDECREF(onmessage_callback);
    onmessage_callback = temp;
    Py_INCREF(Py_None);
    result = Py_None;
  }
  return result;
}

