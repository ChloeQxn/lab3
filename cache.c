#include "cache.h"
#include <comp421/filesystem.h>
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

// get inode from disk
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
} 


/* 
*add a new inode into cache. add it into hash and add it into tail of list
*/
void add_inode_cache(int inum, struct inode* node){
	// if cache is full, delete the head from hash and list
	if (Nsize>=INODE_CACHESIZE) {
		free(Nhead->next);
		Nhead->next = Nhead->next->next;
		Nhead->next->pre= Nhead;
		delete_inode(inum);
		Nsize--;
	}
	// create a Nnode, and add it to list and hash
	struct Nnode* nnode = (struct Nnode*)malloc(sizeof(struct Nnode));
	nnode->node = node;
	add_inode(inum, nnode);
	Ntail->pre->next = nnode;
	nnode->pre = Ntail->pre;
	nnode->next = Ntail;
	Ntail->pre = nnode;
	Nsize++;
}
/* 
*	update Nnode in the list
*/
void update_Nnode_list(struct Nnode *node) {
	// delete it from list
	node->pre->next = node->next;
	node->next->pre = node->pre;
	// put it to the tail
	Ntail->pre->next = node;
	node->pre = Ntail->pre;
	node->next = Ntail;
	Ntail->pre = node;
}



// get inode with given inum, if it is in cache, 
struct inode *get_inode(int inum) {
	TracePrintf(0, "get_inode starts. the inum is %d\n", inum);
	struct my_struct *tmp = find_inode(inum);
	struct inode *node;
	
	// if the inode is not in the cache, read it from the disk
	if (tmp == NULL) {
		TracePrintf(0, "inode of %d is not in cache, read it from disk.", inum);
		node = getInode(inum);
		// put it into cache
		add_inode_cache(inum, node);
	} else {
		// it already in cache, get it from hash, and update its location in the list
		TracePrintf(0, "inode of %d is already in cache.", inum);
		node = tmp->node->node;
		update_Nnode_list(tmp->node);
	}
	return node;
}