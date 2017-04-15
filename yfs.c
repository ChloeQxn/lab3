// /*
// * Program for COMP 521 lab3
// * Author: Chunxiao Zhang, Xiangning Qi
// */

// /*
//  * the first thing you need to be able to do is to start the server
//  * and have it read the disk to build its internal list of free blocks and free inodes.
//  * So start with that.  Then, maybe try Open (you can Open("/"), for example).
//  * Once you have Open, try Link (it's pretty simple).
//  * Then maybe add Read.  Just keep adding features one at a time, testing as you go.
//  */

// /clear/courses/comp421/pub/bin/yalnix -ly 5 yfs
#include <comp421/filesystem.h>
#include <comp421/iolib.h>
#include <comp421/yalnix.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stack.h"
#include "uthash.h"
#include "yfs.h"

// my_msg *msg;

/*
try the hash
*/

struct my_struct {
    int id;            /* we'll use this field as the key */
    char name[3];             
    UT_hash_handle hh; /* makes this structure hashable */
};

struct my_struct *users = NULL;

void add_user(struct my_struct *s) {
    HASH_ADD_INT( users, id, s );    
}

struct my_struct *find_user(int user_id) {
    struct my_struct *s;

    HASH_FIND_INT( users, &user_id, s );  
    return s;
}

 char free_inode[];
 int free_inode_count = 0;
 char free_block[];
 int free_block_count = 0;

void delete_user(struct my_struct *user) {
    HASH_DEL( users, user);  
}

Stack s;

struct inode *getInode(i) {
	int blockIndex = 1 + (i) / (BLOCKSIZE/INODESIZE);
	void *buf = readBlockCache(blockIndex);
	if (buf == NULL) {
		TracePrintf(0, "Block is not in the cache%d \n", blockIndex);
		buf = malloc(SECTORSIZE);
		ReadSector(blockIndex, buf);
		TracePrintf(0, "Read the sector from blockIndex %d \n", blockIndex);
		putBlockCache(buf, blockIndex);
	}
	struct inode* node = (struct inode*)malloc(sizeof(struct inode));
	int offset = i % (BLOCKSIZE/INODESIZE);

	memcpy(node, (struct inode*)buf + offset * INODESIZE, sizeof(struct inode));
	putInodeCache(node, i);
	// free(buf);
	return node;
}

// get the buff of the block with given block number
void *getBlock(int blockIndex) {
	// void *buf = malloc(SECTORSIZE);
	// ReadSector(i, buf);
	void *buf = readBlockCache(blockIndex);
	if (buf == NULL) {
		TracePrintf(0, "Block is not in the cache%d \n", blockIndex);
		buf = malloc(SECTORSIZE);
		ReadSector(blockIndex, buf);
		TracePrintf(0, "Read the sector from blockIndex %d \n", blockIndex);
		putBlockCache(buf, blockIndex);
	}
	return buf;
}

int init() {
	TracePrintf(0, "Enter the init pocess...\n");
	void *buf = malloc(SECTORSIZE);
	// ReadSector(1, buf);
	struct fs_header *header= (struct fs_header *)getInode(0);
	int block_num = header->num_blocks;
	int inode_num = header->num_inodes;
	int i = 1;
	/*
	 * Use an array to store free block, 0 means free, 1 means occupied
	 */ 
	 free_block[0] = 1;
	 int inodeblock_num = (inode_num + 1) / (BLOCKSIZE/INODESIZE);
	for (; i < block_num; i ++) {
		if (i < inodeblock_num) {
			free_block[i] = 1;
		} else {
			free_block[i] = 0; 
			free_block_count++;
		} 
	}
	TracePrintf(0,"Initialize the free inode list\n");
	
	 // * Use the array to store free inode, 0 means free, 1 means occupied
	 
	free_inode[0] = 1;
	for (; i < inode_num; i ++) {
		struct inode *node = getInode(i);
		if (node->type == INODE_FREE) {
			free_inode[i] = 0;
			free_inode_count ++;
		} else {
			/*
			 * Loop the direct and indirect array
			 */
			 int j = 0;
			for (; j < NUM_DIRECT; j ++) {
				if (node->direct[j] == 0) break; 
				free_block[j] = 1;
				free_block_count --;
			 }

			ReadSector(node->indirect, buf);

			
			for(j = 0; j < sizeof(buf) / 4; j ++) {
				free_block[j + node->indirect] = 1; 
				free_block_count --;
			}
		

		}
	}
	TracePrintf(0, "Finish the init process\n");
}

// the name of file is DIRNAMELEN
// return the inode number of file with given filename
int path_file(char *pathname, int dir_inum) {
	// length of pathname
	int len_name = strlen(pathname);
	TracePrintf(0, "start path_file, len_name is %d, pathname is %s\n", len_name, pathname);
	
	// create a stack for inode number
	initStack(&s);

	// use helper to find the inum of the file
	int curr_index = 0;
	int curr_inum = dir_inum;
	TracePrintf(0,"path_file, the curr_inum and dir_inum is %d\n", curr_inum);
	int inum = path_file_helper(pathname, len_name, curr_index, curr_inum, 0);
	clear(&s);
	return inum;
}



int path_file_helper(char *pathname,int len_name, int curr_index, int curr_inum, int num_slink) {
	// check if it is absolute path
	TracePrintf(0, "the curr_index is %d\n", curr_index);
	if (curr_index == 0 && pathname[0] == '/') {
		TracePrintf(0, "it is a absolute path.\n");
		curr_inum = 1;
		return path_file_helper(pathname, len_name, ++curr_index, curr_inum, num_slink);
	}

	// delete the dup dash
	while (curr_index < len_name && pathname[curr_index] == '/') curr_index++;
	TracePrintf(0, "after delete, the index is %d\n:",curr_index);
	
	// check if the last index is dash .
	if (curr_index == len_name) {
		TracePrintf(0, "the last letter is dash, return\n");
		return curr_inum;
	}
	
	// buff the component
	TracePrintf(0, "deal with the component in the pathname:\n");
	int i = 0;
	char component[DIRNAMELEN];
    memset(component,'\0',DIRNAMELEN);
	while (curr_index < len_name && pathname[curr_index] != '/') {
		component[i++] = pathname[curr_index++];
	}
	// TracePrintf(0, "length%d", strlen(component));
	TracePrintf(0, "the component is %s\n", component);
	
	// if the component is .., return the curr_inode as pop stack
	if (component[0] == "." && component[1] == "." && component[2] == '\0') {
		if (!empty(&s)) 
			return path_file_helper(pathname, len_name, curr_index++, pop(&s), num_slink);
		else 
			return ERROR;
	}

	// if the componet is ., just return
	if (component[0] == "." && component[1] == '\0')
		return path_file_helper(pathname, len_name, curr_index++, curr_inum, num_slink);
	
	// find the inode of the curr file name
	TracePrintf(0, "print currnet inum before entry: %d\n", curr_inum);
	curr_inum = entry_query(component, curr_inum);
	if (curr_inum == -1) {
		return -1;
	}
	push(&s, curr_inum);
	TracePrintf(0, "push a curr_inum node in to stack: the curr_inum is %d\n", curr_inum);
	return path_file_helper(pathname, len_name, curr_index++, curr_inum, num_slink);
}

// find the inode of given filename in the directory
int entry_query(char *filename, int dir_inum) {
	TracePrintf(0, "entry_query starts... the filename is %s, the curr directory inum is: %d\n", filename, dir_inum);
	
	// struct inode *dir_inode = (struct inode*)malloc(sizeof(struct inode));
	struct inode *dir_inode = getInode(dir_inum); 
	// return -1 if this is not a directory
	if (dir_inode->type != INODE_DIRECTORY) {
		return -1;
	}

	// the entry number in this dir file
	int entry_num = (dir_inode->size) / sizeof(struct dir_entry);
	int entry_count = 0;
	TracePrintf(0, "the entry_num is %d", entry_num);

	// loop over the direct block array
	int i;
	for (i = 0; i < NUM_DIRECT; i++) {
		int block_num = dir_inode->direct[i];
		struct dir_entry *block_buf = (struct dir_entry*)getBlock(block_num);
		int j;
		for (j = 0; j < SECTORSIZE/sizeof(struct dir_entry); j++) {
			entry_count++;
			if (entry_count > entry_num) return -1;
			if (strcmp(block_buf[j].name, filename) == 0) {
				TracePrintf(0, "find the file inum%d", block_buf[j].inum);
				return block_buf[j].inum;
			}
			
		}
	}

	TracePrintf(0, "not found in the dir block, loop the indirect:");
	// loop over the indirect block, indirect block store the block numbers
	int *block_buf = (int*)getBlock(dir_inode->indirect);
	i = 0;
	for (i = 0; i < SECTORSIZE/4;i++) {
		if (block_buf[i]>0) {
			int block_num = block_buf[i];
			struct dir_entry *block_buf2 = (struct dir_entry*)getBlock(block_num);
			int j;
			for (j = 0; j < SECTORSIZE/sizeof(struct dir_entry); j++) {
				entry_count++;
			    if (entry_count > entry_num) return -1;
				if (strcmp(block_buf2[j].name, filename) == 0) {
					return block_buf2[j].inum;
				}
			}
		}
		
	}
	return -1;
}

// add an entry of given filename
// int add_entry(char *filename, int new_inum, int dir_inum) {
// 	// struct inode *dir_inode = (struct inode*)malloc(sizeof(struct inode));
// 	struct inode *dir_inode = getInode(dir_inum); 
// 	// return -1 if this is not a directory
// 	if (dir_inode->type != INODE_DIRECTORY) {
// 		return -1;
// 	}

// 	// loop over the direct block array
// 	int i;
// 	for (i = 0; i < NUM_DIRECT; i++) {
// 		int block_num = dir_inode->direct[i];
// 		struct dir_entry *block_buf = (struct dir_entry*)getBlock(block_num);
// 		int j;
// 		for (j = 0; j < SECTORSIZE/sizeof(struct dir_entry); j++) {
// 			// if the entry is empty, add new entry here
// 			if (block_buf[j].inum == 0) {
// 				block_buf[j].inum = new_inum;
// 				memcpy(block_buf[j].name, filename, MAXPATHNAMELEN);
// 				return 1;
// 			}
			
// 		}
// 	}

// 	TracePrintf(0, "not found in the dir block, loop the indirect:");
// 	// loop over the indirect block, indirect block store the block numbers
// 	int *block_buf = (int*)getBlock(dir_inode->indirect);
// 	i = 0;
// 	for (i = 0; i < SECTORSIZE/4;i++) {
// 		if (block_buf[i]>0) {
// 			int block_num = block_buf[i];
// 			struct dir_entry *block_buf2 = (struct dir_entry*)getBlock(block_num);
// 			int j;
// 			for (j = 0; j < SECTORSIZE/sizeof(struct dir_entry); j++) {
// 				// if the entry is empty, add new entry heere
// 				if (block_buf2[j].inum == 0) {
// 					memcpy(block_buf2[j].name, filename, MAXPATHNAMELEN);
// 					return 1;
// 				}
				
// 			}
// 		}
		
// 	}
// 	// no tmpty entry available in the dir
// 	return ERROR;
// }

/*
handlers for the message
*/
// open the file with given filename, returns the descriptor of the file if success
int open_handler(my_msg *msg, int sender_pid) {
	TracePrintf(0,"open_handler start...\n");
	// read the msg to local
	char pathname[MAXPATHNAMELEN];
	if (CopyFrom(sender_pid,pathname,msg->addr1,msg->data2+1) == ERROR) {
    	TracePrintf(0, "ERROR\n");
    }
    TracePrintf(0, "open_handleer: the pathname is %s\n", pathname);
    int dir_inum;
    CopyFrom(sender_pid, &dir_inum, &(msg->data1), sizeof(int));
    TracePrintf(0, "open_handler: the dir_inum after CopyFrom:%d\n", dir_inum);
    // find the inum for the open file
    int open_inum = path_file(pathname,dir_inum);

    // 
    if (open_inum<=0) {
        msg->type = ERROR;
    }
    
    return open_inum;
}

// close the file with given fd
int close_handler(int fd) {

}

// create and open the new file
int create_handler(char *pathname){

}

// This request reads data from an open file, beginning at the current position in the file as represented 
// by the given file descriptor fd
int read_handler(int fd, void *buf, int size) {

}

int write_handler(int fd, void *buf, int size) {

}

/* create a head link from new file name to the existing oldname
 * return error if the newname already exists
 * return 0 if success
*/
// int link_handler(Msg *msg, int sender_pid) {
// 	// read the newname form msg
// 	char newname[MAXPATHNAMELEN];
// 	if (CopyFrom(senderPID, newname, msg->addr1, msg->data1+1) == ERROR) {
// 		return ERROR;
// 	}

// 	// firstly find the parent inum of the newname

// 	// if the new name already exists
// 	if (entry_query(newname, curr_inum) != -1) 
// 		return ERROR;

// 	// read oldname and curr_inum from msg
// 	char oldname[MAXPATHNAMELEN];
// 	int curr_inum = 1;
// 	if (CopyFrom(senderPID, oldname, msg->addr2, msg->data2+1) == ERROR) {
// 		return ERROR;
// 	}
// 	if (CopyFrom(senderPID, &curr_inum, &(msg->data3), sizeof(int)) == ERROR) {
// 		return ERROR;
// 	}

// 	// find the inum for the oldname, if it is a dir, return error
// 	int old_inum = entry_query(oldname, curr_inum);
// 	if (getInode(old_inum)->type == INODE_DIRECTORY) return ERROR;

// 	// find the parent file for this entry to be added
// 	// copy the parent filename 
// 	char parent_name[MAXPATHNAMELEN];
// 	int i = MAXPATHNAMELEN;
// 	while(i >= 0 && newname[i] != '/') {
// 		i--;
// 	}
// 	for (;i>=0;i--) {
// 		parent_name[i] = newname[i];
// 	}
// 	// find the parent inum
// 	int parent_inum = path_file();


// 	if (parent_inum != -1) {
// 		add_entry(filename, old_inum, parent_inum);
// 	}


// 	if (add_entry(newname, old_inum, curr_inum) == ERROR) 
// 		return ERROR;
// }

int main(int argc, char **argv) {
	TracePrintf(0, "Running the server process\n");
	int pid;
	struct my_msg msg;
    int senderPID;
	init();
	if (Register(FILE_SERVER)  != 0) {
		fprintf(stderr, "ERROR!!! Failed to initialize the service\n" );
	}
	TracePrintf(0, "Finished the register\n");
    if (argc>1) {
    	pid = Fork();
	    if (pid==0) {
	        Exec(argv[1],argv+1);
	    }
	}
	TracePrintf(0, "Running................\n");
    while (1) {
    	senderPID=Receive(&msg);
    	fprintf((stderr), "sender pid is %d\n", senderPID );
        if (senderPID==ERROR) {
            perror("error receiving message!\n");
            continue;
        }
        TracePrintf(0, "the size of msg:%d \n", sizeof(msg));
        switch (msg.type) {
             case OPEN:
                 open_handler(&msg,senderPID);break;
            // case CREATE:
            //     create_handler(&myMsg,sender_pid);break;
            // case READ:
            //     read_handler(&msg,senderPID);break;
            // case WRITE:
            //     write_handler(&msg,senderPID);break;
            // case LINK:
            //     link_handler(&myMsg,sender_pid);break;
            // case UNLINK:
            //     unlink_handler(&myMsg,sender_pid);break;
            // case SYMLINK:
            //     symlink_handler(&myMsg,sender_pid);break;
            // case READLINK:
            //     readlink_handler(&myMsg,sender_pid);break;
            // case MKDIR:
            //     mkdir_handler(&myMsg,sender_pid);break;
            // case RMDIR:
            //     rmdir_handler(&myMsg,sender_pid);break;
            // case CHDIR:
            //     chdir_handler(&myMsg,sender_pid);break;
            // case STAT:
            //     stat_handler(&myMsg,sender_pid);break;
            // case SYNC:
            //     sync_handler(&myMsg);break;
            // case SHUTDOWN:
            //     shutdown_handler(&myMsg,sender_pid);break;
            default:
                perror("message type error!");
                break;
        }

         }
//         if (Reply(&myMsg,sender_pid)==ERROR) fprintf(stderr, "Error replying to pid %d\n",sender_pid);
//     }
	return 0;
} 

// int main(int argc, char **argv) {

// 	TracePrintf(0, "Running the server process\n");
// 	char *pathname = "////a/b/c/";
// 	TracePrintf(0, "nakme:%s", pathname);
// // 	// path_file(pathname, ROOTINODE);
// // 	// struct inode *my = getInode(3);
// // 	/**/
// 	char name[3] = "tom" ;
// 	TracePrintf(0, "print the name:%s\n", name);
// 	(struct my_struct) ex;
// 	ex = {1,"tom"};
// 	TracePrintf(0,"print the key and name:%d,%s\n",ex->id,ex->name);
// 	add_user(ex);
// 	int value = find_user(1)->name;
// 	TracePrintf(0, "the name is %s:", value);
// 	return 0;	
// } 








