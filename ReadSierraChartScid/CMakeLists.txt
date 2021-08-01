# CMakeList.txt : CMake project for ReadSierraChartScid, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)
set(CMAKE_CXX_STANDARD 20)

# Add source to this project's executable.
add_executable (ReadSierraChartScid "ReadSierraChartScid.cpp" "ReadSierraChartScid.h" "SierraChartFiles/scdatetime.h" "SierraChartFiles/scstructures.h" "SierraChartFiles/sccolors.h" "SierraChartFiles/scconstants.h" "SierraChartFiles/SCString.h" "SierraChartFiles/SCSymbolData.h" "SierraChartFiles/VAPContainer.h" "SierraChartFiles/sierrachart.h" "SierraChartFiles/ACSILDepthBars.h" "SierraChartFiles/IntradayRecord.h" "SierraChartFiles/ACSILCustomChartBars.h")

# TODO: Add tests and install targets if needed.
