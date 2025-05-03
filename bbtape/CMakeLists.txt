set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(bbtape
  files.cpp
  tape.cpp
  utils.cpp
  sort.cpp
  physical.cpp
)

target_include_directories(bbtape PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
