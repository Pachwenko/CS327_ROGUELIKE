#!/usr/bin/env bash

# renames all .cpp files to .c

for f in *.cpp; do mv "$f" "${f%.cpp}.c"; done
