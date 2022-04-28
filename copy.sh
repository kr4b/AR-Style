cp ../assets/cube_diorama/*.png video
cd video
mogrify -format jpg "*.png"
rm *.png
