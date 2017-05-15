# Required for the FindOpenCL.cmake script.
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/.cmake_modules")

# Set libname.
set(SOTL_LIB_NAME sotl)
set(SOTL_LIB_MAJOR 0)
set(SOTL_LIB_MINOR 1)
set(SOTL_LIB_VERSION ${SOTL_LIB_MAJOR}.${SOTL_LIB_MINOR})

# Command line options.
option(USE_DOUBLE       "Use double precision"      OFF)
option(DISABLE_OPENMP   "Disable OpenMP support"    OFF)
option(DISABLE_OPENGL   "Disable OpenGL support"    OFF)

if (USE_DOUBLE)
    add_definitions(-DUSE_DOUBLE=1)
else (USE_DOUBLE)
    add_definitions(-DUSE_DOUBLE=0)
endif (USE_DOUBLE)

# Some needed definitions.
if (USE_DOUBLE)
    add_definitions(-DUSE_DOUBLE=1)
else ()
    add_definitions(-DUSE_DOUBLE=0)
endif ()
add_definitions(-std=c99 -g -O3 -Wno-deprecated -Wall -Wextra -Wno-unused-function)
add_definitions(-DSHADERS_FILES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/src")
add_definitions(-DPROGRAM_NAME="${CMAKE_CURRENT_SOURCE_DIR}/kernel/physics.cl")
add_definitions(-DOCL_INCLUDE="${CMAKE_CURRENT_SOURCE_DIR}/include")

# Required packages.
find_package(OpenCL REQUIRED)
include_directories(${OPENCL_INCLUDE_DIRS})

find_library(M_LIBRARY
    NAMES m
    PATHS /usr/lib /usr/local/lib
)
if (NOT M_LIBRARY)
    message(ERROR "math library 'libm' not found")
endif ()

# Optional packages.
find_package(GLUT)
find_package(OpenGL)
find_package(OpenMP)

# Our sources.
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
set(libsotl_public_hdrs
    ${CMAKE_CURRENT_SOURCE_DIR}/include/sotl.h
)
set(libsotl_sources
    src/atom.c
    src/device.c
    src/domain.c
    src/global_definitions.c
    src/kernel_list.c
    src/ocl.c
    src/ocl_kernels.c
    src/profiling.c
    src/sotl.c
    src/seq.c
    src/util.c
)

# OpenMP sources.
if (OPENMP_FOUND AND NOT DISABLE_OPENMP)
    set(libsotl_sources
        ${libsotl_sources}
        src/openmp.c
    )
    add_definitions(-DHAVE_OMP)
endif ()

# GLUT and OpenGL sources.
if (GLUT_FOUND AND OPENGL_FOUND AND NOT DISABLE_OPENGL)
    set(libsotl_sources
        ${libsotl_sources}
        src/vbo.c
        src/shaders.c
        src/window.c
    )
    add_definitions(-DHAVE_LIBGL)
endif ()

# Now build our target.
include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${OPENCL_INCLUDE_DIRS})
add_library(${SOTL_LIB_NAME} SHARED ${libsotl_sources})
target_link_libraries(${SOTL_LIB_NAME} ${OPENCL_LIBRARIES} ${M_LIBRARY})

# OpenMP library.
if (OPENMP_FOUND)
    if (NOT DISABLE_OPENMP)
        set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
        target_link_libraries(${SOTL_LIB_NAME} gomp)
    endif ()
endif ()

# GLUT and OpenGL libraries.
if (GLUT_FOUND AND OPENGL_FOUND)
    if (NOT DISABLE_OPENGL)
        include_directories(${GLUT_INCLUDE_DIRS} ${OPENGL_INCLUDE_DIRS})
        target_link_libraries(${SOTL_LIB_NAME} ${OPENGL_LIBRARIES} ${GLUT_LIBRARY})
    endif ()
endif ()

# Set a variable with CMake code which creates a symlink from src to dest.
macro(_sotl_generate_symlink_code CODE SRC DEST)
    set(_sotl_gsc_message "Symlinking ${SRC} to ${DEST}")
    set(_sotl_gsc_operation "create_symlink")

    set(${CODE} "
        message(STATUS \"${_png_gsc_message}\")
        execute_process(COMMAND \${CMAKE_COMMAND} -E ${_png_gsc_operation} \"${SRC}\" \"${DEST}\")
    ")
endmacro()

# Create pkg-config files and executables.
if(NOT DEFINED CMAKE_INSTALL_LIBDIR)
    set(CMAKE_INSTALL_LIBDIR lib)
endif(NOT DEFINED CMAKE_INSTALL_LIBDIR)
set(prefix      ${CMAKE_INSTALL_PREFIX})
set(exec_prefix ${CMAKE_INSTALL_PREFIX})
set(libdir      ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR})
set(includedir  ${CMAKE_INSTALL_PREFIX}/include)
set(LIBS        "-lOpenCL -lm -lgomp -lGLU -lGL -lglut")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/libsotl.pc.in
               ${CMAKE_CURRENT_BINARY_DIR}/libsotl.pc @ONLY)
_sotl_generate_symlink_code(SOTL_PC_INSTALL_CODE
                            ${CMAKE_CURRENT_BINARY_DIR}/libsotl.pc
                            ${CMAKE_CURRENT_BINARY_DIR}/libsotl.pc)
install(CODE ${SOTL_PC_INSTALL_CODE})

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/libsotl-config.in
               ${CMAKE_CURRENT_BINARY_DIR}/libsotl-config @ONLY)
_sotl_generate_symlink_code(SOTL_CONFIG_INSTALL_CODE
                            ${CMAKE_CURRENT_BINARY_DIR}/libsotl-config
                            ${CMAKE_CURRENT_BINARY_DIR}/libsotl-config)
install(CODE ${SOTL_CONFIG_INSTALL_CODE})

# Install.
if(NOT SKIP_INSTALL_LIBRARIES AND NOT SKIP_INSTALL_ALL)
    install(TARGETS ${SOTL_LIB_NAME}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()

# Install header files.
if(NOT SKIP_INSTALL_HEADERS AND NOT SKIP_INSTALL_ALL)
    install(FILES ${libsotl_public_hdrs}
            DESTINATION include/libsotl)
endif()

# Install pkg-config files.
if(NOT SKIP_INSTALL_FILES AND NOT SKIP_INSTALL_ALL)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libsotl.pc
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
endif()

# Install pkg-config executables.
if(NOT SKIP_INSTALL_EXECUTABLES AND NOT SKIP_INSTALL_ALL )
    install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/libsotl-config
            DESTINATION bin)
endif()
