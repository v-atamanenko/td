rm -rf build_native

mkdir build_native && cd build_native
cmake ..
cmake --build . --target prepare_cross_compiling
