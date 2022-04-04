include_directories ( ${CMAKE_SOURCE_DIR}/third_party/argparse/include )

add_executable ( bininfo
    bininfo.cpp )

target_link_libraries ( bininfo LINK_PUBLIC libnaaz_shared )

add_executable ( loader_demo
loader_demo.cpp )

target_link_libraries ( loader_demo LINK_PUBLIC libnaaz_shared )

add_executable ( naaz_finder
naaz_finder.cpp )

target_link_libraries ( naaz_finder LINK_PUBLIC libnaaz_shared )

add_executable ( naaz_path_generator
naaz_path_generator.cpp )

target_link_libraries ( naaz_path_generator LINK_PUBLIC libnaaz_shared )
