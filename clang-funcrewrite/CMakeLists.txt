cmake_minimum_required(VERSION 3.13.4)
project(clang-funcrewrite)

set(LLVM_ROOT "~/llvm-project-15.0.7.src/build")
set(Clang_ROOT "~/llvm-project-15.0.7.src/build")


set(CMAKE_CXX_STANDARD 17)

find_package(LLVM REQUIRED CONFIG)
find_package(Clang REQUIRED CONFIG)

# defined in LLVMConfig.cmake
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
message(STATUS "LLVM_INCLUDE_DIRS: ${LLVM_INCLUDE_DIRS}")
message(STATUS "LLVM_LIBRARY_DIRS: ${LLVM_LIBRARY_DIRS}")

# defined in ClangConfig.cmake
message(STATUS "Using ClangConfig.cmake in: ${Clang_DIR}")
message(STATUS "CLANG_INCLUDE_DIRS: ${CLANG_INCLUDE_DIRS}")


include_directories(${LLVM_INCLUDE_DIRS})
include_directories(${CLANG_INCLUDE_DIRS})

link_directories(${LLVM_LIBRARY_DIRS})

add_definitions(${LLVM_DEFINITIONS})

add_executable(clang-funcrewrite ClangFuncRewrite.cpp)

target_compile_options(clang-funcrewrite PRIVATE -fno-rtti)

set(LLVM_LINK_COMPONENTS
  Option
  Support
  Core
  Native
  )

target_link_libraries(clang-funcrewrite
  clangAST
  clangBasic
  clangFrontend
  clangTooling
  )