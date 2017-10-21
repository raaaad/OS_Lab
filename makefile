mysh : mysh.c
	gcc mysh.c -o mysh
exe : mysh
	./mysh
clean : 
	rm -f mysh
