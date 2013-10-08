add_definitions(-Wall -Wno-deprecated -DPACKAGE_VERSION="${PACKAGE_VERSION}")
#add_definitions(-fvisibility=hidden -fvisibility-inlines-hidden -DBUILD_LIBSMATRIX=1)
set(CMAKE_C_FLAGS "-std=c11 -pthread")

add_library(smatrix SHARED smatrix.c)
target_link_libraries(smatrix pthread m)
set_target_properties(smatrix PROPERTIES VERSION ${PACKAGE_VERSION})
