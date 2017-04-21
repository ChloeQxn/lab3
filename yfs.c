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
#include "yfs.h"
#include "cache.h"
// #include "cacheLRU.h"

// the num of total blocks and inodes
int block_num;
int inode_num;

// a list store the status of inode and blocks, 0 means free, 1 means dirty
char *free_inode;
int free_inode_count = 0;
char *free_block;
int free_block_count = 0;

// struct to get the parent inum for files
struct my_comp {
	int par_inum;
	char name[DIRNAMELEN];
};

// hashtable for the inodes
struct my_struct *inodeHash; 
struct my_struct2 *blockHash;
// inode list for the inodes cache
struct Nnode *Nhead;
struct Nnode *Ntail;
int Nsize; 
struct Bnode *Bhead;
struct Bnode *Btail;
int Bsize;
// init cache
void initCache() {
	TracePrintf(0,"enter the init cache\n");
	inodeHash = NULL;
	Nhead = NULL;
	Ntail = NULL;
	blockHash = NULL;
	Bhead = NULL;
	Btail = NULL;
	TracePrintf(0, "leave the init cache\n");
}
// get inode from disk
struct inode *get_inode_disk(int i) {
	// TracePrintf(0, "get inode from disk");
	int blockIndex = 1 + (i) / (BLOCKSIZE/INODESIZE);
	void *buf = malloc(SECTORSIZE);
	ReadSector(blockIndex, buf);
	struct inode* node = (struct inode*)malloc(sizeof(struct inode));
	int offset = i % (BLOCKSIZE/INODESIZE);
	memcpy(node, (struct inode*)buf + offset, sizeof(struct inode));
	free(buf);
	// TracePrintf(0, "get inode from disk ends");
	return node;
}

// // get the buff of the block with given block number
void *get_block_disk(int blockIndex) {
	void *buf = malloc(SECTORSIZE);
	ReadSector(blockIndex, buf);
	return buf;
}

// hash interface
struct my_struct *find_inode(int ikey) {  
    struct my_struct *s;  
HASH_FIND_INT(inodeHash, &ikey, s );  
return s;  
} 

struct my_struct2 *find_block(int ikey) {  
    struct my_struct2 *s;  
HASH_FIND_INT(blockHash, &ikey, s );  
return s;  
}  

// add inode into hashtable
void add_inode(int ikey, struct Nnode* node) {  
    struct my_struct *s;  
    HASH_FIND_INT(inodeHash, &ikey, s);  /* 插入前先查看key值是否已经在hash表inodeHash里面了 */  
    if (s==NULL) {  
      s = (struct my_struct*)malloc(sizeof(struct my_struct));  
      s->ikey = ikey; 
      s->node = node; 
      HASH_ADD_INT(inodeHash, ikey, s );  /* 这里必须明确告诉插入函数，自己定义的hash结构体中键变量的名字 */  
    }  
    // strcpy(s-> value, value_buf);  
} 

// add inode into hashtable
void add_block(int ikey, struct Bnode* node) {  
    struct my_struct2 *s;  
    HASH_FIND_INT(blockHash, &ikey, s);  /* 插入前先查看key值是否已经在hash表inodeHash里面了 */  
    if (s==NULL) {  
      s = (struct my_struct2*)malloc(sizeof(struct my_struct2));  
      s->ikey = ikey; 
      s->node = node; 
      HASH_ADD_INT(blockHash, ikey, s );  /* 这里必须明确告诉插入函数，自己定义的hash结构体中键变量的名字 */  
    }  
    // strcpy(s-> value, value_buf);  
} 

// delete inode from hashtable
void delete_inode(int ikey) {  
    struct my_struct *s = NULL;  
    HASH_FIND_INT(inodeHash, &ikey, s);  
    if (s!=NULL) {  
      HASH_DEL(inodeHash, s);   
      free(s);    
      }           
} 

// delete inode from hashtable
void delete_block(int ikey) {  
    struct my_struct2 *s = NULL;  
    HASH_FIND_INT(blockHash, &ikey, s);  
    if (s!=NULL) {  
      HASH_DEL(blockHash, s);   
      free(s);    
      }           
} 

// create an inode of linked list
struct Nnode *new_Nnode(struct inode *node, int inum) {
	// TracePrintf(0,  "new_Nnode starts\n");
	struct Nnode* nnode = (struct Nnode*)malloc(sizeof(struct Nnode));
	nnode->node = node;
	nnode->inum = inum;
	nnode->pre = NULL;
	nnode->next = NULL;
	nnode->dirty = '0';
	// TracePrintf(0, "new_Nnode ends\n");
	return nnode;
}

// create an inode of linked list
struct Bnode *new_Bnode(void *buf, int index) {
	// TracePrintf(0,  "new_Nnode starts\n");
	struct Bnode* nnode = (struct Bnode*)malloc(sizeof(struct Bnode));
	nnode->buf = buf;
	nnode->index = index;
	nnode->pre = NULL;
	nnode->next = NULL;
	nnode->dirty = '0';
	// TracePrintf(0, "new_Nnode ends\n");
	return nnode;
}
// add inode to the tail of linked list
void add_inode_tail(struct Nnode *nnode) {
	TracePrintf(0, "add_inode_tail starts\n");
	if (Ntail == NULL) {
		Ntail = nnode;
		Nhead = nnode;
	} else {
		Ntail->next = nnode;
		nnode->pre = Ntail;
		Ntail = nnode;
	}
	Nsize++;
	TracePrintf(0, "add_inode_tail ends\n");
}

// add block node to the tail of linked list
void add_block_tail(struct Bnode *nnode) {
	// TracePrintf(0, "add_inode_tail starts\n");
	if (Btail == NULL) {
		Btail = nnode;
		Bhead = nnode;
	} else {
		Btail->next = nnode;
		nnode->pre = Btail;
		Btail = nnode;
	}
	Bsize++;
	// TracePrintf(0, "add_inode_tail ends\n");
}

/* 
*add a new inode into cache. add it into hash and add it into tail of list
*/
void add_inode_cache(int inum, struct inode* node){
	TracePrintf(0,"add to cache, the nsize is %d\n", Nsize);
	// if cache is full, delete the head from hash and list
	if (Nsize>=INODE_CACHESIZE) {
		TracePrintf(0, "add_inode_cache: cache is full.\n");
		// delete from hash
		delete_inode(Nhead->inum);
		// write inode to the block it belongs to
		write_inode_block(Nhead->inum);
		// update the Nhead
		Nhead = Nhead->next;
		free(Nhead->pre);
		Nhead->pre = NULL;
		Nsize--;
	}
	// create a Nnode
	struct Nnode* nnode = new_Nnode(node, inum);
	TracePrintf(0,"add inode cache :ew a inode %d\n", nnode->inum);
	// add it to hash
	add_inode(inum, nnode);
	// add it to list
	add_inode_tail(nnode);
	TracePrintf(0, "add_inode_cache finish");
}

/* 
*add a new inode into cache. add it into hash and add it into tail of list
*/
void add_block_cache(int index, void *buf){
	TracePrintf(0,"add_block_cache starts, the Bsize is %d\n", Bsize);
	// if cache is full, delete the head from hash and list
	if (Bsize>=BLOCK_CACHESIZE) {
		TracePrintf(0, "add_block_cache: cache is full.\n");
		// delete from hash
		delete_block(Bhead->index);
		// write inode to the block it belongs to
		write_block_disk(Bhead->index);
		// update the Nhead
		Bhead = Bhead->next;
		free(Bhead->pre);
		Bhead->pre = NULL;
		Bsize--;
	}
	// create a Nnode
	struct Bnode* nnode = new_Bnode(buf, index);
	// add it to hash
	add_block(index, nnode);
	// add it to list
	add_block_tail(nnode);
	TracePrintf(0, "add_block_cache finish");
}

/* 
*	update Nnode in the list
*/
void update_Nnode_list(struct Nnode *nnode) {
	TracePrintf(0,"update_Nnode_list start\n");

	if (Ntail == nnode) {
		TracePrintf(0,"it is already the tila");
		return;
	}
	// delete it from list
	if (Nhead == nnode) {
		Nhead = Nhead->next;
		Nhead->pre = NULL;
	} else {
		nnode->pre->next = nnode->next;
		nnode->next->pre = nnode->pre;
	}	
	// put it to the tail
	Ntail->next = nnode;
	nnode->pre = Ntail;
	Ntail = nnode;
	nnode->next = NULL;
	TracePrintf(0, "update_Nnode_list ends\n");
}

void update_Bnode_list(struct Bnode *nnode) {
	TracePrintf(0,"update_Bnode_list start\n");
	if (Btail == nnode) return;
	// delete it from list
	if (Bhead == nnode) {
		Bhead = Bhead->next;
		Bhead->pre = NULL;
	} else {
		nnode->pre->next = nnode->next;
		nnode->next->pre = nnode->pre;
	}
	// put it to the tail
	Btail->next = nnode;
	nnode->pre = Btail;
	Btail = nnode;
	nnode->next = NULL;
	TracePrintf(0, "update_Bnode_list ends\n");
}

void write_Nnode(int inum){
	struct my_struct *tmp = find_inode(inum);
	if (tmp == NULL) return ERROR;
	tmp->node->dirty = '1';
}

void write_Bnode(int index){
	struct my_struct2 *tmp = find_block(index);
	if (tmp == NULL) return ERROR;
	tmp->node->dirty = '1';
}

/*
* write inode into its block(which in block cache) when this inode leave cache
*/
void write_inode_block(int inum) {
	TracePrintf(0, "write_inode_block start...\n");
	// if inode is not dirty, return
	struct Nnode *nnode = find_inode(inum)->node;
	if (nnode->dirty == '0') {
		TracePrintf(0,"write_inode_block: %d inode is not dirty.\n", inum);
		return;
	}
	// find the block in the cache and write into it
	int index = 1 + inum / (SECTORSIZE/INODESIZE);
	int offset = inum % (SECTORSIZE/INODESIZE);
	// find_block(index);
	struct my_struct2 *tmp = find_block(index);
	TracePrintf(0, "index %d\n", index);
	// struct Bnode *bnode = find_block(index)->node;
	if (tmp == NULL) TracePrintf(0, "write_inode_block startnull...\n");
	struct Bnode *bnode = tmp->node;
	TracePrintf(0, "write_inode_block start...\n");
	memcpy(((struct inode*)(bnode->buf))+offset, find_inode(inum)->node, sizeof(struct inode));
	bnode->dirty = '1';
	TracePrintf(0, "write_inode_block ends. block index is %d\n", index);
}

/*
* write block into its disk when this block leave cache
*/
void write_block_disk(int i) {
	TracePrintf(0, "write_block_disk start...\n");
	struct Bnode *bnode = find_block(i)->node;
	if (bnode->dirty == '1') {
		WriteSector(i, bnode->buf);
	}
	TracePrintf(0, "write_block_disk ends\n");
}



// get inode with given inum, if it is in cache, 
void *getBlock(int index) {
	TracePrintf(0, "getBlock starts. the index is %d\n", index);
	struct my_struct2 *tmp = find_block(index);
	void *buf;
	
	// if the inode is not in the cache, read it from the disk
	if (tmp == NULL) {
		TracePrintf(0, "block of %d is not in cache, read it from disk.\n", index);
		buf = get_block_disk(index);
		// TracePrintf(0, "try to put it into cache");
		// put it into cache
		add_block_cache(index, buf);
	} else {
		// it already in cache, get it from hash, and update its location in the list
		TracePrintf(0, "block of %d is already in cache.\n", index);
		buf = tmp->node->buf;
		// TracePrintf(0, "getInode: get the node in the hash");
		update_Bnode_list(tmp->node);
	}
	TracePrintf(0, "getBlock ends.\n");
	return buf;
}

struct inode *get_inode_block(int i) {
	// TracePrintf(0, "get inode from disk");
	int blockIndex = 1 + (i) / (BLOCKSIZE/INODESIZE);
	void *buf = getBlock(blockIndex);
	struct inode* node = (struct inode*)malloc(sizeof(struct inode));
	int offset = i % (BLOCKSIZE/INODESIZE);
	memcpy(node, (struct inode*)buf + offset, sizeof(struct inode));
	free(buf);
	// TracePrintf(0, "get inode from disk ends");
	return node;
}

// get inode with given inum, if it is in cache, 
struct inode *getInode(int inum) {
	TracePrintf(0, "getInode starts. the inum is %d\n", inum);
	struct my_struct *tmp = find_inode(inum);
	struct inode *node;
	
	// if the inode is not in the cache, read it from the disk
	if (tmp == NULL) {
		TracePrintf(0, "inode of %d is not in cache, read it from cached block.\n", inum);
		node = get_inode_block(inum);
		// TracePrintf(0, "try to put it into cache");
		// put it into cache
		add_inode_cache(inum, node);
	} else {
		// it already in cache, get it from hash, and update its location in the list
		TracePrintf(0, "inode of %d is already in cache.\n", inum);
		node = tmp->node->node;
		// TracePrintf(0, "getInode: get the node in the hash");
		update_Nnode_list(tmp->node);
	}
	TracePrintf(0, "getInode ends.\n");
	return node;
}
///////////////////////////////////////////////////////////////////////////



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
	int i = 1;
	for (; i <= inode_num; i++) {
		if (free_inode[i] == '0') {
			break;
		}
	}
	free_inode[i] = '1';
	free_inode_count--;
	TracePrintf(0, "get free ionde ends. get a free inode %d\n", i);
	struct inode *node = getInode(i);
	return i;
}

int get_freeBlock() {
	TracePrintf(0, "get_freeBlock starts...\n");
	if (free_block_count == 0) return ERROR;
	int i = 1;
	for(; i <= block_num;i++) {
		if (free_block[i] == '0') {
			break;
		}
	}
	free_block_count--;
	free_block[i] = '1';
	TracePrintf(0, "get free block ends. get a free block %d\n", i);
	return i;
}

void freeInode(int i) {
	TracePrintf(0, "freeInode starts...");
	free_inode[i] = '0';
	struct inode *node = getInode(i);
	node->type = INODE_FREE;
	node->nlink = 0;
	node->size = 0;
	int j = 0;
	for (;j < NUM_DIRECT;j++) {
		node->direct[j] = 0;
	}
	node->indirect = 0;
	free_inode_count++;
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

// // init function by xq
int init() {
	TracePrintf(0, "Enter the init pocess...\n");
	initCache();
	TracePrintf(0, "init cache");
	// read the inode header, the num_blocks is the total num, num_inodes except the header
	void *buf = malloc(SECTORSIZE);
	struct fs_header *header= (struct fs_header *)get_inode_disk(0);
	block_num = header->num_blocks;
	inode_num = header->num_inodes;
	TracePrintf(0, "init: the num of block is%d, the num of inodes is %d\n", block_num, inode_num);
	
	// update the free_block and free inode
	free_block = malloc(block_num * sizeof(char));
	free_inode = malloc((inode_num+1) * sizeof(char));
	free_block[0] = '1';
	free_inode[0] = '1';
	int inodeblock_num = inode_num / (BLOCKSIZE/INODESIZE) + 1; //number of blocks which stores inode
	int i = 1;
	// init the free block and free_inode
	for (; i < block_num; i ++) {
		if (i <= inodeblock_num) {
			free_block[i] = '1';
		} else {
			free_block[i] = '0'; 
			free_block_count++;
		} 
	}
	for (i = 1; i <= inode_num; i ++) {
		struct inode *node = get_inode_disk(i);
		if (node->type == INODE_FREE) {
			free_inode[i] = '0';
			free_inode_count++;
		} else {
			free_inode[i] = '1';
			int j = 0;
			for (; j < NUM_DIRECT; j++) {
				if (node->direct[j] > 0) {
					free_block[node->direct[j]] = '1';
					free_block_count--;
				}
			 }
			// TracePrintf(0, "the SECTORSIZE is %d", SECTORSIZE);
			if (node->indirect>0) {
				int *blockbuf = (int*)get_block_disk(node->indirect);
				for(j = 0; j < SECTORSIZE / 4; j ++) {
					if (blockbuf[j] > 0) {
						free_block[blockbuf[j]] = '1';
						free_block_count--;
					}
				}
			}
		}
		free(node);
	}
	TracePrintf(0, "Finish the init process\n");
	// TracePrintf(0, "%s\n", free_inode);
	// TracePrintf(0, "%s", free_block);
}

// the name of file is DIRNAMELEN
// return the inode number of file with given filename, return -1 if fail
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
	if (curr_inum == -1) {
		return -1;
	}

	// deal with symlink name
	if (getInode(curr_inum)->type == INODE_SYMLINK) {
		// check the num of slink
		num_slink++;
		if (num_slink > MAXSYMLINKS)
			return -1;

		// update the pathname
		struct inode *node = getInode(curr_inum);
		int size = node->size;
		char symname[size + len_name+1];
		memset(symname, '\0', size + len_name+1);
		memcpy(symname, getBlock(node->direct[0]), size);
		TracePrintf(0, "path_file_helper: print the as is name of symlink %s\n", symname);
		memcpy(symname+size, pathname+curr_index, len_name-curr_index);
		TracePrintf(0, "path_file_helper: print the pathname after update\n");
		return path_file_helper(pathname, strlen(pathname), 0, curr_inum, num_slink);
	}
	return path_file_helper(pathname, len_name, curr_index, curr_inum, num_slink);
}

/*
* Get the parent inum and component name of given pathname.
* Return error if parent is not a dir 
*/
struct my_comp *get_parent(char *pathname, int curr_inum) {
	TracePrintf(0, "get_parent starts...the pathname is %s, the curr_inum is %d\n", pathname, curr_inum);
	// get the parent name and my_component name
	char name[DIRNAMELEN];
	memset(name, '\0', DIRNAMELEN);
	int len = strlen(pathname)-1;
	while(len >= 0 && pathname[len] != '/') {
		len--;
	}
	char parentname[len+2];
	memcpy(parentname, pathname, len+1);
	parentname[len+1] = '\0';
	memcpy(name, pathname+len+1, (strlen(pathname))-len-1);
	TracePrintf(0,"get_parent: the parentname is %s\n", parentname);
	TracePrintf(0, "get_parent: the component name is %s\n", name);

	// store the par_inum and copomnet name into the return type.
	struct my_comp *tmp = (struct my_comp*)malloc(sizeof(struct my_comp));


	// get the parent inum
	int par_inum = path_file(parentname, curr_inum);

	
	// if parent inum do not exist or parent is not a dir, return error
	if (par_inum <= 0 || getInode(par_inum)->type != INODE_DIRECTORY) {
		TracePrintf(0,"get_parent: the paretn is not a dir.\n");
		tmp->par_inum = -1;
		return tmp;
	}
	tmp->par_inum = par_inum;
	strcpy(tmp->name,name);
	TracePrintf(0, "get_parent ends. the parent inum is %d, the component name is %s\n", tmp->par_inum, tmp->name);
	return tmp;
}

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
		return -1;
	}

	// the entry number in this dir file
	int entry_num = (dir_inode->size) / sizeof(struct dir_entry);
	int entry_count = 0;
	// TracePrintf(0, "entry_query: the type of curr file is %d\n", dir_inode->type);
	TracePrintf(0, "entry_query: the entry_num is %d\n", entry_num);

	// // loop over the direct block array
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
				TracePrintf(0, "entry_query: find the file inum%d\n", block_buf[j].inum);
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

// used for tests
void print_entry(int dir_inum) {
	TracePrintf(0, "print_entry starts...");
	struct inode *dir_inode = getInode(dir_inum);
	int i;
	int entry_num = (dir_inode->size) / sizeof(struct dir_entry);
	int entry_count = 0;
	for (i = 0; i < NUM_DIRECT; i++) {
		int block_num = dir_inode->direct[i];
		struct dir_entry *block_buf = (struct dir_entry*)getBlock(block_num);

		int j;
		for (j = 0; j < SECTORSIZE/sizeof(struct dir_entry); j++) {
			entry_count++;
			if (entry_count > entry_num) return ERROR;
			TracePrintf(0, "print_query: the name in the dir is %s\n",block_buf[j].name);
			TracePrintf(0, "print_query: the inum for this entry is %d\n",block_buf[j].inum);			
		}
	}
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
		return NULL;
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
			if (entry_count > entry_num) return NULL;
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
	return NULL;
}

/*
add an entry, return the new inum
*/
int add_entry(char *filename, int dir_inum, int new_inum){
		TracePrintf(0, "add_entry starts... the filename is %s, the curr directory inum is: %d, the new_inum is %d\n", filename, dir_inum, new_inum);
	
	// struct inode *dir_inode = (struct inode*)malloc(sizeof(struct inode));
	struct inode *dir_inode = getInode(dir_inum); 
	
	// return ERROR if this is not a directory
	if (dir_inode->type != INODE_DIRECTORY) {
		TracePrintf(0, "add_entry : cur path is not a dir. add_entry ends.\n");
		return ERROR;
	}

	// the entry number in this dir file
	int entry_num = (dir_inode->size) / sizeof(struct dir_entry);
	int entry_count = 0;
	TracePrintf(0, "add_entry: the type of curr file is %d\n", dir_inode->type);
	TracePrintf(0, "add_entry: the entry_num is %d\n", entry_num);

	// loop over the direct block array
	TracePrintf(0, "add_entry: the entried is this dir is :\n");
	int i;
	for (i = 0; i < NUM_DIRECT; i++) {
		int block_num = dir_inode->direct[i];
		struct dir_entry *block_buf = (struct dir_entry*)getBlock(block_num);

		int j;
		for (j = 0; j < SECTORSIZE/sizeof(struct dir_entry); j++) {
			TracePrintf(0, "add_entry: the name in the dir is %s\n",block_buf[j].name);
			TracePrintf(0, "add_entry: the inum for this entry is %d\n",block_buf[j].inum);
			if (block_buf[j].inum == 0) {
				TracePrintf(0, "add_entry: find a empty entry%d\n", j);
				// get a free inode inum for free inode array
				block_buf[j].inum = new_inum;
				strcpy(block_buf[j].name, filename);
				getInode(dir_inum)->size += sizeof(struct dir_entry);
				TracePrintf(0, "copy the name to entry:%s. add_entry ends.\n", block_buf[j].name);
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
				if (block_buf2[j].inum == 0) {
					block_buf2[j].inum = new_inum;
					strcpy(block_buf2[j].name, filename);
					return block_buf2[j].inum;
				}
			}
		}	
	}
	TracePrintf(0,"entry_query ends. No entry found.");
	// no empty entry found
	return ERROR;
}

/*
remove an entry 
*/
// int rm_entry(char *filename, int inum, int dir_inum) t_block_diskname, filename, MAXPATHNAMELEN);
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
void open_handler(my_msg *msg, int sender_pid) {
	TracePrintf(0,"open_handler start...\n");
	// read the msg to local
	char pathname[MAXPATHNAMELEN];
	if (CopyFrom(sender_pid,pathname,msg->addr1,msg->data2+1) == ERROR) {
    	msg->data1 = -1;
    }
    TracePrintf(0, "open_handleer: the pathname is %s\n", pathname);
    int dir_inum = msg->data1;
    TracePrintf(0, "open_handler: the dir_inum after CopyFrom:%d\n", dir_inum);
   
    // find the inum for the open file
    int open_inum = path_file(pathname,dir_inum);

    // 
    if (open_inum == ERROR) {
    	TracePrintf(0,"open_handler ends. file do not exists.\n");
        msg->data1 = -1;
    }
    else {
    	msg->data1 = open_inum;
    	TracePrintf(0, "open_handler ends, the inum for open file is %d\n.", open_inum);
    }
}

/* 
 * create and open the new file, the file type is regular
 * return the new file's inum
*/ 
void create_handler(my_msg *msg, int sender_pid){
	TracePrintf(0, "create hander start...\n");
	
	// get the pathname
	char pathname[MAXPATHNAMELEN];
	if (CopyFrom(sender_pid, pathname, msg->addr1, msg->data1+1) == ERROR) {
		return ERROR;
	}
	TracePrintf(0,"create_handler: the pahname is %s\n", pathname);
	// get the cur inum
	int curr_inum = msg->data2;

	// TracePrintf(0,"print cur addr is %d", &curr_inum);
	// if (CopyFrom(sender_pid, &curr_inum, &(msg->data2), sizeof(int))==ERROR) {
	// 	return ERROR;
	// }
	TracePrintf(0,"create_handler: the curr_inum is %d\n", curr_inum);
	
	// get_parent(pathname, curr_inum);
	struct my_comp *buff;
	buff = get_parent(pathname, curr_inum);
	TracePrintf(0, "create_handler: the par_inum is %d, the component name is %s\n", buff->par_inum, buff->name);

	if (buff->par_inum == ERROR || getInode(buff->par_inum)->type != INODE_DIRECTORY) {
		TracePrintf(0, "create_handler: the parent is not a dir. reply a msg with data -1. create ends.\n");
		msg->data1 = -1;
		return;
	}
	
	// parent is valid, add entry in it
	int inum = entry_query(buff->name, buff->par_inum);

	if (inum != ERROR) {
		if (getInode(inum)->type != INODE_REGULAR) {
			return msg->data1 = -1;
		}
		TracePrintf(0, "create_handler: the file exists, set it as new\n");
		struct inode *node = getInode(inum);
		node->size = 0;
		// set the return msg data1 as the inum for the filename
		return msg->data1 = inum;
	}
	else {
		TracePrintf(0, "create_handler: the file do not exists, add entry.\n");
		int new_inum = get_freeInode();
		int inum = add_entry(buff->name, buff->par_inum, new_inum);
		if (inum == ERROR) return ERROR;
		write_Nnode(new_inum);
		struct inode *node = getInode(inum);
		node->type = INODE_REGULAR;
		node->nlink = 1;
		node->reuse++;
		node->size = 0;
		msg->data1 = inum;
		TracePrintf(0, "create_handler ends. the new inum is %d\n", inum);
	}
	return;
}

/* create a head link from new file name to the existing oldname
 * return error if the newname already exists
 * return 0 if success
*/
int link_handler(my_msg *msg, int sender_pid) {
	// read the newname and old name form msg
	// get the pathname
	TracePrintf(0, "link_handler starts.");
	char oldname[MAXPATHNAMELEN];
	char newname[MAXPATHNAMELEN];
	CopyFrom(sender_pid, oldname, msg->addr1, msg->data1+1);
	CopyFrom(sender_pid, newname, msg->addr2, msg->data2+1);
	TracePrintf(0,"link_handler: the oldname is %s, the newname is %s\n", oldname,newname);
	// get the cur inum
	int curr_inum = msg->data3;
	// if (CopyFrom(sender_pid, &curr_inum, &(msg->data3), sizeof(int))==ERROR) {
	// 	return ERROR;
	// }
	TracePrintf(0,"link_handler: the curr_inum is %d\n", curr_inum);

	// return -1 if newname already exist
	if (path_file(newname, curr_inum) > 0) return msg->data1 = -1;
	
	// find the inum for oldname, return -1 if it is a dir
	int inum = path_file(oldname, curr_inum);
	if (inum <= 0 || getInode(inum)->type == INODE_DIRECTORY) return msg->data1 = -1;

	// find the parent dir for newname, return -1 if parent dir not valid
	struct my_comp *comp = get_parent(newname, curr_inum);
	int par_inum = comp->par_inum;
	if (par_inum <= 0 || getInode(par_inum)->type != INODE_DIRECTORY) return msg->data1 = -1;

	// add an entry in par dir, the inum is oldname inum
	inum = add_entry(comp->name, comp->par_inum, inum);
	if (inum != -1) getInode(inum)->nlink++;
	TracePrintf(0,"link_handler ends. the linked inum is %d\n", inum);
	return msg->data1 = inum;
}

/*
 *remove the directory enty for the pathname, if it is the last link, clean the inode for this file
 *the filename should not be a directory
 */
void unlink_handler(my_msg *msg, int sender_pid) {
	TracePrintf(0,"unlink_handler starts...\n");
	// print_entry(1);
	// read the path name and curr_dir from msg
	char pathname[MAXPATHNAMELEN];
	int curr_inum = msg->data2;
	if (CopyFrom(sender_pid, pathname, msg->addr1, msg->data1+1) == ERROR) {
		return msg->data1 = -1;
	}
	// if (CopyFrom(sender_pid, &curr_inum, &(msg->data2), sizeof(int)) == ERROR) {
	// 	return msg->data1 = -1;
	// }
	TracePrintf(0, "unlink_handler:the pathname is %s, the curr_inum is %d\n", pathname, curr_inum);

	// find the parent inum and component name
	struct my_comp *buf = get_parent(pathname, curr_inum);
	TracePrintf(0, "unlink_handler:find the parent path is %s, par_inum is %d\n", buf->name, buf->par_inum);
	if (buf->par_inum == ERROR) return msg->data1 = -1;

	// find the entry address for the component
	struct dir_entry *entry = search_entry(buf->name, buf->par_inum);
	if (entry == NULL || entry->inum < 0) return msg->data1 = -1;

	// get the inode for this filename
	struct inode *node = getInode(entry->inum);

	// if it is a dir, return error
	if (node->type == INODE_DIRECTORY) return msg->data1 = -1;

	// set inum as 0, if the nlink is 0, free the inode for this file, update the par size
	if (node->nlink == 1) {
		freeInode(entry->inum);
	}
	entry->inum = 0;
	getInode(buf->par_inum)->size -= sizeof(struct dir_entry);
	TracePrintf(0,"unlink_handler: unlink successfully ends.\n");
	return msg->data1 = 1;
}

void symlink_handler(my_msg *msg, int sender_pid) {
	// read the newname and old name form msg
	// get the pathname
	TracePrintf(0, "symlink_handler starts.");
	char oldname[MAXPATHNAMELEN];
	char newname[MAXPATHNAMELEN];
	CopyFrom(sender_pid, oldname, msg->addr1, msg->data1+1);
	CopyFrom(sender_pid, newname, msg->addr2, msg->data2+1);
	TracePrintf(0,"symlink_handler: the oldname is %s, the newname is %s\n", oldname,newname);
	// get the cur inum
	int curr_inum = msg->data3;
	// TracePrintf(0, "symlink_handler: data3 %d\n", msg->data3);
	// if (CopyFrom(sender_pid, &curr_inum, &(msg->data3), sizeof(int))==ERROR) {
	// 	return ERROR;
	// }
	TracePrintf(0,"symlink_handler: the curr_inum is %d\n", curr_inum);

	// return -1 if newname already exist
	if (path_file(newname, curr_inum) > 0) return msg->data1 = -1;

	// find the parent dir for newname, return -1 if parent dir not valid
	struct my_comp *comp = get_parent(newname, curr_inum);
	int par_inum = comp->par_inum;
	if (par_inum <= 0 || getInode(par_inum)->type != INODE_DIRECTORY) return msg->data1 = -1;

	// add an entry in par dir, the inum is oldname inum
	int new_inum = get_freeInode();
	if (new_inum == ERROR) return msg->data1=-1;
	int inum = add_entry(comp->name, comp->par_inum, new_inum);
	if (inum == -1) return msg->data1 = -1;
	write_Nnode(new_inum);
	struct inode *node = getInode(inum);
	node->type=INODE_SYMLINK;
	node->nlink = 1;
	node->reuse++;
	int size = strlen(oldname);
	node->size = size;
	// alloc a block to mem the oldname
	int blockIndex = get_freeBlock();
	write_Bnode(blockIndex);
	node->direct[0] = blockIndex;
	memcpy(getBlock(blockIndex), oldname, strlen(oldname));
	TracePrintf(0,"symlink_handler ends. is as name is %s, new inum for this name is %d\n",getBlock(blockIndex), new_inum);
	TracePrintf(0, " the updated size %d\n", getInode(3)->size);
	return msg->data1 = inum;
}

void readlink_handler(my_msg *msg, int sender_pid){
	TracePrintf(0, "readlink_handler starts.\n");
	
	// read the newname and old name form msg
	char pathname[MAXPATHNAMELEN];
	if (CopyFrom(sender_pid, pathname, msg->addr1, msg->data1+1) == ERROR) return msg->data1 = -1;;
	// get the cur inum
	int curr_inum = msg->data3;
	// if (CopyFrom(sender_pid, &curr_inum, &(msg->data3), sizeof(int))==ERROR) {
	// 	return msg->data1=-1;
	// }
	int len = msg->data2;
	// if (CopyFrom(sender_pid, &len, &(msg->data2), sizeof(int))==ERROR) {
	// 	return msg->data1=-1;
	// }
	TracePrintf(0,"readlink_handler: the pathname is %s, dir_inum is %d, len is %d\n", pathname, curr_inum, len);

	// find the parent of pathname
	struct my_comp *buff;
	buff = get_parent(pathname, curr_inum);
	TracePrintf(0, "readlink_handler: the par_inum is %d, the component name is %s\n", buff->par_inum, buff->name);
	// check if the parent pathname is valid and is a directory
	if (buff->par_inum == ERROR || getInode(buff->par_inum)->type != INODE_DIRECTORY) {
		return msg->data1 = -1;
	}

	// find the inum for pathname
	int inum = entry_query(buff->name, buff->par_inum);
	if (inum <= 0) return msg->data1 = -1;

	// read the content to the buf
	struct inode *node = getInode(inum);
	if (node->type != INODE_SYMLINK) return msg->data1 = -1;
	void *buf = getBlock(node->direct[0]);
	if (node->size <= len) {
		TracePrintf(0, "readlink_handler: the size is %d,copy the content in symlink block %s\n", node->size, buf);
		msg->data1 = node->size;
		CopyTo(sender_pid,msg->addr2,buf,node->size);
	} else {
		TracePrintf(0, "222222222211111111111\n");
		msg->data1 = len;
		CopyTo(sender_pid,msg->addr2,buf,len);
	}
	TracePrintf(0, "readlink_handler ends.\n");
}
/*
* Make a dir. There are two default dir_entry, . and ..
* return error if pathname already exist
*/
void mkdir_handler(my_msg *msg, int sender_pid) {
	TracePrintf(0, "mkdir_handler start...\n");
	
	// get the pathname
	char pathname[MAXPATHNAMELEN];
	if (CopyFrom(sender_pid, pathname, msg->addr1, msg->data1+1) == ERROR) {
		return ERROR;
	}
	TracePrintf(0,"mkdir_handler: the pathname is %s\n", pathname);
	// get the cur inum
	int curr_inum = msg->data2;
	// if (CopyFrom(sender_pid, &curr_inum, &(msg->data2), sizeof(int))==ERROR) {
	// 	return ERROR;
	// }
	TracePrintf(0,"mkdir_handler: the curr_inum is %d\n", curr_inum);
	
	// get_parent(pathname, curr_inum);
	struct my_comp *buff;
	buff = get_parent(pathname, curr_inum);
	TracePrintf(0, "mkdir_handler: the par_inum is %d, the component name is %s\n", buff->par_inum, buff->name);

	// check if the parent pathname is valid and is a directory
	if (buff->par_inum == -1) {
		TracePrintf(0, "mkdir_handler error, pathname is not valid.");
		msg->data1 = -1;
		return;
	}
	
	// check if the name has alreaday exists, if yes, reutrn error, if no, add the entry and alloc the inode
	int inum = entry_query(buff->name, buff->par_inum);
	if (inum != ERROR) {
		TracePrintf(0, "mkdir_handler: the pathname exists, return error\n");
		msg->data1 = -1;
		return;
	} else {
		TracePrintf(0, "mkdir_handler: allocate a new inode and add entry into its parent file.\n");
		int new_inum = get_freeInode();
		inum = add_entry(buff->name, buff->par_inum, new_inum);
		if (inum == ERROR) {
			return msg->data1 = -1;
		}
		write_Nnode(new_inum);
		struct inode *node = getInode(inum);
		node->type = INODE_DIRECTORY;
		node->size = 2 * sizeof(struct dir_entry);
		node->nlink = 1;
		node->reuse++;
		int new_block = get_freeBlock();
		write_Bnode(new_block);
		node->direct[0] = new_block;
		struct dir_entry *tmp = getBlock(new_block);
		int i = 0;
		for(; i<(SECTORSIZE/sizeof(struct dir_entry));i++) {
			if(i == 0) {
				tmp[i].inum = inum;
				strcpy(tmp[i].name, ".");
				continue;
			}
			if (i == 1) {
				tmp[i].inum = buff->par_inum;
				strcpy(tmp[i].name,"..");
				continue;
			}
			tmp[i].inum = 0;
		}
		TracePrintf(0, "mkdir_handler success.The newinum and new bolck for new dir is %d %d\n", new_inum, new_block);
		print_entry(new_inum);
	}
	
	
	return msg->data1 = 1;
}

/*
* remove a dir. dir contains no other dir excpt . and .., dir cannot be root, . or ..
* 
*/
int rmdir_handler(my_msg *msg, int sender_pid) {
	TracePrintf(0, "rmdir_handler start...\n");

	char pathname[MAXPATHNAMELEN];
	if (CopyFrom(sender_pid, pathname, msg->addr1, msg->data1+1) == ERROR) {
		return msg->data1 = -1;
	}

	// get the cur inum
	int curr_inum = msg->data2;
	// if (CopyFrom(sender_pid, &curr_inum, &(msg->data2), sizeof(int))==ERROR) {
	// 	return msg->data1 = -1;
	// }
	TracePrintf(0,"rmdir_handler: the curr_inum is %d\n", curr_inum);
	

	// get_parent(pathname, curr_inum);
	struct my_comp *buff;
	buff = get_parent(pathname, curr_inum);
	TracePrintf(0, "rmdir_handler: the par_inum is %d, the component name is %s\n", buff->par_inum, buff->name);

	// check if the parent pathname is valid and is a directory
	if (buff->par_inum == ERROR || strcmp(buff->name, ".") == 0 || strcmp(buff->name, "..") == 0) {
		return msg->data1 = -1;
	}

	// find the entry address for the component
	struct dir_entry *entry = search_entry(buff->name, buff->par_inum);
	TracePrintf(0, "search_entry ends.\n");
	if (entry == NULL || entry->inum < 0) {
		TracePrintf(0,"rmdir path not found\n");
		return msg->data1 = -1;
	}

	// get the inode for this filename
	struct inode *node = getInode(entry->inum);

	// if it is not a dir or size > 2 dir, return error
	if (node->type != INODE_DIRECTORY || node->size > 2 * sizeof(struct dir_entry)) return msg->data1 = -1;

	// update entry and size of par, free the inode
	freeInode(entry->inum);
	entry->inum = 0;
	getInode(buff->par_inum)->size -= sizeof(struct dir_entry);
	TracePrintf(0, "rmdir_handler success.\n");
	// print_entry(buff->par_inum);
	return msg->data1 = 1;
}

/*
* change to a dir. 
* 
*/
int chdir_handler(my_msg *msg, int sender_pid) {
	TracePrintf(0, "chdir_handler start...\n");

	char pathname[MAXPATHNAMELEN];
	if (CopyFrom(sender_pid, pathname, msg->addr1, msg->data1+1) == ERROR) {
		return msg->data1 = -1;
	}

	// get the cur inum
	int curr_inum = msg->data2;
	// if (CopyFrom(sender_pid, &curr_inum, &(msg->data2), sizeof(int))==ERROR) {
	// 	return msg->data1 = -1;
	// }
	TracePrintf(0,"rmdir_handler: the curr_inum is %d\n", curr_inum);
	
	// get the inum for pathname
	int inum = path_file(pathname, curr_inum);
	if (inum == ERROR || getInode(inum)->type != INODE_DIRECTORY) return msg->data1 = -1;

	// return msg
	msg->data1 = inum;
	TracePrintf(0, "chdir success!, curr dir inum is %d\n", inum);

}

void stat_handler(my_msg *msg, int sender_pid) {
	TracePrintf(0, "stat_handler start...\n");

	// get the pathname and curr_inum
	char pathname[MAXPATHNAMELEN];
	if (CopyFrom(sender_pid, pathname, msg->addr1, msg->data1+1) == ERROR) {
		return msg->data1 = -1;
	}
	int curr_inum = msg->data2;
	// if (CopyFrom(sender_pid, &curr_inum, &(msg->data2), sizeof(int))==ERROR) {
	// 	return msg->data1 = -1;
	// }
	TracePrintf(0,"stat_handler: the pathname is %s, the curr_inum is %d\n", pathname, curr_inum);
	
	// get the parent
	struct my_comp *buff = get_parent(pathname, curr_inum);
	TracePrintf(0, "stat_handler: the par_inum is %d, the component name is %s\n", buff->par_inum, buff->name);

	// check if the parent pathname is valid and is a directory
	if (buff->par_inum == -1 || getInode(buff->par_inum)->type!= INODE_DIRECTORY) {
		TracePrintf(0, "stat_handler error, pathname is not valid.");
		msg->data1 = -1;
		return;
	}
	
	// query the name in parent
	int inum = entry_query(buff->name, buff->par_inum);
	if (inum == ERROR) return msg->data1 = -1;

	// create return msg
	struct inode *node = getInode(inum);
	struct Stat s;
	s.inum = inum;
	s.type = node->type;
	s.size = node->size;
	s.nlink = node->nlink;
	CopyTo(sender_pid, msg->addr2, &s, sizeof(struct Stat));
}

void read_handler(my_msg* msg, int pid) {
	TracePrintf(0, "****************************\n");
	TracePrintf(0, "READ: entering read handler\n");
	/*
	 * Get current position from message
	 * Should be modified later
	 */
	 int start_pos = msg->data1;
	 int len = msg->data2;
	 int inode_num = msg->data3;
	 // void *buf = msg->addr1;
	TracePrintf(0, "READ: Inode number is %d\n", inode_num);
 	 struct inode *inode = getInode(inode_num);
	 TracePrintf(0, "READ: size to read is %d, start position is %d, inode size is %d\n", len, start_pos, inode->size);
	 
	 if (start_pos > inode->size) {
	 	TracePrintf(0, "READ: read position larger than size of the file\n");
	 	return;
	 }

	 if (start_pos + len > inode->size) {
	 	len = inode->size - start_pos;
	 	TracePrintf(0, "READ: size to read is %d, start position is %d\n", len, start_pos);
	 	if(len < 0) return;
	 }
	 int end_pos = start_pos + len;
	 char *tempBuffer = (char*)malloc(sizeof(char)*len);
	 int startOffset = start_pos % BLOCKSIZE;
	 int size = len;
	 int i = 0;
	 int cur = 0;
	 for(; i < NUM_DIRECT; i ++) {
	 	block_num = inode->direct[i];
	 	if (block_num <= 0) break;
	 	if ((cur + BLOCKSIZE)>=start_pos && cur <= start_pos)break;
	 	cur += BLOCKSIZE;
	 }

	 /*
	  * Copy from the indirect field
	  */
	  if (i < NUM_DIRECT) {
	  	TracePrintf(0, "READ: i is %d\n", i);
	  	char *tmp = tempBuffer;
        while (start_pos <end_pos && i<NUM_DIRECT) {
            block_num = inode->direct[i];
            if (block_num>0) {
                int read_in_block;
                if (end_pos<cur+BLOCKSIZE) read_in_block = end_pos - start_pos;
                else read_in_block = cur+BLOCKSIZE-start_pos;
                void *bc = getBlock(block_num);
                memcpy(tmp,bc+start_pos-cur,read_in_block);

                start_pos += read_in_block;
                tmp += read_in_block;
                cur += BLOCKSIZE;
            }
            i++;
        }
        if (i>=NUM_DIRECT) {
            if (inode->indirect!=0) {
                block_num = inode->indirect;
                void *block = getBlock(block_num);
                int j = 0;
                while (start_pos<size && j<BLOCKSIZE) {
                    block_num = *(int*)(block+j);
                    if (block_num>0) {
                        int read_in_block;
                        if (size<cur+BLOCKSIZE) read_in_block = end_pos - start_pos;
                        else read_in_block = cur+BLOCKSIZE-start_pos;
                        void *bc = getBlock(block_num);
                        memcpy(tmp,bc + start_pos-cur,read_in_block);
                        start_pos += read_in_block;
                        tmp += read_in_block;
                        cur += BLOCKSIZE;
                    }
                    j+=4;
                }
            }
        }

	  } else {
	  	if (inode->indirect != 0) {
	  		block_num = inode->indirect;
	  		void *buf = getBlock(block_num);
	  		for (i = 0; i < BLOCKSIZE; i +=4) {
	  			block_num = *(int *)(buf + i);
	  			if (block_num != 0) {
	  				if ((cur - start_pos) < size && (cur - start_pos + BLOCKSIZE) > size) break;
	  				cur += BLOCKSIZE;
	  			}
	  		}

	  		char *tmp = tempBuffer;
	  		int j = 0;
	  		while (start_pos<size && j<BLOCKSIZE) {
                    block_num = *(int*)(buf+j);
                    if (block_num>0) {
                        int read_in_block;
                        if (size<cur+BLOCKSIZE) read_in_block = end_pos - start_pos;
                        else read_in_block = cur+BLOCKSIZE-start_pos;
                        void *bc = getBlock(block_num);
                        memcpy(tmp,bc + start_pos-cur,read_in_block);
                        start_pos += read_in_block;
                        tmp += read_in_block;
                        cur += BLOCKSIZE;
                    }
                    j+=4;
                }
	  	}
	  }

	  if (CopyTo(pid, msg->addr1, tempBuffer, size)) {
	  	 msg->type = ERROR;
	  	 free(tempBuffer);
	  	 return;
	  }
	free(tempBuffer);
	msg->data1 = len;
	TracePrintf(0, "READ: read data length is %d \n", msg->data1);
}

void write_handler(my_msg* msg, int pid) {
	TracePrintf(0, "*************************************\n");
	TracePrintf(0, "Write Handler\n");
	/*
	 *my_msg* msg = (Msg*)malloc(sizeof(Msg*));
	msg->type = WRITE;
	msg->addr1 = buf;
	msg->data2 = size;
	msg->data3 = opened[fd]->inum;
	msg->data1 = opened[fd]->cur_pos;
	 */
	
	int start_pos = msg->data1;
	int size = msg->data2;
	int inode_num = msg->data3;

	char *tempBuffer = (char *)malloc(sizeof(char)*size);
	TracePrintf(0, "WRITE: Inode number is %d\n", inode_num);
	struct inode* inode = getInode(inode_num);
	CopyFrom(pid, tempBuffer, msg->addr1, size);
	TracePrintf(0, "WRITE: Finished CopyFrom\n");
	if (inode->type == INODE_DIRECTORY) {
		msg->type = ERROR;
		return;
	}
	int newBlock = 0;
	/*
	 * Deal with the "\0" problem
	 */
	if (start_pos > inode->size) {
		TracePrintf(0,"Handle seek problem in write, block size is %d\n", inode->size);
		int startBlock = (start_pos - 1) / BLOCKSIZE + 1;
		int startOffset = (start_pos) % BLOCKSIZE;
		int curBlock = (inode->size - 1)/BLOCKSIZE + 1;
		int curOffset = inode->size % BLOCKSIZE;
		int i = curBlock;
		/*
		 *
		 */
		if (startBlock <= NUM_DIRECT) {
			while (i < startBlock) {
				 if (inode->direct[i] != 0 ) { //fill the half block curent offset
					void *block_num = getBlock(inode->direct[i]);

					char *tmp = (char *)malloc(BLOCKSIZE - curOffset);
					int n;
					for (n = 0; n < BLOCKSIZE - curOffset; n ++) {
						tmp[n] = '\0';
					}
					memcpy(block_num + curOffset, tmp, BLOCKSIZE - curOffset);
					inode->size += (BLOCKSIZE - curOffset);
					curOffset = 0;
				} else { //fill full block
					inode->direct[i] = get_freeBlock();
					void *block_num = getBlock(inode->direct[i]);
					char *tmp = (char *)malloc(BLOCKSIZE);
					int n;
					for (n = 0; n < BLOCKSIZE; n ++) {
						tmp[n] = '\0';
					}
					memcpy(block_num, tmp, BLOCKSIZE);
					inode->size += BLOCKSIZE;
				}
				i ++;
			}
			if (i == startBlock && startOffset != 0) {
				inode->direct[i] = get_freeBlock();
				void *block = getBlock(inode->direct[i]);
				int n;
				char *tmp = (char *)malloc(startOffset - inode->size);
				for (n = 0; n < startOffset; n ++) {
					tmp[n] = '\0';
				}
				memcpy(block, tmp, startOffset - inode->size);
				inode->size = startOffset;
				startOffset = 0;
			}			
		} else {
			if (curBlock < NUM_DIRECT) { //cover the direct and indirect field
				while (curBlock < NUM_DIRECT) {
					if (inode->direct[curBlock] != 0 ) { //fill the half block curent offset
						void *block_num = getBlock(inode->direct[curBlock]);
						char *tmp = (char *)malloc(BLOCKSIZE - curOffset);
						int n;
						for (n = 0; n < BLOCKSIZE - curOffset; n ++) {
							tmp[n] = '\0';
						}
						memcpy(block_num + curOffset, tmp, BLOCKSIZE - curOffset);
						inode->size += (BLOCKSIZE - curOffset);
						curOffset = 0;
					} else { //fill full block
						inode->direct[i] = get_freeBlock();
						void *block_num = getBlock(inode->direct[curBlock]);
						char *tmp = (char *)malloc(BLOCKSIZE);
						int n;
						for (n = 0; n < BLOCKSIZE; n ++) {
							tmp[n] = '\0';
						}
						memcpy(block_num, tmp, BLOCKSIZE);
						inode->size += BLOCKSIZE;
					}
					curBlock ++;
				}
			} else { //only write in the indirect field
				void *indirectBlock = getBlock(inode->indirect);
				int curIndex = curBlock - NUM_DIRECT;
				int block_num = *(int *)(indirectBlock + curIndex);
				while (curBlock < startBlock) {
					if (curOffset != 0) {
						void *block_num = getBlock(block_num);
						char *tmp = (char *)malloc(BLOCKSIZE - curOffset);
						int n;
						for (n = 0; n < BLOCKSIZE - curOffset; n ++) {
							tmp[n] = '\0';
						}
						memcpy(block_num + curOffset, tmp, BLOCKSIZE - curOffset);
						inode->size += (BLOCKSIZE - curOffset);
						curOffset = 0;
						curBlock ++;
						curIndex ++;
					} else {
						*(int *)(indirectBlock + curIndex) = get_freeBlock();
						void *newBlock = getBlock(*(int *)(indirectBlock + curIndex));
						char *tmp = (char *)malloc(BLOCKSIZE);
						int n;
						for (n = 0; n < BLOCKSIZE; n ++) {
							tmp[n] = '\0';
						}
						memcpy(newBlock, tmp, BLOCKSIZE);
						curBlock ++;
						curIndex ++;
						inode->size += BLOCKSIZE;
					}
				}
				if (curBlock == startBlock && startOffset != 0) {
					*(int *)(indirectBlock + curIndex) = get_freeBlock();
					void *newBlock = getBlock(*(int *)(indirectBlock + curIndex));
					int n;
					char *tmp = (char *)malloc(startOffset - inode->size);
					for (n = 0; n < startOffset - inode->size; n ++) {
						tmp[n] = '\0';
					}
					memcpy(newBlock, tmp, startOffset - inode->size);
					inode->size = startOffset;
					startOffset = 0;
				}
			}
		}
	}

	write_Nnode(inode_num);
	if (start_pos + size > inode->size) {
		newBlock = (start_pos+size-inode->size - 1) / BLOCKSIZE + 1;
		TracePrintf(0, "WRITE: new block number is %d \n", newBlock);
	}
	/*
	 * Allocate new block to this file
	 */
	int i = 0;
	for(; i < NUM_DIRECT; i ++) {
		if (newBlock <= 0) break;
		if (inode->direct[i] <= 0) {			
			inode->direct[i] = get_freeBlock();
			write_Nnode(inode_num); //to be modified
			newBlock--;
		}
	}
	TracePrintf(0, "WRITE: allocating new blocks\n");
	int data;
	if (newBlock > 0) {
		if (inode->indirect <= 0) {
			inode->indirect = get_freeBlock();
			write_Nnode(inode_num);
		}
		void *indirectBlock = getBlock(inode->indirect);
		int j = 0;
		for (; j < BLOCKSIZE; j += 4) {
			if (newBlock <= 0 ) break;
			data = *(int *)(indirectBlock + j);
			if (data <= 0) {
				data = get_freeBlock();
				write_Bnode(data); //to be modified
				newBlock --;
			}
		}
	}
	// /*
	//  * Write data into block
	//  */
	TracePrintf(0, "WRITE: Writing data\n");
	int cur = 0;
	for (i = 0; i < NUM_DIRECT; i ++) {
		//int blockDirect = inode->direct[i];
		//if (blockDirect <= 0) 
		if (start_pos >= cur && cur + BLOCKSIZE > start_pos ) break;
		cur += BLOCKSIZE;
	}

	if (i < NUM_DIRECT) {
		TracePrintf(0, "WRITE: write data into direct field, block number is %d\n", inode->direct[i]);
		char *tmp = tempBuffer;
		while(size > 0 && i < NUM_DIRECT) {
			int blockDirect = inode->direct[i];
			if (blockDirect > 0) {
				int write_num;
				TracePrintf(0, "WRITE: size to write is %d, block size is %d \n", size, BLOCKSIZE);
				if (size < start_pos + BLOCKSIZE - cur) write_num = size;
				else write_num = cur - start_pos + BLOCKSIZE;
				void *block2write = getBlock(blockDirect);
				TracePrintf(0, "WRITE: memcpy to the block, address is %d\n", block2write);
				TracePrintf(0, "WRITE: start position is %d, now in %d\n", start_pos, cur);
				memcpy(block2write + start_pos - cur, tmp, write_num);
				TracePrintf(0, "WRITE: memcpy finished\n");
				write_Bnode(blockDirect);
				start_pos += write_num;
				tmp += write_num;
				size -= write_num;
				cur += BLOCKSIZE;
				TracePrintf(0, "WRITE: Finished writing the block\n");
			}
			i ++;
		}
		if (i >= NUM_DIRECT) {
			TracePrintf(0, "WRITE: write data into direct and indirect field\n");
			if (inode->indirect != 0) {
				int blockIndirect = inode->indirect;
				void *b = getBlock(blockIndirect);
				int j = 0;
				while (size > 0 && j < BLOCKSIZE) {
					blockIndirect = *(int *)(b+j);
					if (blockIndirect > 0) {
						int writeCount;
						if (size < cur + BLOCKSIZE - start_pos) writeCount = size;
						else writeCount = cur - start_pos + BLOCKSIZE;
						void *block2write = getBlock(blockIndirect);
						memcpy(block2write + start_pos - cur, tmp, writeCount);
						write_Bnode(block2write);

						start_pos += writeCount;
						size -=writeCount;
						cur += BLOCKSIZE;
						tmp += writeCount;
					}
					j +=4;
				} 
			}
		}
	} else {
		TracePrintf(0, "WRITE: write data into indirect field\n");
		if (inode -> indirect != 0) {
			int blockNUM = inode->indirect;
			void *indirectBlock = getBlock(blockNUM);
			int j = 0;
			for(j = 0; j < BLOCKSIZE; j += 4) {
				blockNUM  = *(int*)(indirectBlock + j);
				if (blockNUM != 0) {
					if (cur <= start_pos && cur + BLOCKSIZE > start_pos) break;
					cur+= BLOCKSIZE;
				}
			}
			char *tmp = tempBuffer;
			while (size > 0 && j < BLOCKSIZE) {
				blockNUM = *(int *)(indirectBlock + j);
				if (blockNUM > 0) {
					int writeCount;
					if (writeCount < cur + BLOCKSIZE - start_pos) writeCount = size;
					else writeCount = cur + BLOCKSIZE - start_pos;
					void *block2write = getBlock(blockNUM);
					memcpy(block2write + start_pos - cur, tmp, writeCount);
					write_Bnode(blockNUM);
					start_pos += writeCount;
					tmp += writeCount;
					size -= writeCount;
					cur += BLOCKSIZE;
				}
				j += 4;
			}
		}
	}
	TracePrintf(0, "WRITE: Finished writing handler, inode size 1 is %d\n", inode->size);
	if (inode->size < start_pos) inode->size = start_pos;
	TracePrintf(0, "WRITE: Finished writing handler, inode size 2 is %d\n", inode->size);
	msg->data1 = start_pos - msg->data1;
}

/*
* write all dirty inode to cached block and write all dirty blocks into disk
*/
void sync_handler(my_msg *msg, int sender_pid) {
	// TracePrintf(0,"sync_handler starts...");
	// struct Nnode *tmp = Nhead;
	// while(tmp != NULL) {
	// 	TracePrintf(0,"sync_handler:write inode %d to block", tmp->inum);
	// 	if (tmp->dirty == '1') {
	// 		TracePrintf(0,"syn_handler: write inode %d", tmp->inum);
	// 		write_inode_block(tmp->inum);
	// 	}
	// 	tmp = tmp->next;
	// }
	getInode(2);
	TracePrintf(0, "size %d\n", Nsize);
	TracePrintf(0, "head inum %d\n", Nhead->inum);
	TracePrintf(0, "tead inum %d\n", Nhead->inum);
	getInode(3);
	TracePrintf(0, "head inum %d\n", Nhead->inum);
	TracePrintf(0, "tead inum %d\n", Ntail->inum);
	getInode(4);
	TracePrintf(0, "head inum %d\n", Nhead->inum);
	TracePrintf(0, "tead inum %d\n", Ntail->inum);
	TracePrintf(0, "size %d\n", Nsize);
	getInode(2);
	TracePrintf(0, "head inum %d\n", Nhead->inum);
	TracePrintf(0, "tead inum %d\n", Ntail->inum);
	TracePrintf(0, "size %d\n", Nsize);

	if (Nhead->next == NULL) TracePrintf(0,"fsdadjfskla");
	// print the inode list
	struct Nnode *tmp = Nhead;
	int i = 0;
	while(i<3) {
		TracePrintf(0,"inum %d, dirtry %s\n", tmp->inum, tmp->dirty);
		// if (tmp->dirty == '1') {
		// 	TracePrintf(0,"syn_handler: write inode %d", tmp->inum);
		// 	write_inode_block(tmp->inum);
		// }

		tmp = tmp->next;
	}

	TracePrintf(0, "syn_handler: wiret inode to block complete\n");
	struct Bnode *tmp2 = Bhead;
	while(tmp2 != NULL) {
		TracePrintf(0, "syn_handler: block list index %d\n", tmp2->index);
		if (tmp2->dirty == '1') {
			TracePrintf(0,"syn_handler: write block %d\n", tmp2->index);
			write_block_disk(tmp2->index);
		}
		tmp2 = tmp2->next;
	}
	// test for sync

}

void shutdown_handler(my_msg *msg, int sender_pid)
{
   // sync_handler(msg);
    if (msg->type == ERROR) {
        perror("unable to sync");
        return;
    }
    msg->type = SHUTDOWN;
    if (Reply(msg,sender_pid)==ERROR) fprintf(stderr, "Error replying to pid %d\n",sender_pid);
    //terminate();
    Exit(0);
}

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
    	if (sender_pid == 0) {
    		// perror("no message!\n");
    		continue;
    	}
    	fprintf((stderr), "sender pid is %d\n", sender_pid );
        if (sender_pid==ERROR) {
            perror("error receiving message!\n");
            continue;
        }
        // TracePrintf(0, "the size of msg:%d \n", sizeof(msg));
        switch (msg.type) {
            case OPEN:
            	TracePrintf(0, "receive a open msg~~~~~~~~~~~~`\n");
                 open_handler(&msg,sender_pid);break;
            case CREATE:
            	TracePrintf(0, "receive a create msg~~~~~~~~~~~~`\n");
                 create_handler(&msg,sender_pid);break;
            case READ:
                read_handler(&msg,sender_pid);break;
            case WRITE:
                write_handler(&msg,sender_pid);break;
            case LINK:
            	TracePrintf(0, "receive a link msg~~~~~~~~~~~~~~\n");
                link_handler(&msg,sender_pid);break;
            case UNLINK:
            	TracePrintf(0, "receive a unlink msg~~~~~~~~~~~~~~\n");
                unlink_handler(&msg,sender_pid);break;
            case SYMLINK:
            	TracePrintf(0, "receive a symlink msg~~~~~~~~~~~~~~\n");
                symlink_handler(&msg,sender_pid);break;
            case READLINK:
            	TracePrintf(0, "receive a readlink msg~~~~~~~~~~~~~~\n");
                readlink_handler(&msg,sender_pid);break;
            case MKDIR:
            	TracePrintf(0, "receive a mkdir msg~~~~~~~~~~~~~~\n");
                mkdir_handler(&msg,sender_pid);break;
            case RMDIR:
            	TracePrintf(0, "receive a rmdir msg~~~~~~~~~~~~~~\n");
                rmdir_handler(&msg,sender_pid);break;
            case CHDIR:
            	TracePrintf(0, "receive a chdir msg~~~~~~~~~~~~~~\n");
                chdir_handler(&msg,sender_pid);break;
            case STAT:
            	TracePrintf(0, "receive a stat msg~~~~~~~~~~~~~~\n");
                stat_handler(&msg,sender_pid);break;
            case SYNC:
            	TracePrintf(0, "receive a sync msg~~~~~~~~~~~~~~\n");
                sync_handler(&msg, sender_pid);break;
            case SHUTDOWN:
                shutdown_handler(&msg,sender_pid);break;
            default:
                perror("message type error!");
                break;
        }

        if (Reply(&msg,sender_pid)==ERROR) fprintf(stderr, "fail replying to pid %d",sender_pid);
    }
    terminate();
//         
//     }
	return 0;
} 









