cmake_minimum_required(VERSION 3.0)
set(CMAKE_BUILD_TYPE RelWithDebInfo)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "-w -O0")
set(CMAKE_CXX_FLAGS_DEBUG "-w -O0")
set(CMAKE_C_FLAGS_DEBUG "-w -O0")
set(RUBY "2.7")
Project(hiddenchest)

## Setup options ##

option(SHARED_FLUID "Dynamically link fluidsynth at build time" OFF)
option(WORKDIR_CURRENT "Keep current directory on startup" OFF)
option(FORCE32 "Force 32bit compile on 64bit OS" OFF)
set(BINDING "MRI" CACHE STRING "The Binding Type (MRI, MRUBY, NULL)")
set(EXTERNAL_LIB_PATH "" CACHE PATH "External precompiled lib prefix")

## Misc setup ##

include(cmake/PrepUtils.cmake)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

IF("${CMAKE_SYSTEM}" MATCHES "Linux")
  SET(LINUX ON)
  set(SYSTEM_STRING "linux")
  set(SYSTEM_REAL_STRING "Linux")
ENDIF()

if (WIN32)
  set(SYSTEM_STRING "windows")
  set(SYSTEM_REAL_STRING "Windows")
endif()

if (APPLE)
  set(SYSTEM_STRING "mac")
  set(SYSTEM_REAL_STRING "MacOSX")
endif()

IF(WORKDIR_CURRENT)
  list(APPEND DEFINES WORKDIR_CURRENT)
ENDIF()

IF(FORCE32)
  if(APPLE)
    SET(CMAKE_OSX_ARCHITECTURES "i386")
  else()
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
  endif()
ENDIF()

set(CMAKE_OSX_DEPLOYMENT_TARGET 10.6)

IF(LINUX)
  if(CMAKE_SIZEOF_VOID_P MATCHES "8" AND NOT(FORCE32) )
    set(CMAKE_EXECUTABLE_SUFFIX "")
    set(BIN_RPATH "\$ORIGIN/lib")
    set(LIB_PATH "lib")
  else()
    set(CMAKE_EXECUTABLE_SUFFIX ".b86")
    set(BIN_RPATH "\$ORIGIN/lib")
    set(LIB_PATH "lib")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_FILE_OFFSET_BITS=64")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_FILE_OFFSET_BITS=64")
  endif()
elseif(APPLE)
  SET(BIN_RPATH "@executable_path/../Frameworks")
  set(LIB_PATH "lib")
endif()

set(CMAKE_SKIP_BUILD_RPATH TRUE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH ${BIN_RPATH})
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)

## Locate core libs ##

find_package(PkgConfig REQUIRED)

if (EXTERNAL_LIB_PATH)
  set(CMAKE_PREFIX_PATH ${EXTERNAL_LIB_PATH})
  if(EXISTS "${EXTERNAL_LIB_PATH}/${LIB_PATH}/pkgconfig/")
    SET(ENV{PKG_CONFIG_PATH} "${EXTERNAL_LIB_PATH}/${LIB_PATH}/pkgconfig/")
  endif()
  if(APPLE)
    set(PLATFORM_SHARED_LIBS 
      libSDL2.dylib libSDL2_image-2.0.0.dylib libSDL2_ttf-2.0.0.dylib libSDL_sound-1.0.1.dylib
      libfreetype.6.dylib libsigc-2.0.0.dylib
      CACHE STRING "List of shared libraries that need to be copied into the OS X bundle")
    foreach(lib ${PLATFORM_SHARED_LIBS})
      if(EXISTS ${EXTERNAL_LIB_PATH}/lib/${lib})
        list(APPEND PLATFORM_COPY_LIBS ${EXTERNAL_LIB_PATH}/lib/${lib})
      endif()
    endforeach()
  endif()
endif()

pkg_check_modules(SIGCXX       REQUIRED sigc++-2.0)
pkg_check_modules(PIXMAN       REQUIRED pixman-1)
pkg_check_modules(PHYSFS       REQUIRED physfs>=2.1)
pkg_check_modules(VORBISFILE   REQUIRED vorbisfile)
pkg_check_modules(SDL2         REQUIRED sdl2)
pkg_check_modules(SDL2_TTF     REQUIRED SDL2_ttf)
pkg_check_modules(SDL2_IMAGE   REQUIRED SDL2_image)
pkg_check_modules(SDL_SOUND    REQUIRED SDL_sound)
find_package(OpenAL REQUIRED)
find_package(ZLIB REQUIRED)

## Setup main source ##

set(MAIN_HEADERS
  src/resolution.h
  src/quadarray.h
  src/audio.h
  src/binding.h
  src/bitmap.h
  src/disposable.h
  src/etc.h
  src/etc-internal.h
  src/eventthread.h
  src/flashable.h
  src/font.h
  src/input.h
  src/plane.h
  src/scene.h
  src/sprite.h
  src/msgboxsprite.h
  src/table.h
  src/texpool.h
  src/tilequad.h
  src/transform.h
  src/viewport.h
  src/window.h
  src/serializable.h
  src/shader.h
  src/glstate.h
  src/quad.h
  src/tilemap.h
  src/tilemap-common.h
  src/graphics.h
  src/gl-debug.h
  src/global-ibo.h
  src/exception.h
  src/filesystem.h
  src/serial-util.h
  src/intrulist.h
  src/binding.h
  src/gl-util.h
  src/util.h
  src/config.h
  src/settingsmenu.h
  src/keybindings.h
  src/tileatlas.h
  src/sharedstate.h
  src/al-util.h
  src/boost-hash.h
  src/debugwriter.h
  src/gl-fun.h
  src/gl-meta.h
  src/vertex.h
  src/soundemitter.h
  src/aldatasource.h
  src/alstream.h
  src/audiostream.h
  src/rgssad.h
  src/windowvx.h
  src/tilemapvx.h
  src/tileatlasvx.h
  src/sharedmidistate.h
  src/fluid-fun.h
  src/sdl-util.h #src/SDL_SavePNG.h
  src/clicks.h
)

set(MAIN_SOURCE
  src/main.cpp
  src/audio.cpp
  src/bitmap.cpp
  src/eventthread.cpp
  src/filesystem.cpp
  src/font.cpp
  src/input.cpp
  src/plane.cpp
  src/scene.cpp
  src/sprite.cpp
  src/msgboxsprite.cpp
  src/table.cpp
  src/tilequad.cpp
  src/viewport.cpp
  src/window.cpp
  src/texpool.cpp
  src/shader.cpp
  src/glstate.cpp
  src/tilemap.cpp
  src/autotiles.cpp
  src/graphics.cpp
  src/gl-debug.cpp
  src/etc.cpp
  src/config.cpp
  src/settingsmenu.cpp
  src/keybindings.cpp
  src/tileatlas.cpp
  src/sharedstate.cpp
  src/gl-fun.cpp
  src/gl-meta.cpp
  src/vertex.cpp
  src/soundemitter.cpp
  src/sdlsoundsource.cpp
  src/alstream.cpp
  src/audiostream.cpp
  src/rgssad.cpp
  src/bundledfont.cpp
  src/vorbissource.cpp
  src/windowvx.cpp
  src/tilemapvx.cpp
  src/tileatlasvx.cpp
  src/autotilesvx.cpp
  src/midisource.cpp
  src/fluid-fun.cpp
)

if(WIN32)
  list(APPEND MAIN_HEADERS windows/resource.h)
  list(APPEND MAIN_SOURCE windows/resource.rc)
endif()

source_group("MKXPPLUS Source" FILES ${MAIN_SOURCE} ${MAIN_HEADERS})

## Setup embedded source ##

set(EMBEDDED_INPUT
  shader/common.h
  shader/transSimple.frag
  shader/trans.frag
  shader/hue.frag
  shader/sprite.frag
  shader/plane.frag
  shader/gray.frag
  shader/sepia.frag
  shader/basic_color.frag
  shader/bitmapBlit.frag
  shader/flatColor.frag
  shader/simple.frag
  shader/simpleColor.frag
  shader/simpleAlpha.frag
  shader/simpleAlphaUni.frag
  shader/flashMap.frag
  shader/minimal.vert
  shader/simple.vert
  shader/simpleColor.vert
  shader/sprite.vert
  shader/tex.vert
  shader/tex.frag
  shader/tilemap.vert
  shader/tilemapvx.vert
  shader/blur.frag
  shader/blurH.vert
  shader/blurV.vert
  shader/simpleMatrix.vert
  shader/pixel.frag
  shader/gradient.frag
  shader/gradient_map.frag
  shader/gradient_sprite.frag
  #shader/thermal.frag
  shader/vignette.frag
  shader/vignette_black.frag
  shader/vignette_blue.frag
  shader/vignette_gray.frag
  shader/vignette_green.frag
  shader/vignette_red.frag
  shader/vignette_sepia.frag
  shader/vignette_yellow.frag
  assets/FreeSans.ttf
  assets/icon.png
  assets/app_logo.png
  assets/app_logo_s01.png
  assets/app_logo_s02.png
  assets/app_logo_s03.png
)

if (RGSS2)
  list(APPEND DEFINES RGSS2)
endif()

if (SHARED_FLUID)
  pkg_check_modules(FLUID REQUIRED fluidsynth)
  list(APPEND DEFINES SHARED_FLUID)
endif()

## Process Embeddeds ##

find_program(XXD_EXE xxd DOC "Location of the xxd executable")

macro(ProcessWithXXD outvar inputfile outdir)
  get_filename_component(basefile ${inputfile} NAME)
  set(outputfile ${outdir}/${basefile}.xxd)
  set_source_files_properties(${outputfile} PROPERTIES HEADER_ONLY TRUE)
  add_custom_command(
    OUTPUT ${outputfile}
    COMMAND ${XXD_EXE} -i ${inputfile} ${outputfile}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS ${inputfile}
    COMMENT "Generating XXD for ${inputfile}"
  )
  list(APPEND ${outvar} ${outputfile})
endmacro()

#file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/xxdhack)
#include_directories(${CMAKE_CURRENT_BINARY_DIR}/xxdhack)

foreach(item ${EMBEDDED_INPUT})
  ProcessWithXXD(EMBEDDED_SOURCE ${item} ${CMAKE_CURRENT_BINARY_DIR})
endforeach()

source_group("Embedded Source" FILES ${EMBEDDED_INPUT} ${EMBEDDED_SOURCE})

## Setup binding source ##

if (BINDING STREQUAL "MRI")
  set(MRIVERSION ${RUBY} CACHE STRING "Version of MRI to link with")
  pkg_check_modules(MRI REQUIRED ruby-${MRIVERSION})
  list(APPEND DEFINES BINDING_MRI)
  set(BINDING_HEADERS
    binding/binding-util.h
    binding/binding-types.h
    binding/serializable-binding.h
    binding/disposable-binding.h
    binding/sceneelement-binding.h
    binding/viewportelement-binding.h
    binding/flashable-binding.h
    binding/msgboxsprite-binding.h
    binding/hcextras.h
    binding/output.h
  )
  set(BINDING_SOURCE
    binding/rgss.cpp
    binding/binding-util.cpp
    binding/game.cpp
    binding/table-binding.cpp
    binding/etc-binding.cpp
    binding/bitmap-binding.cpp
    binding/font-binding.cpp
    binding/graphics-binding.cpp
    binding/input-binding.cpp
    binding/mouse.cpp
    binding/sprite-binding.cpp
    binding/msgboxsprite-binding.cpp
    binding/viewport-binding.cpp
    binding/plane-binding.cpp
    binding/window-binding.cpp
    binding/tilemap-binding.cpp
    binding/audio-binding.cpp
    binding/module_rpg.cpp
    binding/filesystem-binding.cpp
    binding/windowvx-binding.cpp
    binding/tilemapvx-binding.cpp
    binding/backdrop.cpp
    binding/scripts.cpp
    binding/system.cpp
    binding/hiddenchest.cpp
    binding/zlib.c
  )
else()
  message(FATAL_ERROR "Must choose a valid binding type.  MRI, MRUBY, or NULL")
endif()

source_group("Binding Source" FILES ${BINDING_SOURCE} ${BINDING_HEADERS})

## Setup main executable ##

if(APPLE)
  find_library(CARBON_LIBRARY Carbon)
  find_library(IOKIT_LIBRARY IOKit)
  mark_as_advanced(CARBON_LIBRARY IOKIT_LIBRARY)
  list(APPEND PLATFORM_LIBRARIES
    ${CARBON_LIBRARY}
    ${IOKIT_LIBRARY}
    "-liconv"
  )
endif()

link_directories(
  ${SIGCXX_LIBRARY_DIRS}
  ${PIXMAN_LIBRARY_DIRS}
  ${PHYSFS_LIBRARY_DIRS}
  ${SDL2_LIBRARY_DIRS} # Blindly assume other SDL bits are in same directory
  ${MRI_LIBDIR}
)

add_executable(${PROJECT_NAME} MACOSX_BUNDLE
  ${MAIN_HEADERS}
  ${MAIN_SOURCE}
  ${BINDING_HEADERS}
  ${BINDING_SOURCE}
  ${EMBEDDED_SOURCE}
)

target_compile_definitions(${PROJECT_NAME} PRIVATE ${DEFINES})
target_include_directories(${PROJECT_NAME} PRIVATE
  src
  windows
  ${SIGCXX_INCLUDE_DIRS}
  ${PIXMAN_INCLUDE_DIRS}
  ${PHYSFS_INCLUDE_DIRS}
  ${SDL2_INCLUDE_DIRS}
  ${MRI_INCLUDE_DIRS}
  ${VORBISFILE_INCLUDE_DIRS}
  ${FLUID_INCLUDE_DIRS}
  ${OPENAL_INCLUDE_DIR}
  ${ZLIB_INCLUDE_DIR}
)

target_link_libraries(${PROJECT_NAME}
  ${SIGCXX_LIBRARIES}
  ${SDL2_LIBRARIES}
  ${SDL2_IMAGE_LIBRARIES}
  ${SDL2_TTF_LIBRARIES}
  ${SDL_SOUND_LIBRARIES}
  ${PHYSFS_LIBRARIES}
  ${PIXMAN_LIBRARIES}
  ${MRI_LIBRARIES}
  ${VORBISFILE_LIBRARIES}
  ${FLUID_LIBRARIES}
  ${OPENAL_LIBRARY}
  ${ZLIB_LIBRARY}
  ${PLATFORM_LIBRARIES}
)

PostBuildMacBundle(${PROJECT_NAME} "" "${PLATFORM_COPY_LIBS}")
