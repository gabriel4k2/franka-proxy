# Download and Patch
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO frankaemika/libfranka
    REF f3b8d775a9c847cab32684c8a316f67867761674
    SHA512 68050fa184f8a94d47165f4747d889b8647c65f3d9fb028f748aba7239d2ee2057089abaa4497b51757f535666404b19031c84887f0d111400356af881a19874
	PATCHES
      0001-fix-common-cmake.patch
)

vcpkg_from_github(
    OUT_SOURCE_PATH COMMON_SOURCE_PATH
    REPO frankaemika/libfranka-common
    REF e6aa0fc210d93fe618bfd8956829a264d5476ba8
    SHA512 225300c47be41a180da6eaabe28a9e69f6a940fb42a2752eb27d0053e6fdcd05d003da0a3987f0e88d9a15a6fc9677c815460d3155526685012f06e37a5b9542
	PATCHES
      0001-fix-cmake.patch
)

file(COPY ${COMMON_SOURCE_PATH}/ DESTINATION ${SOURCE_PATH}/common)

# Build and Install
vcpkg_configure_cmake(
    SOURCE_PATH ${SOURCE_PATH}
    PREFER_NINJA
    OPTIONS
        -DBUILD_COVERAGE=OFF
        -DBUILD_TESTS=OFF
        -DBUILD_EXAMPLES=OFF
        -DBUILD_DOCUMENTATION=OFF
)

vcpkg_install_cmake()

# Fix cmake structure
vcpkg_fixup_cmake_targets(CONFIG_PATH lib/cmake/Franka)
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")

# Handle copyright
file(INSTALL ${SOURCE_PATH}/LICENSE DESTINATION ${CURRENT_PACKAGES_DIR}/share/${PORT} RENAME copyright)

# Post-build test for cmake libraries
#SET(VCPKG_POLICY_DLLS_WITHOUT_LIBS enabled)
#vcpkg_test_cmake(PACKAGE_NAME franka)
