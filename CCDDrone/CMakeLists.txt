cmake_minimum_required(VERSION 2.8)
project( CCDDrone )


set(CMAKE_MODULE_PATH "cmake_modules")
find_package( CFITSIO )
#find_package(sqlite3 REQUIRED)
#MESSAGE( STATUS "Found : ${SQLITE3_LIBRARIES}" )


SET(CMAKE_CXX_FLAGS "-std=gnu++1y -O2 -march=native -m64 -LArcAPI/ARC_API/3.5/Release/x64 -IArcAPI/ARC_API/3.5/CArcDevice/src -IArcAPI/ARC_API/3.5/CArcDeinterlace/src -IArcAPI/ARC_API/3.5/CArcFitsFile/src -IArcAPI/ARC_API/3.5/cfitsio-3330/linux/x64/include -lCArcDeinterlace -lcfitsio -lCArcFitsFile -lCArcDevice")

SET(CMAKE_EXE_LINKER_FLAGS "-LArcAPI/ARC_API/3.5/Release/x64 -lCArcDeinterlace -lcfitsio -lCArcFitsFile -lCArcDevice")



#set(SOURCE
#   ${CMAKE_CURRENT_SOURCE_DIR}/ArcAPI35Ex_1.cpp
#)
#set(HEADERS
   #${CMAKE_CURRENT_SOURCE_DIR}/common/CommonDatatypes.h

#)



add_executable( CCDDrone ArcAPI35Ex_2.cpp )
#target_link_libraries( abub3hs ${OpenCV_LIBS} ${SOURCE} ${HEADERS} lbpuser AbubEntropySubsystem AbubUtility)
#target_link_libraries( abub ${OpenCV_LIBS} ${SOURCE} ${HEADERS} )

target_link_libraries( CCDDrone )

