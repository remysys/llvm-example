set(LLVM_LINK_COMPONENTS
  Support
  )

add_clang_executable(clang-funcname
  ClangFuncname.cpp
  )

clang_target_link_libraries(clang-funcname
  PRIVATE
  clangBasic
  clangFrontend
  clangRewrite
  clangTooling
  )