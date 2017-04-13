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
	// TracePrintf(0, "Running\n");
 //    while (1) {
 //        if ((senderPID=Receive(&myMsg))==ERROR) {
 //            perror("error receiving message!");
 //            continue;
 //        }
 //         switch (myMsg.type) {
 //             case OPEN:
 //                 open_handler(&msg,senderPID);break;
//             case CREATE:
//                 create_handler(&myMsg,sender_pid);break;
//             case READ:
//                 read_handler(&myMsg,sender_pid);break;
//             case WRITE:
//                 write_handler(&myMsg,sender_pid);break;
//             case LINK:
//                 link_handler(&myMsg,sender_pid);break;
//             case UNLINK:
//                 unlink_handler(&myMsg,sender_pid);break;
//             case SYMLINK:
//                 symlink_handler(&myMsg,sender_pid);break;
//             case READLINK:
//                 readlink_handler(&myMsg,sender_pid);break;
//             case MKDIR:
//                 mkdir_handler(&myMsg,sender_pid);break;
//             case RMDIR:
//                 rmdir_handler(&myMsg,sender_pid);break;
//             case CHDIR:
//                 chdir_handler(&myMsg,sender_pid);break;
//             case STAT:
//                 stat_handler(&myMsg,sender_pid);break;
//             case SYNC:
//                 sync_handler(&myMsg);break;
//             case SHUTDOWN:
//                 shutdown_handler(&myMsg,sender_pid);break;
//             default:
//                 perror("message type error!");
//                 break;
 //        }
//         if (Reply(&myMsg,sender_pid)==ERROR) fprintf(stderr, "Error replying to pid %d\n",sender_pid);
//     }
	return 0;	
} 

struct inode *getInode(i) {
	TracePrintf(0, "Get inode function\n");
	struct inode *inode = readInodeCache(i);
	if (inode != NULL) {
		return inode;
	}
	
	void *buf = readBlockCache(i);
	if (buf == NULL) {
		int blockIndex = 1 + (i + 1) / (BLOCKSIZE/INODESIZE);
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
	return node;
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