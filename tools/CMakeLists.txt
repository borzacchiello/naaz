add_executable ( lift_demo
    lift_demo.cpp )

target_link_libraries ( lift_demo LINK_PUBLIC libnaaz_shared )

add_executable ( bininfo
    bininfo.cpp )

target_link_libraries ( bininfo LINK_PUBLIC libnaaz_shared )

add_executable ( loader_demo
loader_demo.cpp )

target_link_libraries ( loader_demo LINK_PUBLIC libnaaz_shared )

add_executable ( executor_demo
executor_demo.cpp )

target_link_libraries ( executor_demo LINK_PUBLIC libnaaz_shared )

add_executable ( gen_path_demo
gen_path_demo.cpp )

target_link_libraries ( gen_path_demo LINK_PUBLIC libnaaz_shared )
