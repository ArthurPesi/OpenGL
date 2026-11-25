cmake_minimum_required(VERSION 3.15)
project(hexagon LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

find_package(glfw3 3.3 REQUIRED)
find_package(GLEW REQUIRED)
find_package(glm REQUIRED)

add_executable(hexagon main.cpp Face.cpp Obj3D.cpp AnimatedObj3D.cpp ObjectConfig.cpp
    Projectile.cpp stb_image_impl.cpp)

# trackgen: standalone B-spline tool that exports a road mesh and car animation
add_executable(trackgen trackgen.cpp TrackPath.cpp)

# stb_image.h is third-party and emits many warnings; isolate its implementation
# in its own translation unit and exempt only that file from -Werror.
set_source_files_properties(stb_image_impl.cpp PROPERTIES COMPILE_FLAGS "-w")

set(WARNING_FLAGS
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -Werror -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function>
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -Werror -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)
target_compile_options(hexagon PRIVATE ${WARNING_FLAGS})
target_compile_options(trackgen PRIVATE ${WARNING_FLAGS})

target_link_libraries(hexagon PRIVATE glfw GLEW::GLEW glm::glm)
target_link_libraries(trackgen PRIVATE glfw GLEW::GLEW glm::glm)

if(WIN32)
    set(PLATFORM_GL_LIBS opengl32 gdi32)
elseif(APPLE)
    set(PLATFORM_GL_LIBS "-framework OpenGL" "-framework Cocoa" "-framework IOKit")
else()
    set(PLATFORM_GL_LIBS GL X11 GLU)
endif()
target_link_libraries(hexagon PRIVATE ${PLATFORM_GL_LIBS})
target_link_libraries(trackgen PRIVATE ${PLATFORM_GL_LIBS})


add_custom_command(TARGET hexagon POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/vertex.vs
        ${CMAKE_SOURCE_DIR}/fragment.fs
        $<TARGET_FILE_DIR:hexagon>
)

add_custom_command(TARGET trackgen POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/track_vertex.vs
        ${CMAKE_SOURCE_DIR}/track_fragment.fs
        $<TARGET_FILE_DIR:trackgen>
)
