#include <zlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

pthread_cond_t read_cv;
pthread_cond_t write_cv;

char rbuf[10][1024];
char wbuf[10][1024];
pthread_mutex_t rmutex[10];
pthread_mutex_t wmutex[10];

int readingDone = 0;
int translatingDone = 0;
int writingDone = 0;

void *tRead(void *inFile){
	gzFile *in = inFile;
	int bufIndx = 0;
	int bytes_read, i;
	for(;!gzeof(*in);){
		pthread_mutex_lock(&rmutex[bufIndx]);
		bytes_read = gzread(*in, rbuf[bufIndx], sizeof(rbuf[bufIndx]) - 1);
		rbuf[bufIndx][bytes_read] = '\0';
		pthread_mutex_unlock(&rmutex[bufIndx]);
		//printf("%s", rbuf[bufIndx]); //prints the contents of buffer
		
		bufIndx = (bufIndx + 1) % 10;
	}
	readingDone = 1;
}

void *tTranslate(void *param){
	int bufIndx = 0;
	int i, length;
	while(1){
		pthread_mutex_lock(&rmutex[bufIndx]);

		if(readingDone && rbuf[bufIndx][0] == '\0') {
			translatingDone = 1;		
			return(0);
		}

		pthread_mutex_lock(&wmutex[bufIndx]);

		strcpy(wbuf[bufIndx], rbuf[bufIndx]);
		
		length = strlen(wbuf[bufIndx]);
		for(i = 0; i < length; i++){
			switch(wbuf[bufIndx][i]) {
				case 'a':
				case 'A':
					wbuf[bufIndx][i] = '4'; break;		
				case 'e':
				case 'E':
					wbuf[bufIndx][i] = '3'; break;	
				case 'i':
				case 'I':
					wbuf[bufIndx][i] = '1'; break;	
				case 'o':
				case 'O':
					wbuf[bufIndx][i] = '0'; break;	
				case 's':
				case 'S':
					wbuf[bufIndx][i] = '5'; break;
			}
		}

		pthread_mutex_unlock(&wmutex[bufIndx]);

		//printf("%s", wbuf[bufIndx]); //prints the contents of buffer

		rbuf[bufIndx][0] = '\0';

		bufIndx = (bufIndx + 1) % 10;

		pthread_mutex_unlock(&rmutex[bufIndx]);

	}	
}
void *tWrite(void *outFile){
	gzFile *out = outFile;
	int bufIndx = 0;

	while(1){		
		
		pthread_mutex_lock(&wmutex[bufIndx]);

		if(translatingDone && wbuf[bufIndx][0] == '\0')	return(0);

		gzwrite(*out, wbuf[bufIndx], strlen(wbuf[bufIndx]));	
		printf("%s", wbuf[bufIndx]); //prints the contents of buffer
		wbuf[bufIndx][0] = '\0'; //effectively empties the writing array
		pthread_mutex_unlock(&wmutex[bufIndx]);

		bufIndx = (bufIndx + 1) % 10;
	}
}

int main(int argc, char **argv){
	pthread_t readThread, translateThread, writeThread;

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

	for(i = 0; i < 10; i++){
		pthread_mutex_init(&rmutex[i], NULL);
		pthread_mutex_init(&wmutex[i], NULL);
	}
	
	pthread_cond_init(&read_cv, NULL);
	pthread_cond_init(&write_cv, NULL);

	pthread_create(&readThread, NULL, tRead, &in);
	pthread_create(&translateThread, NULL, tTranslate, NULL);
	pthread_create(&writeThread, NULL, tWrite, &out);

	pthread_join(readThread, NULL);
	pthread_join(translateThread, NULL);
	pthread_join(writeThread, NULL);

	gzclose(in);
	gzclose(out);
	
		
}

