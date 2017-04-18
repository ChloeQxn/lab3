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
// #include "cache.h"
#include "cacheLRU.h"

// the num of total blocks and inodes
int block_num;
int inode_num;

// a list store the status of inode and blocks, 0 means free, 1 means dirty
int free_inode[];
int free_inode_count = 0;
int free_block[];
int free_block_count = 0;


struct my_comp {
	int par_inum;
	char name[DIRNAMELEN];
};

////////////////
void *getBlock(int blockIndex) {
	TracePrintf(0, "GET BLOCK %d .............\n", blockIndex);
	if (!blockIndex) {
		fprintf(stderr, "ERROR! CANNOT READ BLOCK 0\n");
		TracePrintf(0,"ERROR! CANNOT READ BLOCK 0\n");
	}
	void *buf1 = readBlockCache(blockIndex);
	if (buf1 == NULL) {
		TracePrintf(0, "Block is not in the cache %d \n", blockIndex);
		buf1 = malloc(SECTORSIZE);
		if (ReadSector(blockIndex, buf1) == ERROR) {
			TracePrintf(0, "Read sector error\n");
		}
		TracePrintf(0, "Read the sector from blockIndex %d \n", blockIndex);
		putBlockCache(buf1, blockIndex);
	}
	TracePrintf(0, "Finish get block ................\n");
	return buf1;
}

struct inode *getInode(i) {
	TracePrintf(0, "Get inode %d .................\n", i);
	struct inode* node = readInodeCache(i);
	if (node != NULL) {
		TracePrintf(0, "This inode is found in the cache\n");
		return node;
	}
	int blockIndex = 1 + i / (BLOCKSIZE/INODESIZE);
	void *buf = getBlock(blockIndex);
	int offset = i % (BLOCKSIZE/INODESIZE);
	node = (struct inode *)malloc(INODESIZE);
	//memcpy((void *)node, buf + offset * INODESIZE, INODESIZE);
	memcpy((void *)node, buf + offset, INODESIZE);
	putInodeCache(node, i);
	TracePrintf(0, "Finish get inode %d .................\n", i);
	//free(buf);
	return node;
}

///////////////////////////////////////////////////////////////////////////
// get functions for test without cache
// struct inode *getInode(int i) {
// 	int blockIndex = 1 + (i) / (BLOCKSIZE/INODESIZE);
// 	void *buf = malloc(SECTORSIZE);
// 	ReadSector(blockIndex, buf);
// 	struct inode* node = (struct inode*)malloc(sizeof(struct inode));
// 	int offset = i % (BLOCKSIZE/INODESIZE);
// 	memcpy(node, (struct inode*)buf + offset, sizeof(struct inode));
// 	free(buf);
// 	return node;
// }


// // get the buff of the block with given block number
// void *getBlock(int blockIndex) {
// 	void *buf = malloc(SECTORSIZE);
// 	ReadSector(blockIndex, buf);
// 	return buf;
// }

// /*
//  * free the inode of given index
// */
// int freeInode(int index) {
// 	if (index > inode_num || index == 0 || index == 1) return ERROR;
// 	free_inode[index] = 0;
// 	free_inode_count++;
// 	// struct inode *node = getInode(index);
// 	// node->type = INODE_FREE;
// 	return index;
// }
///////////////////////////////////////////////////////////////////////////

/*
* Get a free inode for use. Return the inum if suceess.
*/
int get_freeInode() {
	TracePrintf(0, "get_freeInode starts...\n");
	if (free_inode_count == 0) return ERROR;
	int i = 2;
	for (; i <= inode_num; i++) {
		if (free_inode[i] == 0) {
			break;
		}
	}
	TracePrintf(0, "get a free inode %d\n", i);
	return i;
}

// int allocInode(int type)
// {
//     int inum;
//     for (i=2; inum<=inode_num; inum++) {
//         if (free_inode[i]==0) break;
//     }
//     if (i > inode_num) return ERROR;

//     // inode_cache *n = read_inode(free_inum);
//     if (type==INODE_REGULAR) {
//         n->data.type = INODE_REGULAR;
//         n->dirty = 1;
//         n->data.nlink = 1;
//         n->data.reuse++;
//         n->data.size = 0;
//         for (i=0; i<NUM_DIRECT; i++)
//             n->data.direct[i] = 0;
//         n->data.indirect = 0;
//     }
//     else if (type==INODE_DIRECTORY) {
//         n->data.type = INODE_DIRECTORY;
//         n->dirty = 1;
//         n->data.nlink = 1;
//         n->data.reuse++;
//         n->data.size = 2*sizeof(struct dir_entry);
//         for (i=1; i<NUM_DIRECT; i++)
//             n->data.direct[i] = 0;
//         n->data.indirect = 0;
//         n->data.direct[0] = alloc_block();
//         block_cache *b = read_block(n->data.direct[0]);
//         struct dir_entry init[2];
//         init[0].inum = free_inum;
//         init[0].name[0] = '.';
//         init[1].inum = pare_inum;
//         init[1].name[0] = '.';
//         init[1].name[1] = '.';
//         memcpy(b->data,init,2*sizeof(struct dir_entry));
//         b->dirty = 1;
//     }
//     else if (type==INODE_SYMLINK) {

//         n->data.type = INODE_SYMLINK;
//         n->dirty = 1;
//         n->data.nlink = 1;
//         n->data.reuse++;
//         n->data.size = 0;
//         for (i=0; i<NUM_DIRECT; i++)
//             n->data.direct[i] = 0;
//         n->data.indirect = 0;
        
//     }
//     free_inode[inum] = 1;
//     return i;
// }
int init() {
	TracePrintf(0, "Enter the init pocess...\n");
	initCache();
	void *buf = malloc(SECTORSIZE);
	// ReadSector(1, buf);
	struct fs_header *header= (struct fs_header *)getInode(0);
	block_num = header->num_blocks;
	inode_num = header->num_inodes;
	int i = 1;
	/*
	 * Use an array to store free block, 0 means free, 1 means occupied
	 */ 
	// free_block = (int *)malloc(sizeof(int) * block_num);
	// free_inode = (int *)malloc(sizeof(int) * inode_num);
	free_block[block_num];
	free_inode[inode_num];
	free_block[0] = 1;
	int inodeblock_num = inode_num / (BLOCKSIZE/INODESIZE) + 1; //number of blocks which stores inode
	for (; i < block_num; i ++) {
		if (i <= inodeblock_num) {
			free_block[i] = 1;
		} else {
			free_block[i] = 0; 
			free_block_count++;
		} 
	}
	TracePrintf(0,"Initialize the free inode list, inode_num is %d\n", inode_num);
	
	 // * Use the array to store free inode, 0 means free, 1 means occupied
	 
	free_inode[0] = 1;
	for (i = 1; i < inode_num; i ++) {
		struct inode *node = getInode(i);
		if (node->type == INODE_FREE) {
			TracePrintf(0, "This node doesn't contain data\n");
			free_inode[i] = 0;
			free_inode_count ++;
		} else {
			TracePrintf(0, "This node contains data\n");
			/*
			 * Loop the direct and indirect array
			 */
			 int j = 0;
			for (; j < NUM_DIRECT; j ++) {
				if (node->direct[j] == 0) break; 
				free_block[j] = 1;
				free_block_count --;
			 }
			TracePrintf(0, "Read indirect block %d \n", node->indirect);
			if (node->indirect) {
				buf = getBlock(node->indirect);
				for(j = 0; j < sizeof(buf) / 4; j ++) {
					free_block[j + node->indirect] = 1; 
					free_block_count --;
				}
			}
		}
	}
	TracePrintf(0, "Finish the init process\n");
}

// // init function by xq
// int init() {
// 	TracePrintf(0, "Enter the init pocess...\n");
// 	// initCache();

// 	// read the inode header, the num_blocks is the total num, num_inodes except the header
// 	void *buf = malloc(SECTORSIZE);
// 	struct fs_header *header= (struct fs_header *)getInode(0);
// 	block_num = header->num_blocks;
// 	inode_num = header->num_inodes;
// 	TracePrintf(0, "init: the num of block is%d, the num of inodes is %d\n", block_num, inode_num);
	
// 	// update the free_block and free inode
// 	free_block[block_num];
// 	free_inode[inode_num+1];
// 	free_block[0] = 1;
// 	free_inode[0] = 1;
// 	int inodeblock_num = inode_num / (BLOCKSIZE/INODESIZE) + 1; //number of blocks which stores inode
// 	int i = 1;
// 	// init the free block and free_inode
// 	for (; i < block_num; i ++) {
// 		if (i <= inodeblock_num) {
// 			free_block[i] = 1;
// 		} else {
// 			free_block[i] = 0; 
// 			free_block_count++;
// 		} 
// 	}
// 	for (i = 1; i <= inode_num; i ++) {
// 		struct inode *node = getInode(i);
// 		if (node->type == INODE_FREE) {
// 			free_inode[i] = 0;
// 			free_inode_count++;
// 		} else {
// 			free_inode[i] = 1;
// 			int j = 0;
// 			for (; j < NUM_DIRECT; j++) {
// 				if (node->direct[j] > 0) {
// 					free_block[node->direct[j]] = 1;
// 					free_block_count--;
// 				}
// 			 }
// 			// TracePrintf(0, "the SECTORSIZE is %d", SECTORSIZE);
// 			if (node->indirect>0) {
// 				int *blockbuf = (int*)getBlock(node->indirect);
// 				for(j = 0; j < SECTORSIZE / 4; j ++) {
// 					if (blockbuf[j] > 0) {
// 						free_block[blockbuf[j]] = 1;
// 						free_block_count--;
// 					}
// 				}
// 			}
// 		}
// 	}
// 	TracePrintf(0, "Finish the init process\n");
// }

// the name of file is DIRNAMELEN
// return the inode number of file with given filename
int path_file(char *pathname, int dir_inum) {
	// length of pathname
	int len_name = strlen(pathname);
	if (len_name == 0) return dir_inum;
	TracePrintf(0, "path_file start, len_name is %d, pathname is %s\n", len_name, pathname);

	// use helper to find the inum of the file
	// int curr_index = 0;
	// int curr_inum = dir_inum;
	// TracePrintf(0,"path_file, the curr_inum and dir_inum is %d\n", curr_inum);
	int inum = path_file_helper(pathname, len_name, 0, dir_inum, 0);
	TracePrintf(0, "path_file ends. the return inum for the pathname is %d\n", inum);
	return inum;
}



int path_file_helper(char *pathname,int len_name, int curr_index, int curr_inum, int num_slink) {
	// check if it is absolute path
	TracePrintf(0, "path_file_helper: the curr_index is %d\n", curr_index);
	if (curr_index == 0 && pathname[0] == '/') {
		TracePrintf(0, "path_file_helper: it is a absolute path.\n");
		curr_inum = 1;
		return path_file_helper(pathname, len_name, ++curr_index, curr_inum, num_slink);
	}

	// delete the dup dash
	while (curr_index < len_name && pathname[curr_index] == '/') curr_index++;
	TracePrintf(0, "path_file_helper: after delete, the index is %d\n:",curr_index);
	
	// check if it is the last index.
	if (curr_index == len_name) {
		TracePrintf(0, "path_file_helper: the last letter is dash, return\n");
		return curr_inum;
	}
	
	// buff the my_componentnent
	int i = 0;
	char name[DIRNAMELEN];
    memset(name,'\0',DIRNAMELEN);
	while (curr_index < len_name && pathname[curr_index] != '/') {
		name[i++] = pathname[curr_index++];
	}
	TracePrintf(0, "path_file_helper: the name is %s\n", name);
	
	// find the inode of the curr file name
	TracePrintf(0, "path_file_helper: print currnet inum before entry: %d\n", curr_inum);
	curr_inum = entry_query(name, curr_inum);
	if (curr_inum == ERROR) {
		return ERROR;
	}
	return path_file_helper(pathname, len_name, curr_index, curr_inum, num_slink);
}

/*
* Get the parent inum and component name of given pathname.
* Return error if parent is not a dir 
*/
// struct my_comp *get_parent(char *pathname, int curr_inum) {
// 	TracePrintf(0, "get_parent starts...the pathname is %s, the curr_inum is %d\n", pathname, curr_inum);
// 	// get the parent name and my_component name
// 	char name[DIRNAMELEN];
// 	memset(name, '\0', DIRNAMELEN);
// 	int len = strlen(pathname)-1;
// 	while(len >= 0 && pathname[len] != '/') {
// 		len--;
// 	}
// 	char parentname[len+2];
// 	memcpy(parentname, pathname, len+1);
// 	parentname[len+1] = '\0';
// 	memcpy(name, pathname+len+1, (strlen(pathname))-len-1);
// 	TracePrintf(0,"get_parent: the parentname is %s\n", parentname);
// 	TracePrintf(0, "get_parent: the component name is %s\n", name);
// 	// get the parent inum
// 	int par_inum = path_file(parentname, curr_inum);

// 	// if parent inum do not exist or parent is not a dir, return error
// 	if (par_inum == ERROR || getInode(par_inum)->type != INODE_DIRECTORY) {
// 		return ERROR;
// 	}

// 	// store the par_inum and copomnet name into the return type.
// 	struct my_comp *tmp = (struct my_comp*)malloc(sizeof(struct my_comp));
// 	tmp->par_inum = par_inum;
// 	strcpy(tmp->name,name);
// 	TracePrintf(0, "get_parent ends. the parent inum is %d, the component name is %s\n", tmp->par_inum, tmp->name);
// 	return tmp;
// }

/*find the inum of given filename in the directory
* return error if the path is not valid or there is no such entry 
*/
int entry_query(char *filename, int dir_inum) {
	TracePrintf(0, "entry_query starts... the filename is %s, the curr directory inum is: %d\n", filename, dir_inum);
	
	// struct inode *dir_inode = (struct inode*)malloc(sizeof(struct inode));
	struct inode *dir_inode = getInode(dir_inum); 
	
	// return ERROR if this is not a directory
	if (dir_inode->type != INODE_DIRECTORY) {
		TracePrintf(0, "entry_query: cur path is not a dir\n");
		return ERROR;
	}

	// the entry number in this dir file
	int entry_num = (dir_inode->size) / sizeof(struct dir_entry);
	int entry_count = 0;
	TracePrintf(0, "entry_query: the type of curr file is %d\n", dir_inode->type);
	TracePrintf(0, "entry_query: the entry_num is %d\n", entry_num);

	// loop over the direct block array
	TracePrintf(0, "entry_query: the entried is this dir is :\n");
	int i;
	for (i = 0; i < NUM_DIRECT; i++) {
		int block_num = dir_inode->direct[i];
		struct dir_entry *block_buf = (struct dir_entry*)getBlock(block_num);

		int j;
		for (j = 0; j < SECTORSIZE/sizeof(struct dir_entry); j++) {
			entry_count++;
			if (entry_count > entry_num) return ERROR;
			TracePrintf(0, "entry_query: the name in the dir is %s\n",block_buf[j].name);
			TracePrintf(0, "entry_query: the inum for this entry is %d\n",block_buf[j].inum);
			if (strcmp(block_buf[j].name, filename) == 0) {
				TracePrintf(0, "entry_query: find the file inum%d", block_buf[j].inum);
				return block_buf[j].inum;
			}
			
		}
	}

	TracePrintf(0, "entry_query: not found in the dir block, loop the indirect:");
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
			    if (entry_count > entry_num) return ERROR;
				if (strcmp(block_buf2[j].name, filename) == 0) {
					return block_buf2[j].inum;
				}
			}
		}
		
	}
	TracePrintf(0,"entry_query ends. No entry found.");
	return ERROR;
}

/*
search an entry
*/
struct dir_entry *search_entry(char *filename, int dir_inum) {
	TracePrintf(0, "search_entry starts... the filename is %s, the curr directory inum is: %d\n", filename, dir_inum);
	
	// struct inode *dir_inode = (struct inode*)malloc(sizeof(struct inode));
	struct inode *dir_inode = getInode(dir_inum); 
	TracePrintf(0, "entry_query: the type of curr file is %d\n", dir_inode->type);

	// return ERROR if this is not a directory
	if (dir_inode->type != INODE_DIRECTORY) {
		return ERROR;
	}

	// the entry number in this dir file
	int entry_num = (dir_inode->size) / sizeof(struct dir_entry);
	int entry_count = 0;
	TracePrintf(0, "the entry_num is %d\n", entry_num);

	// loop over the direct block array
	int i;
	for (i = 0; i < NUM_DIRECT; i++) {
		int block_num = dir_inode->direct[i];
		struct dir_entry *block_buf = (struct dir_entry*)getBlock(block_num);

		int j;
		for (j = 0; j < SECTORSIZE/sizeof(struct dir_entry); j++) {
			entry_count++;
			if (entry_count > entry_num) return ERROR;
			TracePrintf(0, "the name in the dir%s\n",block_buf[j].name);
			TracePrintf(0, "the inum for this entry is %d\n",block_buf[j].inum);
			if (strcmp(block_buf[j].name, filename) == 0) {
				TracePrintf(0, "find the file inum%d", block_buf[j].inum);
				return &block_buf[j];
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
			    if (entry_count > entry_num) return ERROR;
				if (strcmp(block_buf2[j].name, filename) == 0) {
					return &block_buf2[j];
				}
			}
		}
		
	}
	return ERROR;
}

/*
add an entry
*/
// int add_entry(char *filename, int new_inum, int dir_inum) {
// 	TracePrintf(0, "add_entry starts..., the name is %s, the inum for entry is %d, the curr_inum is %d\n", filename, new_inum, dir_inum);
// 	// struct inode *dir_inode = (struct inode*)malloc(sizeof(struct inode));
// 	struct inode *dir_inode = getInode(dir_inum); 
// 	// return ERROR if this is not a directory
// 	if (dir_inode->type != INODE_DIRECTORY) {
// 		TracePrintf(0,"error: the filename is not valid.");
// 		return ERROR;
// 	}

// 	// the entry number in this dir file
// 	int entry_num = (dir_inode->size) / sizeof(struct dir_entry);
// 	int entry_count = 0;
// 	TracePrintf(0, "add_entry: the size of this dir:%d, the size of dir_entry is %d\n", dir_inode->size, sizeof(struct dir_entry));

// 	// loop over the direct block array
// 	int i;
// 	for (i = 0; i < NUM_DIRECT; i++) {
// 		int block_num = dir_inode->direct[i];
// 		struct dir_entry *block_buf = (struct dir_entry*)getBlock(block_num);
// 		int j;
// 		for (j = 0; j < SECTORSIZE/sizeof(struct dir_entry); j++) {
// 			// if the entry is empty, add new entry here
// 			if (block_buf[j].inum == 0) {
// 				TracePrintf(0,"add_entry: find a free entry%d\n", j);
// 				block_buf[j].inum = new_inum;
// 				memcpy(block_buf[j].name, filename, MAXPATHNAMELEN);
// 				TracePrintf(0, "add_entry: the name copied is %s\n", block_buf[j].name);
// 				dir_inode->size = dir_inode->size + sizeof(struct dir_entry);
// 				TracePrintf(0, "add_entry ends. the updated size is %d\n", dir_inode->size);
// 				return new_inum;
// 			}
// 			TracePrintf(0, "add_entry: the inum of %dth entry is %d, the name is %s\n",j,block_buf[j].inum, block_buf[j].name);
// 		}
// 	}

// 	TracePrintf(0, "add_entry: not found in the dir block, loop the indirect:");
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
// 					block_buf2[j].inum = new_inum;
// 					memcpy(block_buf2[j].name, filename, MAXPATHNAMELEN);
// 					dir_inode->size = dir_inode->size + sizeof(struct dir_entry);
// 					TracePrintf(0, "add_entry ends. the updated size is %d\n", dir_inode->size);
// 					return new_inum;
// 				}
				
// 			}
// 		}
		
// 	}
// 	// no tmpty entry available in the dir
// 	return ERROR;
// }

/*
remove an entry 
*/
// int rm_entry(char *filename, int inum, int dir_inum) {
// 	TracePrintf(0, "rm_entry starts..., the name is %s, the inum for entry is %d, the curr_inum is %d\n", filename, new_inum, dir_inum);
// 	// struct inode *dir_inode = (struct inode*)malloc(sizeof(struct inode));
// 	struct inode *dir_inode = getInode(dir_inum); 
// 	// return ERROR if this is not a directory
// 	if (dir_inode->type != INODE_DIRECTORY) {
// 		TracePrintf(0,"error: the filename is not valid.");
// 		return ERROR;
// 	}

// 	// the entry number in this dir file
// 	int entry_num = (dir_inode->size) / sizeof(struct dir_entry);
// 	int entry_count = 0;
// 	TracePrintf(0, "add_entry: the size of this dir:%d, the size of dir_entry is %d\n", dir_inode->size, sizeof(struct dir_entry));

// 	// loop over the direct block array
// 	int i;
// 	for (i = 0; i < NUM_DIRECT; i++) {
// 		int block_num = dir_inode->direct[i];
// 		struct dir_entry *block_buf = (struct dir_entry*)getBlock(block_num);
// 		int j;
// 		for (j = 0; j < SECTORSIZE/sizeof(struct dir_entry); j++) {
// 			// if the entry is empty, add new entry here
// 			if (block_buf[j].inum == 0) {
// 				TracePrintf(0,"add_entry: find a free entry%d\n", j);
// 				block_buf[j].inum = new_inum;
// 				memcpy(block_buf[j].name, filename, MAXPATHNAMELEN);
// 				TracePrintf(0, "add_entry: the name copied is %s\n", block_buf[j].name);
// 				dir_inode->size = dir_inode->size + sizeof(struct dir_entry);
// 				TracePrintf(0, "add_entry ends. the updated size is %d\n", dir_inode->size);
// 				return new_inum;
// 			}
// 			TracePrintf(0, "add_entry: the inum of %dth entry is %d, the name is %s\n",j,block_buf[j].inum, block_buf[j].name);
// 		}
// 	}

// 	TracePrintf(0, "add_entry: not found in the dir block, loop the indirect:");
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
// 					block_buf2[j].inum = new_inum;
// 					memcpy(block_buf2[j].name, filename, MAXPATHNAMELEN);
// 					dir_inode->size = dir_inode->size + sizeof(struct dir_entry);
// 					TracePrintf(0, "add_entry ends. the updated size is %d\n", dir_inode->size);
// 					return new_inum;
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
    if (open_inum == ERROR) {
        msg->type = ERROR;
    }
    else {

    }
    return open_inum;
}

// close the file with given fd
int close_handler(int fd) {

}

/* 
 * create and open the new file
 * return the new file's inum
*/ 
// int create_handler(my_msg *msg, int sender_pid){
// 	TracePrintf(0, "create hander start...\n");
	
// 	// get the pathname
// 	char pathname[MAXPATHNAMELEN];
// 	if (CopyFrom(sender_pid, pathname, msg->addr1, msg->data1+1) == ERROR) {
// 		return ERROR;
// 	}
// 	TracePrintf(0,"create_handler: the pathname is %s\n", pathname);
// 	// get the cur inum
// 	int curr_inum;
// 	if (CopyFrom(sender_pid, &curr_inum, &(msg->data2), sizeof(int))==ERROR) {
// 		return ERROR;
// 	}
// 	TracePrintf(0,"create_handler: the curr_inum is %d\n", curr_inum);
	
// 	// get_parent(pathname, curr_inum);
// 	struct my_comp *buff;
// 	buff = get_parent(pathname, curr_inum);
// 	TracePrintf(0, "create_handler: the par_inum is %d, the component name is %s\n", buff->par_inum, buff->name);

// 	// add entry
// 	int inum = entry_query(buff->name, buff->par_inum);
// 	if (inum != ERROR) {
// 		TracePrintf(0, "create_handler: the file exists, set it as new\n");
// 		struct inode *node = getInode(inum);
// 		node->size = 0;
// 		// set the return msg data1 as the inum for the filename
// 		msg->data1 = inum;
// 	} else {
// 		TracePrintf(0, "create_handler: the file do not exists, add entry.\n");
// 		inum = add_entry(buff->name, get_freeInode(), buff->par_inum);
// 		if (inum != ERROR) {
// 			msg->data1 = inum;
// 		}
// 	}
// 	TracePrintf(0, "create_handler ends. the new inum is %d\n", inum);
// 	return inum;
// }

// This request reads data from an open file, beginning at the current position in the file as represented 
// by the given file descriptor fd
int read_handler(int fd, void *buf, int size) {

}

int write_handler(int fd, void *buf, int size) {

}

// /* create a head link from new file name to the existing oldname
//  * return error if the newname already exists
//  * return 0 if success
// */
// int link_handler(my_msg *msg, int sender_pid) {
// 	// read the newname form msg
// 	char *newname;
// 	if (CopyFrom(sender_pid, newname, msg->addr1, msg->data1+1) == ERROR) {
// 		return ERROR;
// 	}
// 	TracePrintf("the new name is %s:\n", newname);

// 	// read oldname and curr_inum from msg
// 	char *oldname;
// 	int curr_inum = 1;
// 	if (CopyFrom(sender_pid, oldname, msg->addr2, msg->data2+1) == ERROR) {
// 		return ERROR;
// 	}
// 	if (CopyFrom(sender_pid, &curr_inum, &(msg->data3), sizeof(int)) == ERROR) {
// 		return ERROR;
// 	}
// 	TracePrintf("the oldname is %s\n", oldname);
// 	TracePrintf("the curr_inum is %d\n", curr_inum);

// 	// firstly find the parent inum of newname
// 	int len = strlen(newname);
// 	while(len>=0 && newname[len] != '/') len--;
// 	char par_name[len+1];
// 	memcpy(par_name, newname, len);
// 	TracePrintf(0, "the parent name is %s\n", par_name);
// 	int par_inum = path_file(par_name, curr_inum);
// 	if (par_inum == ERROR) return ERROR;
// 	TracePrintf(0, "the par_inum is %d\n", par_inum);

// 	// find the my_componentnetn of new name
// 	char new_my_componentnent[MAXPATHNAMELEN];
// 	memset(new_my_componentnent,'\0',MAXPATHNAMELEN);
// 	memcpy(new_my_componentnent, newname+len, strlen(newname)-len);

// 	// if the newname already exists, return ERROR 
// 	if (entry_query(new_my_componentnent, par_inum) != ERROR) return ERROR;

// 	// find the inum for the oldname, if it is a dir, return error
// 	int old_inum = entry_query(oldname, curr_inum);
// 	if (getInode(old_inum)->type == INODE_DIRECTORY) return ERROR;
	
// 	// add the new entry to the parent dir
// 	if (add_entry(new_my_componentnent, old_inum, curr_inum) == ERROR) 
// 		return ERROR;
// }

// /*
//  *remove the directory enty for the pathname, if it is the last link, clean the inode for this file
//  *the filename should not be a directory
//  */
// int unlink_handler(my_msg *msg, int sender_pid) {
// 	// read the path name and curr_dir from msg
// 	char *pathname;
// 	int curr_inum;
// 	if (CopyFrom(sender_pid, pathname, msg->addr1, msg->data1+1) == ERROR) {
// 		return ERROR;
// 	}
// 	if (CopyFrom(sender_pid, &curr_inum, msg->addr2, sizeof(int)) == ERROR) {
// 		return ERROR;
// 	}

// 	// find the parent inum and component name
// 	my_component *buf = get_parent(pathname, curr_inum);
// 	int par_inum = buf->par_inum;
// 	char name[MAXPATHNAMELEN] = buf->name;

// 	// find the entry address for the component
// 	(struct dir_entry) *entry = search_entry(name, par_inum);

// 	// get the inum for this filename
// 	struct inode *node = getInode(entry->inum);

// 	// if it is a dir, return error
// 	if (getInode(entry->inum)->type == INODE_DIRECTORY) return ERROR;

// 	// set inum as 0, if the nlink is 0, free the inode for this file
// 	if (node->nlink == 1) {
// 		free_inode(inum);
// 	}
// 	entry->inum = 0;
// 	return 0;
// }

// /*
// * Make a dir. There are two default dir_entry, . and ..
// * return error if pathname already exist
// */
// int mkdir_handler(my_msg *msg, int sender_pid) {
// 	TracePrintf(0, "mkdir_handler start...\n");
	
// 	// get the pathname
// 	char pathname[MAXPATHNAMELEN];
// 	if (CopyFrom(sender_pid, pathname, msg->addr1, msg->data1+1) == ERROR) {
// 		return ERROR;
// 	}
// 	TracePrintf(0,"mkdir_handler: the pathname is %s\n", pathname);
// 	// get the cur inum
// 	int curr_inum;
// 	if (CopyFrom(sender_pid, &curr_inum, &(msg->data2), sizeof(int))==ERROR) {
// 		return ERROR;
// 	}
// 	TracePrintf(0,"mkdir_handler: the curr_inum is %d\n", curr_inum);
	
// 	// get_parent(pathname, curr_inum);
// 	struct my_comp *buff;
// 	buff = get_parent(pathname, curr_inum);
// 	TracePrintf(0, "mkdir_handler: the par_inum is %d, the component name is %s\n", buff->par_inum, buff->name);

// 	// check if the parent pathname is valid and is a directory
// 	if (buff->par_inum == ERROR) return ERROR;
	
// 	// check if the name has alreaday exists, if yes, reutrn error, if no, add the entry and alloc the inode
// 	int inum = entry_query(buff->name, buff->par_inum);
// 	if (inum != ERROR) {
// 		TracePrintf(0, "mkdir_handler: the pathname exists, return error\n");
// 		return ERROR;
// 	} else {
// 		TracePrintf(0, "mkdir_handler: allocate a new inode and add entry into its parent file.\n");
// 		inum = add_entry(buff->name, allocInode(INODE_DIRECTORY), buff->par_inum);
// 		if (inum != ERROR) {
// 			msg->data1 = inum;
// 		}
// 	}
// 	TracePrintf(0, "mkdir_handler ends. allocate a %d inode to the new dir.", inum);
// 	return inum;
// }

// /*
// * remove a dir. There are two default dir_entry, . and ..
// * return error if pathname already exist
// */
// int rmdir_handler(my_msg *msg, int sender_pid) {
// 	TracePrintf(0, "rmdir_handler start...\n");
	
// 	// get the pathname
// 	char pathname[MAXPATHNAMELEN];
// 	if (CopyFrom(sender_pid, pathname, msg->addr1, msg->data1+1) == ERROR) {
// 		return ERROR;
// 	}
// 	TracePrintf(0,"mkdir_handler: the pathname is %s\n", pathname);
// 	// get the cur inum
// 	int curr_inum;
// 	if (CopyFrom(sender_pid, &curr_inum, &(msg->data2), sizeof(int))==ERROR) {
// 		return ERROR;
// 	}
// 	TracePrintf(0,"mkdir_handler: the curr_inum is %d\n", curr_inum);
	
// 	// get_parent(pathname, curr_inum);
// 	struct my_comp *buff;
// 	buff = get_parent(pathname, curr_inum);
// 	TracePrintf(0, "mkdir_handler: the par_inum is %d, the component name is %s\n", buff->par_inum, buff->name);

// 	// check if the parent pathname is valid and is a directory
// 	if (buff->par_inum == ERROR) return ERROR;
	
// 	// check if the name has alreaday exists, if yes, reutrn error, if no, add the entry and alloc the inode
// 	int inum = entry_query(buff->name, buff->par_inum);
// 	if (inum == ERROR) {
// 		TracePrintf(0, "mkdir_handler: the pathname do not exist, return error\n");
// 		return ERROR;
// 	} else {
// 		TracePrintf(0, "mkdir_handler: allocate a new inode and add entry into its parent file.\n");
// 		inum = add_entry(buff->name, allocInode(INODE_DIRECTORY), buff->par_inum);
// 		if (inum != ERROR) {
// 			msg->data1 = inum;
// 		}
// 	}
// 	TracePrintf(0, "mkdir_handler ends. allocate a %d inode to the new dir.", inum);
// 	return inum;
// }


int main(int argc, char **argv) {
	TracePrintf(0, "Running the server process\n");
	int pid;
	struct my_msg msg;
    int sender_pid;
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
	// char *name = "an";
	// struct inode *node = get_inode(1);
	// TracePrintf(0, "try the hash%d:\n",node->type);

    while (1) {
    	sender_pid=Receive(&msg);
    	fprintf((stderr), "sender pid is %d\n", sender_pid );
        if (sender_pid==ERROR) {
            perror("error receiving message!\n");
            continue;
        }
        // TracePrintf(0, "the size of msg:%d \n", sizeof(msg));
        switch (msg.type) {
            case OPEN:
                 open_handler(&msg,sender_pid);break;
            // case CREATE:
            // 	TracePrintf(0, "receive a create msg.\n");
            //      create_handler(&msg,sender_pid);break;
            // case READ:
            //     read_handler(&msg,sender_pid);break;
            // case WRITE:
            //     write_handler(&msg,sender_pid);break;
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
        if (Reply(&msg,sender_pid)==ERROR) fprintf(stderr, "Error replying to pid %d\n",sender_pid);
    }
//         
//     }
	return 0;
} 









