vcpkg_from_git(
        OUT_SOURCE_PATH SOURCE_PATH
        URL "file:///E:/code/ROSE"
        REF <some-commit-hash>
)

vcpkg_cmake_configure(SOURCE_PATH "${SOURCE_PATH}")
vcpkg_cmake_install()