cmake_minimum_required(VERSION 3.18.2)
cmake_policy(VERSION 3.18.2)
project(GradidoBlockchainConnector C CXX)
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "bin" )

IF(WIN32)
	set(CMAKE_CXX_FLAGS         "/MP /EHsc /std:c++17")
ELSE()
	SET(CMAKE_CXX_FLAGS "-std=c++17" )
ENDIF()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(INSTALL_BINDIR "bin")
set(INSTALL_PLUGINDIR "bin")

include_directories(
	"dependencies" 
	"dependencies/rapidjson/include"
	"dependencies/gradido_blockchain/include"
	"dependencies/gradido_blockchain/build"
	"dependencies/gradido_blockchain/dependencies/protobuf/third_party/googletest/googletest/include"
	"dependencies/gradido_blockchain/dependencies/protobuf/src"
	"build"
	"build/dependencies/gradido_blockchain/include"
	"iota_rust_clib/include"
	"src"
)

IF(UNIX)
	include_directories(
		"dependencies/poco/Crypto/include" 
#		"dependencies/poco/Data/include" 
		"dependencies/poco/Util/include" 
		"dependencies/poco/Foundation/include"
#		"dependencies/poco/JSON/include"
		"dependencies/poco/Net/include"
		"dependencies/poco/NetSSL_OpenSSL/include"
	)
ELSE()
	include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
	conan_basic_setup()
ENDIF()

############################## bind source files ###################################

FILE(GLOB JSONInterface "src/JSONInterface/*.h" "src/JSONInterface/*.cpp")
FILE(GLOB MODEL "src/model/*.h" "src/model/*.cpp")
FILE(GLOB MODEL_TABLE "src/model/table/*.cpp" "src/model/table/*.h")
FILE(GLOB MAIN "src/*.cpp" "src/*.c"  "src/*.h")

SET(LOCAL_SRCS 
	${MAIN}
	${MODEL} ${MODEL_TABLE}
	${JSONInterface} 
)

aux_source_directory("src" LOCAL_SRCS)

if(MSVC)
	# src
	source_group("Json-Interface" FILES ${JSONInterface})	
	source_group("model\\table" FILES ${MODEL_TABLE})
	source_group("model" FILES ${MODEL})
endif()


if(WIN32)
#	MESSAGE("Poco Setup: ${CMAKE_BINARY_DIR}")
	include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
	conan_basic_setup()
	#message("conan lib debugs earlier: ${CONAN_LIBS_DEBUG}")
endif()

############################## config and add poco ###################################
#SET(SOME_EXPAT_OPTION OFF CACHE BOOL "Use some expat option")
IF(UNIX)
	SET(ENABLE_MONGODB OFF CACHE BOOL "" FORCE) 
	SET(ENABLE_DATA_SQLITE OFF CACHE BOOL "" FORCE) 
	SET(ENABLE_DATA_MYSQL ON CACHE BOOL "" FORCE)
	SET(ENABLE_REDIS OFF CACHE BOOL "" FORCE)
	SET(ENABLE_PAGECOMPILER_FILE2PAGE OFF CACHE BOOL "" FORCE)
	add_subdirectory("dependencies/poco")

	set(POCO_LIBS Poco::Foundation Poco::Data Poco::Util Poco::Net Poco::NetSSL Poco::JWT Poco::DataMySQL)
ENDIF()

############################## find protobuf  #########################################
#add_subdirectory(dependencies/protobuf/cmake)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
# prevent problems with two libs including googletest 
option(protobuf_BUILD_TESTS "Build tests" OFF)

#message("binary dir: ${CMAKE_BINARY_DIR}")
add_subdirectory(dependencies/gradido_blockchain/dependencies/protobuf/cmake)

############################## add gradido blockchain  #########################################
add_subdirectory("dependencies/gradido_blockchain")

if(NOT BUILD_SHARED_LIBS)
	#option(protobuf_BUILD_TESTS "Build tests" OFF)
	#add_subdirectory("dependencies/gradido_blockchain/dependencies/protobuf/cmake")
endif()

add_executable(GradidoBlockchainConnector ${LOCAL_SRCS})

############################# add rust and iota.rs ####################
# https://webdevolutions.blob.core.windows.net/blog/pdf/insider-series-using-rust-code-in-a-cc-project-with-cmake.pdf
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
enable_language(Rust)
include(CMakeCargo)
add_subdirectory(iota_rust_clib)
######################################################################

if(WIN32)
	#MESSAGE("Poco Setup: ${CMAKE_BINARY_DIR}")
#	include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
#	conan_basic_setup()
	#link_directories(
	#	${CONAN_LIB_DIRS_POCO}
	#	${CONAN_LIB_DIRS_LIBSODIUM}
	#)
	#link_directories(${CONAN_LIB_DIRS})
	#message("poco lib dir from conan: ${CONAN_LIB_DIRS_POCO}") 
	#message("libsodium lib dir from conan: ${CONAN_LIB_DIRS_LIBSODIUM}")
	#message("conan lib debugs: ${CONAN_LIBS_DEBUG}")
	#message("conan build modules path: ${CONAN_BUILD_MODULES_PATHS}")
	target_link_libraries(GradidoBlockchainConnector ${CONAN_LIBS} libprotobuf)
endif()

target_link_libraries(GradidoBlockchainConnector $<TARGET_OBJECTS:GradidoBlockchain> iota_rust_clib)
if(WIN32)
	# needed for windows if linking to a rust lib
	target_link_libraries(GradidoBlockchainConnector ws2_32 userenv)
endif()

############################## build gradido blockchain connector ###################################



# install 
if(UNIX)
	target_link_libraries(GradidoBlockchainConnector ${POCO_LIBS} sodium ${MPFR_LIBRARIES} libprotobuf)
	install(TARGETS GradidoBlockchainConnector RUNTIME DESTINATION /usr/local/bin)
	get_target_property(OUT GradidoBlockchainConnector LINK_LIBRARIES)
	message(STATUS ${OUT})
endif()

#  ----------------------    Test -----------------------------------------
add_subdirectory("test")

