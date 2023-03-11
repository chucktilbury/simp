# Makefile for the grammar so that it can be tested against actual
# source code.
TARGET	=	grammar

all: $(TARGET)

%.o:%.c
	gcc -Wall -Wextra -Wno-unused-variable -Wno-sign-compare -c -g -o $@ $<

$(TARGET): parser.o scanner.o
	gcc -g -o $(TARGET) parser.o scanner.o

parser.o: parser.c parser.h
scanner.o: scanner.c

scanner.c: scanner.l
	flex -o scanner.c scanner.l

parser.c: parser.y
	bison -dvo parser.c parser.y

clean:
	-rm -f parser.o scanner.o parser.c parser.h parser.output $(TARGET)