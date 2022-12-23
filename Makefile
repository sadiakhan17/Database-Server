
all: dbserver dbclient

dbserver: dbserver.c
	gcc dbserver.c -o dbserver -Wall -Werror -std=gnu99 -pthread

dbclient: dbclient.c
	gcc dbclient.c -o dbclient -Wall -Werror -std=gnu99 -pthread

clean:
	rm -f dbserver.o dbclient.o
