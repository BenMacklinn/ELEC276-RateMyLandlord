cmake_minimum_required(VERSION 3.16)
project(rml_backend LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Drogon CONFIG REQUIRED)

add_executable(rml_backend
  src/main.cpp
  src/controllers/AuthCtrl.cpp
  src/controllers/UserCtrl.cpp
  src/controllers/LandlordCtrl.cpp
  src/controllers/ReviewCtrl.cpp
)

target_include_directories(rml_backend PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(rml_backend PRIVATE Drogon::Drogon)
