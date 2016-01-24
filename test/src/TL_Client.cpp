#include "TL_Client.h"
#include "TL_TCPClient.h"
#include "TL_Connection.h"
#include "TL_CreateRequest.h"


using namespace std;


namespace ThreeLight
{


template<>
struct ImplOf<TL_Client>
{

	ImplOf():
		_connection(NULL),
		_socket(-1)
	{
	}


	bool connect()
	{
		bool ret = false;
		_socket = _client.connect( _host.c_std(), _port.c_std() );

		if(_socket > 0)
		{
			_connection = new TL_Connection(_socket);
			ret = true;
		}

		return ret;
	}


	bool login()
	{
		
	}
	
	string _host;
	string _port;
	TL_TCPClient _client;
	TL_Connection *_connection;

	int _socket;
};


TL_Client::TL_Client(const char* host_, const char* port_)
{
	_p->_host = host_;
	_p->_port = port_;
}


bool
TL_Client::connect()
{
	return _p->_client.connect();
}


bool
TL_Client::login()
{
	return _p->login();
}


}