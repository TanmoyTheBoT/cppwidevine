#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "widevine::widevine" for configuration "Release"
set_property(TARGET widevine::widevine APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(widevine::widevine PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/widevine.lib"
  )

list(APPEND _cmake_import_check_targets widevine::widevine )
list(APPEND _cmake_import_check_files_for_widevine::widevine "${_IMPORT_PREFIX}/lib/widevine.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
