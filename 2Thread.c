#include <zlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

struct buffer {
	char buf[1025]; //buffer for data
	int needsWriting; //flag representing if the data is translated but not yet written
};

struct buffer rbuf[10];
int readingDone = 0;
int writingDone = 0;

void *tRead(void *inFile){
	gzFile *in = inFile;
	int bufIndx = 0;
	int bytes_read;
	int i;
	while(!gzeof(*in)){
		if(rbuf[bufIndx].needsWriting == 0){
			bytes_read = gzread(*in, rbuf[bufIndx].buf, sizeof(rbuf[bufIndx].buf) - 1);
			rbuf[bufIndx].buf[bytes_read] = '\0';
			rbuf[bufIndx].needsWriting = 1;
			//printf("%s", rbuf[bufIndx].buf); //prints the contents of buffer
			bufIndx++;
			bufIndx = bufIndx % 10;
		}
	}
	readingDone = 1;
}
	
void *tWrite(void *outFile){
	gzFile *out = outFile;
	int bufIndx = 0;
	int i;
	int found;
	while(!readingDone || !writingDone){
		if(readingDone == 1){
			found = 0;	
			for(i = 0; i < 10; i++){
				if(rbuf[i].needsWriting) found = 1;
			}
			if(!found) writingDone = 1;
		}
		if(rbuf[bufIndx].needsWriting == 1){
			for(i = 0; i < strlen(rbuf[bufIndx].buf); i++){
				switch(rbuf[bufIndx].buf[i]) {
					case 'a':
					case 'A':
						rbuf[bufIndx].buf[i] = '4'; break;		
					case 'e':
					case 'E':
						rbuf[bufIndx].buf[i] = '3'; break;	
					case 'i':
					case 'I':
						rbuf[bufIndx].buf[i] = '1'; break;	
					case 'o':
					case 'O':
						rbuf[bufIndx].buf[i] = '0'; break;	
					case 's':
					case 'S':
						rbuf[bufIndx].buf[i] = '5'; break;	
				}
			}
			gzwrite(*out, rbuf[bufIndx].buf, strlen(rbuf[bufIndx].buf));
			rbuf[bufIndx].needsWriting = 0;
			//printf("%s", rbuf[bufIndx].buf); //prints the contents of buffer
			if(++bufIndx == 10) bufIndx = 0;
		}
	}	
}

int main(int argc, char **argv){
	pthread_t readThread, writeThread;

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

	pthread_create(&readThread, NULL, tRead, &in);
	pthread_create(&writeThread, NULL, tWrite, &out);
	pthread_join(readThread, NULL);
	pthread_join(writeThread, NULL);

	gzclose(in);
	gzclose(out);
	
		
}

