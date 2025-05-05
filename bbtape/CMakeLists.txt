set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(bbtape
  file_handler.cpp
  ram_handler.cpp
  config.cpp
  utils.cpp
  sort.cpp
)

target_include_directories(bbtape PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
