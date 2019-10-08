/**
 * Simple program demonstrating the use of shared memory and message queue in POSIX systems using ipc-utils library.
 *
 * The "publishers" publish data in shared memory.
 * The "subscribers" read the them.
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

using namespace std;

int main(void)
{
	string position = "3258.1200N,09642.943W";
	string np = "";
	double altitute = 195.0;
	double na = 0;
	time_t t = time(NULL);
	time_t nt = 0;

	printf("\nStart the tests on shared memory and message queue using ipc-utils library.\n\n");

	// define a shared memory
	ShMem myShMem = ShMem("Roswell");
	printf("Shared memory '%s' has been created.\n\n", myShMem.GetErrorMessage().c_str());

	// define two message queues
	printf("Message queues: \n");
	MsgQ server = MsgQ("main", 1000000L);
	printf("%s\n", server.GetErrorMessage().c_str());
	MsgQ client = MsgQ("client", 1000);
	printf("%s\n\n", client.GetErrorMessage().c_str());

	printf("Now create publishers in shared memory.\n");
	int sh_position = myShMem.CreatePublisher("GPS-position", 0);  // 0 is used for string type
	printf("Shared 'GPS-position' to public with id=%d, error message=%s\n", sh_position, myShMem.GetErrorMessage().c_str());

	int sh_altitute = myShMem.CreatePublisher("GPS-altitute", sizeof(altitute));
	printf("Shared 'GPS-altitute' to public with id=%d, error message=%s\n", sh_altitute, myShMem.GetErrorMessage().c_str());

	int sh_time = myShMem.CreatePublisher("GPS-time", sizeof(t));
	printf("Shared 'GPS-time' to public with id=%d, error message=%s\n\n", sh_time, myShMem.GetErrorMessage().c_str());
	
	int ret;
	ret = myShMem.Write(sh_position, (void*)& position);
	printf("Publish 'GPS-position=%s' with size=%d, error message=%s\n", position.c_str(), ret, myShMem.GetErrorMessage().c_str());

	ret = myShMem.Write(sh_altitute, (void*)& altitute);
	printf("Publish 'GPS-altitute=%f' with size=%d, error message=%s\n", altitute, ret, myShMem.GetErrorMessage().c_str());

	int type;
	int len;
	int chn;
	char buf[1024];
	
	client.SendMsg(1, MSG_ONBOARD, 0, NULL);
	printf("%s\n", client.GetErrorMessage().c_str());
	client.SendMsg(1, MSG_COMMAND, position.length(), (void*)position.c_str());
	printf("%s\n", client.GetErrorMessage().c_str());

	for (int i = 0; i < 10; i++)
	{
		time(&t);
		ret = myShMem.Write(sh_time, (void*)& t);
		printf("\n[%d]:\nPublished new 'GPS-time=%ld'\nElements\tOriginal data, \tshared data\n", i, t);

		ret = myShMem.Read(sh_position, (void*)& np);
		ret = myShMem.Read(sh_altitute, (void*)& na);
		ret = myShMem.Read(sh_time, (void*)& nt);

		printf("GPS-position\t%s\t%s\n", position.c_str(), np.c_str());
		printf("GPS-altitute\t%f\t%f\n", altitute, na);
		printf("GPS-epoch\t%ld\t%ld\n", t, nt);
		
		// the receiving here is blocking with timeout
		type = server.ReceiveMsg(&chn, &len, buf);
		if (type == MSG_ONBOARD)
		{
			printf("The client '%s' is now onboard\n", server.GetChannelName(chn).c_str());
		}
		else if (type == MSG_COMMAND)
		{
			printf("Get a command from '%s': %s\n", server.GetChannelName(chn).c_str(), buf);
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
