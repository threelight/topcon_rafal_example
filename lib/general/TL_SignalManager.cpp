#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <iostream>
#include <map>
#include <set>
#include <list>


#include "TL_SignalManager.h"
#include "TL_Thread.h"
#include "TL_Exception.h"
#include "TL_Semaphore.h"
#include "TL_OSUtil.h"


using namespace std;


namespace ThreeLight
{


int __supportedSignals[] = {SIGUSR1, SIGQUIT, SIGINT, SIGHUP, 0};


template<>
struct ImplOf<TL_SignalManager>
{
	typedef void (*Handler)(int);

	ImplOf()
	{
		int i=0;

		for(i=0; __supportedSignals[i]; ++i)
		{
			_SemaphoreArray.add(__supportedSignals[i]);
		}

		for(i=0; __supportedSignals[i]; ++i)
		{
			signalAction(__supportedSignals[i],
				SignalHandler,
				signalsToBlock() );
		}
	}

	void
	signalAction(int signum_, Handler handler_, sigset_t blocked_)
	{
		int sa_flags = 0;

		if(signum_ == SIGCHLD)
		{
			// If signum is SIGCHLD, do not
			// receive notification when child
			// processes stop (i.e., when  they  receive
			// one of SIGSTOP, SIGTSTP, SIGTTIN or SIGTTOU)
			// or resume (i.e., they receive SIGCONT) (see wait(2)).
			sa_flags |= SA_NOCLDSTOP;

			//(Linux  2.6  and  later) If signum is SIGCHLD,
			// do not transform children into zombies
			// when they terminate.  See also waitpid(2).
			sa_flags |= SA_NOCLDWAIT;
		}

		// Provide behaviour compatible with BSD
		// signal semantics by making certain
		// system  calls  restartable across signals.
		sa_flags |= SA_RESTART;
		
		struct sigaction action;

		action.sa_handler = handler_;
		action.sa_mask = blocked_;
		action.sa_flags = sa_flags;
		
		sigProcMask(SIG_UNBLOCK, signum_);

		if( sigaction(signum_, &action, NULL) == -1 )
		{
			throw TL_Exception<TL_SignalManager>("ImplOf<TL_SignalManager>::signalAction",
				TL_OSUtil::GetErrorString(errno) );
		}
	}

	void
	sigProcMask(int how_, int signum_)
	{	
		sigset_t set;
		
		sigemptyset(&set);
		sigaddset(&set, signum_);
		pthread_sigmask(how_, &set, NULL);
	}

	sigset_t
	signalsToBlock()
	{
		sigset_t blocked;
		// we are blocking all signals
		// during receiving any other.
		sigfillset(&blocked);
		return blocked;
	}
	
	// not used for now
	void
	setUpPipe()
	{
		if(pipe(_SignalPipe) == -1)
		{
			throw TL_Exception<TL_SignalManager>("ImplOf<TL_SignalManager>::setUpPipe",
				TL_OSUtil::GetErrorString(errno) );
		}
		
		fcntl(_SignalPipe[0], F_SETFD, O_NONBLOCK);
		fcntl(_SignalPipe[1], F_SETFD, O_NONBLOCK);
	}

	static void
	SignalHandler(int signum_)
	{
		if(pthread_equal( TL_Thread::MainThreatId(), pthread_self() ))
		{
			_SemaphoreArray[signum_]->post();
			_SignalAny.post();
		}
	}
	
	class TL_SemaphoreArray
	{
	public:
		TL_SemaphoreArray(){};

		~TL_SemaphoreArray()
		{
			std::map<int, TL_Semaphore*>::const_iterator
				iter = _array.begin();
			for(; iter!=_array.end(); ++iter)
			{
				delete (*iter).second;
			}
		};
		
		void
		add(int signum_)
		{
			std::map<int, TL_Semaphore*>::const_iterator
				iter = _array.find(signum_);

			assert(iter==_array.end());

			_array[signum_] = new TL_Semaphore();
		}

		TL_Semaphore*
		operator[](int signum_)
		{
			std::map<int, TL_Semaphore*>::iterator
				iter = _array.find(signum_);

			assert(iter!=_array.end());

			return (*iter).second;
		}

	private:
		std::map<int, TL_Semaphore*> _array;
	};

	// not used for now
	int _SignalPipe[2];

	static TL_SemaphoreArray _SemaphoreArray;
	static TL_Semaphore _SignalAny;

	static TL_SignalManager *_Manager;

	typedef std::map<int, std::set<TL_SignalManager_I*> > ReceiversType;

	ReceiversType _receivers;
};


ImplOf<TL_SignalManager>::TL_SemaphoreArray ImplOf<TL_SignalManager>::_SemaphoreArray;

TL_Semaphore ImplOf<TL_SignalManager>::_SignalAny;

TL_SignalManager* ImplOf<TL_SignalManager>::_Manager = NULL;



TL_SignalManager&
TL_SignalManager::Instance()
{
	if(ImplOf<TL_SignalManager>::_Manager == NULL)
	{
		// only main thread can instantiate the manager
		assert(pthread_equal(TL_Thread::MainThreatId(), pthread_self()) != 0);
		ImplOf<TL_SignalManager>::_Manager =  new TL_SignalManager();
	}
	return *ImplOf<TL_SignalManager>::_Manager;
}


void
TL_SignalManager::Destroy()
{
	if(ImplOf<TL_SignalManager>::_Manager != NULL)
	{
		assert(pthread_equal(TL_Thread::MainThreatId(), pthread_self()) != 0);
		delete ImplOf<TL_SignalManager>::_Manager;
		ImplOf<TL_SignalManager>::_Manager = NULL;
	}
}


void
TL_SignalManager::addSignalReceiver(TL_SignalManager_I *receiver_, int signum_)
{
	_p->_receivers[signum_].insert(receiver_);
}


void
TL_SignalManager::removeSignalReceiver(TL_SignalManager_I *receiver_, int signum_)
{
	_p->_receivers[signum_].erase(receiver_);
}


void
TL_SignalManager::sigProcMask(int how_, int signum_)
{
	_p->sigProcMask(how_, signum_);
}


void
TL_SignalManager::process(const struct timespec* absTime_)
{
	std::list<int> receivedSignals;
	TL_Semaphore *ptr;
	int value;
	bool signalExists = false;

	do
	{
		for(int i=0; __supportedSignals[i]; ++i)
		{
			ptr = _p->_SemaphoreArray[ __supportedSignals[i] ];
			value = ptr->getValue();
			for(int k=0; k<value; ++k)
			{
				receivedSignals.push_back(__supportedSignals[i]);
				signalExists=true;
			}
			ptr->wait(value);
		}

		if(signalExists) break;

	} while( ImplOf<TL_SignalManager>::_SignalAny.timedWait(absTime_) );

	std::list<int>::const_iterator iter = receivedSignals.begin();
	std::set<TL_SignalManager_I*>::iterator receiverIter;

	for(; iter!=receivedSignals.end(); ++iter)
	{
		receiverIter = _p->_receivers[*iter].begin();
		for(; receiverIter != _p->_receivers[*iter].end(); ++receiverIter)
		{
			(*receiverIter)->signal(*iter);
		}
	}
}


TL_SignalManager::TL_SignalManager()
{
	// by default ignore SIGPIPE for the main proces
	
	
}


TL_SignalManager::~TL_SignalManager()
{
}


} // end of ThreeLight namespace
