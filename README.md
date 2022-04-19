## For Linux Build

sudo apt install libsodium-dev libmariadb3 libmariadb-dev protobuf-compiler libprotobuf-dev
sudo ln -s /usr/include/mariadb /usr/include/mysql


## For Windows Build

mkdir build
cd build
conan install .. -s build_type=Debug
cmake .. 
make protoc
cmake ..
