set(RPMALLOC_HEADERS rpnew.h rpmalloc.h)

add_library(rpmalloc rpmalloc.c)

target_include_directories(rpmalloc PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:include/rpmalloc>
)

install(TARGETS rpmalloc
  EXPORT rpmalloc-targets
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

foreach(header ${RPMALLOC_HEADERS})
  get_filename_component(dir ${header} DIRECTORY )
  install(FILES ${header}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/rpmalloc/${dir}
  )
endforeach()

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  "${PROJECT_BINARY_DIR}/cmake/rpmalloc-config-version.cmake"
  VERSION ${CMAKE_PROJECT_VERSION}
  COMPATIBILITY AnyNewerVersion
)

configure_file(${PROJECT_SOURCE_DIR}/build/rpmalloc-config.cmake.in
  ${PROJECT_BINARY_DIR}/cmake/rpmalloc-config.cmake
  COPYONLY
)
install(FILES ${PROJECT_BINARY_DIR}/cmake/rpmalloc-config.cmake
  ${PROJECT_BINARY_DIR}/cmake/rpmalloc-config-version.cmake
  DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/rpmalloc
)
install(EXPORT rpmalloc-targets
  DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/rpmalloc
)
