# CPM Package Lock
# This file should be committed to version control

# CPMPackageProject
CPMDeclarePackage(CPMPackageProject
  NAME PackageProject.cmake
  GITHUB_REPOSITORY TheLartians/PackageProject.cmake
  VERSION 1.11.2
)

# googletest
CPMDeclarePackage(googletest
  NAME googletest
  VERSION 1.12.1
  GIT_TAG release-1.12.1
  GITHUB_REPOSITORY google/googletest
  OPTIONS
    "INSTALL_GTEST OFF"
    "gtest_force_shared_crt"
    "ECLUDE_FROM_ALL True"
)
