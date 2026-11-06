# Windows Store (UWP) build on a Windows machine with clang-cl and the Windows
# SDK installed. Building a static library needs no store CRT linkage; set a
# CMAKE_SYSTEM_VERSION if the default SDK version is not the one you want.
set(CMAKE_SYSTEM_NAME WindowsStore)
set(CMAKE_SYSTEM_VERSION 10.0)
set(CMAKE_SYSTEM_PROCESSOR AMD64)

set(CMAKE_C_COMPILER clang-cl)
set(CMAKE_CXX_COMPILER clang-cl)

set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreadedDLL)
# Executables built here would take their imports from the umbrella library.
set(CMAKE_EXE_LINKER_FLAGS_INIT "WindowsApp.lib")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "WindowsApp.lib")
