cmake_minimum_required(VERSION 3.18.2)
project(GradidoBlockchainConnectorTest)

# GoogleTest requires at least C++11
set(CMAKE_CXX_STANDARD 11)

# collect source code files
FILE(GLOB TEST_MAIN "*.cpp" "*.h")
FILE(GLOB TEST_LIB "lib/*.cpp" "lib/*.h")
FILE(GLOB TEST_JSON_INTERFACE "JSONInterface/*.cpp" "JSONInterface/*.h")
FILE(GLOB SERVER_CONFIG "../src/ServerConfig.*")
FILE(GLOB JSON_INTERFACE "../src/JSONInterface/*.cpp" "../src/JSONInterface/*.h")
FILE(GLOB PROTO_FILES 
    "../dependencies/gradido_blockchain/build/proto/gradido/*.h"
    "../dependencies/gradido_blockchain/build/proto/gradido/*.cc"
)

SET(TEST_SRC ${TEST_MAIN} ${SERVER_CONFIG} ${TEST_LIB} ${TEST_JSON_INTERFACE} ${JSON_INTERFACE} ${PROTO_FILES}) 

if(MSVC)
	# src
	source_group("Test\\lib" FILES ${TEST_LIB})
    source_group("Test\\JSONInterface" FILES ${TEST_JSON_INTERFACE})
    source_group("JSONInterface" FILES ${JSON_INTERFACE})    
    source_group("proto" FILES ${PROTO_FILES})
endif()

IF(WIN32)
    include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    conan_basic_setup()
ENDIF()

enable_testing()

#include(FetchContent)
#FetchContent_Declare(
 # googletest
  #URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
#)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
#FetchContent_MakeAvailable(googletest)

add_executable(GradidoBlockchainConnectorTest ${TEST_SRC})
target_link_libraries(GradidoBlockchainConnectorTest GradidoBlockchain libprotobuf)
IF(WIN32) 
    target_link_libraries(GradidoBlockchainConnectorTest ${CONAN_LIBS})
ENDIF()

include(GoogleTest)
gtest_discover_tests(GradidoBlockchainConnectorTest)