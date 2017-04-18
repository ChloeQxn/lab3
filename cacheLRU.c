#include "cacheLRU.h"
#include <stdlib.h>
#include <comp421/filesystem.h>
#include <comp421/iolib.h>
#include <comp421/yalnix.h>

struct cacheInode *cacheHead = NULL;
struct cacheInode *cacheTail = NULL;

struct cacheBlock *cacheBlockHead = NULL;
struct cacheBlock *cacheBlockTail = NULL;

struct hashNode *hash_table[INODE_HASH_SIZE];
//hash_table = (struct hashNode *)malloc(INODE_HASH_SIZE * sizeof(hashNode));
struct hashBlockNode *hash_block_table[BLOCK_HASH_SIZE];
// hash_block_table = (struct hashBlockNode *)malloc(BLOCK_HASH_SIZE * sizeof(hashBlockNode));
int inode_cache_len = 0;
int block_cache_len = 0;
void initCache() {
	// TracePrintf
	// int i = 0;
	// for (; i < INODE_HASH_SIZE; i ++) {
	// 	hash_table[i] = (hashNode *)malloc(sizeof(hashNode));
	// 	hash_table[i]->key = 0;
	// 	hash_table[i]->inode = NULL;
	// 	hash_table[i]->next = NULL;
	// }
	// for (i=0; i<BLOCK_HASH_SIZE; i ++) {
	// 	hash_block_table[i] = (hashBlockNode *)malloc(sizeof(hashBlockNode));
	// 	hash_block_table[i]->key = 0;
	// 	hash_block_table[i]->block = NULL;
	// 	hash_block_table[i]->next = NULL;
	// }
	
}
/*
 * Get an inode from hash
 */
cacheInode* get_hash_inode(int i) {
    int idx = i % INODE_HASH_SIZE;
    hashNode *tmp = hash_table[idx];
    while (tmp!=NULL) {
        if (tmp->key==i){          
            return tmp->inode;
        }
        tmp = tmp->next;
    }
    TracePrintf(0, "inode not in hash %d\n", idx);
    return NULL;
}
/*
 * Read inode from cache
 */
struct inode* readInodeCache(int i) {
	//struct cacheInode *inode ;
	struct cacheInode *inode = get_hash_inode(i);
	if (inode == NULL) {
		TracePrintf(0, "This inode is not in the cache\n");
		return NULL; //no such inode stored in cache previously
	}
	if (inode == cacheHead) return cacheHead->inode;
	else if (inode == cacheTail) {
		cacheTail = inode->pre;
		cacheTail->next = NULL;
	}
	inode->pre->next = inode->next;
	inode->pre = NULL;
	inode->next = cacheHead;
	cacheHead = inode;
    return cacheHead->inode;
}

/*
 * Get a block from hash
 */
cacheBlock* get_hash_block(int b) {
	int idx = b % BLOCK_HASH_SIZE;
    hashBlockNode *tmp = hash_block_table[idx];
    while (tmp!=NULL) {
    	TracePrintf(0, "Not null\n");
        if (tmp->key==b){          
            return tmp->block;
        }
        tmp = tmp->next;
    }
    TracePrintf(0, "null %d\n", b);
    return NULL;
}

/*
 * Read block from cache by inode number 
 */
void *readBlockCache(int block_num) {
	//int block_num = 1 + (i + 1) / (BLOCKSIZE/INODESIZE);
	cacheBlock *block = get_hash_block(block_num);
	if (block == NULL) {
		TracePrintf(0, "This block is not in the cache\n");
		return NULL; //no such inode stored in cache previously
	}
	if (block == cacheBlockHead) return cacheBlockHead->data;
	else if (block == cacheBlockTail) {
		cacheBlockTail = block->pre;
		cacheBlockTail->next = NULL;
	}
	block->pre->next = block->next;
	block->pre = NULL;
	block->next = cacheBlockHead;
	cacheBlockHead = block;
    return cacheBlockHead->data;
}
/*
 *typedef struct cacheBlock {
	int dirty; //1 means dirty, 0 means clean.
	int block_num;
	struct cacheBlock *pre;
	struct cacheBlock *next;
	void *data;
}cacheBlock;
 */
void writeBackBlock() {
	if (cacheBlockTail->dirty) {
		cacheBlockTail->dirty = 0;
		int blockNum = cacheBlockTail->block_num;
		void *buf = getBlock();
		if (WriteSector(blockNum, buf) == ERROR) {
			perror("error in write back sector");
		}
	}
}
void removeBlockFromHash(cacheBlock *cacheTail) {
	int blockNum = cacheTail->block_num;
	int idx = blockNum % BLOCK_HASH_SIZE;
	hashBlockNode *hashBlock = hash_block_table[idx];
	if (hashBlock->key == blockNum) hash_block_table[idx] = NULL;

	while (hashBlock->next != NULL && hashBlock->next->key != blockNum ) {
		hashBlock = hashBlock->next;
	}

	if (hashBlock->next != NULL && hashBlock->next->key == blockNum) {
		hashBlockNode *tofree = hashBlock->next;
		hashBlock->next = hashBlock->next->next;
		free(tofree);
	} 
}
void removeBlockFromCache() {
	removeBlockFromHash(cacheBlockTail);
	cacheBlockTail = cacheBlockTail->pre;
	free(cacheBlockTail->next);
	cacheBlockTail->next = NULL;
	block_cache_len --;
}
/*
 * Put a block into hash table
 typedef struct hashBlockNode {
	int key;
	cacheBlock *block;
	struct hashBlockNode *next;
}hashBlockNode;
 */
int putBlockHash(struct cacheBlock *block) {
	
	int index = (block->block_num) % BLOCK_HASH_SIZE;
	TracePrintf(0, "Put Block Hash %d \n", index);

	struct hashBlockNode *tmp = hash_block_table[index];
    hash_block_table[index] = (hashBlockNode*)malloc(sizeof(hashBlockNode));
    hash_block_table[index]->key = block->block_num;
    hash_block_table[index]->block = block;
    hash_block_table[index]->next = tmp;
	return 0;
}
/*
 * Put a block into cache
 */
int putBlockCache(void *data, int block_num) {
	TracePrintf(0, "Put block cache %d\n", block_cache_len);
	if (block_cache_len >= BLOCK_CACHESIZE) {
		writeBackBlock();
		removeBlockFromCache();
	}
	struct cacheBlock *newBlockCache = (struct cacheBlock *)malloc(sizeof(struct cacheBlock));
	newBlockCache->dirty = 0;
	newBlockCache->block_num = block_num;
	newBlockCache->pre = NULL;
	newBlockCache->next = cacheBlockHead;
	newBlockCache->data = data;

	if (cacheBlockHead != NULL) cacheBlockHead->pre = newBlockCache;
	cacheBlockHead = newBlockCache;
	block_cache_len ++;
	putBlockHash(cacheBlockHead);
	// TracePrintf(0, "Finished put block cache\n");
	return 0;
}
/*
 *typedef struct cacheInode {
	int dirty; //1 means dirty, 0 means clean.
	int inode_num;
	struct cacheInode *pre;
	struct cacheInode *next;
	struct inode *inode;
}cacheInode;
 */
void writeBackInode() {
	TracePrintf(0, "Write back inode\n");
	if (cacheTail->dirty) {
		cacheTail->dirty = 0;
		int inodeNum = cacheTail->inode_num;
		int blockNum = 1 + inodeNum / (BLOCKSIZE/INODESIZE);
		// void *block = getBlock(blockNum);
		setBlockCacheDirty(blockNum);
	}
}
/*
 *typedef struct cacheBlock {
	int dirty; //1 means dirty, 0 means clean.
	int block_num;
	struct cacheBlock *pre;
	struct cacheBlock *next;
	void *data;
}cacheBlock;
 */
void setBlockCacheDirty(int blockNum) {
	cacheBlock *tmp = cacheBlockHead;
	while (tmp != NULL) {
		if (tmp->block_num == blockNum) {
			tmp->dirty = 1;
		}
		tmp = tmp->next;
	}
}
void removeInodeFromHash() {
	TracePrintf(0, "Remove Inode from hash\n");
	int inodeNum = cacheTail->inode_num;
	TracePrintf(0, "Remove %d Inode from hash\n", inodeNum);
	int idx = inodeNum % INODE_HASH_SIZE;
	hashNode *tmp = hash_table[idx];
	if (tmp->key == inodeNum) hash_table[idx] = NULL;
	while (tmp->next != NULL && tmp->next->key != inodeNum ) {
		tmp = tmp->next;
	}
	if (tmp->next != NULL && tmp->next->key == inodeNum) {
		hashNode *tofree = tmp->next;
		tmp->next = tmp->next->next;
		free(tofree);
	 } 
		//else {
	// 	TracePrintf(0, "This block is not in this hashtable\n");
	// }
}
void removeInodeFromCache() {
	removeInodeFromHash();
	TracePrintf(0, "Remove inode from cache\n");
	cacheTail = cacheTail->pre;
	free(cacheTail->next);
	cacheTail->next = NULL;
	inode_cache_len --;
}

int putInodeHash(struct cacheInode *inode) {
	int index = inode->inode_num % INODE_HASH_SIZE;
	TracePrintf(0, "Put inode Hash index %d \n", index);
	hashNode *tmp = hash_table[index];
	hash_table[index] = (hashNode *)malloc(sizeof(hashNode));
	hash_table[index]->key = inode->inode_num;
	hash_table[index]->inode = inode;
	hash_table[index]->next = tmp;
	return 0;
}
/*
 * Put an inode into cache
 */
int putInodeCache(struct inode *inode, int i) {
	TracePrintf(0, "Put inode Cache %d, current cache size %d \n", i, inode_cache_len);

//check if the cache length exceeds the maximum size
	if (inode_cache_len >= INODE_CACHESIZE) {
		writeBackInode(cacheTail);
		removeInodeFromCache();
	}
	struct cacheInode *newCache = (struct cacheInode *)malloc(sizeof(struct cacheInode));
		newCache->dirty = 0;
		newCache->inode_num = i;
		newCache->pre = NULL;
		newCache->next = cacheHead;
		newCache->inode = inode;

	if (cacheHead != NULL) cacheHead->pre = newCache;	
	
	cacheHead = newCache;
	if (cacheTail != NULL && cacheTail->pre == NULL) cacheTail->pre = newCache;
	if (cacheTail == NULL) cacheTail = newCache;

	inode_cache_len ++;
	putInodeHash(newCache);
	return 0;
}


