# Compilador
CC = gcc

# Flags de compilação
CFLAGS = -Wall

# Executáveis
EXECUTABLES = uspsh ep1

# Alvo padrão
all: $(EXECUTABLES)

# Compilar uspsh
uspsh: uspsh.c
	$(CC) $(CFLAGS) -o uspsh uspsh.c -lreadline

# Compilar ep1
ep1: ep1.c
	$(CC) $(CFLAGS) -o ep1 ep1.c
