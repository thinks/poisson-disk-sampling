if(PROJECT_IS_TOP_LEVEL)
  set(
      CMAKE_INSTALL_INCLUDEDIR "include/tph_poisson-${PROJECT_VERSION}"
      CACHE STRING ""
  )
  set_property(CACHE CMAKE_INSTALL_INCLUDEDIR PROPERTY TYPE PATH)
endif()

# Project is configured with no languages, so tell GNUInstallDirs the lib dir
set(CMAKE_INSTALL_LIBDIR lib CACHE PATH "")

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

# find_package(<package>) call for consumers to find this project
set(package tph_poisson)

install(
    DIRECTORY include/
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    COMPONENT tph_poisson_Development
)

install(
    TARGETS thinks_tph_poisson
    EXPORT tph_poissonTargets
    INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

write_basic_package_version_file(
    "${package}ConfigVersion.cmake"
    COMPATIBILITY SameMinorVersion
    ARCH_INDEPENDENT
)

# Allow package maintainers to freely override the path for the configs
set(
    tph_poisson_INSTALL_CMAKEDIR "${CMAKE_INSTALL_DATADIR}/${package}"
    CACHE STRING "CMake package config location relative to the install prefix"
)
set_property(CACHE tph_poisson_INSTALL_CMAKEDIR PROPERTY TYPE PATH)
mark_as_advanced(tph_poisson_INSTALL_CMAKEDIR)

install(
    FILES cmake/install-config.cmake
    DESTINATION "${tph_poisson_INSTALL_CMAKEDIR}"
    RENAME "${package}Config.cmake"
    COMPONENT tph_poisson_Development
)

install(
    FILES "${PROJECT_BINARY_DIR}/${package}ConfigVersion.cmake"
    DESTINATION "${tph_poisson_INSTALL_CMAKEDIR}"
    COMPONENT tph_poisson_Development
)

install(
    EXPORT tph_poissonTargets
    NAMESPACE thinks::
    DESTINATION "${tph_poisson_INSTALL_CMAKEDIR}"
    COMPONENT tph_poisson_Development
)

if(PROJECT_IS_TOP_LEVEL)
  include(CPack)
endif()
