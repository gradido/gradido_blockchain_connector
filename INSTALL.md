## For Linux Build

sudo apt install libsodium-dev libmariadb3 libmariadb-dev protobuf-compiler libprotobuf-dev
sudo ln -s /usr/include/mariadb /usr/include/mysql
install mpfr if not exist:
https://www.mpfr.org/mpfr-current/mpfr.html#Installing-MPFR

## For Windows Build

mkdir build
cd build
conan install .. -s build_type=Debug
cmake .. 
make protoc
cmake ..
