#include <comp421/filesystem.h>
#include <comp421/iolib.h>
#include <comp421/yalnix.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "yfs.h"
typedef struct File {
	int inum;
	int cur_pos;
	int open; //1 means opened, 0 means closed
}File;

struct File opened[MAX_OPEN_FILES]; 
int cur_dir = ROOTINODE;
int initialize = 0;

void initial() {
	if (initialize) return;
	int i = 0;	
	for (;i<MAX_OPEN_FILES; i ++) {
		opened[i].inum = 0;
		opened[i].cur_pos = 0;
		opened[i].open = 0;
	}
	initialize = 1;
}

/*
* return the fd if inum already opened
*/
int is_open(int inum) {
	int i;
	for(;i<MAX_OPEN_FILES;i++) {
		if(opened[i].inum==inum && opened[i].open == 1){
			return i;
		}
	}
	return -1;
}

int Open(char *pathname) {
	initial();
	// create msg to send
	my_msg msg;
	msg.type = OPEN;
	msg.data1 = cur_dir;
	msg.data2 = (int)strlen(pathname);
	msg.addr1 = pathname;
	fprintf(stderr, "path name in msg library%s\n", (char*)msg.addr1);

	// send the msg
	if (Send(&msg, -1) == ERROR) {
		// free(msg);
		return ERROR;
	}

	// receive the return msg
	if (msg.type == OPEN) {
		int fd = 0;
		for (;fd < MAX_OPEN_FILES; fd ++) {
			fprintf(stderr, "%d\n", fd);
			if (opened[fd].open == 0) {
				break;
			}
		}
		if(fd == MAX_OPEN_FILES || strlen(pathname) > MAXPATHNAMELEN) {
		fprintf(stderr, "fd error \n" );
		return ERROR;
		}
		opened[fd].open = 1;
		opened[fd].cur_pos = 0;
		opened[fd].inum = cur_dir;
	}
	return 0;
}
int Close(int fd) {
	// if (fd < 0 || fd > MAX_OPEN_FILES) return ERROR;
	// opened[fd]->open = 0;
	return 0;
}

/*
* This request creates and opens the new file named pathname
*
*/
int Create(char *pathname) {
	initial();
	// create the create msg
	my_msg msg;
    msg.type = CREATE;
    msg.addr1 = pathname;
    msg.data1 = (int) strlen(pathname);
    msg.data2 = cur_dir;
    fprintf(stderr, "the cur_dir is %d \n", msg.data2);
    
    // find fd for the created file
    int fd = 0;
	for (;fd < MAX_OPEN_FILES; fd ++) {
		fprintf(stderr, "%d\n", fd);
		if (opened[fd].open == 0) {
			break;
		}
	}
	if(fd == MAX_OPEN_FILES || strlen(pathname) > MAXPATHNAMELEN) {
		fprintf(stderr, "fd error \n" );
		return ERROR;
	}
   
    // send the msg
    if (Send(&msg,-1)==ERROR) {
        perror("error sending create message");
        return ERROR;
    }
    
    // receive the return msg
    if (msg.type == CREATE) {
    	opened[fd].open = 1;
		opened[fd].cur_pos = 0;
		opened[fd].inum = msg.data1;
    }
	return 0;
}
/*
 * Return value of length is stored in msg->wtype field
 */
int Read(int fd, void *buf, int size) {
// 	if (fd < 0 || fd > MAX_OPEN_FILES || opened[fd]->open == 0) return ERROR;
// 	my_msg* msg = (Msg*)malloc(sizeof(Msg*));
// 	msg->type = READ;
// //	msg->data1 = cur_dir;
// 	msg->addr1 = buf;
// 	msg->data2 = size;
// 	msg->data3 = opened[fd]->inum;
// 	msg->data1 = opened[fd]->cur_pos;
// 	if (Send((void *)msg, -FILE_SEVER) == ERROR) {
// 		free(msg);
// 		return ERROR;
// 	}
// 	if(msg->type == ERROR) return ERROR;
// 	free(msg);
// 	int len = msg->type;
// 	return len;
	return 0;
	/*
	 * Still need some work to hand new position
	 */
}
/*
 * Write data to an opened file, begin
 */
int Write(int fd, void *buf, int size) {
	// if (fd < 0 || fd > MAX_OPEN_FILES || opened[fd]->open == 0) return ERROR;
	// my_msg* msg = (Msg*)malloc(sizeof(Msg*));
	// msg->type = WRITE;
	// msg->addr1 = buf;
	// msg->data2 = size;
	// msg->data3 = opened[fd]->inum;
	// msg->data1 = opened[fd]->cur_pos;
	// if(Send((void *)msg, -FILE_SEVER) == ERROR) {
	// 	free(msg);
	// 	return ERROR;
	// }
	// if (msg->type == ERROR) return ERROR;
	// free(msg);
	// opened[fd]->cur_pos += msg->type; //advance new position
	// int len = msg->type;
	// return len;
	return 0;
}
int Seek(int fd, int offset, int whence) {

}
int Link(char *oldname, char *newname) {

}
int Unlink(char *pathname) {

}
int SymLink(char *oldname, char *newname) {

}
int ReadLink(char *pathname, char *buf, int len) {

}
int MkDir(char *pathname) {

}
int RmDir(char *pathname) {

}
int ChDir(char *pathname) {

}
int Stat(char *pathname, struct Stat *statbuf) {

}
int Sync() {

}
int Shutdown() {

}