#include <stdio.h>
#include <string.h>
#include <stdlib.h>

const char *text = "[TEST_OK]";

int main(int argc, char **argv)
{
	char *buffer;
	int i, dim, text_size;
	FILE *out;
	
	if ( argc < 2 ) {
	error:
		fprintf(stderr, "Usage: %s BLKSIZE\n", argv[0]);
		fprintf(stderr, "BLKSIZE = 1 (128b), 2 (1Kb), 3 (2Kb) 4 (4Kb)\n");
		return 1;
	}
	
	switch ( argv[1][0] ) {
		case '1': dim = 128;  break;
		case '2': dim = 1024; break;
		case '3': dim = 2048; break;
		case '4': dim = 4096; break;
		default: goto error;
	}
	
	text_size = strlen(text);
	
	buffer = malloc(dim*2+1);
	memcpy( buffer, text, i=text_size);
	
	while (i < dim*2 - text_size) {
		if (i == dim-2) {
			memcpy(buffer+i, text, text_size);
			i += text_size;
		}
		else
			buffer[i++] = 'A' + ( rand() % ('Z'-'A'+1) );
	}
	
	memcpy(buffer+i, text, text_size);
	buffer[dim*2] = '\n';
	
	out = fopen("out.txt", "w");
	
	fprintf(out, "append /f1 ");
	fwrite(buffer, 1, dim*2+1, out);
	fprintf(out, "fread /f1 %d %d\n", 0, text_size);
	fprintf(out, "fread /f1 %d %d\n", dim-2, text_size);
	fprintf(out, "fread /f1 %d %d\n", i, text_size);
	fprintf(out, "quit\n");
	
	return 0;
}

