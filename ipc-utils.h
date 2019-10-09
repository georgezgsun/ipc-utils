#pragma once
#include <sys/mman.h> // for shared memory related
#include <sys/stat.h> // for mode constants
#include <mqueue.h>  // for message queue
#include <fcntl.h> // for O_* constants
#include <unistd.h> // for usleep
#include <cstdint>
#include <cstring> // for str copy
#include <string>  // for c++ string

#define MAX_ELEMENTS 256
#define MAX_MESSAGECHANNELS 256
#define MAX_MESSAGELENGTH 1024
#define MSG_NULL 0
#define MSG_DATA 1
#define MSG_QUERY 2
#define MSG_LOG 3
#define MSG_WATCHDOG 4
#define MSG_DOWN 5
#define MSG_COMMAND 6
#define MSG_ONBOARD 11
#define MSG_LIST 12
#define MSG_UPDATE 13

using namespace std;

// ShMem class
// Objective: provide utilities for shared memory in publisher-subscriber model. 
// Principle: High cohesion and low coupling.
//	1.	All modules in Roswell or assigned projects shall use these utilities to publish or subscribe public data.
//	2.	Shared memory here is designed to follow publisher - subscriber model. Any elements in the shared memory can 
//		have only single publisher but multiple subscribers.
//	3.	Publisher can publish its elements at any moments regardless the behaviors of any other publishers or subscribers. 
//		The publisher does not care or know who are the subscribers. Multiple publishers are allowed to publish their own elements in different process/thread simutaneously. 
//	4.	Subscribers can read its subscriptions (the shared elements) at any moments regardless who is writing or reading them. 
//		Multiple subscribers are allowed to read the same elements in different process/thread simutaneously.
//	5.	Every module is allowed to have multiple publishers together with multiple subscribers. 
//	6.	Each shared element always occupies the same location in the shared memory no matter how many times it is declared or loaded in the module.
//	7.	The writing and reading to the shared memory are all operations that are non blocking, non locking, and multithreaded safe.
//	8.	255 publishers can be created in this implementation, where each of them can have any data structure with a size less than 32K.
//	9.	The total data of all elements can has a size also no more than 32K.
//	10.	Each element is identified by its name in string for at most 15 characters. Refering the name in every process/thread will always has
//		the same result.
//	11.	The element published by a publisher will remain in the kernel even when the process is terminated.
//

// MsgQ class
// Objective: provide basic utilities for message queue. 
// Principle: High cohesion and low coupling.
//	1.	All modules in Roswell or assigned projects shall use these utilities to send or receive messages to each other.
//	2.	Message queue defined here follows single reader and multiple writers model. Any message queues can have only single receiver 
//		but multiple senders.
//	3.	Senders can send messages to the destnate receiver at any moments without knowing the state of the receiver and other senders. 
//		Multiple senders are allowed to send their messages to a receiver in different process/thread simutaneously. 
//	4.	Receiver can read its messages at its convenient time. The messages are in FIFO series.
//	5.	Multiple receivers and senders can be defined and worked in single module. 
//	6.	The receiving of a message is a blocking opertion with timeout. It will block until either the message queue has a message 
//		or the timeout expires. The timeout can be specified between 10us to 1s.
//  7. 	The sending of a message is also a blocking operation with timeout. It will block until either the message queue has available 
//		space for the new message or a small 1ms timeout expires.
//	8.	The receiving and the sending of message are all non locking and multithreaded safe.
//	9.	The message queue will remain in the kernel even when the process that created it is terminated. All messages in the queue remains there.
//	10. Each message queue is identified by its name in string with at most 8 characters. 
//	11.	We introduce the concept of message channel here for the conience to distinguish the message sending and reading. They ocuppy different channels.
// 

// The preparation. We need to have several common directories setup and an environment variable LD_LIBRARY_PATH been created/setup.
//	mkdir ~/projects
//	mkdir ~/projects/common
//	mkdir ~/projects/common/include
//	mkdir ~/projects/common/lib
//	export LD_LIBRARY_PATH=$HOME/projects/common/lib:$LD_LIBRARY_PATH

struct shm_header
{
	uint16_t offset;
	uint16_t size;
};

struct mq_buffer
{
	uint64_t name;
	uint32_t ts;
	uint16_t type;
	uint16_t len;
	char buf[MAX_MESSAGELENGTH];
};

class ShMem
{
public:
	ShMem();
	// Constructor of the shared memory, the name is specified
	ShMem(string title = "Roswell");
	~ShMem();

	// create a publisher
	// @param name	the name of the shared element
	// @param size	the size of the shared element
	// return 		the element ID, 	positive for the valid element ID in current module, negative for error code. 0 is reserved. 
	//				In case there already exists the element with the same name, its ID is reused.
	int CreatePublisher(string name, int size);

	// subscribe a publisher
	// @param name 	the name of the element to subscribe from
	// return 		the element ID, positive for the valid element ID, negative for error code. 
	int Subscribe(string name);

	// update the shared element with new data
	// @param 	ElementID	the id of the element that is going to be updated
	// @param 	p			the pointer point to the buffer of the new data
	// @return				the bytes actually write, positive on success, negtive for error code
	int Write(int ElementID, void* p);

	// Read the shared element
	// @param 	ElementID	the id of the element that is going to be read
	// @param 	p			the pointer point to the buffer of the read data
	// @return				the bytes actually read, positive on success, negtive for error code
	int Read(int ElementID, void* p);

	// get the error message of last operation
	// @return		the error message
	string GetErrorMessage();

protected:
	shm_header* m_headers; // the header area, each has an offset and a size. [0] is the header of headers
	char(*m_names)[16];  // the element names area, each name has upto 15 characters
	void* m_data;	// the data area

	string m_title; // the title of the shared memory
	int m_fd; // the file
	int m_size; // the total size of the shared memory

	int m_err;
	string m_message;
};

class MsgQ
{
public:
	MsgQ();
	// open new a channel for messages receiving with specified timeout
	// @param	my_chn_name	the name of my channel, 1-8 characters
	// @param	timeout_usec	timeout in microseconds, 10-1,000,000, 10us - 1s
	MsgQ(string my_chn_name, long timeout_usec=10);
	~MsgQ();

	// get the channel for message sending by its name. 
	// @param	chn_name	the name of the channel, 1-8 characters
	// @return	the channel ID	number greater than 1, 1 is reserved for main, negtive for error code
	int GetChannelForSending(string chn_name);

	// get the name of the channel
	// @param channel	the channel number
	// @return			the name of the channel, empty for no such a channel
	string GetChannelName(int channel);
		
	// receive a message
	// @param senderID (out)	the sender ID
	// @param len (out)		the length of the message data, 0-1024
	// @param data (out)	the pointer to the buffer that holds the message data
	// @return				the message type, can be MSG_NULL (0), MSG_DATA (1), MSG_COMMAND (6), ..., negtive for error code
	int ReceiveMsg(int* senderChn, int* len, void* data);

	// send a message
	// @param destChn	the destnation channel, 0 for reply to last sender, 1 for main
	// @param type		the type of the message, for example MSG_COMMAND (6)
	// @param len		the length of the message net data, can be 0 or positive
	// @param data		the pointer to the data to be sent, can be NULL in case len is 0
	// @return			number of byte been sent, positive for success, negtive for error code
	int SendMsg(int destChn, int type, int len, void* data);

	// get the error message of last operation
	string GetErrorMessage();

protected:
	mq_buffer send_buf;
	mq_buffer receive_buf;
	uint64_t m_myChnName;
	int m_myChn;
	int m_totalChannels;
	uint32_t m_ts;
	mqd_t m_Channels[MAX_MESSAGECHANNELS];
	int m_timeout;
	uint64_t m_ChnNames[MAX_MESSAGECHANNELS];

	int m_err;
	string m_message;
};
