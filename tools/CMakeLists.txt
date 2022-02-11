add_executable ( lift_demo
    lift_demo.cpp )

target_link_libraries ( lift_demo LINK_PUBLIC libnaaz_shared )

add_executable ( bininfo
    bininfo.cpp )

target_link_libraries ( bininfo LINK_PUBLIC libnaaz_shared )
