all: client_SMTP

client_smtp: client_SMTP.o
	gcc -Wall -o client_SMTP client_SMTP.o

client_smtp.o: client_SMTP.c
	gcc -Wall -c client_SMTP.c -o client_SMTP.o

clean:
	rm -r *.o
