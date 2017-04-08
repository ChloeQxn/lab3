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
#include <stdio.h>
 char free_inode[];
 int free_inode_count = 0;
 char free_block[];
 int free_block_count = 0;
 
int main(int argc, char **argv) {
	init();
	return 0;	
} 
void *getInode(i) {
	int blockIndex = (i + 1) / (BLOCKSIZE/INODESIZE) + 1;
	void *buf = malloc(SECTORSIZE);
	ReadSector(blockIndex, buf);
	return buf;
}
int init() {
	void *buf = malloc(SECTORSIZE);
	ReadSector(1, buf);
	struct fs_header *header= *buf;
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
	/*
	 * Use the array to store free inode, 0 means free, 1 means occupied
	 */
	free_inode[0] = 1;
	for (; i < inode_num; i ++) {
		struct inode *node = getInode(i);
		if (node->type == INODE_FREE {
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
}