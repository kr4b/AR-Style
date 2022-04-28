
if [ $1 = "live" ]; then
    firejail --net=none --whitelist=~/projects/RP/source/artoolkitx prime-run SDK/bin/artoolkitx_square_tracking_example --vconf "-module=V4L2 -format=BGRA"
# elif [ $1 = "" ]; then
#     firejail --net=none --private=./ prime-run SDK/bin/artoolkitx_square_tracking_example --vconf "-module=V4L2 -width=1280 -height=720 -dev=/dev/video4 -format=BGRA"
elif [ $1 = "single" ]; then
    firejail --net=none --whitelist=~/projects/RP/source/artoolkitx sh -c "cd video; prime-run ../SDK/bin/artoolkitx_square_tracking_example --vconf \"-module=Image -image=0000.jpg\""
else
    firejail --net=none --whitelist=~/projects/RP/source/artoolkitx sh -c "cd video; prime-run ../SDK/bin/artoolkitx_square_tracking_example --vconf \"-module=Image $(python video/arguments.py) -loop\""
fi
