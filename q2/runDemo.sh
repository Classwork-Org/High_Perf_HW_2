#!/bin/bash
for ((i=1;i<=16;i*=2)); do echo THRD_$i; ./main 1000000000 $i; done