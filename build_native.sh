rm -rf build_native
rm -rf build

mkdir build_native && cd build_native
cmake ..
cmake --build . --target prepare_cross_compiling
