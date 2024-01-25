# Compile MKFNet

cd MKFNet
mkdir build
cd build
cmake .. -G "Ninja"
ninja

# Run MKFNetTest

cd MKFNetTest
dotnet run
