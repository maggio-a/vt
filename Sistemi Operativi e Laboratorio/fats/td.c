#include <stdio.h>
#include <stdlib.h>

#define MAX_LEN_NAME 8 

struct dir_entry {
    char used;
    char name[MAX_LEN_NAME + 1];
    char attr;
    unsigned int index;
    unsigned int len;
};

int main(int argc, char **argv)
{
	int numentries, size, i;
	FILE *input, *ok;

	if (argc != 2 ) {
		fprintf(stderr, "Uso %s blksize", argv[0]);
		return 1;
	}

	size = atoi( argv[1] );
	numentries = size / sizeof(struct dir_entry);
	numentries = numentries + 3;

	printf("sizeof(struct dir_entry) == %lu, size == %d, numentries == %d\n",
		   sizeof(struct dir_entry), size, numentries);

	input = fopen("test.input", "w+");
	if (!input) {
		perror("fopen");
		return 1;
	}

	ok = fopen("test.ok", "w+");
	if (!ok) {
		perror("fopen");
		return 1;
	}

	fprintf(input, "mkdir /dir1\nmkfile /f1\nls /\n");
	fprintf(ok, ".\t..\tdir1\tf1\n");

	fprintf(ok, ".\t..\t");
	for (i = 0; i < numentries; i++) {
		fprintf(input, "mkdir /dir1/sd%d\n", i);
		fprintf(ok, "sd%d\t", i);
	}
	
	fprintf(input, "mkfile /dir1/f2\n");
	fprintf(ok, "f2\n");

	fprintf(input, "ls /dir1\nquit\n");

	return 0;
}