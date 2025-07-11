cmake_minimum_required(VERSION 3.10)

set(PROYECTO_NOMBRE main_program)
project(${PROYECTO_NOMBRE})

set(CMAKE_CXX_STANDARD 20)

# Integración con vcpkg
if(DEFINED ENV{VCPKG_ROOT})
  set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

# Buscar cpprestsdk
find_package(cpprestsdk CONFIG REQUIRED)

# Lista de fuentes
set(SRC_FILES
  Sistema.cpp
)

list(TRANSFORM SRC_FILES PREPEND "src/")
list(APPEND SRC_FILES main.cpp)
add_executable(${PROYECTO_NOMBRE} ${SRC_FILES})

# ✅ Directorio de headers corregido
target_include_directories(${PROYECTO_NOMBRE}
  PRIVATE
    include
    excepciones
)

target_link_libraries(${PROYECTO_NOMBRE} PRIVATE cpprestsdk::cpprest)
