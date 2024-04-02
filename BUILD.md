# Compile MKFNet

cd MKFNet
mkdir build
cd build
cmake .. -G "Ninja"
ninja

# Run MKFNetTest

cd MKFNetTest
dotnet run

# Cross-Compile MKFNet

sudo apt-get install texinfo
git clone https://github.com/Zeranoe/mingw-w64-build.git
cd mingw-w64-build/
./mingw-w64-build x86_64 -r /opt/mingw-w64
export PATH=$PATH:/opt/mingw-w64

git clone git@github.com:OpenMagnetics/MKFNet.git
cd MKFNet/MKFNet
mkdir build
cd build
wget https://gist.githubusercontent.com/ebraminio/2e32c8f6d032a8e01606f7f564d2b1ee/raw/191e770c28e1c73644986f272809e79628230a92/mingw-w64-x86_64.cmake
cmake -DCMAKE_TOOLCHAIN_FILE=./mingw-w64-x86_64.cmake .. -G "Ninja"
ninja