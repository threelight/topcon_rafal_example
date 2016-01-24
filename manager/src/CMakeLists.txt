INCLUDE( ${THREELIGHT_SOURCE_DIR}/CMakeListsCommon.txt )

include_directories (${THREELIGHT_SOURCE_DIR}/manager/include)
include_directories (${THREELIGHT_SOURCE_DIR}/lib/general/include)
include_directories (${THREELIGHT_SOURCE_DIR}/lib/net/include)

add_executable (Manager Manager.cpp)



SET_TARGET_PROPERTIES(Manager PROPERTIES LINK_FLAGS "-Wall -pthread -lrt -lxml2")

link_directories (${THREELIGHT_BINARY_DIR}/lib/src)

target_link_libraries (Manager TLLibNet TLLibGeneral)