/*
* Program for COMP 521 lab3
* Author: Chunxiao Zhang, Xiangning Qi
*/

/*
 * Start small, and add features and test as you go.
 * For example, the first thing you need to be able to do is to start the server
 * and have it read the disk to build its internal list of free blocks and free inodes.
 * So start with that.  Then, maybe try Open (you can Open("/"), for example).
 * Once you have Open, try Link (it's pretty simple).
 * Then maybe add Read.  Just keep adding features one at a time, testing as you go.

 */
#include <comp421/filesystem.h>
#include <comp421/iolib.h>
#include <stdio.h>
int main() {
	return 0;
	int i;
	// read from 
	void *buff = malloc(SECTORSIZE);
	for (int i = 0; i < NUMSECTORS; i++) {
		ReadSector(i, buff);
		printf(buff);
	}
} 

