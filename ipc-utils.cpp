#include "ipc-utils.h"
#include <cstdio>
#include <time.h>
#include <chrono>

ShMem::ShMem()
{
	m_data = NULL;
	m_title = "Roswell";
	m_fd = -1;
	m_size = 0;

	m_err = 0;
	m_message = "";
}

// Constructor of the shared memory, the name is specified
ShMem::ShMem(string title)
{
	if (title.empty())
	{
		m_title = "Roswell";
	}
	else if (title.length() >= 15)
	{
		m_title.assign(title.substr(0, 15));
	}
	else
	{
		m_title.assign(title);
	}
	title = "/" + m_title; // add the / in front of the title
	//fprintf(stderr, "Using %s as the title to create the shared memory\n", title.c_str());

	// create the shared memory object
	m_fd = shm_open(title.c_str(), O_CREAT | O_RDWR, 0666);
	//fprintf(stderr, "The shared memory has an ID of %d, ", m_fd);

	// configure the total size of the shared memory object
	int size_headers = MAX_PUBLISHERS * sizeof(shm_header);
	int size_names = MAX_PUBLISHERS * 16;
	int size_data = 65536;
	m_size = size_headers + size_names + size_data; // total size of the headers, names, and data
	//fprintf(stderr, "total size is %d\n", m_size);
	ftruncate(m_fd, m_size);

	// memory map the shared memory object
	void* base = mmap(0, m_size, PROT_READ | PROT_WRITE, MAP_SHARED, m_fd, 0);
	m_headers = (shm_header*)base;
	m_names = (char(*)[16]) base + size_headers;
	m_data = base + size_headers + size_names;
	m_err = strlen(m_names[0]);

	// check if the shared memory has been setup before
	if (strlen(m_names[0]) != m_title.length() || strcmp(m_names[0], m_title.c_str()))
	{
		memset(base, 0, m_size); // clear the whole shared memory
		strcpy(m_names[0], m_title.c_str()); // assign the title to be the first ID
		//fprintf(stderr, "First time initial the shared memory. %d\nClear the memory and assign the title %s, ", 
		//	m_err, m_names[0]);
		m_err = strlen(m_names[0]);
		//fprintf(stderr, "new title length %d\n", m_err);
	}
	//else
	//{
	//	fprintf(stderr, "Shared memory has been setup before using title %s.\n", m_names[0]);
	//}

	m_message.assign(m_names[0]);
}

ShMem::~ShMem()
{
	munmap((void*)m_headers, m_size);
}

// publish new data with the publisher
// @param PublisherName	the name of the shared element or publisher, 1-15 characters
// @param size			the size of the shared element, 0-1024. 0 for string up to 63 characters
// @param ptr			the pointer to the data
// @return				the publisher ID, positive for success, negtive for error code.
//						The publisher ID keeps unchanged for the same publisher name among all processes/threads.
int ShMem::Write(string PublisherName, int size, void* ptr)
{
	int id = Subscribe(PublisherName);
	
	if (id > 0 && Write(id, ptr) < 0)
	{
		return m_err;
	}
	
	return id;	
}

// publish new data in integer with the publisher
// @param PublisherName	the name of the shared element or publisher, 1-15 characters
// @param n				the data in integer to be published
// @return				the publisher ID, positive for success, negtive for error code
//						The publisher ID keeps unchanged for the same publisher name among all processes/threads.
int ShMem::Write(string PublisherName, int n)
{
	int id = Subscribe(PublisherName);
	
	if (id > 0 && Write(id, &n) < 0)
	{
		return m_err;
	}
	
	return id;	
}

// publish new data in double with the publisher
// @param PublisherName	the name of the shared element or publisher, 1-15 characters
// @param t				the data in double to be published
// @return				the publisher ID, positive for success, negtive for error code
//						The publisher ID keeps unchanged for the same publisher name among all processes/threads.
int ShMem::Write(string PublisherName, double t)
{
	int id = Subscribe(PublisherName);
	
	if (id > 0 && Write(id, &t) < 0)
	{
		return m_err;
	}
	
	return id;	
}

// publish new data in string with the publisher
// @param PublisherName	the name of the shared element or publisher, 1-15 characters
// @param s				the data in string to be published, 0-63 characters
// @return				the publisher ID, positive for success, negtive for error code
//						The publisher ID keeps unchanged for the same publisher name among all processes/threads.
int ShMem::Write(string PublisherName, string s)
{
	int id = Subscribe(PublisherName);
	
	if (id > 0 && Write(id, &s) < 0)
	{
		return m_err;
	}
	
	return id;	
} 
	
// Read the shared element
// @param PublisherName	the name of the shared element or publisher, 1-15 characters
// @param len (out)		the size of the shared element, 0-1024. 0 for string up to 63 characters
// @param ptr (out)		the pointer to the data read
// @return				the publisher ID, positive for success, negtive for error code
//						The publisher ID keeps unchanged for the same publisher name among all processes/threads.
int ShMem::Read(string PublisherName, int* len, void* ptr)
{
	int id = Subscribe(PublisherName);
	if (id > 0)
	{
		*len = Read(id, ptr);
		
		if (*len < 0)
		{
			return *len;
		}
	}
	
	return id;
}

// Read the shared element in integer
// @param PublisherName	the name of the shared element or publisher, 1-15 characters
// @param n (out)		the pointer to the integer read
// @return				the publisher ID, positive for success, negtive for error code
//						The publisher ID keeps unchanged for the same publisher name among all processes/threads.
int ShMem::Read(string PublisherName, int* n)
{
	int id = Subscribe(PublisherName);
	if (id > 0 && Read(id, n) < 0)
	{
		id = -1;
	}
	
	return id;
}
	
// Read the shared element in double
// @param PublisherName	the name of the shared element or publisher, 1-15 characters
// @param t (out)		the pointer to the double read
// @return				the publisher ID, positive for success, negtive for error code
//						The publisher ID keeps unchanged for the same publisher name among all processes/threads.
int ShMem::Read(string PublisherName, double* t)
{
	int id = Subscribe(PublisherName);
	if (id > 0 && Read(id, t) < 0)
	{
		id = -1;
	}
	
	return id;
}

// Read the shared element in string
// @param PublisherName	the name of the shared element or publisher, 1-15 characters
// @param s (out)		the pointer to the string read
// @return				the publisher ID, positive for success, negtive for error code
//						The publisher ID keeps unchanged for the same publisher name among all processes/threads.
int ShMem::Read(string PublisherName, string* s)
{
	int id = Subscribe(PublisherName);
	if (id > 0 && Read(id, s) < 0)
	{
		id = -1;
	}
	
	return id;
}
	
// create a publisher
// @param PublisherName	the name of the shared element
// @param size			the size of the shared element
// @return				the publisher ID, positive for success, negtive for error code
//						The publisher ID keeps unchanged for the same publisher name among all processes/threads.
int ShMem::CreatePublisher(string PublisherName, int size)
{
	// check the name length
	if (PublisherName.length() == 0 || PublisherName.length() > 15)
	{
		m_err = -1;
		m_message = "invalid length of name";
		return m_err;
	}

	// check size
	if (size < 0 || size >= 32768)
	{
		m_err = -2;
		m_message = "invalid sharing size";
		return m_err;
	}

	// check and wait till the headers is unlocked
	unsigned int usecs = 0; // sleep time in us
	while (m_headers[0].offset >= MAX_PUBLISHERS)
	{
		usecs += 100;
		if (usecs > 500)
		{
			m_headers[0].offset &= 0xFF; // unlock the header
			m_err = -1;
			m_message = "others locked the headers";
		}
		usleep(usecs);  // sleep for a little while to wait for unlock
	}

	// lock the header
	m_headers[0].offset |= 0xFF00;

	// now starts to add the element into the sharing
	uint16_t total_elements = m_headers[0].offset & 0xFF;
	uint16_t offset = m_headers[0].size; // the offset is the second 16bits
	uint16_t u_size = static_cast<uint16_t>(size);

	// check if the elements has been create before
	for (uint16_t i = 1; i < total_elements; i++)
	{
		if (strcmp(m_names[i], PublisherName.c_str()) == 0)
		{
			if (u_size <= m_headers[i].size)
			{
				m_err = 0;
				m_message = "found valid previously shared element";
				m_headers[0].offset &= 0x00FF; // unlock the header
				return i; // reuse the previouse 
			}
			m_err = -1;
			m_message = "invalid sharing size, larger than previous";
			m_headers[0].offset &= 0x00FF; // unlock the header
			return m_err;
		}
	}

	// add a new publisher
	total_elements++;
	uint16_t temp_size = m_headers[0].size + (u_size ? u_size : 64);  // modify the default offset
	if (temp_size < m_headers[0].size)
	{
		m_err = -1;
		m_message = "total size overflowed: " + to_string(u_size) + " " + to_string(m_headers[0].size);
		m_headers[0].offset &= 0x00FF; // unlock the header
		return m_err;
	}

	strcpy(m_names[total_elements], PublisherName.c_str()); // add the name to the name list
	m_headers[total_elements].offset = offset; // update the offset of the element
	m_headers[total_elements].size = u_size; // update the size of the element
	m_headers[0].size = temp_size;  // modify the default offset
	m_headers[0].offset = total_elements; // increment the element counter and unlock the header

	m_err = 0;
	m_message = "new sharing added";
	return total_elements;
}

// subscribe a publisher or get the publisher id by name
// @param PublisherName	the name of the shared element
// @return				the publisher ID, positive for success, negtive for error code
//						The publisher ID keeps unchanged for the same publisher name among all processes/threads.
int ShMem::Subscribe(string PublisherName)
{
	uint16_t total_elements = m_headers[0].offset & 0xFF;

	// check if the elements has been create before
	for (uint16_t i = 1; i < total_elements; i++)
	{
		if (strcmp(m_names[i], PublisherName.c_str()) == 0)
		{
			m_err = 0;
			m_message = "found the element";
			return i; // reuse the previouse 
		}
	}

	m_err = -1;
	m_message = "cannot find the element";
	return m_err;
}

// update the shared element with new data
// @param PublisherID	the ID of the shared element or publisher, 1-256
// @param 	p			the pointer point to the buffer of the new data
// @return				the bytes actually write, positive on success, negtive for error code
int ShMem::Write(int PublisherID, void* p)
{
	uint16_t total_elements = m_headers[0].offset & 0xFF;

	if (PublisherID == 0 || PublisherID > total_elements)
	{
		m_err = -1;
		m_message = "element ID is out of range";
		return m_err;
	}

	size_t size = static_cast<size_t>(m_headers[PublisherID].size);
	uint16_t offset = m_headers[PublisherID].offset ^ 0x8000; // write the data to the opposite offset
	//fprintf(stderr, "the offset for reading is %d, for writing is %d\n", m_headers[PublisherID].offset, offset);

	//check if it is a string type
	if (size)
	{
		memcpy(m_data + offset, p, size);  // copy the data to the destination
	}
	else
	{
		if (static_cast<string*>(p)->length() > 63)
		{
			m_message = "string too long";
			static_cast<string*>(p)->assign(static_cast<string*>(p)->substr(0, 63));
		}
		strcpy((char*)(m_data + offset), static_cast<string*>(p)->c_str());

		//fprintf(stderr, "now the new string is %s", (char*)(m_data + offset));
	}
	m_headers[PublisherID].offset = offset; // revert the ping-pong flag

	m_err = 0;
	m_message = "element is updated";
	return m_headers[PublisherID].size;
}


// Read the shared element
// @param PublisherID	the ID of the shared element or publisher, 1-256
// @param p				the pointer point to the buffer of the read data
// @return				the bytes actually read, positive on success, negtive for error code
int ShMem::Read(int PublisherID, void* p)
{
	uint16_t total_elements = m_headers[0].offset & 0xFF; // the 

	if (PublisherID == 0 || PublisherID > total_elements)
	{
		m_err = -1;
		m_message = "element ID is out of range";
		return m_err;
	}

	size_t size = static_cast<size_t>(m_headers[PublisherID].size);
	uint16_t offset = m_headers[PublisherID].offset; // the offset of the data according to the ping-pong flag
	if (size)
	{
		memcpy(p, m_data + offset, size);  // read the data other than string
	}
	else
	{
		static_cast<string*>(p)->assign((char*)m_data + offset);  // read string
	}

	m_err = 0;
	m_message = "";
	return m_headers[PublisherID].size;
}

// get the error message of last operation
// @return		the error message
string ShMem::GetErrorMessage()
{
	return m_message;
}

MsgQ::MsgQ()
{
	memset(&send_buf, 0, sizeof(send_buf));
	memset(&receive_buf, 0, sizeof(receive_buf));
	m_myChnName = 0;
	m_myChn = 0;
	m_totalChannels = 0;
	memset(m_Channels, 0, sizeof(m_Channels));
	memset(m_ChnNames, 0, sizeof(m_ChnNames));
	m_ts = 0;
	m_timeout = 10;

	m_err = 0;
	m_message = "";
}

// open new a channel for messages receiving
// @param	my_chn_name	the name of my channel, 1-8 characters
// @param	timeout_usec	timeout in microseconds, 10-1,000,000
MsgQ::MsgQ(string my_chn_name, long timeout_usec)
{
	memset(&send_buf, 0, sizeof(send_buf));
	memset(&receive_buf, 0, sizeof(receive_buf));
	memset(m_Channels, 0, sizeof(m_Channels));
	memset(m_ChnNames, 0, sizeof(m_ChnNames));

	my_chn_name = my_chn_name.length() > 8 ? my_chn_name.substr(0, 8) : my_chn_name;
	m_timeout = timeout_usec < 10 ? 10 : timeout_usec;
	m_timeout = timeout_usec > 1000000L ? 1000000L : timeout_usec;

	// assign the attributes for the message queue
	mq_attr attr;
	attr.mq_flags = 0;
	attr.mq_maxmsg = 10;
	attr.mq_msgsize = 2048;
	attr.mq_curmsgs = 0;

	// m_myChn is the channel for reading only, try to open it if not existing
	string name = "/" + my_chn_name;
	char buffer[10];
	memset(buffer, 0, sizeof(buffer));
	buffer[0] = '/';
	strcpy(buffer + 1, my_chn_name.c_str());
	m_myChn = mq_open(buffer, O_RDONLY | O_CREAT, 0660, NULL);
	if (m_myChn < 0)
	{
		perror ("Server: mq_open (main)");
	}

	// channels in the list are all opened for writting in nonblocking mode only
	// channel 1 is reserved for main, so open it here in advance
	m_totalChannels = 1;
	m_Channels[1] = mq_open("/main", O_WRONLY | O_NONBLOCK);
	strcpy((char*)m_ChnNames, my_chn_name.c_str()); // assign m_ChnNames[0], the /0 of a 8 characters is taken care of
	my_chn_name.assign((char*)m_ChnNames);
	strcpy((char*)(m_ChnNames + 1), "main");
	m_myChnName = m_ChnNames[0];

	m_err = 0;
	mq_getattr(m_myChn, &attr);
	m_message = "My message queue '" + my_chn_name 
		+ "' is created with id=" + to_string(m_myChn) 
		+ " for receiving.\nMessage queue 'main' is opened with id=" + to_string(m_Channels[1]) 
		+ " for sending.\nMy queue currently has " + to_string(attr.mq_curmsgs) 
		+ "s messages to be received in queue. \nThe max message size of my queue is " + to_string(attr.mq_msgsize) 
		+ ". The max number of messages on my queue is " + to_string(attr.mq_maxmsg);
}

MsgQ::~MsgQ()
{
	// close all the message queue opened in this class before. The message queue is still in the kernel without been deleted.
	for (int i = 1; i <= m_totalChannels; i++)
	{
		mq_close(m_Channels[i]);
	}
	mq_close(m_myChn);
}

// get the channel for message sending by its name. 
// @param	chn_name	the name of the channel, 1-8 characters
// @return	the channel ID	number greater than 1, 1 is reserved for main, negtive for error code
int MsgQ::GetDestChannel(string chn_name)
{
	if (chn_name.empty() || chn_name.length() > 8)
	{
		m_err = -1;
		m_message = "invalid channel name. 1-8 characters";
		return m_err;
	}

	// n is the temporal variable to hold chn_name in uint64_t style
	uint64_t n = 0;
	if (chn_name.length() == 8)
	{
		memcpy(&n, chn_name.c_str(), 8);
	}
	else
	{
		strcpy((char*)&n, chn_name.c_str());
	}

	// check if the channel name has been defined
	for (int i = 1; i < m_totalChannels + 1; i++)
	{
		if (m_ChnNames[i] == n)
		{
			return i;
		}
	}

	// this is a new name, try to open it for messages sending
	chn_name = "/" + chn_name; // adding the / in front of the channel name
	m_message = "message queue " + chn_name;
	mqd_t ret = mq_open(chn_name.c_str(), O_WRONLY | O_NONBLOCK);
	if (ret < 0)
	{
		m_err = ret;		
		m_message += " does not exist";
		return m_err;
	}

	m_message += " is opened for messages sending";
	m_totalChannels++;
	m_ChnNames[m_totalChannels] = n;
	m_Channels[m_totalChannels] = ret;
	return m_totalChannels;
}

// get the name of the channel
// @param channel	the channel number
// @return			the name of the channel, empty for no such a channel
string MsgQ::GetChannelName(int channel)
{
	if (channel < 0 || channel > m_totalChannels)
	{
		m_err = -1;
		m_message = "invalid channel";
		return "";
	}

	//m_message = "(" + to_string(channel) + ") " + to_string(m_ChnNames[channel]) + ":";
	m_message.assign((char*)(m_ChnNames + channel));
	return m_message;
}

// receive a message sent to me
// @param SenderName (out)	the sender name in string, 1-15 characters
// @param type (out)		the type of the message, can be MSG_NULL (0), MSG_DATA (1), MSG_COMMAND (6), ...
// @param len (out)			the length of the message data, 0-1024
// @param data (out)		the pointer to the received data without message header
// @return					the sender channel, positive for success, negtive for error code
int MsgQ::ReceiveMsg(string* SenderName, int* type, int* len, void* data)
{
	*len = 0;
	timespec timeout;
	uint64_t usec = m_timeout;
	usec += chrono::duration_cast<chrono::microseconds>(chrono::high_resolution_clock::now().time_since_epoch()).count();
	timeout.tv_sec = usec / 1000000L;
	timeout.tv_nsec = (usec - timeout.tv_sec * 1000000L) * 1000;

	// read the message queue
	m_err = mq_timedreceive(m_myChn, (char *)&receive_buf, 8192, 0, &timeout);
	if (m_err < 0)
	{
		if (errno == EAGAIN)
		{
			m_message = "no message";
			m_err = 0;
		}
		else if (errno == EBADF)
		{
			m_message = "The descriptor specified in mqdes was invalid.";
		} 
		else if (errno == EINVAL)
		{
			m_message = "The call would have blocked, and abs_timeout was invalid, either because tv_sec was less than zero, or because tv_nsec was less than zero or greater than 1000 million.";
		}
		else if (errno == EMSGSIZE)
		{
			m_message = to_string(sizeof(receive_buf)) + " was less than the mq_msgsize attribute of the message queue.";
		} 
		else if (errno == ETIMEDOUT)
		{
			m_message = "The call timed out before a message could be transferred.";
			m_err = 0;
		}
		else
		{
			m_message = "error when receiving message";
		}
		
		return m_err;
	}

	// got a message, parses it
	m_ChnNames[0] = receive_buf.name;
	m_message.assign((char *)&receive_buf);
	SenderName->assign((char *)&receive_buf); // the time stamp has a /0 in the front 
	m_ts = receive_buf.ts;
	*len = receive_buf.len;
	*type = receive_buf.type;
	memcpy(data, receive_buf.buf, receive_buf.len);

	usec = timeout.tv_nsec / 1000;
	usec = m_ts > usec ? 1000000L + usec - m_ts : usec -m_ts;

	for (int i = 1; i < m_totalChannels + 1; i++)
	{
		if (m_ChnNames[i] == m_ChnNames[0])
		{
			m_Channels[0] = m_Channels[i];
			return i;
		}
	}

	// for a new sender channel, add it to the list
	char buffer[10];
	memset(buffer, 0, sizeof(buffer));
	buffer[0] = '/';
	memcpy(buffer + 1, m_ChnNames, sizeof(uint64_t));

	m_Channels[0] = mq_open(buffer, O_WRONLY | O_NONBLOCK);
	m_totalChannels++;
	m_ChnNames[m_totalChannels] = m_ChnNames[0];
	m_Channels[m_totalChannels] = m_Channels[0];
	m_message = "new sender " + m_message;
	return m_totalChannels;
}

// send a message to the destnation
// @param DestName	the destnation name, empty for reply to the last sender
// @param type		the type of the message, for example MSG_COMMAND (6)
// @param len		the length of the message net data, can be 0 or positive
// @param data		the pointer to the data to be sent, can be NULL in case len is 0
// @return			the destnation channel, positive for success, negtive for error code
int MsgQ::SendMsg(string DestName, int type, int len, void* data)
{
	int chn = GetDestChannel(DestName);
	if (chn > 0 && SendMsg(chn, type, len, data) < 0)
	{
		return -1;
	}
	
	return chn;
}

// send a message to the destnation
// @param DestChn	the destnation channel, 0 for reply to last sender, 1 for main
// @param type		the type of the message, for example MSG_COMMAND (6)
// @param len		the length of the message net data, can be 0 or positive
// @param data		the pointer to the data to be sent, can be NULL in case len is 0
// @return			bytes of data actually sent, positive for success, negtive for error code.
int MsgQ::SendMsg(int DestChn, int type, int len, void* data)
{
	if (type <= 0 || type > 255)
	{
		m_err = -1;
		m_message = "invalid sending message type";
		return m_err;
	}

	if (len < 0 || len > MAX_MESSAGELENGTH)
	{
		m_err = -2;
		m_message = "invalid sending message length";
		return m_err;
	}

	if (DestChn < 0 || DestChn > m_totalChannels)
	{
		m_err = -3;
		m_message = "invalid dest ID";
		return m_err;
	}
	
	// calculate the time stamp and the timeout
	timespec timeout;
	uint64_t usec = chrono::duration_cast<chrono::microseconds>(chrono::high_resolution_clock::now().time_since_epoch()).count();
	timeout.tv_sec = usec / 1000000L;  //convert into second
	send_buf.ts = usec - timeout.tv_sec * 1000000L;  // get the remaining microseconds 
	timeout.tv_nsec = send_buf.ts + 1000; // +1ms for sending timeout
	if (timeout.tv_nsec >= 1000000L)
	{
		timeout.tv_sec++;
		timeout.tv_nsec -= 1000;
	}
	timeout.tv_nsec *= 1000; // convert into nanoseconds

	send_buf.name = m_myChnName;
	send_buf.type = type;
	send_buf.len = len;
	printf("%d: (name=%ld, ts=%d, type=%d, len=%d) \n", len, send_buf.name, send_buf.ts, send_buf.type, send_buf.len);
	memcpy(send_buf.buf, data, len);
	len += sizeof(send_buf) - MAX_MESSAGELENGTH;

	m_err = mq_timedsend(m_Channels[DestChn], (const char*)&send_buf, len, 0, &timeout);
	if (m_err < 0)
	{
		if (errno == EAGAIN)
		{
			m_message = "the queue was full";
		}
		else if (errno == EBADF)
		{
			m_message = "The descriptor specified was invalid.";
		}
		else if (errno == EINTR)
		{
			m_message = "The call was interrupted by a signal handler";
		}
		else if (errno == EINVAL)
		{
			m_message = "The call would have blocked, and abs_timeout was invalid";
		}
		else if (errno == EMSGSIZE)
		{
			m_message = "msg_len was greater than the mq_msgsize attribute of the message queue.";
		}
		else if (errno == ETIMEDOUT)
		{
			m_message = "The call timed out before a message could be transferred.";
		}
		else
		{
			m_message = "error while sending";
		}
		
		return m_err;
	}

	m_err = len;
	m_message = "message (type=" + to_string(send_buf.type) + ", len=" + to_string(send_buf.len) + ") was sent to ";
	m_message.append((char*)(m_ChnNames + DestChn));
	m_message.append(" at channel " + to_string(m_Channels[DestChn]) + " with size=" + to_string(len));
	return m_err;
}

// get the error message of last operation
// @return		the error message
string MsgQ::GetErrorMessage()
{
	return m_message;
}

