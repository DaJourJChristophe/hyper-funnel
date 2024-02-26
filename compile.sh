#!/bin/bash

set -e

/usr/bin/gcc -c -Iinclude -fPIC -ggdb3 -o src/internal/command.o src/internal/command.c
/usr/bin/gcc -c -Iinclude -fPIC -ggdb3 -o src/internal/util.o src/internal/util.c

/usr/bin/gcc -c -Iinclude -fPIC -ggdb3 -o src/channel.o src/channel.c
/usr/bin/gcc -c -Iinclude -fPIC -ggdb3 -o src/command.o src/command.c
/usr/bin/gcc -c -Iinclude -fPIC -ggdb3 -o src/load_balance.o src/load_balance.c
/usr/bin/gcc -c -Iinclude -fPIC -ggdb3 -o src/observable.o src/observable.c
/usr/bin/gcc -c -Iinclude -fPIC -ggdb3 -o src/observer.o src/observer.c
/usr/bin/gcc -c -Iinclude -fPIC -ggdb3 -o src/scheduler.o src/scheduler.c

/usr/bin/gcc -shared -o libexec/libhyperfunnel.so \
  src/internal/command.o \
  src/internal/util.o \
  src/channel.o \
  src/command.o \
  src/load_balance.o \
  src/observable.o \
  src/observer.o \
  src/scheduler.o




/usr/bin/gcc -c -Iinclude -ggdb3 -o examples/basic.o examples/basic.c
/usr/bin/gcc -Llibexec -o bin/basic examples/basic.o -lpthread -ljemalloc -lturnpike -lhyperfunnel

rm -rf examples/*.o src/*.o src/**/*.o
