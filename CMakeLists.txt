cmake_minimum_required(VERSION 3.3)
project(pricewatch)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")

set(CMAKE_C_COMPILER "c:/Qt/Qt5.5.1/Tools/mingw492_32/bin/gcc.exe")
set(CMAKE_CXX_COMPILER "c:/Qt/Qt5.5.1/Tools/mingw492_32/bin/g++.exe")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -march=i686 ")

#enable_testing()
#set(GTEST_INCLUDE_DIR "c:/googletest-master/googletest/include")
#set(GTEST_LIBRARY "c:/googletest-master/googletest/bin/Debug/libgtest.a")
#set(GTEST_MAIN_LIBRARY "c:/googletest-master/googletest/bin/Debug/libgtest_main.a")
#find_package(GTest REQUIRED)
#include_directories(${GTEST_INCLUDE_DIRS})

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set (CMAKE_PREFIX_PATH "c:/Qt/Qt5.5.1/5.5/mingw492_32")

set(BOOST_ROOT "c:/boost_1_59_0")
set(Boost_INCLUDE_DIR "c:/boost_1_59_0")
set(Boost_LIBRAYDIR "c:/boost_1_59_0/stage/lib")

set(Boost_USE_STATIC_LIBS        ON)
set(Boost_USE_MULTITHREADED      ON)
set(Boost_USE_STATIC_RUNTIME    OFF)

find_package(Boost REQUIRED  COMPONENTS log log_setup thread date_time filesystem system regex program_options)

message(status "** Boost Include: ${Boost_INCLUDE_DIR}")
message(status "** Boost Libraries: ${Boost_LIBRARY_DIRS}")
message(status "** Boost Libraries: ${Boost_LIBRARIES}")

if (Boost_FOUND)
        include_directories(${Boost_INCLUDE_DIRS})
        message("Boost ${Boost_FIND_VERSION} found.")
        if (Boost_FIND_COMPONENTS)
                message("Found Boost components:")
                message("   ${Boost_FIND_COMPONENTS}")
        endif()
endif()


set(CMAKE_AUTOMOC ON)
find_package(Qt5Widgets)
find_package(Qt5WebKitWidgets)
find_package(Qt5Sql)
find_package(Qt5Network)
# Add the include directories for the Qt 5 Widgets module to
# the compile lines.
include_directories(${Qt5Widgets_INCLUDE_DIRS})

# Use the compile definitions defined in the Qt 5 Widgets module
add_definitions(${Qt5Widgets_DEFINITIONS})

# Add compiler flags for building executables (-fPIE)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}")

qt5_generate_moc(main.cpp main.moc)

get_property(inc_dirs DIRECTORY PROPERTY INCLUDE_DIRECTORIES) 

message("include dirs: ${inc_dirs}")

set(SOURCE_FILES main.cpp MarketWebPage.cpp MarketWebPage.hpp
        EMagWebPage.cpp EMagWebPage.hpp
        ProductCategory.cpp ProductCategory.hpp
        WebMarket.cpp WebMarket.hpp
        EMagMarket.cpp EMagMarket.hpp MarketProduct.cpp MarketProduct.hpp AltexMarket.cpp AltexMarket.hpp)
add_executable(pricewatch ${SOURCE_FILES})
#target_link_libraries(pricewatch ${GTEST_BOTH_LIBRARIES${Boost_LIBRARIES} Qt5::Widgets Qt5::WebKitWidgets Qt5::Sql)

IF(WIN32)
ENDIF()
target_link_libraries(pricewatch ${Boost_LIBRARIES} Qt5::Widgets Qt5::WebKitWidgets Qt5::Sql Qt5::Network ws2_32)
