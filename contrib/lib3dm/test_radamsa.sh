#!/bin/bash
while true
 do
   cat $(find Samples/ | shuf -n 1) | ./radamsa > fuzzed.3dm
   ./main fuzzed.3dm
   test $? -gt 127 && break
 done
