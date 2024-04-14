#!/bin/bash
gcc alex-pi.cpp serial.cpp serialize.cpp -pthread -o alex-pi -lncurses
