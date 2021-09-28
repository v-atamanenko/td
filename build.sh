cp -r vitaports/* /usr/local/vitasdk/arm-vita-eabi/include/

rm -rf build
mkdir build && cd build

export CROSS=/usr/local/vitasdk/bin/arm-vita-eabi
export CC=${CROSS}-gcc
export CXX=${CROSS}-g++
export CMAKE_PREFIX_PATH=/usr/local/vitasdk/arm-vita-eabi/

echo "STAGE: Configuring compiler for cross-compilation"

cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_CROSSCOMPILING=True -DOPENSSL_INCLUDE_DIR=/usr/local/vitasdk/arm-vita-eabi/include/ -DOPENSSL_LIBRARIES="/usr/local/vitasdk/arm-vita-eabi/lib/libssl.a;/usr/local/vitasdk/arm-vita-eabi/lib/libcrypto.a" ..

echo "STAGE: Compilation"
rm -rf tdutils/generate
cp -r ../build_native/tdutils/generate tdutils/
rm -rf td/generate
cp -r ../build_native/td/generate td/


cmake --build .