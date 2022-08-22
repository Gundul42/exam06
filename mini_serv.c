/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   mini_serv.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: graja <graja@student.42wolfsburg.de>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/08/21 13:24:43 by graja             #+#    #+#             */
/*   Updated: 2022/08/22 11:17:34 by graja            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/*
Write a program that will listen for client to connect on a certain port on 127.0.0.1 
and will let clients to speak with each other

This program will take as first argument the port to bind to
If no argument is given, it should write in stderr "Wrong number of arguments" 
followed by a \n and exit with status 1
If a System Calls returns an error before the program start accepting connection, it should 
write in stderr "Fatal error" followed by a \n and exit with status 1
If you cant allocate memory it should write in stderr "Fatal error" followed by a \n 
and exit with status 1

Your program must be non-blocking but client can be lazy and if they don't read your message 
you must NOT disconnect them...

Your program must not contains #define preproc
Your program must only listen to 127.0.0.1
The fd that you will receive will already be set to make 'recv' or 'send' to block if select hasn't 
be called before calling them, but will not block otherwise. 

When a client connect to the server:
- the client will be given an id. the first client will receive the id 0 and each new client 
will received the last client id + 1
- %d will be replace by this number
- a message is sent to all the client that was connected to the server: 
"server: client %d just arrived\n"

clients must be able to send messages to your program.
- message will only be printable characters, no need to check
- a single message can contains multiple \n
- when the server receive a message, it must resend it to all the other client with "client %d: " 
before every line!

When a client disconnect from the server:
- a message is sent to all the client that was connected to the server: "server: client %d just left\n"

Memory or fd leaks are forbidden

To help you, you will find the file main.c with the beginning of a server and maybe some useful 
functions. (Beware this file use forbidden functions or write things that must not be there in 
your final program)

Warning our tester is expecting that you send the messages as fast as you can. Don't do 
un-necessary buffer.

Allowed functions: write, close, select, socket, accept, listen, send, recv, bind, strstr, malloc, realloc, free, calloc, bzero, atoi, sprintf, strlen, exit, strcpy, strcat, memset
*/
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>

void	fatal(int fd)
{
	write(2, "Fatal error\n", 12);
	close (fd);
	exit (1);
}

int extract_message(char **buf, char **msg)
{
	char	*newbuf;
	int	i;

	*msg = 0;
	if (*buf == 0)
		return (0);
	i = 0;
	while ((*buf)[i])
	{
		if ((*buf)[i] == '\n')
		{
			newbuf = calloc(1, sizeof(*newbuf) * (strlen(*buf + i + 1) + 1));
			if (newbuf == 0)
				return (-1);
			strcpy(newbuf, *buf + i + 1);
			*msg = *buf;
			(*msg)[i + 1] = 0;
			*buf = newbuf;
			return (1);
		}
		i++;
	}
	return (0);
}

char *str_join(char *buf, char *add)
{
	char	*newbuf;
	int		len;

	if (buf == 0)
		len = 0;
	else
		len = strlen(buf);
	newbuf = malloc(sizeof(*newbuf) * (len + strlen(add) + 1));
	if (newbuf == 0)
		return (0);
	newbuf[0] = 0;
	if (buf != 0)
		strcat(newbuf, buf);
	free(buf);
	strcat(newbuf, add);
	return (newbuf);
}


int main(int argc, char **argv) 
{
	int sockfd, connfd, len, port, bytes, maxfd;
	struct sockaddr_in servaddr, cli;
	fd_set	readfd, readyfd;

	if (argc != 2)
	{
			write(2, "Wrong number of arguments\n", 26);
			exit (1);
	}
	port = atoi(argv[1]);
	// socket create and verification 
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1)
			fatal(sockfd);
	bzero(&servaddr, sizeof(servaddr)); 

	// assign IP, PORT 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(port); 
  
	// Binding newly created socket to given IP and verification 
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0)
			fatal(sockfd);
	if (listen(sockfd, 10) != 0)
			fatal(sockfd);
	maxfd = sockfd + 1;
	FD_ZERO(&readfd);
	FD_SET(sockfd, &readfd);
	while (1)
	{
		readyfd = readfd;
		bytes = select(maxfd, &readyfd, NULL, NULL, NULL);
		if (bytes < 0)
				fatal(sockfd);
		if (bytes)
			printf("%d bytes sent\n", bytes);
		else
			printf("---\n");
		for (int i = 0; i < maxfd; i++)
		{
				if (FD_ISSET(i, &readyfd))
				{
						if (i == sockfd)
						{
							//if i equals sockfd we have a new connection !
							len = sizeof(cli);
							connfd = accept(sockfd, (struct sockaddr *)&cli, &len);
							if (connfd < 0)
							{ 
						        printf("server acccept failed...\n"); 
					   		    fatal(sockfd); 
						    }
							printf("We have new connection from %d\n", connfd);
							FD_SET(connfd, &readfd);
							maxfd++;
						}
						else
						{
							//do something with connection
							printf("%d) send us some data\n", i);
							FD_CLR(i, &readyfd);
						}
				}
		}
	}
	return (0);
}
