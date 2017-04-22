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
	int size;
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
	// fprintf(stderr, "lib: OPEN\n");
	initial();
	// create msg to send
	my_msg msg;
	msg.type = OPEN;
	msg.data1 = cur_dir;
	msg.data2 = (int)strlen(pathname);
	msg.addr1 = pathname;
	fprintf(stderr, "the cur_dir is %d \n", msg.data1);
	//fprintf(stderr, "path name in msg library%s\n", (char*)msg.addr1);

	// send the msg
	if (Send(&msg, -1) == ERROR) {
		// free(msg);
		return ERROR;
	}
	int fd = 0;

	// receive the return msg
	if (msg.type == OPEN) {
		// fprintf(stderr, "message opened successfully\n");
		
		for (;fd < MAX_OPEN_FILES; fd ++) {
			//fprintf(stderr, "%d\n", fd);
			if (opened[fd].open == 0) {
				break;
			}
		}
		if(fd == MAX_OPEN_FILES || strlen(pathname) > MAXPATHNAMELEN) {
		// fprintf(stderr, "fd error \n" );
		return ERROR;
		}
		if (msg.data1==-1) return ERROR;
		opened[fd].open = 1;
		opened[fd].cur_pos = 0;
		opened[fd].inum = msg.data1;
		// fprintf(stderr, "lib open: inode number is %d\n", opened[fd].inum);
	}
	return fd;
}

/*
* This request creates and opens the new file named pathname
*
*/
int Create(char *pathname) {
	// fprintf(stderr, "CREATE\n");
	initial();
	// create the create msg
	my_msg msg;
    msg.type = CREATE;
    msg.addr1 = pathname;
    msg.data1 = (int) strlen(pathname);
    msg.data2 = cur_dir;
    fprintf(stderr, "the cur_dir is %d \n", msg.data2);
   // fprintf(stderr, "CREATE %d\n", msg.data2);
    
    
    
    // find fd for the created file
    int fd = 0;
	for (;fd < MAX_OPEN_FILES; fd ++) {
		if (opened[fd].open == 0) {
			break;
		}
	}
	if(fd == MAX_OPEN_FILES || strlen(pathname) > MAXPATHNAMELEN) {
		// fprintf(stderr, "fd error \n" );
		return ERROR;
	}
   
    // send the msg
    if (Send(&msg,-1)==ERROR) {
        perror("error sending create message");
        return ERROR;
    }
    // fprintf(stderr, "CREATE REPLY\n");
    // receive the return msg
    if (msg.type == CREATE) {
    	if (msg.data1 == -1) return ERROR;
    	opened[fd].open = 1;
		opened[fd].cur_pos = 0;
		opened[fd].inum = msg.data1;
    }
    // fprintf(stderr, "fd of %d is %d\n", fd, opened[fd].inum);
	return fd;
}
/*
 * Return value of length is stored in msg->wtype field
 */
int Read(int fd, void *buf, int size) {
	initial();
	if (fd < 0 || fd > MAX_OPEN_FILES || opened[fd].open == 0) return ERROR;
	if (opened[fd].inum == 0) {
		perror("Please open the file before you read it\n"); 
		return ERROR;
	}
	my_msg* msg = (my_msg*)malloc(sizeof(my_msg*));
	msg->type = READ;
//	msg->data1 = cur_dir;
	msg->addr1 = buf;
	msg->data2 = size;
	msg->data3 = opened[fd].inum;
	msg->data1 = opened[fd].cur_pos;
	if (Send((void *)msg, -1) == ERROR) {
		free(msg);
		return ERROR;
	}
	if(msg->type == ERROR) return ERROR;
	opened[fd].cur_pos += msg->data1;
	int length = msg->data1;
	free(msg);
	return length;
}
/*
 * Write data to an opened file, begin
 */
int Write(int fd, void *buf, int size) {
	initial();
	fprintf(stderr, "WRITE\n");
	if (fd < 0 || fd > MAX_OPEN_FILES || opened[fd].open == 0) return ERROR;
	if (opened[fd].inum == 0) {
		perror("Please open this file before you write it\n");
		return ERROR;
	}
	my_msg* msg = (my_msg*)malloc(sizeof(my_msg*));
	msg->type = WRITE;
	msg->addr1 = buf;
	msg->data2 = size;
	msg->data3 = opened[fd].inum;
	msg->data1 = opened[fd].cur_pos;
	if(Send((void *)msg, -1) == ERROR) {
		free(msg);
		return ERROR;
	}
	if (msg->type == ERROR) return ERROR;
	opened[fd].cur_pos += msg->data1; //advance new position
	int len = msg->data1;
	if (opened[fd].size < opened[fd].cur_pos + 1) opened[fd].size = opened[fd].cur_pos;
	free(msg);

	return len;
}
int Seek(int fd, int offset, int whence) {
	initial();
	if (fd < 0 || fd > MAX_OPEN_FILES || opened[fd].open == 0) return ERROR;
	if (opened[fd].inum == 0) {
		perror("Please open this file before you write it\n");
		return ERROR;
	}
	/*
	 * The Seek file system operation does allow the file offset 
	 * to be set beyond the end of the existing
	 * size of the file; this is not an error.
	 */
	if (whence == SEEK_SET) {
		if (offset < 0) {
			perror("offset cannot go beyond the begin of the file\n");
			return ERROR;
		} else {
			opened[fd].cur_pos = offset;
			return opened[fd].cur_pos;
		}
	} else if (whence == SEEK_CUR) {
		if (offset + opened[fd].cur_pos < 0) {
			perror("offset cannot go beyond the begin of the file\n");
			return ERROR;
		} else {
			opened[fd].cur_pos += offset;
			return opened[fd].cur_pos;
		}
	} else if (whence == SEEK_END) {
		if (offset + opened[fd].size < 0) {
			perror("offset cannot go beyond the begin of the file\n");
			return ERROR;
		} else {
			opened[fd].cur_pos = offset + opened[fd].size;
			return opened[fd].cur_pos;
		}
	} else {
		perror("Unknown whence type, check your input\n");
		return ERROR;
	}
}

int Link(char *oldname, char *newname) {
	// create the msg
	my_msg msg;
    msg.type = LINK;
    msg.addr1 = oldname;
    msg.data1 = (int) strlen(oldname);
    msg.addr2 = newname;
    msg.data2 = (int) strlen(newname);
    msg.data3 = cur_dir;
    fprintf(stderr, "the cur_dir is %d \n", msg.data3);
    
    // send the msg
    if (Send(&msg,-1)==ERROR) {
        perror("error sending link message");
        return ERROR;
    }
    
    // receive the return msg
    if (msg.type == LINK) {
    	if (msg.data1 == -1) return ERROR;
    }
	return 0;
}
int Unlink(char *pathname) {
	// create the msg
	my_msg msg;
    msg.type = UNLINK;
    msg.addr1 = pathname;
    msg.data1 = (int) strlen(pathname);
    msg.data2 = cur_dir;
    fprintf(stderr, "the cur_dir is %d \n", msg.data2);
    
    // send the msg
    if (Send(&msg,-1)==ERROR) {
        perror("error sending unlink message");
        return ERROR;
    }
    
    // receive the return msg
    if (msg.type == UNLINK) {
    	if (msg.data1 == -1) return ERROR;
    }
	return 0;
}
int SymLink(char *oldname, char *newname) {
	// if the newname is null return error
	if (newname == NULL) return ERROR;
	// create the msg
	my_msg msg;
    msg.type = SYMLINK;
    msg.addr1 = oldname;
    msg.data1 = (int) strlen(oldname);
    msg.addr2 = newname;
    msg.data2 = (int) strlen(newname);
    msg.data3 = cur_dir;
    fprintf(stderr, "the cur_dir is %d \n", msg.data3);
    
    // send the msg
    if (Send(&msg,-1)==ERROR) {
        perror("error sending link message");
        return ERROR;
    }
    
    // receive the return msg
    if (msg.type == SYMLINK) {
    	if (msg.data1 == -1) return ERROR;
    }
	return 0;
}

int ReadLink(char *pathname, char *buf, int len) {
	fprintf(stderr, "lib: readlink. the len is %d \n", len);
	my_msg msg;
    msg.type = READLINK;
    msg.addr1 = pathname;
    msg.data1 = (int) strlen(pathname);
    msg.addr2 = buf;
    msg.data2 = len;
    msg.data3 = cur_dir;
    fprintf(stderr, "the cur_dir is %d \n", msg.data3);
    
    // send the msg
    if (Send(&msg,-1)==ERROR) {
        perror("error sending link message");
        return ERROR;
    }
    
    // receive the return msg
    if (msg.type == READLINK) {
    	if (msg.data1 == -1) return ERROR;
    }
	return msg.data1;
}

int MkDir(char *pathname) {
	// create the msg
	my_msg msg;
    msg.type = MKDIR;
    msg.addr1 = pathname;
    msg.data1 = (int) strlen(pathname);
    msg.data2 = cur_dir;
    fprintf(stderr, "the cur_dir is %d \n", msg.data2);
    
    // send the msg
    if (Send(&msg,-1)==ERROR) {
        perror("error sending create message");
        return ERROR;
    }
    
    // receive the return msg
    if (msg.type == MKDIR) {
    	if (msg.data1 == -1) return ERROR;
    }
	return 0;
}
int RmDir(char *pathname) {
	// create the msg
	my_msg msg;
    msg.type = RMDIR;
    msg.addr1 = pathname;
    msg.data1 = (int) strlen(pathname);
    msg.data2 = cur_dir;
    fprintf(stderr, "the cur_dir is %d \n", msg.data2);
    
    // send the msg
    if (Send(&msg,-1)==ERROR) {
        perror("error sending RmDir message");
        return ERROR;
    }
    
    // receive the return msg
    if (msg.type == RMDIR) {
    	if (msg.data1 == -1) return ERROR;
    }
	return 0;
}

int ChDir(char *pathname) {
	// create the msg
	my_msg msg;
    msg.type = CHDIR;
    msg.addr1 = pathname;
    msg.data1 = (int) strlen(pathname);
    msg.data2 = cur_dir;
    fprintf(stderr, "the cur_dir is %d \n", msg.data2);
    
    // send the msg
    if (Send(&msg,-1)==ERROR) {
        perror("error sending RmDir message");
        return ERROR;
    }
    
    // receive the return msg
    if (msg.type == CHDIR) {
    	if (msg.data1 == -1) return ERROR;
    }
    cur_dir = msg.data1;
    return 0;
}
int Stat(char *pathname, struct Stat *statbuf) {
	// create the msg
	my_msg msg;
    msg.type = STAT;
    msg.addr1 = pathname;
    msg.data1 = (int) strlen(pathname);
    msg.data2 = cur_dir;
    msg.addr2 = statbuf;
    fprintf(stderr, "the cur_dir is %d \n", msg.data2);
    
    // send the msg
    if (Send(&msg,-1)==ERROR) {
        perror("error sending stat message");
        return ERROR;
    }
    
    // receive the return msg
    if (msg.type == STAT) {
    	if (msg.data1 == -1) return ERROR;
    }
    return 0;

}
int Sync() {
	// create the msg
	my_msg msg;
    msg.type = SYNC;
    fprintf(stderr, "the cur_dir is %d \n", msg.data2);
    
    // send the msg
    if (Send(&msg,-1)==ERROR) {
        perror("error sending sync message");
        return ERROR;
    }
    
    // receive the return msg
    if (msg.type == SYNC) {
    	return 0;
    }
}

int Shutdown() {
 	initial();
    my_msg msg;
    msg.type=SHUTDOWN;

    if (Send(&msg,-FILE_SERVER)==ERROR) {
        perror("error sending shutdown message");
        return ERROR;
    }

     if (msg.type!=ERROR) {
        printf("file server is shutdown!\n");
        return 0;
    } else
        return ERROR;
}

int Close(int fd) {
	fprintf(stderr, "CLOSE\n");
	initial();
	if (fd < 0 || fd > MAX_OPEN_FILES) return ERROR;
	opened[fd].open = 0;
	return 0;
}



