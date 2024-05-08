cmake_minimum_required(VERSION 3.10)

project(track-exceptions)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})
add_compile_options(-fno-rtti)
# set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++ -lc++abi")


find_package(LLVM REQUIRED)
find_package(Clang REQUIRED)

include_directories(${LLVM_INCLUDE_DIR})

add_executable(${PROJECT_NAME} main.cpp ExceptionsTracker.cpp)

target_link_libraries(${PROJECT_NAME} ${LLVM_LIBRARIES})
target_link_libraries(${PROJECT_NAME} ${Clang_LIBRARIES})

target_link_libraries(${PROJECT_NAME} clangAST)
target_link_libraries(${PROJECT_NAME} clangASTMatchers)
target_link_libraries(${PROJECT_NAME} clangAnalysis)
target_link_libraries(${PROJECT_NAME} clangBasic)
target_link_libraries(${PROJECT_NAME} clangFrontend)
target_link_libraries(${PROJECT_NAME} clangLex)
target_link_libraries(${PROJECT_NAME} clangRewrite)
target_link_libraries(${PROJECT_NAME} clangSema)
target_link_libraries(${PROJECT_NAME} clangSerialization)
target_link_libraries(${PROJECT_NAME} clangTooling)
target_link_libraries(${PROJECT_NAME} clangToolingCore)
target_link_libraries(${PROJECT_NAME} clangTidy)
target_link_libraries(${PROJECT_NAME} clangStaticAnalyzerCore)
target_link_libraries(${PROJECT_NAME} clangStaticAnalyzerFrontend)
target_link_libraries(${PROJECT_NAME} LLVMVEAsmParser)
target_link_libraries(${PROJECT_NAME} LLVMX86AsmParser)
target_link_libraries(${PROJECT_NAME} LLVMTarget)

include(GNUInstallDirs)
install(FILES include/annotate_throw.hpp
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/annotate_throw
)

install(TARGETS track-exceptions
        DESTINATION ${CMAKE_INSTALL_BINDIR}
)
