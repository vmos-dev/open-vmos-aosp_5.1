#!/bin/bash

for img in *.png *.gif *.jpg
do
  FOUND=`grep -R $img ../site_src`
  if [ -z "$FOUND" ]
  then
    mv $img useless/
  fi
done
