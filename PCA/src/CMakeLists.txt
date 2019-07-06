cmake_minimum_required(VERSION 3.13)
project(PCAAlgorithm)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(CMAKE_CXX_STANDARD 14)

# libigl
option(LIBIGL_WITH_OPENGL            "Use OpenGL"         ON)
option(LIBIGL_WITH_OPENGL_GLFW       "Use GLFW"           ON)
option(LIBIGL_WITH_OPENGL_GLFW_IMGUI "Use IMGUI"          ON)

find_package(LIBIGL REQUIRED QUIET)
find_package(OpenCV REQUIRED )
# Add your project files
#file(GLOB SRC_FILES *.cpp)
include_directories(${PROJECT_SOURCE_DIR}
                    ${PROJECT_SOURCE_DIR}/../../../../spectra-0.7.0/include
                    ${OpenCV_INCLUDE_DIRS}
                    ${PROJECT_SOURCE_DIR}/../../../../ANN/include
        )

set(SRC_FILES ${PROJECT_SOURCE_DIR}/main.cpp
              ${PROJECT_SOURCE_DIR}/mytools.cpp
              ${PROJECT_SOURCE_DIR}/mytools.h
              ${PROJECT_SOURCE_DIR}/nanoflann.hpp
              )
              
              
add_executable(${PROJECT_NAME}_bin ${SRC_FILES})
target_link_libraries(${PROJECT_NAME}_bin igl::core igl::opengl_glfw igl::opengl_glfw_imgui )


#cmake_minimum_required(VERSION 3.14)
#set(CMAKE_CXX_STANDARD 14)


target_link_libraries( ${PROJECT_NAME}_bin ${OpenCV_LIBS} ${CMAKE_CURRENT_SOURCE_DIR}/../../../../ANN/lib/libANN.a)
