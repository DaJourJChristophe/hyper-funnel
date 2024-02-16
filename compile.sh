#!/bin/bash

set -e

/usr/bin/gcc -c -Iinclude -fPIC -o src/observable.o src/observable.c
/usr/bin/gcc -c -Iinclude -fPIC -o src/observer.o src/observer.c

/usr/bin/gcc -shared -o libexec/libhyperfunnel.so \
  src/observable.o \
  src/observer.o




/usr/bin/gcc -c -Iinclude -o examples/basic.o examples/basic.c
/usr/bin/gcc -Llibexec -o bin/basic examples/basic.o -lpthread -ljemalloc -lturnpike -lhyperfunnel

rm -rf examples/*.o src/*.o
