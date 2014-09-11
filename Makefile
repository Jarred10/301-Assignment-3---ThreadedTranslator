all:
	gcc -o threadedTranslator threadedTranslator.c -lz -pthread
1:
	gcc -o 1Thread 1Thread.c -lz
2:
	gcc -o 2Thread 2Thread.c -lz -pthread
