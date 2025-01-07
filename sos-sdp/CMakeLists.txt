cmake_minimum_required(VERSION 3.14)
project(MyProject)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_CXX_FLAGS_DEBUG_INIT "-Wall -g")
#set(CMAKE_CXX_FLAGS_DEBUG_INIT "-O2")
#set(CMAKE_CXX_FLAGS_DEBUG "-Wall -g")

# Debug build options
#set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -Wall -pedantic -fsanitize=undefined -fsanitize=address")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -Wall -fsanitize=undefined -fsanitize=address -fsanitize=signed-integer-overflow -fopenmp")

# Release build options
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -fopenmp")

# linker flags
# set(CMAKE_EXE_LINKER_FLAGS "-static")


include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Add the include directory to the include paths
include_directories(include)

# create file list
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Add the source files to the project
add_executable(sos-sdp
        ${SOURCES}
        main.cpp include/testproj.h
        src/testproj.cpp
        include/symbolicRing.h
        src/symbolicRing.cpp
        include/sdpEncoder.h
        src/sdpEncoder.cpp
        src/solver.cpp
        include/solver.h
        include/debugTools.h
        include/programExpression.h
        include/tokenizer.h
        include/program.h
        include/program.h
        include/programParser.h
        src/programParser.cpp
        include/expressionParser.h
        src/expressionParser.cpp
        include/automaitcComplexityEstimator.h
        include/combinatorics.h include/hacks.h
        include/pythonCodeGen.h
        include/stringRoutines.h
        src/stringRoutines.cpp
        include/sdpProblem.h include/templateEngine.h)



# Set the path to the CPLEX ILOG installation
#set(CPLEX_DIR "/opt/ibm/ILOG/CPLEX_Studio2211")

# Include the CPLEX ILOG header files
#include_directories(${CPLEX_DIR}/cplex/include)
#include_directories(${CPLEX_DIR}/concert/include)

# Link against the CPLEX ILOG libraries
#link_directories(${CPLEX_DIR}/cplex/lib/x86-64_linux/static_pic)
#link_directories(${CPLEX_DIR}/concert/lib/x86-64_linux/static_pic)

#target_link_libraries(sos-sdp
#        ${CPLEX_DIR}/cplex/lib/x86-64_linux/static_pic/libcplex.a
#        ${CPLEX_DIR}/concert/lib/x86-64_linux/static_pic/libconcert.a
#        -lm -lpthread -ldl
#        )


#set(MOSEK_DIR "/home/sergey/Soft/mosektoolslinux64x86/mosek/10.0/tools/platform/linux64x86")

if (DEFINED ENV{MOSEK_LIB})
    set(MOSEK_DIR $ENV{MOSEK_LIB})
else()
    add_definitions(-DNO_MOSEK)
    message("MOSEK not found")
endif()


message("MOSEK directory: ${MOSEK_DIR}")



include_directories(${MOSEK_DIR}/h)
link_directories(${MOSEK_DIR}/bin)
target_link_libraries(sos-sdp
        ${MOSEK_DIR}/bin/libmosek64.so
        )
target_link_libraries(sos-sdp
        ${MOSEK_DIR}/bin/libfusion64.so.10.1
        )



#add_executable(examples
#        src/sdpEncoder.cpp
#        src/symbolicRing.cpp
#        src/solver.cpp
#        include/solver.h
#        include/debugTools.h
#        include/programExpression.h
#        include/tokenizer.h
#        include/program.h
#        include/program.h
#        include/programParser.h
#        src/programParser.cpp
#        include/expressionParser.h
#        src/expressionParser.cpp
#        include/automaitcComplexityEstimator.h
#        include/combinatorics.h
#        include/hacks.h
#        include/pythonCodeGen.h
#        include/stringRoutines.h
#        src/stringRoutines.cpp
#        include/sdpProblem.h)
#
#target_link_libraries(examples
#        ${CPLEX_DIR}/cplex/lib/x86-64_linux/static_pic/libcplex.a
#        ${CPLEX_DIR}/concert/lib/x86-64_linux/static_pic/libconcert.a
#        -lm -lpthread -ldl
#        )
#target_link_libraries(examples
#        ${MOSEK_DIR}/bin/libmosek64.so
#        )
#target_link_libraries(examples
#        ${MOSEK_DIR}/bin/libfusion64.so.10.0
#        )




