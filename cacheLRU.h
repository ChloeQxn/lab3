#include <comp421/filesystem.h>
#include <comp421/iolib.h>
#include <stdio.h>

#define BLOCK_HASH_SIZE 32
#define INODE_HASH_SIZE 16
typedef struct cacheInode {
	int dirty; //1 means dirty, 0 means clean.
	int inode_num;
	struct cacheInode *pre;
	struct cacheInode *next;
	struct inode *inode;
}cacheInode;

typedef struct cacheBlock {
	int dirty; //1 means dirty, 0 means clean.
	int block_num;
	struct cacheBlock *pre;
	struct cacheBlock *next;
	void *data;
}cacheBlock;

typedef struct hashNode {
	int key;
	cacheBlock *inode;
	struct hashBlockNode *next;
}hashNode;

typedef struct hashBlockNode {
	int key;
	cacheBlock *block;
	struct hashBlockNode *next;
}hashBlockNode;