cd Source/build-linux-x86_64/
cmake .. -DCMAKE_BUILD_TYPE=${DEBUG+Debug}${DEBUG-Release}
make
make install
