# exit when any command fails
set -e

for VARIABLE in android_armv7 android_arm64_v8a android_x86 android_x86_64 gcc_64
do
    rm -rf build-$VARIABLE
    mkdir build-$VARIABLE
    cd build-$VARIABLE
    chmod a+x $Qt6_DIR_BASE/$VARIABLE/bin/qt-configure-module
    chmod a+x $Qt6_DIR_BASE/$VARIABLE/bin/qt-cmake-private
    $Qt6_DIR_BASE/$VARIABLE/bin/qt-configure-module ..
    cmake -DMBGL_QT_WITH_INTERNAL_ICU:BOOL=On .
    ninja
    ninja install
    cd ..    
done
