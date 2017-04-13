#include "cacheLRU.h"

struct cacheInode *cacheHead;
struct cacheInode *cacheTail;
struct hashNode *hash_table[INODE_HASH_SIZE];
struct cacheBlock *cacheBlockHead;
struct cacheBlock *cacheBlockTail;
struct hashBlockNode *hash_block_table[BLOCK_HASH_SIZE];
int inode_cache_len = 0;
int block_cache_len = 0;

hashNode* initHash() {

}
/*
 * Get an inode from hash
 */
cacheInode* get_hash_inode(int i) {
	TracePrintf(0, "Get hash inode\n");
    int idx = i % INODE_HASH_SIZE;
    hashNode *tmp = hash_table[idx];
    while (tmp!=NULL) {
        if (tmp->key==i){          
            return tmp->inode;
        }
        tmp = tmp->next;
    }
    return NULL;
}
/*
 * Read inode from cache
 */
struct inode* readInodeCache(int i) {
//	
	TracePrintf(0, "Read inode cache...\n");
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
	TracePrintf(0, "Get block from hash\n");
	int idx = b % BLOCK_HASH_SIZE;
    hashBlockNode *tmp = hash_block_table[idx];
    while (tmp!=NULL) {
        if (tmp->key==b){          
            return tmp->block;
        }
        tmp = tmp->next;
    }
    return NULL;
}


/*
 * Read block from cache by inode number 
 */
void *readBlockCache(int block_num) {
	TracePrintf(0, "Read block cache\n");
	//int block_num = 1 + (i + 1) / (BLOCKSIZE/INODESIZE);
	cacheBlock *block = get_hash_block(block_num);
	if (block == NULL) {
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
 * Put a block into cache
 */
int putBlockCache(void *data, int block_num) {
	TracePrintf(0, "Put block cache\n");
	if (block_cache_len >= BLOCK_CACHESIZE) {
		cacheBlockTail = cacheBlockTail->pre;
		cacheBlockTail->next = NULL;
	}
	struct cacheBlock *newBlockCache = (struct cacheBlock *)malloc(sizeof(struct cacheBlock*));
	newBlockCache->dirty = 0;
	newBlockCache->block_num = block_num;
	newBlockCache->pre = NULL;
	newBlockCache->next = cacheBlockHead;
	newBlockCache->data = data;

	if (cacheBlockHead != NULL) cacheBlockHead->pre = newBlockCache;
	cacheBlockHead = newBlockCache;
	putBlockHash(newBlockCache);
	TracePrintf(0, "Finished put block cache\n");
	return 0;
}
/*
 * Put a block into hash table
 */
int putBlockHash(struct cacheBlock *block) {
	
	int index = block->block_num % BLOCK_HASH_SIZE;
	TracePrintf(0, "Put block hash %d \n", index);
	struct hashBlockNode *tmp = hash_block_table[index];
	if (tmp == NULL) {
		TracePrintf(0, "This array is null\n");
		struct hashBlockNode *newNode = (struct hashBlockNode *)malloc(sizeof(struct hashBlockNode*));
		newNode->key = block->block_num;
		newNode->block = block;
		newNode->next = NULL;
		tmp = newNode;
		return 0;
	}
	while (tmp -> next != NULL) {
		tmp = tmp->next;
	}
	struct hashBlockNode *newNode;
	newNode->key = block->block_num;
	newNode->block = block;
	newNode->next = NULL;
	tmp->next = newNode;
	return 0;
}
/*
 * Put an inode into cache
 */
int putInodeCache(struct inode *inode, int i) {
	if (inode_cache_len >= INODE_CACHESIZE) {
		cacheTail = cacheTail->pre;
		cacheTail->next = NULL;
	}
	struct cacheInode *newCache = (struct cacheInode *)malloc(sizeof(struct cacheInode*));
		newCache->dirty = 0;
		newCache->inode_num = i;
		newCache->pre = NULL;
		newCache->next = cacheHead;
		newCache->inode = inode;

	if (cacheHead != NULL) {
		cacheHead->pre = newCache;			
	} 
	cacheHead = newCache;
	putInodeHash(newCache);
	return 0;
}

int putInodeHash(struct cacheInode *inode) {
	int index = inode->inode_num % INODE_HASH_SIZE;
	hashNode *tmp = hash_table[index];
	if (tmp == NULL) {
		struct hashNode *newNode = (struct hashNode *)malloc(sizeof(struct hashNode *));
		newNode->key = inode->inode_num;
		newNode->inode = inode;
		newNode->next = NULL;
		tmp = newNode;
		return 0;
	}
	while (tmp -> next != NULL) {
		tmp = tmp->next;
	}
	struct hashNode *newNode;
	newNode->key = inode->inode_num;
	newNode->inode = inode;
	newNode->next = NULL;
	tmp->next = newNode;
	return 0;
}

