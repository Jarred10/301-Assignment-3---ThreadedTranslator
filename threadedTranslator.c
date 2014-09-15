#include <zlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

char rbuf[10][1024];
char wbuf[10][1024];
pthread_mutex_t rmutex[10];
pthread_mutex_t wmutex[10];

pthread_cond_t rcond;
pthread_cond_t wcond;

int readingDone = 0;
int translatingDone = 0;
int writingDone = 0;

void *tRead(void *inFile){
	gzFile *in = inFile;
	int bufIndx = 0;
	int bytes_read, i;

	for(;!gzeof(*in);){
		pthread_mutex_lock(&rmutex[bufIndx]);
		if(rbuf[bufIndx][0] != '\0')
			pthread_cond_wait(&rcond, &rmutex[bufIndx]);
		bytes_read = gzread(*in, rbuf[bufIndx], sizeof(rbuf[bufIndx]) - 1);
		rbuf[bufIndx][bytes_read] = '\0';
		pthread_cond_signal(&rcond);
		pthread_mutex_unlock(&rmutex[bufIndx]);
	
		bufIndx = (bufIndx + 1) % 10;
		
	}

	readingDone = 1;
}

void *tTranslate(void *param){
	int bufIndx = 0;
	int i, length;
	while(1){
		pthread_mutex_lock(&rmutex[bufIndx]);

		if(rbuf[bufIndx][0] == '\0'){
			if(readingDone){
				translatingDone = 1;		
				return(0);
			}
			pthread_cond_wait(&rcond, &rmutex[bufIndx]);
		}
		pthread_mutex_lock(&wmutex[bufIndx]);
		if(wbuf[bufIndx][0] != '\0')
			pthread_cond_wait(&wcond, &wmutex[bufIndx]);

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
		
		rbuf[bufIndx][0] = '\0';	
	
		pthread_cond_signal(&rcond);
		pthread_cond_signal(&wcond);
		pthread_mutex_unlock(&rmutex[bufIndx]);
		pthread_mutex_unlock(&wmutex[bufIndx]);

		bufIndx = (bufIndx + 1) % 10;
			

	}	
}
void *tWrite(void *outFile){
	gzFile *out = outFile;
	int bufIndx = 0;

	while(1){		
		
		pthread_mutex_lock(&wmutex[bufIndx]);

		if(wbuf[bufIndx][0] == '\0'){
			if(translatingDone){
				writingDone = 1;
				return(0);
			}
			pthread_cond_wait(&wcond, &wmutex[bufIndx]);
		}
		gzwrite(*out, wbuf[bufIndx], strlen(wbuf[bufIndx]));

		wbuf[bufIndx][0] = '\0'; //effectively empties the writing array

		pthread_cond_signal(&wcond);
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

	pthread_cond_init(&rcond, NULL);
	pthread_cond_init(&wcond, NULL);
	
	pthread_create(&readThread, NULL, tRead, &in);
	pthread_create(&translateThread, NULL, tTranslate, NULL);
	pthread_create(&writeThread, NULL, tWrite, &out);

	pthread_join(readThread, NULL);
	pthread_join(translateThread, NULL);
	pthread_join(writeThread, NULL);

	gzclose(in);
	gzclose(out);

	pthread_cond_destroy(&rcond);
	pthread_cond_destroy(&wcond);
	
	
		
}

