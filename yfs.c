/*
* Program for COMP 521 lab3
* Author: Chunxiao Zhang, Xiangning Qi
*/

/*
 * the first thing you need to be able to do is to start the server
 * and have it read the disk to build its internal list of free blocks and free inodes.
 * So start with that.  Then, maybe try Open (you can Open("/"), for example).
 * Once you have Open, try Link (it's pretty simple).
 * Then maybe add Read.  Just keep adding features one at a time, testing as you go.
 */
#include <comp421/filesystem.h>
#include <comp421/iolib.h>
#include <comp421/yalnix.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

 char free_inode[];
 int free_inode_count = 0;
 char free_block[];
 int free_block_count = 0;

 
int main(int argc, char **argv) {
	TracePrintf(0, "Running the server process\n");
	int pid;
	//my_msg msg;
    int senderPID;
	init();
	if (Register(FILE_SERVER) == ERROR) {
		fprintf(stderr, "Failed to initialize the service\n" );
	}
	TracePrintf(0, "Finished the register\n");
    if (argc>1) {
    	pid = Fork();
	    if (pid==0) {
	        Exec(argv[1],argv+1);
	    }
	}
	TracePrintf(0, "Running\n");
    while (1) {
        if ((senderPID=Receive(&myMsg))==ERROR) {
            perror("error receiving message!");
            continue;
        }
         switch (myMsg.type) {
             case OPEN:
                 open_handler(&msg,senderPID);break;
            // case CREATE:
            //     create_handler(&myMsg,sender_pid);break;
            case READ:
                read_handler(&myMsg,sender_pid);break;
            // case WRITE:
            //     write_handler(&myMsg,sender_pid);break;
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

void read_handler()

struct inode *getInode(i) {
	TracePrintf(0, "Get inode function\n");
	struct inode *inode = readInodeCache(i);
	if (inode != NULL) {
		return inode;
	}
	int blockIndex = 1 + (i + 1) / (BLOCKSIZE/INODESIZE);
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
	free(buf);
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
	/*
	 * Use the array to store free inode, 0 means free, 1 means occupied
	 */
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
	int len_name = sizeof(pathname);
	return path_file_helper(pathname, len_name, 0, dir_inum, 0);
}
int path_file_helper(char *pathname,int len_name, int curr_index, int curr_inum, int num_slink) {
	// check if it is absolute path
	TracePrintf(0, "check the first char find if it is a complet path:");
	if (curr_index == 0 && pathname[0] == '/') {
		curr_inum = 0;
		return path_file_helper(pathname, len_name, curr_index++, curr_inum, num_slink);
	}

	// delete the dup dash
	TracePrintf(0, "several dash situation:");
	while (curr_index < len_name && pathname[curr_index] == '/') curr_index++;

	// if the last index is dash or the componnet is .
	if (curr_index == len_name) {
		return curr_inum;
	}
	// buff the curr filename
	TracePrintf(0, "deal with the component in the pathname:");
	int i = 0;
	char *file_name = (char *)malloc(DIRNAMELEN);
	while (curr_index < len_name && pathname[curr_index] != '/') {
		file_name[i++] = pathname[curr_index++];
	}

	// if the component is . or ..
	if (strcmp(file_name,'.') == 0)
		return path_file_helper(pathname, len_name, curr_index++, curr_inum, num_slink);
	// how to get the parent's inode number, parent inode number is stored in the inode cache?
	if (strcmp(file_name,'..') == 0)
		return path_file_helper(pathname, len_name, curr_index++, curr_inum, num_slink);
	
	// find the inode of the curr file name
	curr_inum = entry_query(file_name, curr_inum);
	return path_file_helper(pathname, len_name, curr_index++, curr_inum, num_slink);
}

// find the inode of given filename in the directory
int entry_query(char *filename, int dir_inum) {
	// struct inode *dir_inode = (struct inode*)malloc(sizeof(struct inode));
	struct inode *dir_inode = getInode(dir_inum); 
	// return -1 if this is not a directory
	if (dir_inode->type != INODE_DIRECTORY) {
		return -1;
	}

	// loop over the direct block array
	int i;
	for (i = 0; i < NUM_DIRECT; i++) {
		int block_num = dir_inode->direct[i];
		struct dir_entry *block_buf = (struct dir_entry*)getBlock(block_num);
		int j;
		for (j = 0; j < SECTORSIZE/sizeof(struct dir_entry); j++) {
			if (strcmp(block_buf[j].name, filename) == 0) {
				return block_buf[j].inum;
			}
		}
	}

	// loop over the indirect block, indirect block store the block numbers
	int *block_buf = (int*)getBlock(dir_inode->indirect);
	i = 0;
	for (i = 0; i < SECTORSIZE/4;i++) {
		if (block_buf[i]>0) {
			int block_num = block_buf[i];
			struct dir_entry *block_buf2 = (struct dir_entry*)getBlock(block_num);
			int j;
			for (j = 0; j < SECTORSIZE/sizeof(struct dir_entry); j++) {
				if (strcmp(block_buf2[j].name, filename) == 0) {
					return block_buf2[j].inum;
				}
			}
		}
		
	}
	return ERROR;
}





