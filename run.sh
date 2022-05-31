if [ "$1" = "live" ]; then
    firejail --net=none --whitelist=~/projects/RP/source/artoolkitx SDK/bin/artoolkitx_square_tracking_example --vconfl "-module=V4L2 -format=BGRA" --vconfr "-module=Image -image=video/0000.jpg"
# elif [ $1 = "" ]; then
#     firejail --net=none --private=./ prime-run SDK/bin/artoolkitx_square_tracking_example --vconf "-module=V4L2 -width=1280 -height=720 -dev=/dev/video4 -format=BGRA"
elif [ "$1" = "single" ]; then
    firejail --net=none --whitelist=~/projects/RP/source/artoolkitx sh -c "cd video; ../SDK/bin/artoolkitx_square_tracking_example --vconfl \"-module=Image -image=ar2.jpg\" --vconfr \"-module=Image -image=ar2.jpg\""
else
    firejail --net=none --whitelist=~/projects/RP/source/artoolkitx sh -c "cd video; ../SDK/bin/artoolkitx_square_tracking_example --vconfl \"-module=Image $(python video/arguments.py L)\" --vconfr \"-module=Image $(python video/arguments.py R)\""
fi
