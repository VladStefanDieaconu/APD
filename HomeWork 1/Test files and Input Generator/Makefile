all:  inputGenerator compareOutputs

compareOutputs: compareOutputs.c
	gcc -o compareOutputs compareOutputs.c -O3 -lm -Wall

inputGenerator: inputGenerator.c
	gcc -o inputGenerator inputGenerator.c -O3 -lm -Wall

clean:
	rm inputGenerator compareOutputs