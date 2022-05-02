#!/bin/sh

cp ../assets/cube_diorama/*.png video
cp ../assets/cube_diorama/*.png video
cd video
mogrify -format jpg "*.png"
rm *.png
# for f in *.jpg; do
#     mv -- "$f" "${f#000}" 2>/dev/null
#     mv -- "$f" "${f#00}" 2>/dev/null
#     mv -- "$f" "${f#0}" 2>/dev/null
# done
# rename .jpg "" *.jpg
