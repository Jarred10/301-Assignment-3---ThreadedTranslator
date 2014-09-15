#include <zlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

//buffers
char rbuf[10][1024];
char wbuf[10][1024];

//mutexes for each buffer
pthread_mutex_t rmutex[10];
pthread_mutex_t wmutex[10];

//conditions for reading and writing
pthread_cond_t rcond;
pthread_cond_t wcond;

//flags for reading and translating
int readingDone = 0;
int translatingDone = 0;

//read method for thread
void *tRead(void *inFile){
	gzFile *in = inFile;
	int bufIndx = 0;
	int bytes_read, i;

	//loops for all input
	for(;!gzeof(*in);){
		//lock on mutex
		pthread_mutex_lock(&rmutex[bufIndx]);
		//if current buffer isnt empty, cond_wait until it is signalled
		if(rbuf[bufIndx][0] != '\0')
			pthread_cond_wait(&rcond, &rmutex[bufIndx]);
		//reads into the buffer and sets the null byte at the end of string
		bytes_read = gzread(*in, rbuf[bufIndx], sizeof(rbuf[bufIndx]) - 1);
		rbuf[bufIndx][bytes_read] = '\0';
		//signals translator that there is data to be translated and releases mutex
		pthread_cond_signal(&rcond);
		pthread_mutex_unlock(&rmutex[bufIndx]);
	
		bufIndx = (bufIndx + 1) % 10;
		
	}

	readingDone = 1;
}

//translate method for thread
void *tTranslate(void *param){
	int bufIndx = 0;
	int i, length;
	//infinite loop
	while(1){
		//obtains mutex on current buffer
		pthread_mutex_lock(&rmutex[bufIndx]);

		//if the buffer is empty
		if(rbuf[bufIndx][0] == '\0'){
			//if reading is done, set flag and return out of method. no need to close reading buffer mutex
			if(readingDone){
				translatingDone = 1;		
				return(0);
			}
			//if reading wasnt done, wait until buffer is signalled to have data
			pthread_cond_wait(&rcond, &rmutex[bufIndx]);
		}
		//get mutex on write buffer, if it isn't empty wait for writer to signal that it is
		pthread_mutex_lock(&wmutex[bufIndx]);
		if(wbuf[bufIndx][0] != '\0')
			pthread_cond_wait(&wcond, &wmutex[bufIndx]);
		
		//copies contents of read buffer into write buffer
		strcpy(wbuf[bufIndx], rbuf[bufIndx]);
		
		//find length
		length = strlen(wbuf[bufIndx]);
		//loop through all characters in buffer, changing letters into numbers
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
		
		//empties read buffer
		rbuf[bufIndx][0] = '\0';	
		
		//signals reader there is an empty read buffer incase it was waiting
		pthread_cond_signal(&rcond);
		//signals writer there is an fill write buffer incase it was waiting
		pthread_cond_signal(&wcond);
		//unlocks all mutexes
		pthread_mutex_unlock(&rmutex[bufIndx]);
		pthread_mutex_unlock(&wmutex[bufIndx]);

		bufIndx = (bufIndx + 1) % 10;
			

	}	
}

//write method for thread
void *tWrite(void *outFile){
	gzFile *out = outFile;
	int bufIndx = 0;

	while(1){		
		
		pthread_mutex_lock(&wmutex[bufIndx]);
		//if write buffer isnt empty
		if(wbuf[bufIndx][0] == '\0'){
			//if translating is done, all writing is finished
			if(translatingDone) return(0);
			//other wait for the buffer to be filled
			pthread_cond_wait(&wcond, &wmutex[bufIndx]);
		}
		//write the contents of buffer
		gzwrite(*out, wbuf[bufIndx], strlen(wbuf[bufIndx]));

		//effectively empties the writing array
		wbuf[bufIndx][0] = '\0'; 
		
		//signals translater that buffer is empty and unlocks mutex
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
	
	//check for both parameters
	if(!argv[1] || !argv[2]){
		fprintf(stderr, "no argument for filename of input or output\n");
		return(1);
	}
	
	//try to open first argument, report errors if fails
	in = gzopen(argv[1], "rb");
	if(!in){
		fprintf(stderr, "gzopen of '%s' failed.\n", argv[1]);
		return(1);
	}

	//try to open second argument, report errors if fails
	out = gzopen(argv[2], "wb");
	if(!out){
		fprintf(stderr, "gzopen of '%s' failed.\n", argv[2]);
		return(1);
	}
	
	//initializes all mutexes for read and write
	for(i = 0; i < 10; i++){
		pthread_mutex_init(&rmutex[i], NULL);
		pthread_mutex_init(&wmutex[i], NULL);
	}
	
	//initializes the cond variables
	pthread_cond_init(&rcond, NULL);
	pthread_cond_init(&wcond, NULL);
	
	//creates all three threads
	pthread_create(&readThread, NULL, tRead, &in);
	pthread_create(&translateThread, NULL, tTranslate, NULL);
	pthread_create(&writeThread, NULL, tWrite, &out);
	
	//joins all three threads, waiting for all three to finish before moving past here
	pthread_join(readThread, NULL);
	pthread_join(translateThread, NULL);
	pthread_join(writeThread, NULL);
	
	//closes both input and output
	gzclose(in);
	gzclose(out);

	//destroys all condition variables
	pthread_cond_destroy(&rcond);
	pthread_cond_destroy(&wcond);

	//destroys all mutexes
	for(i = 0; i < 10; i++){
		pthread_mutex_destroy(&rmutex[i]);
		pthread_mutex_destroy(&wmutex[i]);
	}
	
	
		
}

