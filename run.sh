if [ "$1" = "live" ]; then
    SDK/bin/artoolkitx_stereo_stylization_coherence --vconfl "-module=V4L2 -format=BGRA" --vconfr "-module=Image -image=video/0000L.jpg"
elif [ "$1" = "single" ]; then
    cd video; ../SDK/bin/artoolkitx_stereo_stylization_coherence --vconfl "-module=Image -image=ar2.jpg" --vconfr "-module=Image -image=ar2.jpg"
else
    cd video; ../SDK/bin/artoolkitx_stereo_stylization_coherence --vconfl "-module=Image $(python video/arguments.py L)" --vconfr "-module=Image $(python video/arguments.py R)"
fi
