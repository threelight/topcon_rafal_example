

INCLUDE( ${THREELIGHT_SOURCE_DIR}/CMakeListsCommon.txt )

include_directories (${THREELIGHT_SOURCE_DIR}/lib/general/include)
include_directories (/usr/include/libxml++-1.0)

add_library (TLLibGeneral
		TL_Exception.cpp
		TL_Thread.cpp
		TL_Mutex.cpp
		TL_ThreadManager.cpp
		TL_Time.cpp
		TL_OSUtil.cpp
		TL_Log.cpp
		TL_AssocArray.cpp
		TL_Xml2Array.cpp
		TL_Semaphore.cpp
		TL_SignalManager.cpp
		TL_CondVariable.cpp
		TL_GlobalStaticDestroyer.cpp
		TL_MemoryLeakReport.cpp
		)

#SET(CMAKE_CXX_FLAGS "-Wall")
