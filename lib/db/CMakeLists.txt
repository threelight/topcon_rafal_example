INCLUDE( ${THREELIGHT_SOURCE_DIR}/CMakeListsCommon.txt )

include_directories (${THREELIGHT_SOURCE_DIR}/lib/general/include)
include_directories (${THREELIGHT_SOURCE_DIR}/lib/db/include)
include_directories (/usr/include/mysql++)
include_directories (/usr/include/mysql)

add_library (TLLibDb
		TL_Db.cpp
		TL_Login.cpp
		)

