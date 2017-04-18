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
struct Nnode *Nhead;
struct Nnode *Ntail;
int Nsize; 

// struct of inode hash
struct my_struct {  
    int ikey;                    /* key */  
    struct Nnode *node;
	UT_hash_handle hh;           
};  
struct my_struct *inodeHash = NULL; 