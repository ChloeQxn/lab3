/**
 * This is a test program
 */

#include <stdio.h>
#include <comp421/yalnix.h>
#include <comp421/hardware.h>
// #include "global_yfs.h"
#include <stdlib.h>
#include <comp421/iolib.h>

int main(int argc, char **argv){

  TracePrintf(0, "This is test program main \n");

  printf("\n\n");
  MkDir("dir1");
  printf("\n\n");
  MkDir("dir1/dir2");
  printf("\n\n");
  MkDir("/dir1/dir2/dir3");
  printf("\n\n");
  Create("/dir1/dir2/dir3/file1");
  printf("\n\n");

  printf("\n\n");
  MkDir("dir11");
  printf("\n\n");
  MkDir("dir11/dir22");
  printf("\n\n");
  MkDir("/dir11/dir22/dir33");
  printf("\n\n");
  Create("/dir11/dir22/dir33/file2");
  printf("\n\n");

  Link("dir1/dir2/dir3","dir11/dir22/dir33/file3");
  printf("\n\n");
  Link("dir1/dir2/dir3/file1","dir11/dir22/dir33/file2");
  printf("\n\n");
  Link("dir1/dir2/dir3/file1","dir11/dir22/dir33/file3");
  printf("\n\n");
  Unlink("dir11/dir22/dir33/file3");
  printf("\n\n");

  SymLink("dir3/file1", "dir1/dir2/file1-alias");
  printf("\n\n");

  int file = Open("dir1/dir2/dir3/file1");

  printf("open file %d \n\n", file);

  char * buf = (char*)malloc(sizeof(char)*1024);
  int len1 = Write(file, "abcdefg",7);
  int len2 = Write(file, "abcdefg",20);
  printf("write len %d \n\n", len2);

  int loc = Seek(file, 0, SEEK_SET);
  printf("loc is %d\n\n", loc);
  memset(buf, '\0', 1024);
  int len3 = Read(file, buf, 7);

  printf("len3 is %d\n\n", len3);
  printf("buf is %s\n\n", buf);

  Close(file);
  loc = Seek(file, 0, SEEK_SET);
  printf("loc is %d\n\n", loc);

  memset(buf, '\0', 1024);
  len3 = ReadLink("dir1/dir2/file1-alias", buf, 100);
  printf("len3 is %d\n\n", len3);
  printf("buf is %s\n\n", buf);

  Shutdown();

  char *path = argv[1];
  printf("pathname: (%s)\n", path);
  Stat(path, NULL);




  Exit(0);
  return 0;

}