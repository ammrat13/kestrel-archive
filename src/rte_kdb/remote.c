/* $Id: remote.c,v 1.9 2001/06/13 22:47:56 ericp Exp $ */
/* 
 * Kestrel Run Time Environment 
 * Copyright (C) 1998 Regents of the University of California.
 * 
 * remote.c
 */

#include "globals.h"
#include "remote.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include "interface.h"

#define SERV_TCP_PORT 6543
#define SERV_HOST_ADDR_1 "128.114.63.29"  
#define SERV_HOST_ADDR_2 "128.114.55.100" 

int checkOpenConnection(int sockfd)
{
  int status;
  unsigned char buf[10];
  int ret;

  read_from_kestrel = sockfd;
  write(sockfd, Put_in_Pipe(STATUS_REG_ADDRESS,READ),	PIPE_WIDTH);
  ret = KestrelRead(buf, PIPE_WIDTH);
  if (ret == 0) {
    return 1;
  }
  return 0;
}

int InitializeSocket(int machine_select)
{
   int sockfd, returnedValue;
   struct sockaddr_in serv_addr_1;
   struct sockaddr_in serv_addr_2;
   char temp[10];

   bzero((char*) &serv_addr_1, sizeof(serv_addr_1));
   serv_addr_1.sin_family = AF_INET;
   serv_addr_1.sin_addr.s_addr = inet_addr(SERV_HOST_ADDR_1);
   serv_addr_1.sin_port = htons(SERV_TCP_PORT);

   bzero((char*) &serv_addr_2, sizeof(serv_addr_2));
   serv_addr_2.sin_family = AF_INET;
   serv_addr_2.sin_addr.s_addr = inet_addr(SERV_HOST_ADDR_2);
   serv_addr_2.sin_port = htons(SERV_TCP_PORT);


   if (machine_select == 0 || machine_select == 1) {
     if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
       ErrorPrint("InitializeSocket", "can't open socket to kestrel server");
       return 0;
     }
     fcntl(sockfd, F_SETFD, O_NONBLOCK);
     returnedValue = connect(sockfd, (struct sockaddr *) &serv_addr_1, sizeof(serv_addr_1));
     if (returnedValue < 0 || (checkOpenConnection(sockfd))) {
       close(sockfd);
       if (machine_select != 0) {
	 ErrorPrint("InitializeSocket", "can't connect to kestrel server");
	 return 0;
       }
     } else {
       sprintf(print_msg, "Connected to kestrel server %s at port %d.\n", 
	       SERV_HOST_ADDR_1, SERV_TCP_PORT);

       ScreenPrint(print_msg);
       read_from_kestrel = sockfd;
       write_to_kestrel = sockfd;
       return (1);
     }
   }
   
   if (machine_select == 0 || machine_select == 2) {
     if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
       ErrorPrint("InitializeSocket", "can't open socket to kestrel server");
       return 0;
     }
     returnedValue = connect(sockfd, (struct sockaddr *) &serv_addr_2, sizeof(serv_addr_2));

     if (returnedValue < 0 ||  (checkOpenConnection(sockfd))) {
       ErrorPrint("InitializeSocket", "can't connect to kestrel server");
       return 0;
     } else {
       sprintf(print_msg, "Connected to kestrel server %s at port %d.\n", 
	       SERV_HOST_ADDR_2, SERV_TCP_PORT);

       ScreenPrint(print_msg);
       read_from_kestrel = sockfd;
       write_to_kestrel = sockfd;
       return (1);   
     }
   }

   return 0;
}
