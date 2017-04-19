#include <stdio.h>

#include <comp421/yalnix.h>
#include <comp421/iolib.h>

int
main()
{
	int status;

	status = Create("/a");
	printf("Create status %d\n", status);
	status = Create("/a");
	printf("Create status %d\n", status);
	status = Open("/a");
	printf("open status %d\n", status);



	status = Create("/a/b");
	printf("Create status %d\n", status);

	// status = Link("/a", "/b");
	// printf("Link status %d\n", status);

	Shutdown();
}
