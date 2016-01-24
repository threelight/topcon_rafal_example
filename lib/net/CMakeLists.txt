

INCLUDE( ${THREELIGHT_SOURCE_DIR}/CMakeListsCommon.txt )

include_directories (${THREELIGHT_SOURCE_DIR}/lib/general/include)
include_directories (${THREELIGHT_SOURCE_DIR}/lib/net/include)
include_directories (${THREELIGHT_SOURCE_DIR}/lib/db/include)


add_library (TLLibNet
		TL_TCPServer.cpp
		TL_TCPClient.cpp
		TL_EPollServer.cpp
		TL_EventBuffer.cpp
		TL_SocketEventManager.cpp
		TL_ThreadGroupedClients.cpp
		TL_MessageDispatcher.cpp
		TL_LoggedInClients.cpp
		TL_Connection.cpp
		TL_ProtocolLogic.cpp
		)
