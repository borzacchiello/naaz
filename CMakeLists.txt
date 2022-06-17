cmake_minimum_required ( VERSION 3.16 )

include ( CheckCXXCompilerFlag )

project ( NAAZ )

option ( ASAN "Compile with asan (only for debug builds)" OFF )
option ( OPTIMIZE_NATIVE "Compile with -march=native" OFF )

# libcsleigh
include_directories ( ${CMAKE_SOURCE_DIR}/third_party/sleigh )
link_directories ( ${CMAKE_SOURCE_DIR}/third_party/sleigh/build )

# libz3
include_directories ( ${CMAKE_SOURCE_DIR}/third_party/z3/build/install/include )
link_directories ( ${CMAKE_SOURCE_DIR}/third_party/z3/build/install/lib )

# libpugixml
include_directories ( ${CMAKE_SOURCE_DIR}/third_party/pugixml/src )
link_directories ( ${CMAKE_SOURCE_DIR}/third_party/pugixml/build )

# json
include_directories ( ${CMAKE_SOURCE_DIR}/third_party/json )

# libbfd
include_directories ( ${CMAKE_SOURCE_DIR}/third_party/binutils/bfd )
include_directories ( ${CMAKE_SOURCE_DIR}/third_party/binutils/include )
link_directories ( ${CMAKE_SOURCE_DIR}/third_party/binutils/build )

# libgmp
include_directories ( ${CMAKE_SOURCE_DIR}/third_party/gmp )
link_directories ( ${CMAKE_SOURCE_DIR}/third_party/gmp/build )

set ( CMAKE_CXX_STANDARD 20 )

set ( naaz_src
    util/ioutil.cpp
    util/strutil.cpp
    util/parseutil.cpp
    util/config.cpp
    expr/BVConst.cpp
    expr/FPConst.cpp
    expr/Expr.cpp
    expr/ExprBuilder.cpp
    expr/util.cpp
    state/MapMemory.cpp
    state/State.cpp
    state/Solver.cpp
    state/File.cpp
    state/FileSystem.cpp
    state/PluginManager.cpp
    state/Linux64Platform.cpp
    state/UnknownPlatform.cpp
    models/Linker.cpp
    models/libc/libc_start_main.cpp
    models/libc/exit.cpp
    models/libc/posix_file_handling.cpp
    models/libc/string.cpp
    models/libc/string_utils.cpp
    models/libc/stdio.cpp
    models/libc/random.cpp
    models/libc/ptrace.cpp
    models/libc/alloc.cpp
    models/linux_syscalls/open.cpp
    models/linux_syscalls/read.cpp
    arch/Arch.cpp
    arch/x86_64.cpp
    arch/arm32LE.cpp
    lifter/PCodeLifter.cpp
    loader/AddressSpace.cpp
    loader/BFDLoader.cpp
    executor/PCodeExecutor.cpp
    executor/ExplorationTechnique.cpp
    executor/BFSExplorationTechnique.cpp
    executor/DFSExplorationTechnique.cpp
    executor/RandDFSExplorationTechnique.cpp
    executor/CovExplorationTechnique.cpp
    solver/ConstraintManager.cpp
    solver/Z3Solver.cpp )

set ( naaz_linked_libs
    bfd
    iberty
    z
    csleigh
    pugixml
    z3
    gmp
    dl
    pthread )

add_library ( objLibNaaz OBJECT ${naaz_src} )

set_property ( TARGET objLibNaaz PROPERTY POSITION_INDEPENDENT_CODE 1 )

add_library ( libnaaz_static STATIC $<TARGET_OBJECTS:objLibNaaz> )
add_library ( libnaaz_shared SHARED $<TARGET_OBJECTS:objLibNaaz> )

set_target_properties ( libnaaz_static PROPERTIES OUTPUT_NAME naaz )
set_target_properties ( libnaaz_shared PROPERTIES OUTPUT_NAME naaz )

target_link_libraries ( libnaaz_shared LINK_PUBLIC ${naaz_linked_libs} )
target_link_libraries ( libnaaz_static LINK_PUBLIC ${naaz_linked_libs} )

if ( ASAN )
    set ( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address,undefined" )
    set ( CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address,undefined" )
endif ()

if ( OPTIMIZE_NATIVE )
    CHECK_CXX_COMPILER_FLAG ( "-march=native" COMPILER_SUPPORTS_MARCH_NATIVE )
    if ( COMPILER_SUPPORTS_MARCH_NATIVE )
        set ( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native" )
    endif ()
endif ()

add_subdirectory ( tools )
add_subdirectory ( tests )
