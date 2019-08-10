cmake_minimum_required(VERSION 3.15)

add_executable(test_app test_app.cpp)
target_link_libraries(test_app malloc_geiger)
target_include_directories(test_app PRIVATE ../malloc_geiger)
install(TARGETS test_app)