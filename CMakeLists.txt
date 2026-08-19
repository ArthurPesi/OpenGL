cmake_minimum_required(VERSION 3.15)
project(hexagon LANGUAGES C)

set(CMAKE_C_STANDARD 11)

find_package(glfw3 3.3 REQUIRED)
find_package(GLEW REQUIRED)

add_executable(hexagon main.c)

target_compile_options(hexagon PRIVATE
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -Werror -Wno-unused-parameter>
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -Werror -Wno-unused-parameter>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

target_link_libraries(hexagon PRIVATE glfw GLEW::GLEW)

if(WIN32)
    target_link_libraries(hexagon PRIVATE opengl32 gdi32)
else()
    target_link_libraries(hexagon PRIVATE GL X11 GLU)
endif()

add_custom_command(TARGET hexagon POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/hexagon.vs
        ${CMAKE_SOURCE_DIR}/hexagon.fs
        $<TARGET_FILE_DIR:hexagon>
)
