cmake_minimum_required(VERSION 3.10)
project(onair C)

add_executable(onair WIN32 main.c resource.rc)
target_link_libraries(onair shell32 user32 gdi32 advapi32)

# 最小体积优化
if(MSVC)
    target_compile_options(onair PRIVATE /Os /GS- /GL)
    target_link_options(onair PRIVATE /OPT:REF /OPT:ICF /LTCG /NODEFAULTLIB:libcmt)
else()
    target_compile_options(onair PRIVATE -Os -s)
    target_link_options(onair PRIVATE -s -static)
endif()
