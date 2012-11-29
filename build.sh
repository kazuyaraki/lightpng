#! /bin/sh
scons -c --no-opensource
scons -c

if [ -d build ] ; then
    rm -rf build
fi

mkdir build
mkdir build/lightpng_mac_opensource
mkdir build/lightpng_mac_noopensource
mkdir build/lightpng_win_opensource

scons
mv lightpng build/lightpng_mac_opensource/
cp LICENSE.rst build/lightpng_mac_opensource/
cp README.rst build/lightpng_mac_opensource/
cp README_JP.rst build/lightpng_mac_opensource/
zip -9 -r build/lightpng_mac_opensource_`date +%Y%m%d`.zip build/lightpng_mac_opensource

scons --no-opensource
mv lightpng build/lightpng_mac_noopensource/
cp LICENSE.rst build/lightpng_mac_noopensource/
cp README.rst build/lightpng_mac_noopensource/
cp README_JP.rst build/lightpng_mac_noopensource/
cp LICENSE_PVRTexLib.rst build/lightpng_mac_noopensource/
zip -9 -r build/lightpng_mac_noopensource_`date +%Y%m%d`.zip build/lightpng_mac_noopensource

scons --mingw32
mv lightpng.exe build/lightpng_win_opensource/
cp LICENSE.rst build/lightpng_win_opensource/
cp README.rst build/lightpng_win_opensource/
cp README_JP.rst build/lightpng_win_opensource/
zip -9 -r build/lightpng_win_opensource_`date +%Y%m%d`.zip build/lightpng_win_opensource

