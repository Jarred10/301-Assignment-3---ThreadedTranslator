#include <zlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char rbuf[10][1025];

int main(int argc, char **argv){

	gzFile in;
	gzFile out;
	
	int i;

	if(!argv[1] || !argv[2]){
		fprintf(stderr, "no argument for filename of input or output\n");
		return(1);
	}

	in = gzopen(argv[1], "rb");
	if(!in){
		fprintf(stderr, "gzopen of '%s' failed.\n", argv[1]);
		return(1);
	}

	out = gzopen(argv[2], "wb");
	if(!out){
		fprintf(stderr, "gzopen of '%s' failed.\n", argv[2]);
		return(1);
	}

	int bufIndx = 0;
	int bytes_read;
	while(!gzeof(in)){
		bytes_read = gzread(in, rbuf[bufIndx], sizeof(rbuf[bufIndx]) - 1);
		rbuf[bufIndx][bytes_read] = '\0';
		for(i = 0; i < bytes_read; i++){
			switch(rbuf[bufIndx][i]) {
				case 'a':
				case 'A':
					rbuf[bufIndx][i] = '4'; break;	
				case 'e':
				case 'E':
					rbuf[bufIndx][i] = '3'; break;	
				case 'i':
				case 'I':
					rbuf[bufIndx][i] = '1'; break;	
				case 'o':
				case 'O':
					rbuf[bufIndx][i] = '0'; break;	
				case 's':
				case 'S':
					rbuf[bufIndx][i] = '5'; break;	
			}
		}
		gzwrite(out, rbuf[bufIndx], strlen(rbuf[bufIndx]));
		if(++bufIndx == 10) bufIndx = 0;
	}
	gzclose(in);
	gzclose(out);
	
		
}

