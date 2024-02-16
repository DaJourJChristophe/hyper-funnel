#!/bin/bash

set -e

/usr/bin/gcc -c -Iinclude -o observable.o observable.c

/usr/bin/gcc -Llibexec -o bin/main observable.o -lpthread -ljemalloc -lturnpike

rm -rf *.o
