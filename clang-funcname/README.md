1. place the clang-funcname folder in llvm-project-15.0.7.src/clang/tools/
2. add add_clang_subdirectory(clang-funcname) to the end of llvm-project-15.0.7.src/clang/tools/CMakeLists.txt
3. build llvm
4. run

```
cd llvm-project-15.0.7.src/build/bin
./clang-funcname example.c
```