cmake_minimum_required(VERSION 3.16)
project(rml_backend LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Drogon CONFIG REQUIRED)
find_package(CURL REQUIRED)

find_path(SODIUM_INCLUDE_DIR sodium.h)
find_library(SODIUM_LIBRARY sodium)
if (NOT SODIUM_INCLUDE_DIR OR NOT SODIUM_LIBRARY)
  message(FATAL_ERROR "libsodium not found. Install it with: sudo apt install libsodium-dev")
endif()

add_executable(rml_backend
  src/main.cpp
  src/controllers/AuthCtrl.cpp
  src/controllers/UserCtrl.cpp
  src/controllers/LandlordCtrl.cpp
  src/controllers/ReviewCtrl.cpp
  src/controllers/AdminCtrl.cpp
  src/controllers/SupabaseHelper.cpp
)

target_include_directories(rml_backend PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${SODIUM_INCLUDE_DIR}
)

find_library(SODIUM_LIB sodium)
if(NOT SODIUM_LIB)
  message(FATAL_ERROR "libsodium not found. Install libsodium-dev.")
endif()

target_link_libraries(rml_backend PRIVATE
  Drogon::Drogon
  CURL::libcurl
  ${SODIUM_LIBRARY}
)
