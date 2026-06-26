# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles\\infant_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\infant_autogen.dir\\ParseCache.txt"
  "infant_autogen"
  )
endif()
