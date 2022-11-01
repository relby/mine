#!/bin/sh

set -xe

cc -Wall -Wextra -std=c11 -pedantic *.c -o main.out
