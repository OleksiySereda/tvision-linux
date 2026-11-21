# Turbo Vision 1.0 for Windows (MSVC).  On Linux use the Makefile.
#
#   cmake -B build                          # finds Visual Studio
#   cmake --build build --config Release    # lib/ and bin/ as with make
#   ctest --test-dir build -C Release       # the tests of the add-ons
#
# Visual Studio opens this directory as it is (File > Open > Folder).
# The library is the same as on Linux; only the terminal driver differs:
# source/platform/win32/ instead of source/platform/posix/.
#
# Another project uses the libraries with
#
#   add_subdirectory( path/to/tvision tvision EXCLUDE_FROM_ALL )
#   tv_addons_program( app app.cpp )     # or target_link_libraries( app tvaddons )
#
# The targets tv and tvaddons carry their include paths, compiler options
# and /WHOLEARCHIVE; tv_program() and tv_addons_program() are add_executable()
# with them.

cmake_minimum_required( VERSION 3.16 )
project( tvision CXX )

if( NOT WIN32 )
    message( FATAL_ERROR "On Linux build with make (see the Makefile)" )
endif()

set( CMAKE_CXX_STANDARD 14 )            # MSVC has no C++11 mode
set( CMAKE_CXX_STANDARD_REQUIRED ON )

# lib/ and bin/ next to the sources, as with make (one configuration
# at a time)
foreach( cfg "" _DEBUG _RELEASE _RELWITHDEBINFO _MINSIZEREL )
    set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY${cfg} ${PROJECT_SOURCE_DIR}/lib )
    set( CMAKE_RUNTIME_OUTPUT_DIRECTORY${cfg} ${PROJECT_SOURCE_DIR}/bin )
endforeach()

# ---- the libraries ---------------------------------------------------------

file( GLOB LIBSRC CONFIGURE_DEPENDS
      source/*.cpp source/platform/*.cpp source/platform/win32/*.cpp )
add_library( tv STATIC ${LIBSRC} )

# include/compat holds the Borland headers (dos.h, iostream.h, ...);
# include/compat/win32 the POSIX ones the library uses (unistd.h)
target_include_directories( tv PUBLIC
    ${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/include/compat
    ${PROJECT_SOURCE_DIR}/include/compat/win32 )

# also for the programs: the library headers need them
target_compile_options( tv PUBLIC
    /utf-8 /permissive- /Zc:__cplusplus
    /Zc:strictStrings-                  # string literals as char *, as in DOS
    /W3
    /wd4244 /wd4267 /wd4305 /wd4309     # narrowing conversions, as -Wno-narrowing
    /wd4996                             # "unsafe" or POSIX named CRT functions
    /wd4800 /wd4065 /wd4101 /wd4018 /wd4146 /wd4838
    /wd4250                             # inherited via dominance (collections)
    /wd4291                             # placement new without its delete
    /wd4312                             # small numbers sent as void * (messages)
)
target_compile_definitions( tv PUBLIC _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_WARNINGS
                            NOMINMAX WIN32_LEAN_AND_MEAN )

# every streamable class registers itself from a static constructor, as
# the DOS library relied on __link(): the whole library goes into the
# program (-Wl,--whole-archive on Linux)
target_link_options( tv INTERFACE /WHOLEARCHIVE:tv.lib )

file( GLOB ADDSRC CONFIGURE_DEPENDS addons/source/*.cpp )
add_library( tvaddons STATIC ${ADDSRC} )
target_include_directories( tvaddons PUBLIC ${PROJECT_SOURCE_DIR}/addons/include )
target_link_libraries( tvaddons PUBLIC tv )
target_link_options( tvaddons INTERFACE /WHOLEARCHIVE:tvaddons.lib )

function( tv_program name )
    add_executable( ${name} ${ARGN} )
    target_link_libraries( ${name} PRIVATE tv )
endfunction()

function( tv_addons_program name )
    add_executable( ${name} ${ARGN} )
    target_link_libraries( ${name} PRIVATE tvaddons )
endfunction()

# ---- the demos ---------------------------------------------------------------

tv_program( tvdemo demos/tvdemo1.cpp demos/tvdemo2.cpp demos/tvdemo3.cpp
            demos/ascii.cpp demos/calendar.cpp demos/calc.cpp
            demos/fileview.cpp demos/gadgets.cpp demos/mousedlg.cpp
            demos/puzzle.cpp )
tv_program( tvedit demos/tvedit1.cpp demos/tvedit2.cpp demos/tvedit3.cpp )
tv_program( hello demos/hello.cpp )

set( FORMSLIB demos/forms.cpp demos/datacoll.cpp demos/fields.cpp demos/listdlg.cpp )
tv_program( tvforms demos/tvforms.cpp ${FORMSLIB} )
tv_program( genphone demos/genform.cpp ${FORMSLIB} )
target_compile_definitions( genphone PRIVATE PHONENUM )
tv_program( genparts demos/genform.cpp ${FORMSLIB} )
target_compile_definitions( genparts PRIVATE PARTS )

# the data files of TVFORMS and the help of TVDEMO, in bin/
add_custom_command( TARGET genphone POST_BUILD
                    COMMAND genphone WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/bin )
add_custom_command( TARGET genparts POST_BUILD
                    COMMAND genparts WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/bin )
add_custom_command( TARGET tvdemo POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                            ${PROJECT_SOURCE_DIR}/demos/demohelp.hlp ${PROJECT_SOURCE_DIR}/bin/ )

tv_program( tvhc help/tvhc.cpp )
tv_program( keytest tools/keytest.cpp )

# YAMLEDIT: TVEDIT with TYamlEditWindow
tv_addons_program( yamledit addons/demos/yamledit.cpp demos/tvedit2.cpp demos/tvedit3.cpp )
target_include_directories( yamledit PRIVATE ${PROJECT_SOURCE_DIR}/demos )

# the examples of the Turbo Vision guide; 06-16 display their own source
file( GLOB DOCDEMOS CONFIGURE_DEPENDS docdemos/*.cpp )
foreach( src ${DOCDEMOS} )
    get_filename_component( name ${src} NAME_WE )
    tv_program( ${name} ${src} )
    target_compile_definitions( ${name} PRIVATE TV_DOCDEMOS_DIR="${PROJECT_SOURCE_DIR}/docdemos" )
endforeach()

# ---- the tests -------------------------------------------------------------

tv_addons_program( yamlsyntax-test addons/tests/yamlsyntax_test.cpp )
tv_addons_program( advedit-test addons/tests/advedit_test.cpp )
tv_addons_program( widgets-test addons/tests/widgets_test.cpp )
# only when this is the project being built, not under add_subdirectory()
if( CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR )
    enable_testing()
    add_test( NAME yamlsyntax COMMAND yamlsyntax-test )
    add_test( NAME advedit COMMAND advedit-test )
    add_test( NAME widgets COMMAND widgets-test )
endif()
