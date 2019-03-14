cmake_minimum_required(VERSION 2.8)
project( CCDDrone )

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/CMakeModules")
find_package( CFITSIO )

#find_package(sqlite3 REQUIRED)
#MESSAGE( STATUS "Found : ${SQLITE3_LIBRARIES}" )

SET(CMAKE_CXX_FLAGS "-std=gnu++1y -O2 -march=native -m64")


set(SOURCE
   ${CMAKE_CURRENT_SOURCE_DIR}/LeachController.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/LeachControllerExpose.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/FitsOps.cpp
   )
set(HEADERS
   ${CMAKE_CURRENT_SOURCE_DIR}/picosha2.h
   ${CMAKE_CURRENT_SOURCE_DIR}/LeachController.hpp
   ${CMAKE_CURRENT_SOURCE_DIR}/CCDControlDataTypes.hpp
   )

INCLUDE_DIRECTORIES(
    ${CMAKE_CURRENT_SOURCE_DIR}/ArcAPI/CArcDevice/src
    ${CMAKE_CURRENT_SOURCE_DIR}/ArcAPI/CArcDeinterlace/src
    ${CMAKE_CURRENT_SOURCE_DIR}/ArcAPI/CArcFitsFile/src
    ${CFITSIO_INCLUDE_DIR}
    #${CMAKE_CURRENT_SOURCE_DIR}/ArcAPI/ARC_API/3.5/cfitsio-3330/linux/x64/include
    )

LINK_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/ArcAPI/Release/x64)



add_executable( CCDDExpose CCDDExpose.cpp ${SOURCE} ${HEADERS} )
target_link_libraries( CCDDExpose -lCArcDeinterlace -lCArcFitsFile -lCArcDevice ${CFITSIO_LIBRARIES})

add_executable( CCDDStartupAndErase CCDDStartupAndErase.cpp ${SOURCE} ${HEADERS})
target_link_libraries( CCDDStartupAndErase -lCArcDeinterlace -lCArcFitsFile -lCArcDevice ${CFITSIO_LIBRARIES})

