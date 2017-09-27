
# ----------------------------------------------------------------------------------
# Find ImageMagick.
# ----------------------------------------------------------------------------------
# Source of this section: https://stackoverflow.com/a/38546197 (20170927)
#                     and https://stackoverflow.com/a/37776099 (20170927).
# Note: I slightly modified the above to remove caching and listification.
#
find_package(ImageMagick COMPONENTS Magick++ MagickCore)
if(NOT ImageMagick_FOUND)
    message(ERROR "ImageMagick not found!")
endif(NOT ImageMagick_FOUND)

# Find Imagemagick Library directory.
get_filename_component(MAGICK_LIB_DIR ${ImageMagick_MagickCore_LIBRARY} DIRECTORY)

# Find where Magick++-config lives.
file(GLOB_RECURSE MAGICK_CONFIG FOLLOW_SYMLINKS ${MAGICK_LIB_DIR}/Magick++-config)
message(STATUS " Magick++-config found at: ${MAGICK_CONFIG}")

# Ask about CXX and lib flags/locations.
set(MAGICK_CONFIG ${MAGICK_CONFIG} CACHE string "Path to Magick++-config utility")
execute_process(COMMAND "${MAGICK_CONFIG}" "--cxxflags" OUTPUT_VARIABLE MAGICK_CXX_FLAGS)
execute_process(COMMAND "${MAGICK_CONFIG}" "--libs"     OUTPUT_VARIABLE MAGICK_LD_FLAGS)
message(STATUS " Magick++-config --cxxflags = ${MAGICK_CXX_FLAGS}")
message(STATUS " Magick++-config --libs     = ${MAGICK_LD_FLAGS}")

# Remove trailing whitespace (CMAKE dies over this; against their "policy").
string(STRIP "${MAGICK_CXX_FLAGS}" MAGICK_CXX_FLAGS)
string(STRIP "${MAGICK_LD_FLAGS}" MAGICK_LD_FLAGS)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${MAGICK_CXX_FLAGS}")

# ----------------------------------------------------------------------------------
# Find Freeglut.
# ----------------------------------------------------------------------------------
find_package(GLUT REQUIRED)
include_directories(${GLUT_INCLUDE_DIR})
if(NOT GLUT_FOUND)
    message(ERROR "GLUT not found!")
endif(NOT GLUT_FOUND)

# ----------------------------------------------------------------------------------
# Find OpenGL.
# ----------------------------------------------------------------------------------
find_package(OpenGL REQUIRED)
include_directories(${OpenGL_INCLUDE_DIRS})
link_directories(${OpenGL_LIBRARY_DIRS})
add_definitions(${OpenGL_DEFINITIONS})
if(NOT OPENGL_FOUND)
    message(ERROR "OPENGL not found!")
endif(NOT OPENGL_FOUND)



# ----------------------------------------------------------------------------------


# Executable.
add_executable(mosaic
    Mosaic.cc
    Structs.cc     
    Flashcard_Parsing.cc     
    Text.cc     
)
target_link_libraries(mosaic
    ygor 
    sqlite3

    ${GLUT_LIBRARIES}
    ${OPENGL_LIBRARIES}
    ${MAGICK_LD_FLAGS}
)


# Installation info.
install(TARGETS 
    mosaic

    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

