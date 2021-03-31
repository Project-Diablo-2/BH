cmake_minimum_required(VERSION 3.7)
find_library(STORM_LIBRARY NAMES StormLib HINTS "ThirdParty")
add_subdirectory("BH")
