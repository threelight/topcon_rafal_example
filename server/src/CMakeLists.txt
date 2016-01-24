

INCLUDE( ${THREELIGHT_SOURCE_DIR}/CMakeListsCommon.txt )

include_directories (${THREELIGHT_SOURCE_DIR}/server/include)
include_directories (${THREELIGHT_SOURCE_DIR}/lib/general/include)
include_directories (${THREELIGHT_SOURCE_DIR}/lib/net/include)
include_directories (${THREELIGHT_SOURCE_DIR}/lib/db/include)
include_directories (/usr/include/mysql++)
include_directories (/usr/include/mysql)

add_executable (ThreeLight
		server.cpp
		TL_Central.cpp
		)



SET_TARGET_PROPERTIES(ThreeLight
	PROPERTIES LINK_FLAGS
		"-Wall -pthread -lrt -lxml2 -lmysqlpp -lmysqlclient_r")

link_directories (${THREELIGHT_BINARY_DIR}/lib/src)

target_link_libraries (ThreeLight TLLibNet TLLibGeneral TLLibDb)

