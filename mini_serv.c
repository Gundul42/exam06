/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   mini_serv.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: graja <graja@student.42wolfsburg.de>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/08/21 13:24:43 by graja             #+#    #+#             */
/*   Updated: 2022/08/31 18:31:42 by graja            ###   ########.fr       */
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

static
void	fatal(int fd)
{
	write(2, "Fatal error\n", 12);
	close (fd);
	exit (1);
}

static
int extract_message(char **buf, char **msg)
{
	char	*newbuf;
	int	i;

	*msg = 0;
	if (*buf == 0)
		return (0);
	i = 0;
	while ((*buf)[i] != '\0')
	{
		if ((*buf)[i] == '\n')
		{
			newbuf = calloc(strlen(*buf)  + 1, sizeof(char));
			if (newbuf == NULL)
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

static
char	*getData(int fd)
{
		int			bytes;
		static char	buf[1025];

		memset(buf, 0, 1024);
		bytes = recv(fd, buf, 1024, 0);
		if (bytes < 0)
			fatal(fd);
		else if (bytes > 0)
			return (buf);
		close (fd);
		return (NULL);
}

static
void sendToAll(int maxfd, fd_set wfd, int current, char *str)
{
		for (int i = 0; i < maxfd; i++)
		{
			if (FD_ISSET(i, &wfd))
			{
				if (i != current)
				{
					write(i, str, strlen(str));
				}
			}
		}
}

int main(int argc, char **argv) 
{
	int sockfd, connfd, len, port, bytes, maxfd;
	struct sockaddr_in servaddr, cli;
	fd_set	rfd, wfd, readyfd;
	char	*str;
	char	*msg;
	char	*buffer;
	int		clients[FD_SETSIZE];
	int		count = 0;

	if (argc != 2)
	{
			write(2, "Wrong number of arguments\n", 26);
			exit (1);
	}
	port = atoi(argv[1]);
	if (port > 65535)
		fatal(-1);
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
	msg = calloc(1024, sizeof(char));
	if (!msg)
		fatal(sockfd);
	memset(&clients, -1, FD_SETSIZE);
	maxfd = sockfd + 1;
	FD_ZERO(&rfd);
	FD_ZERO(&wfd);
	FD_SET(sockfd, &rfd);
	while (1)
	{
		readyfd = rfd;
		bytes = select(maxfd, &readyfd, NULL, NULL, NULL);
		if (bytes < 0)
				fatal(sockfd);
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
			   		    fatal(sockfd);
					count++;
					sprintf(msg,"server: client %d just arrived\n", count);
					sendToAll(maxfd, wfd, i, msg);
					FD_SET(connfd, &rfd); //add to check_for_read array
					FD_SET(connfd, &wfd); //add to check_for_write array
					clients[connfd] = count;
					if (connfd >= maxfd)
						maxfd = connfd + 1;
				}
				else
				{
					// FD is ready for read, so do it baby !
					str = getData(i);
					if (str && strlen(str) > 0) // we got something 
					{
						while (extract_message(&str, &buffer))
						{
							sprintf(msg, "client %d: %s", clients[i], buffer);
							sendToAll(maxfd, wfd, i, msg);
						}
					}
					FD_CLR(i, &readyfd);
					if (!str) // we got NULL that means client quit
					{
						sprintf(msg, "server: client %d just left\n", clients[i]);
						sendToAll(maxfd, wfd, i, msg);
						FD_CLR(i, &rfd); //delete from check_for_read array
						FD_CLR(i, &wfd); //delete from check_for_write array
						if (i == maxfd - 1)
							maxfd--;
						clients[i] = -1;
					}
				}
			}
		}
	}
	return (0);
}
