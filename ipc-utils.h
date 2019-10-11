#pragma once
#include <sys/mman.h> // for shared memory related
#include <sys/stat.h> // for mode constants
#include <mqueue.h>  // for message queue
#include <fcntl.h> // for O_* constants
#include <unistd.h> // for usleep
#include <cstdint>
#include <cstring> // for str copy
#include <string>  // for c++ string

#define MAX_PUBLISHERS 256
#define MAX_MESSAGECHANNELS 256
#define MAX_MESSAGELENGTH 1024
#define MSG_NULL 0

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
	ShMem(string title);
	~ShMem();
	
	// publish new data with the publisher
	// @param PublisherName	the name of the shared element or publisher, 1-15 characters
	// @param size			the size of the shared element, 0-1024. 0 for string up to 63 characters
	// @param ptr			the pointer to the data
	// @return				the publisher ID, positive for success, negtive for error code.
	//						The publisher ID keeps unchanged for the same publisher name among all processes/threads.
	int Write(string PublisherName, int size, void* ptr); 
	
	// publish new data in integer with the publisher
	// @param PublisherName	the name of the shared element or publisher, 1-15 characters
	// @param n				the data in integer to be published
	// @return				the publisher ID, positive for success, negtive for error code
	//						The publisher ID keeps unchanged for the same publisher name among all processes/threads.
	int Write(string PublisherName, int n); 

	// publish new data in double with the publisher
	// @param PublisherName	the name of the shared element or publisher, 1-15 characters
	// @param t				the data in double to be published
	// @return				the publisher ID, positive for success, negtive for error code
	//						The publisher ID keeps unchanged for the same publisher name among all processes/threads.
	int Write(string PublisherName, double t); 

	// publish new data in string with the publisher
	// @param PublisherName	the name of the shared element or publisher, 1-15 characters
	// @param s				the data in string to be published, 0-63 characters
	// @return				the publisher ID, positive for success, negtive for error code
	//						The publisher ID keeps unchanged for the same publisher name among all processes/threads.
	int Write(string PublisherName, string s); 
	
	// Read the shared element
	// @param PublisherName	the name of the shared element or publisher, 1-15 characters
	// @param len (out)		the size of the shared element, 0-1024. 0 for string up to 63 characters
	// @param ptr (out)		the pointer to the data read
	// @return				the publisher ID, positive for success, negtive for error code
	//						The publisher ID keeps unchanged for the same publisher name among all processes/threads.
	int Read(string PublisherName, int* len, void* ptr);

	// Read the shared element in integer
	// @param PublisherName	the name of the shared element or publisher, 1-15 characters
	// @param n (out)		the pointer to the integer read
	// @return				the publisher ID, positive for success, negtive for error code
	//						The publisher ID keeps unchanged for the same publisher name among all processes/threads.
	int Read(string PublisherName, int* n);
	
	// Read the shared element in double
	// @param PublisherName	the name of the shared element or publisher, 1-15 characters
	// @param t (out)		the pointer to the double read
	// @return				the publisher ID, positive for success, negtive for error code
	//						The publisher ID keeps unchanged for the same publisher name among all processes/threads.
	int Read(string PublisherName, double* t);

	// Read the shared element in string
	// @param PublisherName	the name of the shared element or publisher, 1-15 characters
	// @param s (out)		the pointer to the string read
	// @return				the publisher ID, positive for success, negtive for error code
	//						The publisher ID keeps unchanged for the same publisher name among all processes/threads.
	int Read(string PublisherName, string* s);
	
	// create a publisher for quick publish
	// @param PublisherName	the name of the shared element
	// @param size			the size of the shared element
	// @return				the publisher ID, positive for success, negtive for error code
	//						The publisher ID keeps unchanged for the same publisher name among all processes/threads.
	int CreatePublisher(string PublisherName, int size);

	// subscribe a publisher
	// @param PublisherName	the name of the shared element
	// @return				the publisher ID, positive for success, negtive for error code
	//						The publisher ID keeps unchanged for the same publisher name among all processes/threads.
	int Subscribe(string PublisherName);

	// publish new data with the publisher
	// @param PublisherID	the ID of the shared element or publisher, 1-256
	// @param ptr			the pointer to the data to be published
	// @return				the actual bytes of data written, positive for success, negtive for error code.
	int Write(int PublisherID, void* ptr);

	// publish new data in integer with the publisher
	// @param PublisherID	the ID of the shared element or publisher, 1-256
	// @param n				the data in integer to be published
	// @return				the actual bytes of data written, positive for success, negtive for error code.
	int Write(int PublisherID, int n);

	// publish new data in double with the publisher
	// @param PublisherID	the ID of the shared element or publisher, 1-256
	// @param t				the data in double to be published
	// @return				the actual bytes of data written, positive for success, negtive for error code.
	int Write(int PublisherID, double t);

	// publish new data in string with the publisher
	// @param PublisherID	the ID of the shared element or publisher, 1-256
	// @param s				the data in string to be published, 0-63 characters
	// @return				the actual bytes of data written, positive for success, negtive for error code.
	int Write(int PublisherID, string s);

	// Read the shared element
	// @param PublisherID	the ID of the shared element or publisher, 1-256
	// @param ptr (out)		the pointer to the data read
	// @return				the actual bytes of data read, positive for success, negtive for error code.
	int Read(int PublisherID, void* ptr);

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

	// receive a message sent to me
	// @param SenderName (out)	the sender name in string, 1-15 characters
	// @param type (out)		the type of the message, can be MSG_NULL (0), MSG_DATA (1), MSG_COMMAND (6), ...
	// @param len (out)			the length of the message data, 0-1024
	// @param data (out)		the pointer to the received data without message header
	// @return					the sender channel, positive for success, negtive for error code
	int ReceiveMsg(string* SenderName, int* type, int* len, void* data);

	// send a message to the destnation
	// @param DestName	the destnation name, empty for reply to the last sender
	// @param type		the type of the message, for example MSG_COMMAND (6)
	// @param len		the length of the message net data, can be 0 or positive
	// @param data		the pointer to the data to be sent, can be NULL in case len is 0
	// @return			the destnation channel, positive for success, negtive for error code
	int SendMsg(string DestName, int type, int len, void* data);

	// send a command to the destnation
	// @param DestName	the destnation name, empty for reply to the last sender
	// @param n			the integer data to be sent
	// @return			the destnation channel, positive for success, negtive for error code
	int SendCmd(string DestName, string s);

	// get the channel of destnation by its name. 
	// @param DestName	the destnation name, empty for the last sender
	// @return			the channel of  ID	number greater than 1, 1 is reserved for main, negtive for error code
	int GetDestChannel(string DestName);

	// get the name of the channel
	// @param channel	the channel number
	// @return			the name of the channel, empty for no such a channel
	string GetChannelName(int channel);

	// send a message to the destnation
	// @param DestChn	the destnation channel, 0 for reply to last sender, 1 for main
	// @param type		the type of the message, for example MSG_COMMAND (6)
	// @param len		the length of the message net data, can be 0 or positive
	// @param data		the pointer to the data to be sent, can be NULL in case len is 0
	// @return			bytes of data actually sent, positive for success, negtive for error code.
	int SendMsg(int DestChn, int type, int len, void* data);

	// send a command to the destnation
	// @param DestChn	the destnation channel, 0 for reply to last sender, 1 for main
	// @param s			the string data to be sent
	// @return			bytes of data actually sent, positive for success, negtive for error code.
	int SendCmd(int DestChn, string s);

	// get the error message of last operation
	// @return 		the error message of last operation
	string GetErrorMessage();
	
	// get the timestamp of last received message
	// @return 		the time stamp of last received message, it is actually the remain microsecond of the moment the message was sent
	int GetMsgTimestamp();

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
