/**
 * Simple program demonstrates the use of ipc-utils library which takes the advantage of POSIX shared memory and message queue.
 *
 * The "publishers" publish data in shared memory.
 * The "subscribers" read them.
 *
 * The "server" listens to the message queue.
 * The "client" sends messages to the server.
 *
 * Version 1.0
 *
 * @author George Sun
 * 2019/10
 */

#include "ipc-utils.h"
#include <cstdio>
#include <string>
#include <ctime>
#include <time.h>

#define MSG_ONBOARD 11

using namespace std;

timespec GetEpochTime()
{
	timespec tp;
	clock_gettime(CLOCK_REALTIME_COARSE, &tp);
	return tp;
}

int main(void)
{
	string position = "3258.1200N,09642.943W";
	string np = "";
	double altitute = 195.0;
	double na = 0;
	time_t t = time(NULL);
	time_t nt = 0;
	timespec tp = GetEpochTime();
	timespec ntp;

	printf("\nStart the tests on shared memory and message queue using ipc-utils library.\n\n");

	// define a shared memory
	ShMem myShMem = ShMem("Roswell");
	printf("Shared memory '%s' has been created.\n\n", myShMem.GetErrorMessage().c_str());

	// define two message queues
	printf("Message queues: \n");
	MsgQ server = MsgQ("main", 1000000L);
	MsgQ client = MsgQ("client", 1000);

	printf("Now create publishers in shared memory.\n");
	int sh_position = myShMem.CreatePublisher("GPS-position", 0);  // demo a publisher in string. 0 is used for string type
	printf("Shared 'GPS-position' to public with id=%d, error message=%s\n", sh_position, myShMem.GetErrorMessage().c_str());

	int sh_altitute = myShMem.CreatePublisher("GPS-altitute", sizeof(altitute));  // demo a publisher in double 
	printf("Shared 'GPS-altitute' to public with id=%d, error message=%s\n", sh_altitute, myShMem.GetErrorMessage().c_str());

	int sh_time = myShMem.CreatePublisher("GPS-Epoch", sizeof(tp)); // demo a publisher with complex data structure
	printf("Shared 'GPS-Epoch' to public with id=%d, error message=%s\n\n", sh_time, myShMem.GetErrorMessage().c_str());
	
	int ret;
	ret = myShMem.Write(sh_position, &position);
	printf("Publish 'GPS-position=%s' with size=%d, error message=%s\n", position.c_str(), ret, myShMem.GetErrorMessage().c_str());

	ret = myShMem.Write(sh_altitute, &altitute);
	printf("Publish 'GPS-altitute=%f' with size=%d, error message=%s\n", altitute, ret, myShMem.GetErrorMessage().c_str());

	int type;
	int len;
	int chn;
	char buf[1024];
	string SenderName;
	
	printf("%s\n", server.GetErrorMessage().c_str());
	printf("Read off messages remain in 'main'\n");
	do 
	{
		chn = server.ReceiveMsg(&SenderName, &type, &len, buf);
		if (chn <= 0)
		{
			printf("No message in main.\n");
			break;
		}
		printf("Read a old message from '%s' with type=%d, len=%d %s\n", SenderName.c_str(), type, len, buf);
	} while (1);
	
	printf("%s\n\n", client.GetErrorMessage().c_str());
	client.SendMsg(1, MSG_ONBOARD, 0, NULL);  // demo sending message to destination by channel number
	printf("%s\n", client.GetErrorMessage().c_str());
	//client.SendMsg(1, MSG_COMMAND, position.length(), (void*)position.c_str());
	//client.SendMsg("main", MSG_COMMAND, position.length(), (void*)position.c_str());
	client.SendCmd("main", "position");  // demo sending message to destination by channel name
	client.SendCmd(1, "reload");
	printf("%s\n", client.GetErrorMessage().c_str());

	for (int i = 0; i < 10; i++)
	{
		//time(&t);
		tp = GetEpochTime();
		ret = myShMem.Write(sh_time, &tp);  // demo publish a complex structure of data
		printf("\n[%d]:\nPublished new 'GPS-time=%ld.%ld'\nElements\tOriginal data, \tshared data\n", i, tp.tv_sec, tp.tv_nsec);

		//ret = myShMem.Read("GPS-position", &np);
		ret = myShMem.Read(sh_position, &np);
		ret = myShMem.Read("GPS-altitute", &na);

		printf("GPS-position\t%s\t%s\n", position.c_str(), np.c_str());
		printf("GPS-altitute\t%f\t%f\n", altitute, na);

		len = myShMem.Read(sh_time, &ntp);
		printf("GPS-epoch\t%ld.%ld\t%ld.%ld\n", tp.tv_sec, tp.tv_nsec, ntp.tv_sec, ntp.tv_nsec);
		
		// the receiving here is blocking with timeout
		chn = server.ReceiveMsg(&SenderName, &type, &len, buf);
		if (chn <= 0)
		{
			printf("No message.\n");
			continue;
		}
		int ts = server.GetMsgTimestamp();
		
		if (type == MSG_ONBOARD)
		{
			printf("The module '%s' is onboard at %dus\n", SenderName.c_str(), ts);
		}
		else if (type == MSG_COMMAND)
		{
			printf("Get a command from '%s': len=%d  command=%s\n", server.GetChannelName(chn).c_str(), len, buf);
		}
		else if (type < 0)
		{
			printf("Error code is %d %s\n", type, server.GetErrorMessage().c_str());
		}
		else
		{
			printf("Channel=%d Length=%d %s\n", chn, len, server.GetErrorMessage().c_str());
		}
	}

	return 0;
}
