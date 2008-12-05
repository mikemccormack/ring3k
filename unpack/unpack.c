#include <stdio.h>
#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include "mspack.h"

void strlower( char *str )
{
	while (*str)
	{
		*str = tolower( *str );
		str++;
	}
}

int main(int argc, char **argv)
{
	struct mscab_decompressor* decomp;
	struct mscabd_cabinet *cab;
	struct mscabd_file *file;
	char *cabfile;
	char targetfile[1024];
	char *targetdir = "";
	int n, len;

	for (n=1; n<argc; n++)
	{
		if (argv[n][0] == '-' && argv[n][1] == 'd' && (n+1) < argc)
			targetdir = argv[++n];
		else
			break;
	}

	if (n == argc)
	{
		fprintf(stderr, "unpack [-d directory] filename\n");
		exit(1);
	}

	cabfile = argv[n];

	decomp = mspack_create_cab_decompressor( NULL );
	if (!decomp)
	{
		fprintf(stderr, "failed to create decompressor\n");
		exit(1);
	}

	cab = decomp->open( decomp, cabfile );
	if (!cab)
	{
		fprintf(stderr, "failed to open %s\n", cabfile );
		exit(1);
	}

	file = cab->files;
	if (file->next)
	{
		fprintf(stderr, "unpack doesn't handle multiple files\n");
		exit(1);
	}

	len = strlen( targetdir );
	if (len)
	{
		strcpy( targetfile, targetdir );
		if (targetfile[len-1] != '/')
			targetfile[len++] = '/';
	}
	strcpy( targetfile+len, file->filename );
	strlower( targetfile+len );

	decomp->extract( decomp, file, targetfile );

	decomp->close( decomp, cab );

	return 0;
}
