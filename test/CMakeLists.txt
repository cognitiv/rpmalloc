add_executable(test-override main-override.cc thread.c)
target_link_libraries(test-override rpmalloc)

add_executable(test main.c thread.c)
target_link_libraries(test rpmalloc)


add_executable(test-fch main.cc)
target_link_libraries(test-fch rpmalloc)
