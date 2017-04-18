#include <comp421/filesystem.h>
#include <comp421/iolib.h>
#include <comp421/yalnix.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "uthash.h"
// struct of lru list
struct Nnode{
	int inum;
	struct inode *node;
	struct inode *pre;
	struct inode *next;
};
struct Nnode *Nhead;
struct Nnode *Ntail;
int Nsize; 

// init cache
void initCache() {
	Nhead->node = NULL;
	Nhead->pre = NULL;
	Nhead->next = Ntail;
	Ntail->node = NULL;
	Ntail->pre = Nhead;
	Ntail->next = NULL;
	Nsize = 0;
}

// get from disk
struct inode *getInode(int i) {
	int blockIndex = 1 + (i) / (BLOCKSIZE/INODESIZE);
	void *buf = malloc(SECTORSIZE);
	ReadSector(blockIndex, buf);
	struct inode* node = (struct inode*)malloc(sizeof(struct inode));
	int offset = i % (BLOCKSIZE/INODESIZE);
	memcpy(node, (struct inode*)buf + offset, sizeof(struct inode));
	free(buf);
	return node;
}

struct my_struct {  
    int ikey;                    /* key */  
    struct Nnode *node;
	UT_hash_handle hh;           
    };  

struct my_struct *inodeHash = NULL; 

// hash interface
struct my_struct *find_inode(int ikey) {  
    struct my_struct *s;  
HASH_FIND_INT(inodeHash, &ikey, s );  
return s;  
}  

void add_inode(int ikey, struct Nnode* node) {  
    struct my_struct *s;  
    HASH_FIND_INT(inodeHash, &ikey, s);  /* 插入前先查看key值是否已经在hash表inodeHash里面了 */  
    if (s==NULL) {  
      s = (struct my_struct*)malloc(sizeof(struct my_struct));  
      s->ikey = ikey; 
      s->node = node; 
      HASH_ADD_INT(inodeHash, ikey, s );  /* 这里必须明确告诉插入函数，自己定义的hash结构体中键变量的名字 */  
      Nsize++;
    }  
    // strcpy(s-> value, value_buf);  
} 

void delete_inode(int ikey) {  
    struct my_struct *s = NULL;  
    HASH_FIND_INT(inodeHash, &ikey, s);  
    if (s!=NULL) {  
      HASH_DEL(inodeHash, s);   
      free(s);               
} 
///////////////
// list interfacevoid add_inode_cache(int inum, (struct inode) *node)
void add_inode_cache(int inum, (struct inode) *node){
	if (Nsize>=INODE_HASH_SIZE) {
		// delete the head from hash and list
		int inum = Nhead->inum;
		free(Nhead->next);
		Nhead->next = Nhead->next->next;
		Nhead->next->pre= Nhead;
		delete_inode(inum);
	}
	// add to hash and add to the tail
	struct Nnode* node = (struct Nnode*)malloc(sizeof(struct Nnode));
	add_inode(inum, node);
	Ntail->pre->next = node;
	node->pre = Ntail->pre;
	node->next = Ntail;
	Ntail->pre = node;
}


struct inode *get_inode(inum) {
	struct my_struct *tmp = find_inode(inum);
	struct inode *node;
	// if the inode is not in the cache, read it from the disk
	if (tmp == NULL) {
		TracePrintf(0, "inode of %d is not in cache, read it from disk.", inum);
		node = getInode(inum);
		// put it into hashtable
		add_inode_cache(inum, node);
	} else {
		// it already in cache
		TracePrintf(0, "inode of %d is already in cache.", inum);
		struct Nnode *
		node = find_inode(inum)->node;
	}
	return node;
}