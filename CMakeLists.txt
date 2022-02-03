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
	"dependencies/gradido_blockchain/src/cpp"
	"dependencies/gradido_blockchain/build/proto"
	"dependencies/protobuf/src"
	"build"
	"src/cpp"
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

FILE(GLOB JSONInterface "src/cpp/JSONInterface/*.h" "src/cpp/JSONInterface/*.cpp")

FILE(GLOB MAIN "src/cpp/*.cpp" "src/cpp/*.c"  "src/cpp/*.h")

# used only for test project 
FILE(GLOB TEST "src/cpp/test/*.cpp" "src/cpp/test/*.h")
FILE(GLOB TEST_JSON_INTERFACE "src/cpp/test/JSONInterface/*.cpp" "src/cpp/test/JSONInterface/*.h")
FILE(GLOB TEST_LIB "src/cpp/test/lib/*.cpp" "src/cpp/test/lib/*.h")

SET(LOCAL_SRCS 
	${MAIN}
	${JSONInterface} 
)
SET(LOCAL_TEST_SRC
	${TEST} ${TEST_MODEL} ${TEST_JSON_INTERFACE} ${TEST_LIB}
)
aux_source_directory("src/cpp" LOCAL_SRCS)

if(MSVC)
	# src
	source_group("Json-Interface" FILES ${JSONInterface})	
	source_group("Test\\Json-Interface" FILES ${TEST_JSON_INTERFACE})
	source_group("Test\\lib" FILES ${TEST_LIB})
	source_group("Test" FILES ${TEST})
endif()



add_executable(GradidoBlockchainConnector ${LOCAL_SRCS})
add_subdirectory("dependencies/gradido_blockchain")
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


############################## build gradido blockchain connector ###################################


if(WIN32)
MESSAGE(${CMAKE_BINARY_DIR})
	include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
	conan_basic_setup()

	target_link_libraries(GradidoBlockchainConnector ${CONAN_LIBS})
else() # unix
	target_link_libraries(GradidoBlockchainConnector ${POCO_LIBS})
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
#find_package(GTest CONFIG REQUIRED)
#add_subdirectory("dependencies/protobuf/third_party/googletest")


add_executable(GradidoBlockchainConnectorTest ${LOCAL_SRCS} ${LOCAL_TEST_SRC})
target_compile_definitions(GradidoBlockchainConnectorTest PUBLIC "_TEST_BUILD")

if(WIN32)
	target_link_libraries(GradidoBlockchainConnectorTest ${CONAN_LIBS} libprotobuf)
else()
#	include(FetchContent)
#	FetchContent_Declare(
#	googletest
#	URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
#	)
#	FetchContent_MakeAvailable(googletest)
	target_link_libraries(GradidoBlockchainConnectorTest ${POCO_LIBS} sodium gtest libprotobuf)
endif()

add_test(NAME main COMMAND GradidoBlockchainConnectorTest)
