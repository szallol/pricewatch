cmake_minimum_required(VERSION 3.3)
project(pricewatch)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -march=i686")
# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set (CMAKE_PREFIX_PATH "c:/Qt/Qt5.5.1/5.5/mingw492_32")

set(BOOST_ROOT "c:/boost_1_59_0")
set(Boost_INCLUDE_DIR "c:/boost_1_59_0")
set(Boost_LIBRAYDIR "c:/boost_1_59_0/stage/lib")

set(Boost_USE_STATIC_LIBS        ON)
set(Boost_USE_MULTITHREADED      ON)
set(Boost_USE_STATIC_RUNTIME    OFF)

find_package(Boost REQUIRED  COMPONENTS system regex program_options)

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
find_package(Qt5WebkitWidgets)
find_package(Qt5Sql)
# Add the include directories for the Qt 5 Widgets module to
# the compile lines.
include_directories(${Qt5Widgets_INCLUDE_DIRS})

# Use the compile definitions defined in the Qt 5 Widgets module
add_definitions(${Qt5Widgets_DEFINITIONS})

# Add compiler flags for building executables (-fPIE)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}")

qt5_generate_moc(main.cpp main.moc)

#c:\Qt\Qt5.5.1\5.5\mingw492_32\lib\cmake

set(SOURCE_FILES main.cpp MarketWebPage.cpp MarketWebPage.hpp
        EMagWebPage.cpp EMagWebPage.hpp
        ProductCategory.cpp ProductCategory.hpp
        WebMarket.cpp WebMarket.hpp
        EMagMarket.cpp EMagMarket.hpp MarketProduct.cpp MarketProduct.hpp)
add_executable(pricewatch ${SOURCE_FILES})
target_link_libraries(pricewatch ${Boost_LIBRARIES} Qt5::Widgets Qt5::WebKitWidgets Qt5::Sql)