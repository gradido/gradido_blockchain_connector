cmake_minimum_required(VERSION 3.18.2)
project(GradidoBlockchainConnector C CXX)
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "bin" )

SET(CMAKE_CXX_FLAGS "-std=c++17" )
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

IF(WIN32)
	set(CMAKE_CXX_FLAGS         "/MP /EHsc")
ENDIF()

set(INSTALL_BINDIR "bin")
set(INSTALL_PLUGINDIR "bin")

include_directories(
	"dependencies" 
	"dependencies/rapidjson/include"
	"dependencies/gradido_blockchain/include"
	"dependencies/gradido_blockchain/build"
	"dependencies/protobuf/src"
	"dependencies/gradido_blockchain/dependencies/protobuf/third_party/googletest/googletest/include"
	"build"
	"build/dependencies/gradido_blockchain/include"
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
ENDIF()

############################## bind source files ###################################

FILE(GLOB JSONInterface "src/JSONInterface/*.h" "src/JSONInterface/*.cpp")

FILE(GLOB MAIN "src/*.cpp" "src/*.c"  "src/*.h")

SET(LOCAL_SRCS 
	${MAIN}
	${JSONInterface} 
)

aux_source_directory("src" LOCAL_SRCS)

if(MSVC)
	# src
	source_group("Json-Interface" FILES ${JSONInterface})	
endif()


if(WIN32)
#	MESSAGE("Poco Setup: ${CMAKE_BINARY_DIR}")
	include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
	conan_basic_setup()
	#message("conan lib debugs earlier: ${CONAN_LIBS_DEBUG}")
endif()

add_subdirectory("dependencies/gradido_blockchain")

if(NOT BUILD_SHARED_LIBS)
	#option(protobuf_BUILD_TESTS "Build tests" OFF)
	#add_subdirectory("dependencies/gradido_blockchain/dependencies/protobuf/cmake")
endif()

add_executable(GradidoBlockchainConnector ${LOCAL_SRCS})

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
	target_link_libraries(GradidoBlockchainConnector ${CONAN_LIBS})
endif()

target_link_libraries(GradidoBlockchainConnector $<TARGET_OBJECTS:GradidoBlockchain>)
############################## config and add poco ###################################
#SET(SOME_EXPAT_OPTION OFF CACHE BOOL "Use some expat option")
IF(UNIX)
	SET(ENABLE_MONGODB OFF CACHE BOOL "" FORCE) 
	SET(ENABLE_DATA_SQLITE OFF CACHE BOOL "" FORCE) 
	SET(ENABLE_REDIS OFF CACHE BOOL "" FORCE)
	SET(ENABLE_PAGECOMPILER_FILE2PAGE OFF CACHE BOOL "" FORCE)
	add_subdirectory("dependencies/poco")

	set(POCO_LIBS Poco::Foundation Poco::Util Poco::Net Poco::NetSSL)
ENDIF()

############################## find protobuf  #########################################
#add_subdirectory(dependencies/protobuf/cmake)
target_link_libraries(GradidoBlockchainConnector libprotobuf)

############################## build gradido blockchain connector ###################################



# install 
if(UNIX)
	target_link_libraries(GradidoBlockchainConnector ${POCO_LIBS} sodium)
	install(TARGETS GradidoBlockchainConnector RUNTIME DESTINATION /usr/local/bin)
endif()

#  ----------------------    Test -----------------------------------------
add_subdirectory("test")

