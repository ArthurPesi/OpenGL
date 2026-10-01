cmake_minimum_required(VERSION 3.15)
project(hexagon LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

find_package(glfw3 3.3 REQUIRED)
find_package(GLEW REQUIRED)
find_package(glm REQUIRED)

add_executable(hexagon main.cpp Mesh.cpp Group.cpp Face.cpp Obj3D.cpp Projectile.cpp stb_image_impl.cpp)

# stb_image.h is third-party and emits many warnings; isolate its implementation
# in its own translation unit and exempt only that file from -Werror.
set_source_files_properties(stb_image_impl.cpp PROPERTIES COMPILE_FLAGS "-w")

target_compile_options(hexagon PRIVATE
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -Werror -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function>
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -Werror -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

target_link_libraries(hexagon PRIVATE glfw GLEW::GLEW glm::glm)

if(WIN32)
    target_link_libraries(hexagon PRIVATE opengl32 gdi32)
elseif(APPLE)
    target_link_libraries(hexagon PRIVATE "-framework OpenGL" "-framework Cocoa" "-framework IOKit")
else()
    target_link_libraries(hexagon PRIVATE GL X11 GLU)
endif()

add_custom_command(TARGET hexagon POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/vertex.vs
        ${CMAKE_SOURCE_DIR}/fragment.fs
        $<TARGET_FILE_DIR:hexagon>
)
