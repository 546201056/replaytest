#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h> 
#include <poll.h>
#include <linux/input.h>
#include <linux/time.h>
#include "uinput.h"

const char *EV_PREFIX  = "/dev/input/";
const char *workDir = "/sdcard/script/";
const char *OUT_FN = "myscript";
const char *TIME = "myscripttime";
/* const char *OUT_PREFIX = "/sdcard/"; */

/* NB event4 is the compass -- not required for tests. */
char *ev_devices[] = {"event0", "event1", "event2", "event3" /*, "event4" */};
#define NUM_DEVICES (sizeof(ev_devices) / sizeof(char *))

struct pollfd in_fds[NUM_DEVICES];
/*
int out_fds[NUM_DEVICES];
*/
int out_fd;
int out_time;

int
init()
{
	char buffer[256];
	int i, fd;

	// 创建文件夹
	if(access(workDir,0) != 0)
		if(mkdir(workDir,777)!=0)
			printf("Couldn't create work dir...\n");

	// 获取时间戳
	struct timeval now;
	gettimeofday(&now, NULL);
	printf("record start time is %ld\n", now.tv_sec);
	//生成时间戳为名字的文件夹路径 
	char newScriptDir[64] = {0};
	char timestamp[16];
	sprintf(timestamp, "%ld", now.tv_sec);
	strcat(newScriptDir, workDir);
	strcat(newScriptDir, timestamp);
	strcat(newScriptDir, "/");
	// 创建新路径文件夹
	if(access(newScriptDir,0) != 0)
		if(mkdir(newScriptDir,777)!=0)
			printf("Couldn't create newScriptDir ...\n");

	//根据新的文件路径创建新的文件
	char new_out[64] = {0};
	strcpy(new_out, newScriptDir);
	strcat(new_out, OUT_FN);

	char new_time[64] = {0};
	strcpy(new_time, newScriptDir);
	strcat(new_time, TIME);	




	out_fd = open(new_out, O_WRONLY | O_CREAT | O_TRUNC);
	if(out_fd < 0) {
		printf("Couldn't open output file\n");
		return 1;
	}

	out_time = open(new_time, O_WRONLY | O_CREAT | O_TRUNC);
	if(out_time < 0) {
		printf("Couldn't open output time file\n");
		return 1;
	}

	for(i = 0; i < NUM_DEVICES; i++) {
		sprintf(buffer, "%s%s", EV_PREFIX, ev_devices[i]);
		in_fds[i].events = POLLIN;
		in_fds[i].fd = open(buffer, O_RDONLY | O_NDELAY);
		if(in_fds[i].fd < 0) {
			printf("Couldn't open input device %s\n", ev_devices[i]);
			return 2;
		}

		#if 0
		sprintf(buffer, "%s%s", OUT_PREFIX, ev_devices[i]);
		out_fds[i] = open(buffer, O_WRONLY | O_CREAT);
		if(out_fds[i] < 0) {
			printf("Couldn't open output file %s\n", ev_devices[i]);
			return 2;
		}
		#endif
	}
	return 0;
}

int
record()
{
	int i, numread;
	struct input_event event;

	while(1) {
		if(poll(in_fds, NUM_DEVICES, -1) < 0) {
			printf("Poll error\n");
			return 1;
		}

		for(i = 0; i < NUM_DEVICES; i++) {
			if(in_fds[i].revents & POLLIN) {
				/* Data available */
				numread = read(in_fds[i].fd, &event, sizeof(event));
				if(numread != sizeof(event)) {
					printf("Read error\n");
					return 2;
				}
				if(write(out_fd, &i, sizeof(i)) != sizeof(i)) {
					printf("Write error\n");
					return 3;
				}
				if(write(out_fd, &event, sizeof(event)) != sizeof(event)) {
					printf("Write error\n");
					return 4;
				}
				char path_t[32] = {0};
				char str[16];
				sprintf(str, "%ld", event.time.tv_sec);
				strcat(path_t, str);
				strcat(path_t, "\n");
				if(write(out_time, path_t, strlen(path_t)) != strlen(path_t)) {
					printf("Write error\n");
					return 5;
				}				

				printf("input %d, time %ld.%06ld, type %d, code %d, value %d\n", i,
						event.time.tv_sec, event.time.tv_usec, event.type, event.code, event.value);
			}
		}
	}
}

int main()
{
	if (init() != 0) {
		printf("Init failed");
		return 1;
	}

	record();
}


