from distutils.core import setup, Extension

libwebsock_module = Extension('libwebsock', sources=['libwebsock.c'],
    libraries=['websock'],
    library_dirs = ['/usr/local/lib'])

setup(name = 'libwebsock', version='1.0', description='libwebsock', ext_modules=[libwebsock_module])

