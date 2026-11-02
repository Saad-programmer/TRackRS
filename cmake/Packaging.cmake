# Preserve the existing distribution's exclusion of unlicensed plugin archives.
set(TRACKRS_UNLICENSED event-rscup event-westernchallenge map-crossmountain map-ghats
  map-greengrounds map-helicoil map-icypeak map-monza map-mountainpass map-pulp
  map-roundhouse map-santa map-serpentine map-snowlevel map-snowyhills map-tea map-tobago)
set(asset_exclusions)
foreach(pack IN LISTS TRACKRS_UNLICENSED)
  list(APPEND asset_exclusions PATTERN "${pack}.zip" EXCLUDE)
endforeach()
install(TARGETS trackrs RUNTIME DESTINATION bin)
install(DIRECTORY data/ DESTINATION share/games/trackrs ${asset_exclusions})
file(READ "${CMAKE_SOURCE_DIR}/bin/trackrs.config.defs" install_config)
string(REPLACE "../data" "../share/games/trackrs" install_config "${install_config}")
file(WRITE "${CMAKE_BINARY_DIR}/install-config/trackrs.config.defs" "${install_config}")
install(FILES "${CMAKE_BINARY_DIR}/install-config/trackrs.config.defs" DESTINATION bin)
install(FILES doc/COPYING.txt doc/DATA_AUTHORS.txt debian/copyright DESTINATION share/doc/trackrs)
install(DIRECTORY docs/licenses/ DESTINATION share/doc/trackrs/dependency-notices)
install(FILES data/metainfo/trackrs.desktop DESTINATION share/applications)
install(FILES data/icon/trigger-256.png DESTINATION share/pixmaps RENAME trackrs.png)
install(FILES "${CMAKE_BINARY_DIR}/generated/org.trackrs.TrackRS.metainfo.xml" DESTINATION share/metainfo)
set(CPACK_PACKAGE_NAME trackrs)
set(CPACK_PACKAGE_VENDOR "TrackRS contributors")
set(CPACK_PACKAGE_CONTACT "TrackRS contributors")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Single-player rally game")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY TrackRS)
set(CPACK_PACKAGE_EXECUTABLES trackrs TrackRS)
if(WIN32)
  set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION bin)
  include(InstallRequiredSystemLibraries)
  set(CPACK_GENERATOR "ZIP;NSIS")
  # MinGW does not copy dependency DLLs beside the executable. Resolve them from
  # the discovered SDK and compiler bin directories, never an assumed drive.
  set(TRACKRS_DLL_DIRS "$<TARGET_FILE_DIR:trackrs>")
  get_filename_component(compiler_bin "${CMAKE_CXX_COMPILER}" DIRECTORY)
  list(APPEND TRACKRS_DLL_DIRS "${compiler_bin}")
  foreach(library IN LISTS TRACKRS_LIBRARIES)
    get_filename_component(library_dir "${library}" DIRECTORY)
    get_filename_component(sdk_bin "${library_dir}/../bin" ABSOLUTE)
    list(APPEND TRACKRS_DLL_DIRS "${library_dir}" "${sdk_bin}")
  endforeach()
  list(REMOVE_DUPLICATES TRACKRS_DLL_DIRS)
  install(CODE "set(TRACKRS_PACKAGE_DLL_DIRS \"${TRACKRS_DLL_DIRS}\")")
  install(CODE [[
    if(POLICY CMP0207)
      cmake_policy(SET CMP0207 NEW)
    endif()
    file(GET_RUNTIME_DEPENDENCIES EXECUTABLES "$<TARGET_FILE:trackrs>"
      RESOLVED_DEPENDENCIES_VAR resolved UNRESOLVED_DEPENDENCIES_VAR unresolved
      DIRECTORIES ${TRACKRS_PACKAGE_DLL_DIRS}
      PRE_EXCLUDE_REGEXES "api-ms-.*" "ext-ms-.*"
      POST_EXCLUDE_REGEXES "^[A-Za-z]:.[Ww][Ii][Nn][Dd][Oo][Ww][Ss].*")
    if(unresolved)
      message(FATAL_ERROR "Unresolved package DLLs: ${unresolved}")
    endif()
    file(INSTALL ${resolved} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE SHARED_LIBRARY)
  ]])
else()
  set(CPACK_GENERATOR "DEB;ZIP")
  set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
  # These stable ABI packages also cover rootless SDKs absent from dpkg status.
  set(CPACK_DEBIAN_PACKAGE_DEPENDS "libsdl2-image-2.0-0, libalut0, libphysfs1")
  set(CPACK_DEBIAN_PACKAGE_SECTION games)
  set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
  set(CPACK_PACKAGING_INSTALL_PREFIX /usr)
endif()
include(CPack)

if(UNIX AND NOT APPLE)
  add_custom_target(appimage
    COMMAND "${CMAKE_SOURCE_DIR}/tools/package_appimage.sh" "${CMAKE_BINARY_DIR}"
    DEPENDS trackrs USES_TERMINAL)
endif()
