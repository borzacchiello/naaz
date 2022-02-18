add_subdirectory( ${PROJECT_SOURCE_DIR}/third_party/catch2 ${CMAKE_CURRENT_BINARY_DIR}/catch2 )

add_executable ( expr_tests
    common_main.cpp
    expr_tests.cpp )

add_executable ( state_tests
    common_main.cpp
    state_tests.cpp )

target_link_libraries ( expr_tests LINK_PUBLIC libnaaz_shared )
target_link_libraries ( expr_tests PRIVATE Catch2::Catch2WithMain )

target_link_libraries ( state_tests LINK_PUBLIC libnaaz_shared )
target_link_libraries ( state_tests PRIVATE Catch2::Catch2WithMain )
