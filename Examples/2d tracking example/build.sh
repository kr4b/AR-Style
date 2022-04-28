cd Linux/build
cmake .. -DCMAKE_BUILD_TYPE=${DEBUG+Debug}${DEBUG-Release}
make install
