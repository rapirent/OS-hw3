#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>     /* getopt, ssize_t */
#include "status.h"
#include <pthread.h>
#include <netinet/in.h>
#include <errno.h>

#define BACKLOG 20 //queue length for waiting connection
#define BUF_SIZE 2048
#define STACK_MAX 1024


struct dns_data{
    int ip1;
    int ip2;
    int ip3;
    int ip4;
    char domain[BUF_SIZE];
};
struct dns_data dns_stack[STACK_MAX];
int stack_top = 0;
pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;
void *server_thread(void *socket);
int main(int argc, char *argv[])
{
    const char *server = "127.0.0.1", *port = "12345";
    int arg_opt;
    while((arg_opt = getopt(argc, argv, "hs:p:")) != -1) {
            switch (arg_opt) {
                    case 's':
                            server = optarg;
                            break;
                    case 'p':
                            port = optarg;
                            break;
                    default:
                            fprintf(stderr,
                                    "Usage: %s [-s server] [-p port]\n"
                                    "  -s server: specify the server name or address, default: 127.0.0.1\n"
                                    "  -p port: specify the server port, default: 12345\n",
                                    argv[0]);
                            exit(EXIT_FAILURE);
            }
    }
	struct sockaddr_storage their_addr;
	socklen_t addr_size;
	struct addrinfo hints, *res,*p;
	int sockfd, new_fd;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	int s,yes=1;
	if((s=getaddrinfo(NULL, port, &hints, &res))!=0)
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
		exit(1);
	}
	// printf("%s\n", port);
	// if((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol))==-1)
	// {
	// 	fprintf(stderr, "init create socket error!\n");
	// 	exit(1);
	// }
//
	for(p = res; p != NULL; p = p->ai_next)
	{
		if((sockfd = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1)
		{
			perror("server: socket");
			continue;
    	}

	    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,sizeof(int)) == -1)
	    {
			perror("setsockopt");
			exit(1);
	    }

	    if(bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
	    {
			close(sockfd);
			perror("server: bind");
		    continue;
    	}
	    break;
	}

	if(p == NULL)
	{
	    fprintf(stderr, "server: failed to bind\n");
	    exit(1);
	}
	//
	// if(bind(sockfd, res->ai_addr, res->ai_addrlen)==-1)
	// {
	// 	fprintf(stderr, "bind error:%d!\n",errno);
	// 	exit(1);
	// }
	freeaddrinfo(res); // 全部都用這個 structure
	if(listen(sockfd, BACKLOG) == -1 )
	{
		fprintf(stderr, "listen error:%d!\n",errno);
		exit(1);
	}
	pthread_t thread_t;
	int thread_fd;
	while(1)
	{
		printf("\nwaiting for a new connection......\n");
		addr_size = sizeof(their_addr);
		if((new_fd= accept(sockfd, (struct sockaddr*)&their_addr, &addr_size)) == -1 )
		{
			fprintf(stderr, "\naccept a connection error!\n");
			exit(1);
		}
		printf("\nthere is a new connection!\n");
		if(thread_fd=pthread_create(&thread_t, NULL, server_thread, (void*)&new_fd)<0)
		{
			fprintf(stderr, "\ncreate new thread error!\n");
			exit(1);
		}
	}
	close(sockfd);
	return 0;
}
void *server_thread(void *socket)
{
	// printf("server thread created\n");
	int socket_fd = *((int*)socket);
	int recv_len;
	char buf[BUF_SIZE];
	// int recv(int sockfd, void *buf, int len, int flags);
	// char domain[BUF_SIZE],ip_address[BUF_SIZE];
	while(read(socket_fd, &recv_len, sizeof(size_t))>0)
	{
		// read(socket_fd, buf, BUF_SIZE, 0)!=-1
		// recv_len
		if(read(socket_fd, buf, recv_len)==-1)
		{
			fprintf(stderr,"recv error:%d!\n",errno);
			exit(1);
		}
		size_t size_request, size_response;
		printf("buf=%s\n",buf);
		char domain[BUF_SIZE]={'\0'},ip_address[BUF_SIZE]={'\0'},opt[BUF_SIZE]={'\0'};
		sscanf(buf,"%s %s %s",opt,domain,ip_address);
		if(strncmp(opt,"SET",3)==0)
		{
			// char domain[BUF_SIZE]={'\0'},ip_address[BUF_SIZE]={'\0'};
			// sprintf(buf,"%s %s %s",opt,domain,ip_address);
			int ip1=-1,ip2=-1,ip3=-1,ip4=-1;
			sscanf(ip_address,"%d.%d.%d.%d",&ip1,&ip2,&ip3,&ip4);
			if((ip1>=256||ip1<0)||(ip2>=256||ip2<0)||(ip3>=256||ip3<0)||(ip4>=256||ip4<0))
			{
				char *msg = "400 \"Bad Request\"";
				size_t len = printf("%s", msg);
				printf("\n");
				// len--;
				printf("send_size = %u\n",len);
				if(write(socket_fd, &len, sizeof(size_t)) == -1)
				{
					fprintf(stderr,"send error, part1!%d\n",errno);
					exit(1);
				}
				if(write(socket_fd, msg, len)==-1)
				{
					fprintf(stderr,"send error, part2!%d\n",errno);
					exit(1);
				}
			}
			else
			{
				int count=0,domain_len=strlen(domain),founded = 0;
				pthread_mutex_lock(&mutex);
				for(;count<stack_top;count++)
				{
					if(strncmp(dns_stack[count].domain, domain, domain_len)==0)
					{
						founded = 1;
						break;
					}
				}
				if(founded == 0)
				{
					// stack_top++;
					dns_stack[stack_top].ip1=ip1;
					dns_stack[stack_top].ip2=ip2;
					dns_stack[stack_top].ip3=ip3;
					dns_stack[stack_top].ip4=ip4;
					strncpy(dns_stack[stack_top].domain,domain,domain_len);
					stack_top++;
				}
				else if(founded == 1)
				{
					dns_stack[count].ip1=ip1;
					dns_stack[count].ip2=ip2;
					dns_stack[count].ip3=ip3;
					dns_stack[count].ip4=ip4;
				}
				pthread_mutex_unlock(&mutex);
				char *msg = "200 \"Ok\"";
				size_t len = printf("%s", msg);
				printf("\n");
				// len--;
				printf("send_size = %u\n",len);
				if(write(socket_fd, &len, sizeof(size_t)) == -1)
				{
					fprintf(stderr,"send error, part1!%d\n",errno);
					exit(1);
				}
				if(write(socket_fd, msg, len)==-1)
				{
					fprintf(stderr,"send error, part2!%d\n",errno);
					exit(1);
				}
			}
		}
		else if(strncmp(opt, "GET", 3)==0)
		{
			int count=0,domain_len=strlen(domain),founded = 0;
			pthread_mutex_lock(&mutex);
			for(;count<stack_top;count++)
			{
				if(strncmp(dns_stack[count].domain, domain, domain_len)==0)
				{
					founded = 1;
					break;
				}
			}
			pthread_mutex_unlock(&mutex);
			if(founded == 0)
			{
				char *msg = "404 \"Not Found\"";
				size_t len = printf("%s", msg);
				printf("\n");
				// len--;
				printf("send_size = %u\n",len);
				if(write(socket_fd, &len, sizeof(size_t)) == -1)
				{
					fprintf(stderr,"send error, part1!%d\n",errno);
					exit(1);
				}
				if(write(socket_fd, msg, len)==-1)
				{
					fprintf(stderr,"send error, part2!%d\n",errno);
					exit(1);
				}
			}
			else if(founded == 1)
			{
				char msg[BUF_SIZE]={'\0'};
				pthread_mutex_lock(&mutex);
				sprintf(msg, "200 \"OK\" %d.%d.%d.%d",dns_stack[count].ip1,dns_stack[count].ip2,dns_stack[count].ip3,dns_stack[count].ip4);
				pthread_mutex_unlock(&mutex);
				size_t len = printf("%s", msg);
				printf("\n");
				// len--;
				printf("send_size = %u\n",len);
				if(write(socket_fd, &len, sizeof(size_t)) == -1)
				{
					fprintf(stderr,"send error, part1!%d\n",errno);
					exit(1);
				}
				if(write(socket_fd, msg, len)==-1)
				{
					fprintf(stderr,"send error, part2!%d\n",errno);
					exit(1);
				}
			}
		}
		else if(strncmp(opt, "INFO", 4)==0)
		{
			pthread_mutex_lock(&mutex);
			char msg[BUF_SIZE]={'\0'};
			sprintf(msg, "200 \"OK\" %d",stack_top);
			pthread_mutex_unlock(&mutex);
			size_t len = printf("%s", msg);
			printf("\n");
			// len--;
			printf("send_size = %u\n",len);
			if(write(socket_fd, &len, sizeof(size_t)) == -1)
			{
				fprintf(stderr,"send error, part1!%d\n",errno);
				exit(1);
			}
			if(write(socket_fd, msg, len)==-1)
			{
				fprintf(stderr,"send error, part2!%d\n",errno);
				exit(1);
			}
		}
		else
		{
			char *msg = "405 \"Method Not Allowed\"";
			size_t len = printf("%s", msg);
			printf("\n");
			// len--;
			printf("send_size = %u\n",len);
			if(write(socket_fd, &len, sizeof(size_t)) == -1)
			{
				fprintf(stderr,"send error, part1!%d\n",errno);
				exit(1);
			}
			if(write(socket_fd, msg, len)==-1)
			{
				fprintf(stderr,"send error, part2!%d\n",errno);
				exit(1);
			}
			//error
		}
		memset(buf, 0, sizeof(buf));
		memset(&recv_len, 0, sizeof(recv_len));
	}
	return NULL;
}