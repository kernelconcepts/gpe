#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h> 
#include <string.h>
#include <pthread.h>
#include <sqlite.h>
#include <assert.h>
#include <netdb.h>
#include <stdlib.h>

#include "xdr.h"
#include "usql.h"
#include "usqld-protocol.h"


struct usqld_conn{
  int server_fd;
  int awaiting_interrupted;
};

void usqld_interrupt(usqld_conn * conn){
  XDR_tree * interrupt_packet;
  assert(conn!=NULL);
  if(conn->server_fd==-1)
    return;
  
  interrupt_packet = XDR_tree_new_union(PICKLE_INTERRUPT,
					XDR_tree_new_void());
  
  usqld_send_packet(conn->server_fd,interrupt_packet);
  XDR_tree_free(interrupt_packet);
  conn->awaiting_interrupted =1;
  
}


struct usqld_conn * usqld_connect(const char * server,
				  const char * database,
				  char ** errmsg){
  usqld_packet * p = NULL;
  XDR_tree *elems;
  
  struct sockaddr_in saddr;   
  struct usqld_conn* new_conn;
  struct hostent * he;
  int sock_fd;
  int rv;
  
  saddr.sin_family = PF_INET;
  saddr.sin_port = htons(USQLD_SERVER_PORT);
   
  saddr.sin_addr.s_addr = inet_addr(server);
  if(saddr.sin_addr.s_addr == INADDR_NONE)
    {
      
      if(NULL==(he=gethostbyname(server))|| 
	 NULL==he->h_addr){
	*errmsg = strdup("host name lookup failure");
	return NULL;
      }
      
	
      memcpy(&saddr.sin_addr,
	     *he->h_addr_list,
	     sizeof(struct in_addr));
      
    }
  
   sock_fd = socket(PF_INET,SOCK_STREAM,0);
   if(sock_fd==-1){
      *errmsg = strdup("unable to allocate local socket");    
      return NULL;
   }
   
   if(0!=connect(sock_fd,(struct sockaddr*)&saddr,sizeof(saddr))){
      *errmsg = strdup("unable to connect socket to server");    
      return NULL;
   }
   
   
   assert(USQLD_OK==XDR_tree_new_compound(XDR_STRUCT,
					  2,
					  (XDR_tree_compound**)&elems));
   
   XDR_t_set_comp_elem(XDR_TREE_COMPOUND(elems),0,
		       XDR_tree_new_string(USQLD_PROTOCOL_VERSION));
   XDR_t_set_comp_elem(XDR_TREE_COMPOUND(elems),1,
		       XDR_tree_new_string((char *)database));
   
   p = XDR_tree_new_union(PICKLE_CONNECT,
			  elems);
   
   if(USQLD_OK!=usqld_send_packet(sock_fd,
				  p)){
     *errmsg = strdup("usqld network: error while sending connect");
     XDR_tree_free(p);
     return NULL;
   }
   fprintf(stderr,"connect sent\n");
   XDR_tree_free(p);  
   fprintf(stderr,"waiting for reply\n");
   if(USQLD_OK!=(rv=usqld_recv_packet(sock_fd,
				      &p))){
     *errmsg = strdup("usqld network: error while receiving response");    
     return NULL;
   }
   fprintf(stderr,"got a reply\n");
   switch(usqld_get_packet_type(p)){
   case PICKLE_ERROR:
     {
      unsigned int errcode;
      char * error_str;
      errcode = XDR_t_get_uint
        (XDR_TREE_SIMPLE
	 (XDR_t_get_comp_elem
	  (XDR_TREE_COMPOUND(XDR_t_get_union_t
			     (XDR_TREE_COMPOUND(p))),0)));
      
      error_str = XDR_t_get_string
	(XDR_TREE_STR((XDR_t_get_comp_elem
		       (XDR_TREE_COMPOUND(XDR_t_get_union_t
					  (XDR_TREE_COMPOUND(p))),1)))); 
      *errmsg = error_str;
      XDR_tree_free(p);
      return NULL;
    }
  case PICKLE_OK:
    {
      new_conn = XDR_malloc(struct usqld_conn);
      assert(new_conn!=NULL);
      new_conn->server_fd = sock_fd;
      new_conn->awaiting_interrupted = 0;
      XDR_tree_free(p);
      return new_conn;
    }
  default:
    {
      *errmsg = strdup("protocol:umm huh?");
      XDR_tree_free(p);
      return NULL;
    }
  }
  
  return NULL;
}


/*
  fairly icky hack probably will freeze, 
  might work
 */
int usqld_exec(
  struct usqld_conn*con,                      /* An open database */
  const char *sql,              /* SQL to be executed */
  usqld_callback cb,              /* Callback function */
  void * arg,                       /* 1st argument to callback function */
  char **errmsg                 /* Error msg written here */
  ){

  usqld_packet * out_p = NULL,*in_p = NULL;
  int rv,complete;
  char ** heads=  NULL,**rowdata = NULL;
  unsigned int nrows = 0;

  if(con->server_fd==-1){
    *errmsg =strdup("Connection does not seem to be open");
    return SQLITE_ERROR;
  }
  
  out_p = XDR_tree_new_union(PICKLE_QUERY,
			 XDR_tree_new_string((char*)sql));
  
  if(USQLD_OK!=(rv=usqld_send_packet(con->server_fd,out_p))){
    *errmsg = strdup("usqld network: unable to send query packet");
    XDR_tree_free(out_p);
    return rv;
  }

  XDR_tree_free(out_p);
  

  complete = 0;

  while(!complete || con->awaiting_interrupted){
    if(USQLD_OK!=(rv=usqld_recv_packet(con->server_fd,&in_p))){
      *errmsg = strdup("usqld network: unable to get a response from query");
      return rv;
    }
    
    switch(usqld_get_packet_type(in_p)){
    case PICKLE_OK:
      complete =1;
      break;
    case PICKLE_INTERRUPTED:
      con->awaiting_interrupted =0;
      complete =1;
      rv = SQLITE_INTERRUPT;
      break;
    case PICKLE_STARTROWS:
      {
	int i;
	nrows = XDR_t_get_comp_len
	  (XDR_TREE_COMPOUND(XDR_t_get_union_t(XDR_TREE_COMPOUND(in_p))));
	
	heads = XDR_mallocn(char*,nrows);
	bzero(heads,sizeof(char *)*nrows);

	rowdata = XDR_mallocn(char*,nrows);
	 assert(rowdata!=NULL);
	bzero(rowdata,sizeof(char *)*nrows);
	
	for(i =0;i<nrows;i++){
	  heads[i] = strdup(XDR_t_get_string
			    (XDR_TREE_STR
			     (XDR_t_get_comp_elem
			      (XDR_TREE_COMPOUND
			       (XDR_t_get_union_t
				(XDR_TREE_COMPOUND(in_p))),i))));
	}	
      }
      break;
    case PICKLE_ROW:
      {

	if(!heads || !rowdata){
	  rv = USQLD_PROTOCOL_ERROR;
	  *errmsg = strdup("usqld protocol: received rows without heads!");
	  complete =1;
	  break;
	}
	
	if(nrows!=XDR_t_get_comp_len(XDR_TREE_COMPOUND
				     (XDR_t_get_union_t
				      (XDR_TREE_COMPOUND(in_p))))){
	  rv = USQLD_PROTOCOL_ERROR;
	  *errmsg = strdup("usqld protocol: number of rows != number of heads, wierd");
	  break;
	}
	
	if(cb!=NULL && !con->awaiting_interrupted){ //HACK HACK HACK should actually interrupt on protocol
	  
	  int i;
	  for(i =0;i<nrows;i++){
	    rowdata[i] = XDR_t_get_string
	      (XDR_TREE_STR
	       (XDR_t_get_comp_elem
		(XDR_TREE_COMPOUND
		 (XDR_t_get_union_t
		  (XDR_TREE_COMPOUND(in_p))),i)));
	  }
	  
	  if(0!=cb(arg,nrows,rowdata,heads)){
	    rv = SQLITE_ABORT;
	    if(!con->awaiting_interrupted)
	      usqld_interrupt(con);

				     
	  }
	}
	bzero(rowdata,sizeof(char *) * nrows);
      }
      break;
     case PICKLE_ERROR:
	 {
	   complete =1;
	   *errmsg = strdup(XDR_t_get_string(XDR_TREE_STR(XDR_t_get_comp_elem(XDR_TREE_COMPOUND(XDR_t_get_union_t(XDR_TREE_COMPOUND(in_p))),1))));
	   
	   rv =XDR_t_get_uint
	     (XDR_TREE_SIMPLE
	      (XDR_t_get_comp_elem
	       (XDR_TREE_COMPOUND(XDR_t_get_union_t
				  (XDR_TREE_COMPOUND(in_p))),0)));
	   
	 }
	 break;
    default:
      {
	
	rv = USQLD_PROTOCOL_ERROR;
	*errmsg = strdup("usqld protocol: huh? what");
      }
    }
    XDR_tree_free(in_p);
  }
  
  if(heads!=NULL){
    int i;
    for(i = 0;i<nrows;i++){
      if(heads[i]!=NULL)
	free(heads[i]);
    }
    free(heads);
  }
  
  return rv;
}
    

void usqld_disconnect(usqld_conn * con){
  usqld_packet *out_p,*in_p;
 
  out_p= XDR_tree_new_union(PICKLE_DISCONNECT,
			    XDR_tree_new_void());

  if(USQLD_OK!=usqld_send_packet(con->server_fd,out_p)){
    XDR_tree_free(out_p);
    return;
  }
  if(USQLD_OK!=usqld_recv_packet(con->server_fd,&in_p)){
    return;
  }
  XDR_tree_free(in_p);
  return;
  
}


int usqld_complete(const char *zSql){
  int isComplete = 0;
  while( *zSql ){
    switch( *zSql ){
      case ';': {
        isComplete = 1;
        break;
      }
      case ' ':
      case '\t':
      case '\n':
      case '\f': {
        break;
      }
      case '[': {
        isComplete = 0;
        zSql++;
        while( *zSql && *zSql!=']' ){ zSql++; }
        if( *zSql==0 ) return 0;
        break;
      }
      case '\'': {
        isComplete = 0;
        zSql++;
        while( *zSql && *zSql!='\'' ){ zSql++; }
        if( *zSql==0 ) return 0;
        break;
      }
      case '"': {
        isComplete = 0;
        zSql++;
        while( *zSql && *zSql!='"' ){ zSql++; }
        if( *zSql==0 ) return 0;
        break;
      }
      case '-': {
        if( zSql[1]!='-' ){
          isComplete = 0;
          break;
        }
        while( *zSql && *zSql!='\n' ){ zSql++; }
        if( *zSql==0 ) return isComplete;
        break;
      } 
      default: {
        isComplete = 0;
        break;
      }
    }
    zSql++;
  }
  return isComplete;
}


/*
** Wrapper for sqlite_last_insert_rowid
*/
int usqld_last_insert_rowid(usqld_conn *con)
{
  usqld_packet *out_p,*in_p;
  int ret;
  
  out_p= XDR_tree_new_union(PICKLE_REQUEST_ROWID,
			    XDR_tree_new_void());
  
  if(USQLD_OK!=usqld_send_packet(con->server_fd,out_p)){
    XDR_tree_free(out_p);
    return -1;
  }
  if(USQLD_OK!=usqld_recv_packet(con->server_fd,&in_p)){
    return -1;
  }
  
  ret = XDR_t_get_uint(XDR_TREE_SIMPLE(XDR_t_get_comp_elem(XDR_TREE_COMPOUND(in_p),1)));
  
  XDR_tree_free(in_p);
  return ret;
}
