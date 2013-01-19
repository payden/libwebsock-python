#include <Python.h>
#include <stdio.h>
#include <stdlib.h>

#include <websock/websock.h>

typedef struct {
  PyObject_HEAD
  libwebsock_client_state *state;
} libwebsock_ClientStateObject;


static libwebsock_context *ws_ctx = NULL;
static PyObject *onopen_callback = NULL;
static PyObject *onclose_callback = NULL;
static PyObject *onmessage_callback = NULL;
static PyObject *connected_clients_list = NULL;

static PyObject *libwebsockpy_onopen(PyObject *self, PyObject *args);
static PyObject *libwebsockpy_onclose(PyObject *self, PyObject *args);
static PyObject *libwebsockpy_onmessage(PyObject *self, PyObject *args);
static PyObject *libwebsockpy_run(PyObject *self, PyObject *args);
static PyObject *libwebsockpy_send(PyObject *self, PyObject *args);
static PyObject *libwebsockpy_connected_clients(PyObject *self);
static PyObject *libwebsockpy_ClientState_getsockfd(libwebsock_ClientStateObject *ClientState, void *closure);
static PyObject *libwebsockpy_ClientState_getaddr(libwebsock_ClientStateObject *ClientState, void *closure);

static PyMethodDef LibwebsockMethods[] = {
  {"onopen", libwebsockpy_onopen, METH_VARARGS, "Set onopen callback"},
  {"onclose", libwebsockpy_onclose, METH_VARARGS, "Set onclose callback"},
  {"onmessage", libwebsockpy_onmessage, METH_VARARGS, "Set onmessage callback"},
  {"run", libwebsockpy_run, METH_VARARGS, "Run WebSocket server"},
  {"send", libwebsockpy_send, METH_VARARGS, "Send WebSocket Message to client"},
  {"connected_clients", (PyCFunction)libwebsockpy_connected_clients, METH_NOARGS, "Return list of connected clients"},
  {NULL, NULL, 0, NULL}
};

static PyGetSetDef libwebsock_ClientStateGetSet[] = {
  {"sock", (getter)libwebsockpy_ClientState_getsockfd, (setter)0, "socket descriptor", NULL},
  {"addr", (getter)libwebsockpy_ClientState_getaddr, (setter)0, "connecting address", NULL},
  {NULL}
};

static PyTypeObject libwebsock_ClientStateType = {
  PyObject_HEAD_INIT(NULL)
  0,                                    /*ob_size*/
  "libwebsock.ClientState",             /*tp_name*/
  sizeof(libwebsock_ClientStateObject), /*tp_basicsize*/
  0,                                    /*tp_itemsize*/
  0,                                    /*tp_dealloc*/
  0,                                    /*tp_print*/
  0,                                    /*tp_getattr*/
  0,                                    /*tp_setattr*/
  0,                                    /*tp_compare*/
  0,                                    /*tp_repr*/
  0,                                    /*tp_as_number*/
  0,                                    /*tp_as_sequence*/
  0,                                    /*tp_as_mapping*/
  0,                                    /*tp_hash*/
  0,                                    /*tp_call*/
  0,                                    /*tp_str*/
  0,                                    /*tp_getattro*/
  0,                                    /*tp_setattro*/
  0,                                    /*tp_as_buffer*/
  Py_TPFLAGS_DEFAULT,                   /*tp_flags*/
  "libwebsock objects",                 /*tp_doc*/
  0,                                    /*tp_traverse*/
  0,                                    /*tp_clear*/
  0,                                    /*tp_richcompare*/
  0,                                    /*tp_weaklistoffset*/
  0,                                    /*tp_iter*/
  0,                                    /*tp_iternext*/
  0,                                    /*tp_methods*/
  0,                                    /*tp_members*/
  libwebsock_ClientStateGetSet,         /*tp_getset*/
  0,                                    /*tp_base*/
  0,                                    /*tp_dict*/
  0,                                    /*tp_descr_get*/
  0,                                    /*tp_descr_set*/
  0,                                    /*tp_dictoffset*/
  0,                                    /*tp_init*/
  0,                                    /*tp_alloc*/
  0,                                    /*tp_new*/
};


static int ws_onopen(libwebsock_client_state *state)
{
  PyObject *stateObject;
  PyObject *arglist;

  stateObject = PyObject_CallObject((PyObject *)&libwebsock_ClientStateType, NULL);
  ((libwebsock_ClientStateObject *)stateObject)->state = state;

  if(PyList_Append(connected_clients_list, stateObject) != 0) {
    Py_DECREF(stateObject);
  }

  if(onopen_callback) {
    arglist = Py_BuildValue("(O)", stateObject);
    PyObject_CallObject(onopen_callback, arglist);
    Py_DECREF(arglist);
  }
  return 0;
}

static int ws_onclose(libwebsock_client_state *state)
{
  //generate python client state object and call python callable
  PyObject *stateObject = NULL;
  PyObject *arglist;

  PyObject **clients_array;

  Py_ssize_t clients_len;
  int clients_idx;

  clients_len = PySequence_Size(connected_clients_list);
  clients_array = PySequence_Fast_ITEMS(connected_clients_list);

  for(clients_idx = 0; clients_idx < clients_len; clients_array++, clients_idx++) {
    if(*clients_array && ((libwebsock_ClientStateObject *)*clients_array)->state == state) {
      stateObject = *clients_array;
      break;
    }
  }

  if(stateObject) {
    PySequence_DelItem(connected_clients_list, PySequence_Index(connected_clients_list, stateObject));
    if(onclose_callback) {
      arglist = Py_BuildValue("(O)", stateObject);
      PyObject_CallObject(onclose_callback, arglist);
      Py_DECREF(arglist);
    }
    Py_DECREF(stateObject);
  }
  return 0;
}

static int ws_onmessage(libwebsock_client_state *state, libwebsock_message *msg)
{
  //generate python objects and call python callable

  PyObject *stateObject = NULL;
  PyObject *arglist;

  PyObject **clients_array;

  Py_ssize_t clients_len;
  int clients_idx;

  clients_len = PySequence_Size(connected_clients_list);
  clients_array = PySequence_Fast_ITEMS(connected_clients_list);

  for(clients_idx = 0; clients_idx < clients_len; clients_array++, clients_idx++) {
    if(*clients_array && ((libwebsock_ClientStateObject *)*clients_array)->state == state) {
      stateObject = *clients_array;
      break;
    }
  }

  if(onmessage_callback && stateObject) {
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
  connected_clients_list = PyList_New(0);
  m = Py_InitModule("libwebsock", LibwebsockMethods);
  Py_INCREF(&libwebsock_ClientStateType);
  PyModule_AddObject(m, "ClientState", (PyObject *)&libwebsock_ClientStateType);
}

static PyObject *libwebsockpy_ClientState_getaddr(libwebsock_ClientStateObject *ClientState, void *closure)
{
  char hbuf[NI_MAXHOST];
  int rv;
  rv = getnameinfo((struct sockaddr *)ClientState->state->sa, sizeof(struct sockaddr_storage), hbuf, sizeof(hbuf), NULL, 0, NI_NUMERICHOST);
  if(rv == 0) {
    return PyString_FromString(hbuf);
  } else {
    fprintf(stderr, "Error getting ip address.\n");
    return NULL;
  }
}

static PyObject *libwebsockpy_ClientState_getsockfd(libwebsock_ClientStateObject *ClientState, void *closure)
{
  return PyInt_FromLong(ClientState->state->sockfd);
}

static PyObject *libwebsockpy_send(PyObject *self, PyObject *args)
{
  char *message;
  libwebsock_client_state *state;
  PyObject *stateObject;

  if(!PyArg_ParseTuple(args, "Os", &stateObject, &message)) {
    return NULL;
  }

  state = ((libwebsock_ClientStateObject *)stateObject)->state;
  libwebsock_send_text(state, message);
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *libwebsockpy_run(PyObject *self, PyObject *args)
{
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



static PyObject *libwebsockpy_onopen(PyObject *self, PyObject *args)
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

static PyObject *libwebsockpy_onclose(PyObject *self, PyObject *args)
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

static PyObject *libwebsockpy_onmessage(PyObject *self, PyObject *args)
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

static PyObject *libwebsockpy_connected_clients(PyObject *self)
{
  Py_INCREF(connected_clients_list);
  return connected_clients_list;
}

