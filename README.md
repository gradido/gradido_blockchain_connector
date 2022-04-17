## For Linux Build

sudo apt install libsodium-dev libmariadb3 libmariadb-dev
sudo ln -s /usr/local/mariadb /usr/local/mysql
## For Windows Build

mkdir build
cd build
conan install .. -s build_type=Debug
cmake .. 
make protoc
cmake ..
