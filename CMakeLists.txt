cmake_minimum_required ( VERSION 3.18 )

project ( NAAZ )

# libcsleigh
include_directories ( ${CMAKE_SOURCE_DIR}/third_party/sleigh )
link_directories ( ${CMAKE_SOURCE_DIR}/third_party/sleigh/build )

# libpugixml
include_directories ( ${CMAKE_SOURCE_DIR}/third_party/pugixml/src )
link_directories ( ${CMAKE_SOURCE_DIR}/third_party/pugixml/build )

set ( CMAKE_CXX_STANDARD 20 )

set ( naaz_src
    util/ioutil.cpp
    expr/Expr.cpp
    expr/ExprBuilder.cpp
    arch/Arch.cpp
    lifter/PCodeLifter.cpp
    loader/AddressSpace.cpp
    loader/BFDLoader.cpp )

add_library ( objLibNaaz OBJECT ${naaz_src} )

set_property ( TARGET objLibNaaz PROPERTY POSITION_INDEPENDENT_CODE 1 )

add_library ( libnaaz_static STATIC $<TARGET_OBJECTS:objLibNaaz> )
add_library ( libnaaz_shared SHARED $<TARGET_OBJECTS:objLibNaaz> )

set_target_properties ( libnaaz_static PROPERTIES OUTPUT_NAME naaz )
set_target_properties ( libnaaz_shared PROPERTIES OUTPUT_NAME naaz )

target_link_libraries ( libnaaz_shared LINK_PUBLIC bfd )
target_link_libraries ( libnaaz_static LINK_PUBLIC bfd )

target_link_libraries ( libnaaz_shared LINK_PUBLIC csleigh )
target_link_libraries ( libnaaz_static LINK_PUBLIC csleigh )

target_link_libraries ( libnaaz_shared LINK_PUBLIC pugixml )
target_link_libraries ( libnaaz_static LINK_PUBLIC pugixml )

set ( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address,undefined" )
set ( CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address,undefined" )

add_subdirectory ( tools )
add_subdirectory ( tests )
