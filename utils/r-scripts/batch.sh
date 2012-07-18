#!/bin/bash

for p in `ls -1 $1`; do
  echo "Processing $p"
  Rscript visualizator.r "$1$p"
done
