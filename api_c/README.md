# API core

> C implementation of the Koheron API core functions

Multiplatform library to communicate with KServer. The library is coded in C to allow easy integration with as many programs as possible.

## Options to integrate C into Python

Interesting [discussion](http://stackoverflow.com/questions/1942298/wrapping-a-c-library-in-python-c-cython-or-ctypes)

### ctypes

Allows to link a library (DLL or so).

[documentation](https://docs.python.org/2/library/ctypes.html#module-ctypes)

We choose that since it allows to develop an *external library* that can be used outside of any Python context, in a C/C++ code for example.

### Python API

Using Python API, it is possible to access [Python run-time](https://docs.python.org/2/extending/extending.html).

But works only with CPython.

## Windows cross-compilation

[Using MinGW](http://arrayfire.com/cross-compile-to-windows-from-linux/)

Install MinGW:
```sh
$ sudo apt-get install mingw-w64
```

Don't forget to clean before compiling:
```sh
$ make clean all
```
Otherwise a previously compiled object file for Linux would induce an error during the DLL linking.

## GDB and Python

To use GDB on a Python script run, you need to install ```python-dbg```:
```sh
$ sudo apt-get install python-dbg
```

To debug ```script.py```. First go into the directory of the script:
```sh
$ cd /path/to/script
```

Then call GDB:
```sh
$ gdb /usr/bin/python script.py
```

Then
```sh
(gdb) run
```

The Python console then starts. You can now launch the script:
```sh
>>> execfile("script.py")
```
