#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h> 
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <signal.h>

#include "usqld-protocol.h"
#include "usqld-server.h"
#include "usqld-conhandler.h"
#include "xdr.h"
#include "usql.h"
#include "sqlite.h"

void print_usage();

int main(int argc, char * argv[])
{
   FILE * pid_file = NULL;
   struct sockaddr_in myaddr,from_addr;   
   int ssock_fd;
   usqld_config *conf;
   int ssop;

   int single_thread =0;
   char * database_dir = NULL;
   char * pid_filename = NULL;

   int c;
   
   while((c = getopt(argc,argv,"xd:p:h"))!=EOF){
      switch(c){	 
       case 'x':
	 single_thread = 1;
	 break;
       case  'd':
	 database_dir = optarg;
	 break;
       case  'p':
	 pid_filename = optarg;
	 break;
       case  'h':
	 print_usage();
	 exit(1);
	 break;
       case '?':
	 if (isprint (optopt))
	   fprintf (stderr, "Unknown option -%c'.\n", optopt);
	 else
	   fprintf (stderr,
		    "Unknown option character \\x%x'.\n",
		    optopt);
	 break;
       default:
	 printf("unknown error while parsing command line command was %c\n",c);
	 exit(1);
      }
   }
   
   
	
   if(!database_dir)
     database_dir = USQLD_DATABASE_DIR;
   if(!pid_filename)
     pid_filename = USQLD_PID_FILE;

   printf("single-thread mode is :%s\n",(single_thread?"on":"off"));
   printf("database directory is:%s\n",database_dir);

   conf = XDR_malloc(usqld_config);
   conf->db_base_dir = strdup(database_dir);
   
   if(-1==(ssock_fd = socket(PF_INET,SOCK_STREAM,0)))
     {
       fprintf(stderr,"couldn't create socket\n");
       exit(1);
     }

   myaddr.sin_family = PF_INET;
   myaddr.sin_port = htons(USQLD_SERVER_PORT);
   myaddr.sin_addr.s_addr = INADDR_ANY;
   ssop = 1;
   setsockopt(ssock_fd,SOL_SOCKET,SO_REUSEADDR,(char *)&ssop,sizeof(int));
   if(-1==bind(ssock_fd,(struct sockaddr * ) &myaddr,sizeof(myaddr)))
      {
 	fprintf(stderr,"couldn't bind socket to port\n");
	exit(1);
      }
   
   listen(ssock_fd,5);
   printf("listening on port %d\n",USQLD_SERVER_PORT);
	
   
   pid_file =fopen(pid_filename,"w");
   if(NULL==pid_file)
     {
	fprintf(stderr,"couldn't open pid file: %s\n",pid_filename);
	exit(1);
     }
   
   //fork away from the terminal 
   //for the time being we keep stdout and stderr open 
   //for spurious nonsense
   if(!single_thread)
     {
	if(fork()!=0)
	  exit(0);
	else
	  {
	     fprintf(pid_file,"%u\n",getpid());
	     fclose(pid_file);
	  }
     }
   
   
   while(1)
     {
	
	int csock;
	socklen_t sz = sizeof(from_addr);
	pthread_t new_thread;
	usqld_conhand_init * new_thread_init;
	csock =accept(ssock_fd, (struct sockaddr *) &from_addr,&sz);
	printf("got connection spawning thread\n");
	new_thread_init = XDR_malloc(usqld_conhand_init);
	new_thread_init->fd = csock;
	new_thread_init->config = conf;
	if(!single_thread){
	  pthread_create(&new_thread,
			 NULL,
			 (void* (*) (void*)) usqld_conhandler_main,
			 (void*)new_thread_init);
	}else{
	  usqld_conhandler_main(new_thread_init);
	}
     }
}


void print_usage()
{
   printf("usqld [-d <dir>] [-p <pidfile>] [-x] [-h]\n"
	  "\t uSQL daemon version %s\n"
	  "\t protocol version %s\n"
	  "\t built against sqlite %s\n"
	  "\t -d <dir>  - specifies database directory. default:%s\n"
	  "\t -p <pidfile> - where to keep the pidfile. default:%s\n"
	  "\t -x  - run in single threaded mode (don't detatch from termina)\n"
	  "\t -h print this message\n",
	  USQLD_VERSION,
	  USQLD_PROTOCOL_VERSION,
	  sqlite_version,
	  USQLD_DATABASE_DIR,
	  USQLD_PID_FILE);
	  
}
