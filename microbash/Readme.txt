Autore: La Corte Lorenzo - S4784539

Compilabile con make, che esegue il comando:
gcc -Wall -pedantic -Werror microbash.c -lreadline -o microbash

Non dovrebbe presentare alcun errore, ne warning, ne problemi di gestione di memoria dinamica con valgrind.

Questa riconsegna migliora solo la pipe, che prima era implementata solo singolarmente (poi sfruttava la popen), mentre adesso gestisce pipe infinite.