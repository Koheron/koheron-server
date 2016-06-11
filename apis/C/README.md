
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
