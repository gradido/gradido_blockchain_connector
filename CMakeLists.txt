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
	"build"
	"build/proto"
	"build/http_pages"
	"src/cpp"
)

IF(UNIX)
	include_directories(
#		"dependencies/poco/Crypto/include" 
#		"dependencies/poco/Data/include" 
		"dependencies/poco/Util/include" 
		"dependencies/poco/Foundation/include"
#		"dependencies/poco/JSON/include"
		"dependencies/poco/Net/include"
		"dependencies/poco/NetSSL_OpenSSL/include"
	)
ENDIF()

############################## find protobuf  #########################################
add_subdirectory(dependencies/protobuf/cmake)

############################## parse protobuf files ###################################

FILE(TO_NATIVE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/src/proto GRADIDO_PROTO_MODEL_PATH)
FILE(TO_NATIVE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/build/proto PROTOBINDING_PATH)
file(MAKE_DIRECTORY ${PROTOBINDING_PATH})
file(MAKE_DIRECTORY ${PROTOBINDING_PATH}/gradido)

FILE(GLOB DATAMODEL_GRADIDO_PROTOS "${GRADIDO_PROTO_MODEL_PATH}/gradido/*.proto")

IF(WIN32)
	set(PROTOC_BIN_PATH "build/dependencies/protobuf/cmake/bin")
	find_program(PROTOBUF_PROTOC_EXECUTABLE protoc PATHS "${PROTOC_BIN_PATH}/Debug" )
	find_program(PROTOBUF_PROTOC_EXECUTABLE protoc PATHS "${PROTOC_BIN_PATH}/Release" )
ELSE()
	find_program(PROTOBUF_PROTOC_EXECUTABLE protoc PATHS "build/dependencies/protobuf/cmake/bin" )
ENDIF()
FILE(TO_NATIVE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/protobuf/src GOOGLE_PROTOBUF_INCLUDES)
MESSAGE("protoc: ${PROTOBUF_PROTOC_EXECUTABLE} in build/dependencies/protobuf/bin/${CMAKE_BUILD_TYPE}")
FOREACH(proto ${DATAMODEL_GRADIDO_PROTOS})
   FILE(TO_NATIVE_PATH ${proto} proto_native)
   get_filename_component(proto_parsed ${proto} NAME_WLE)
   FILE(TO_NATIVE_PATH ${PROTOBINDING_PATH}/gradido/${proto_parsed}.pb.h proto_parsed_native)
   
   IF(${proto_native} IS_NEWER_THAN ${proto_parsed_native})
		EXECUTE_PROCESS(
			COMMAND 
			${PROTOBUF_PROTOC_EXECUTABLE} 
			--proto_path=${GRADIDO_PROTO_MODEL_PATH} 
			--cpp_out=${PROTOBINDING_PATH} 
			${proto_native}
			RESULT_VARIABLE rv
		)
		# Optional, but that can show the user if something have gone wrong with the proto generation 
		IF(${rv})
			MESSAGE("Generation of data model returned ${rv} for proto ${proto_native}")
		ELSE()
			MESSAGE("Parsed: src/proto/gradido/${proto_parsed}.proto")
		ENDIF()
   ENDIF()

ENDFOREACH(proto)


############################## bind source files ###################################

FILE(GLOB CONTROLLER "src/cpp/controller/*.cpp" "src/cpp/controller/*.h")
FILE(GLOB JSONInterface "src/cpp/JSONInterface/*.h" "src/cpp/JSONInterface/*.cpp")
FILE(GLOB SINGLETON_MANAGER "src/cpp/SingletonManager/*.h" "src/cpp/SingletonManager/*.cpp")
FILE(GLOB LIB_SRC "src/cpp/lib/*.h" "src/cpp/lib/*.cpp")
FILE(GLOB MODEL "src/cpp/model/*.h" "src/cpp/model/*.cpp")
FILE(GLOB MODEL_GRADIDO "src/cpp/model/gradido/*.h" "src/cpp/model/gradido/*.cpp")
FILE(GLOB MAIN "src/cpp/*.cpp" "src/cpp/*.c"  "src/cpp/*.h")
FILE(GLOB PROTO_GRADIDO "build/proto/gradido/*.cc" "build/proto/gradido/*.h")

# used only for test project 
FILE(GLOB TEST "src/cpp/test/*.cpp" "src/cpp/test/*.h")
FILE(GLOB TEST_JSON_INTERFACE "src/cpp/test/JSONInterface/*.cpp" "src/cpp/test/JSONInterface/*.h")

SET(LOCAL_SRCS 
	${MAIN} 
	${JSONInterface} 
	${MODEL} ${MODEL_GRADIDO}
	${SINGLETON_MANAGER} ${LIB_SRC}
	${PROTO_GRADIDO} 
)
SET(LOCAL_TEST_SRC
	${TEST} ${TEST_MODEL} ${TEST_JSON_INTERFACE}
)
aux_source_directory("src/cpp" LOCAL_SRCS)

if(MSVC)
	# src
	source_group("proto\\gradido" FILES ${PROTO_GRADIDO})
	source_group("model\\gradido" FILES ${MODEL_GRADIDO})
	source_group("model" FILES ${MODEL})
	source_group("SingletonManager" FILES ${SINGLETON_MANAGER})
	source_group("lib" FILES ${LIB_SRC})
	source_group("Json-Interface" FILES ${JSONInterface})	
	source_group("Test\\Json-Interface" FILES ${TEST_JSON_INTERFACE})
	source_group("Test" FILES ${TEST})
endif()

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

add_executable(GradidoBlockchainConnector ${LOCAL_SRCS})

############################## config and add poco ###################################
#SET(SOME_EXPAT_OPTION OFF CACHE BOOL "Use some expat option")
IF(UNIX)
	SET(ENABLE_MONGODB OFF CACHE BOOL "" FORCE) 
	SET(ENABLE_DATA_SQLITE OFF CACHE BOOL "" FORCE) 
	SET(ENABLE_REDIS OFF CACHE BOOL "" FORCE)
	SET(ENABLE_PAGECOMPILER_FILE2PAGE OFF CACHE BOOL "" FORCE)
	add_subdirectory("dependencies/poco")

	set(POCO_LIBS PocoFoundation PocoUtil PocoNet PocoNetSSL)
ENDIF()


############################## build gradido blockchain connector ###################################


if(WIN32)
	target_link_libraries(GradidoBlockchainConnector libprotobuf ${CONAN_LIBS})
else() # unix
	target_link_libraries(GradidoBlockchainConnector ${POCO_LIBS} sodium libprotobuf)
endif()

# install 
if(UNIX)
install(TARGETS GradidoBlockchainConnector RUNTIME DESTINATION /usr/local/bin)
endif(UNIX)

#  ----------------------    Test -----------------------------------------

enable_testing()

option(COLLECT_COVERAGE_DATA "Use cov to collect coverage informations" OFF)
set(COVERAGE_TOOL "Coverage Tool (gcovr|lcov|fastcov)" CACHE STRING "gcovr")

if(COLLECT_COVERAGE_DATA)
	
	include(cmake/CodeCoverage.cmake)
	append_coverage_compiler_flags()
	set(EXCLUDE_FOR_HTML_COV 
		"${CMAKE_CURRENT_SOURCE_DIR}/build/proto/*" 
		"${CMAKE_CURRENT_SOURCE_DIR}/dependencies/*" 
		"${CMAKE_CURRENT_SOURCE_DIR}/src/cpp/test/*"
		"${CMAKE_CURRENT_SOURCE_DIR}/googletest/googletest/include/gtest/internal/*"
		"${CMAKE_CURRENT_SOURCE_DIR}/googletest/googletest/src/"
	)
	if("${COVERAGE_TOOL}" STREQUAL "gcovr")
		setup_target_for_coverage_gcovr_html(
			NAME coverage
			EXECUTABLE GradidoBlockchainConnectorTest
			EXCLUDE ${EXCLUDE_FOR_HTML_COV}
			GCOVR_ADDITIONAL_ARGS "--txt "
			#DEPENDENCIES lib/libmariadb.so.3
		)
	endif()

	set(EXCLUDE_FOR_COV 
		${EXCLUDE_FOR_HTML_COV}
		"/usr/include/*"
	)
	if("${COVERAGE_TOOL}" STREQUAL "lcov")
		setup_target_for_coverage_lcov(
			NAME coverage
			EXECUTABLE GradidoBlockchainConnectorTest
			EXCLUDE "${EXCLUDE_FOR_COV}"
			#DEPENDENCIES lib/libmariadb.so.3
		)
	endif()
	
	if("${COVERAGE_TOOL}" STREQUAL "fastcov")
		setup_target_for_coverage_fastcov(
	     NAME coverage                    # New target name
	     EXECUTABLE GradidoBlockchainConnectorTest -j ${PROCESSOR_COUNT} # Executable in PROJECT_BINARY_DIR
	     #BASE_DIRECTORY "../"                        # Base directory for report
	                                                 #  (defaults to PROJECT_SOURCE_DIR)
	     EXCLUDE "${EXCLUDE_FOR_COV}"                # Patterns to exclude.
	     NO_DEMANGLE                                 # Don't demangle C++ symbols
	                                                 #  even if c++filt is found
	     SKIP_HTML                                   # Don't create html report
	 )
	endif()

endif()

project(GradidoBlockchainConnectorTest C CXX)
#_TEST_BUILD

add_executable(GradidoBlockchainConnectorTest ${LOCAL_SRCS} ${LOCAL_TEST_SRC})
target_compile_definitions(GradidoBlockchainConnectorTest PUBLIC "_TEST_BUILD")

if(WIN32)
	target_link_libraries(GradidoBlockchainConnectorTest ${CONAN_LIBS} libprotobuf)
else()
	target_link_libraries(GradidoBlockchainConnectorTest ${POCO_LIBS} sodium gtest)
endif()

add_test(NAME main COMMAND GradidoBlockchainConnectorTest)
