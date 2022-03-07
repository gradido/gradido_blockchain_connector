mkdir build
cd build
conan install .. -s build_type=Debug
cmake .. 
make protoc
cmake ..
