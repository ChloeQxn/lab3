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

	status = RmDir("/abc");
	printf("rmdir /abd status %d\n", status);

	// status = RmDir("/dir/dir3");
	// printf("rmdir /dir/dir3 status %d\n", status);

	status = ChDir("/dir");
	printf("chdir /dir status %d\n", status);

	status = ChDir("dir3");
	printf("ChDir dir3 status %d\n", status);

	status = ChDir("/dir3");
	printf("chdir /dir3 status %d\n", status);

	status = ChDir("dir2");
	printf("chdir dir2 status %d\n", status);

	status = ChDir("/dir2");
	printf("ChDir /dir2 status %d\n", status);

	status = Create("/a");
	printf("Create /a status %d\n", status);

	status = Link("/a", "/b");
	printf("Link status %d\n", status);

	status = Open("/b");
	printf("open /b status %d\n", status);
	Shutdown();

	status = Unlink("/b");
	printf("unLink /b status %d\n", status);

	status = Open("/b");
	printf("open /b status %d\n", status);
	Shutdown();

	status = Unlink("/a");
	printf("Link status %d\n", status);

	status = Open("/a");
	printf("open /a status %d\n", status);
	Shutdown();

}
