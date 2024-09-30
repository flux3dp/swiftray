include(BundleUtilities)

# Get the app bundle path
get_filename_component(BUNDLE_PATH "${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app" ABSOLUTE)

set(LIB_SEARCH_DIRS "/usr/local/Cellar/gcc/14.1.0_1/lib/gcc/current;/usr/local/opt/opencv/lib;/usr/local/Cellar/abseil/20240116.2/lib;/usr/local/Cellar/poppler/24.04.0/lib")
# Use fixup_bundle to copy and fix dependencies
fixup_bundle("${BUNDLE_PATH}" "" "${LIB_SEARCH_DIRS}")

# Verify the bundle
verify_app("${BUNDLE_PATH}")