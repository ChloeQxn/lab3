#include <comp421/filesystem.h>
#include <comp421/iolib.h>
#include <comp421/yalnix.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "uthash.h"
// struct of inode lru list
struct Nnode{
	int inum;
	struct inode *node;
	struct Nnode *pre;
	struct Nnode *next;
};

// struct of inode hash
struct my_struct {  
    int ikey;                    /* key */  
    struct Nnode *node;
	UT_hash_handle hh;           
};  

// // init cache
// void initCache();

// // get inode from disk
// struct inode *get_node_disk(int i);

// // hash interface
// struct my_struct *find_inode(int ikey) ;

// void add_inode(int ikey, struct Nnode* node);

// void delete_inode(int ikey);

 
// *add a new inode into cache. add it into hash and add it into tail of list

// void add_inode_cache(int inum, struct inode* node);
// /* 
// *	update Nnode in the list
// */
// void update_Nnode_list(struct Nnode *node);

// // get inode with given inum, if it is in cache, 
// struct inode *getInode(int inum);
