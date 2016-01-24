#ifndef __TL_MESSAGEDISPATCHER__
#define __TL_MESSAGEDISPATCHER__


#include <stdint.h>
#include <list>


#include "TL_Pimpl.h"
#include "TL_Thread.h"


namespace ThreeLight
{


/*! \brief Accepts messages to clients
*	ID + Message
*/

class TL_MessageDispatcher:
	public TL_Thread
{
public:
	static TL_MessageDispatcher* Instance();


	typedef struct Message
	{
		Message()
		{}

		Message(char *message_, uint64_t client_):
			_message(message_), _client(client_)
		{}

		char *_message;
		uint64_t _client;
	} TL_MessageType;

	typedef std::list<TL_MessageType> TL_MessageListType;
	
	void addMessage(uint64_t client_, char* message_);
	void addMessages(const TL_MessageListType& messages_);

	/*!\brief Fuction called by ThreadGroupClients
	 *	bulk way of retrieving messages
	 */
	void getMessages(std::list<uint64_t>& clients_, std::list<TL_MessageType>& messages_);

protected:
	/*!\brief Checks for non loged clients and send messages to some storege (?)
	 */
	void run_I() throw();

private:
	TL_MessageDispatcher(uint32_t maxQueueSize_);
	~TL_MessageDispatcher();

	TL_Pimpl<TL_MessageDispatcher>::Type _p;
};


}

#endif
