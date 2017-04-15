#define OPEN 16
#define CLOSE 1
#define CREATE 2
#define READ 3
#define WRITE 4
#define SEEK 5
#define LINK 6
#define UNLINK 7
#define SYMLINK 8
#define READLINK 9
#define MKDIR 10
#define RMDIR 11
#define CHDIR 12
#define STAT 13
#define SYNC 14
#define SHUTDOWN 15
// #include <comp421/filesystem.h>
/*
 * First field should be request type 4 bytes
 * Open:
 * Leave one address field for pathname type void * 8bytes
 * Read:
 * Leave a field for buf address type void * 8bytes
 * Leave a field of inode number type int 4 bytes
 * Leave a field of current postion type int 4 bytes
 * Leave a field of size type int 4 bytes
 */
typedef struct my_msg {
	int type; /*4bytes*/
	int data1;
	int data2;
	int data3;
	void *addr1;
	void *addr2; 

} my_msg;