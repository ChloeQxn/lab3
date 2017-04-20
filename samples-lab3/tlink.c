#include <stdio.h>

#include <comp421/yalnix.h>
#include <comp421/iolib.h>

int
main()
{
	int status;

	status = MkDir("/dir");
	printf("mkdir /dir status %d\n", status);

	status = MkDir("/dir2");
	printf("mkdir /dir2 status %d\n", status);

	status = MkDir("/dir/dir3");
	printf("mkdir /dir/dir3 status %d\n", status);

	status = ChDir("/dir");
	printf("Create status %d\n", status);

	status = ChDir("dir3");
	printf("Create status %d\n", status);

	status = ChDir("/dir3");
	printf("Create status %d\n", status);

	status = ChDir("dir2");
	printf("Create status %d\n", status);

	status = ChDir("/dir2");
	printf("Create status %d\n", status);

	status = Create("/a");
	printf("Create status %d\n", status);

	status = Link("/a", "/b");
	printf("Link status %d\n", status);

	Shutdown();
}
