
#include <sys/types.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include "xdr.h"
#include <pthread.h>

#include "usqld-protocol.h"

struct {
  char * name;
  unsigned int p_type;
} protocol_elem_names[] = {
  {"PICKLE_CONNECT", PICKLE_CONNECT},
  {"PICKLE_DISCONNECT", PICKLE_DISCONNECT},
  {"PICKLE_QUERY", PICKLE_QUERY},
  {"PICKLE_ERROR", PICKLE_ERROR},
  {"PICKLE_OK", PICKLE_OK },
  {"PICKLE_STARTROWS", PICKLE_STARTROWS},
  {"PICKLE_ROW", PICKLE_ROW},
  {"PICKLE_EOF", PICKLE_EOF},
  {"PICKLE_MAX", PICKLE_MAX},
  {NULL,0x0}
};


char * usqld_find_packet_name(int packet_type){
  int i; 
  for(i= 0;protocol_elem_names[i].name!=NULL;i++){
    if(protocol_elem_names[i].p_type==packet_type)
      return protocol_elem_names[i].name;
  }
  return NULL;
}

/*
  a lock to prevent simultaneous instantiation of the protocol
 */
pthread_mutex_t protocol_schema_lock = PTHREAD_MUTEX_INITIALIZER;

/**
   the one and only instance of the usqld protocol
 */
XDR_schema * usqld_protocol_schema = NULL;

/**
   creates a new instance of the usqld protcol
 */
XDR_schema * usqld_make_protocol(){
  XDR_schema * protocol,
    *connect_packet,
    *disconnect_packet,
    *query_packet,
    *start_rows_packet,
    *rows_packet,
    *eof_packet,
    *error_packet;
  
  XDR_schema * connect_elems[2];
  XDR_schema * err_elems[2];
  
  XDR_union_discrim  elems[8];
  
  elems[0].t = eof_packet = XDR_schema_new_void();
  elems[0].d = PICKLE_EOF;

  elems[1].t = rows_packet = XDR_schema_new_array(XDR_schema_new_string(),0);
  elems[1].d = PICKLE_ROW;
  
  elems[2].t = start_rows_packet = 
    XDR_schema_new_array(XDR_schema_new_string(),0);
  elems[2].d = PICKLE_STARTROWS;

  elems[3].t = query_packet = XDR_schema_new_string();
  elems[3].d = PICKLE_QUERY;
  
  elems[4].t = disconnect_packet = XDR_schema_new_void();
  elems[4].d = PICKLE_DISCONNECT;

  connect_elems[1] = connect_elems[0] = XDR_schema_new_string();  
  elems[5].t  = connect_packet = XDR_schema_new_struct(2,connect_elems);
  elems[5].d = PICKLE_CONNECT; 
  
  err_elems[0] = XDR_schema_new_uint();
  err_elems[1] = XDR_schema_new_string();
  elems[6].t = error_packet = XDR_schema_new_struct(2,err_elems);
  elems[6].d =PICKLE_ERROR;

  elems[7].t = XDR_schema_new_void();
  elems[7].d = PICKLE_OK;
  
  protocol = XDR_schema_new_type_union(8,elems);   
  
  return protocol;
}

/**
   returns an instance of the usqld protocol
  we only bother to instantiate the protocol once per session
  as it is read-only and can never be freed anyway.
  this does rather assume pthreads...
 */
XDR_schema * usqld_get_protocol(){
   assert(0==pthread_mutex_lock(&protocol_schema_lock));
   {  
     if(usqld_protocol_schema==NULL)
       usqld_protocol_schema=usqld_make_protocol();
   }
   assert(0==pthread_mutex_unlock(&protocol_schema_lock));
   return usqld_protocol_schema;
}

/**
   recieves a protocol packet using the usqld protocol schema
   
   fd : a file descriptor to read from
   packet where to put the recieved packet will always be NULL
   if the function returns anything  byt USQLD_OK;
   
   return value: 
   one of the possible XDR error codes for deserialization
 */
int usqld_recv_packet(int fd,XDR_tree ** packet){
   int rv = XDR_deserialize_elem(usqld_get_protocol(),fd,packet);
   if(rv==USQLD_OK)
     {
       fprintf(stderr,"got a %s packet\n",
	       usqld_find_packet_name(
		 XDR_t_get_union_disc(XDR_TREE_COMPOUND(*packet))));
       
       XDR_tree_dump(*packet);
     }else
     {
	fprintf(stderr,"packet recieve failed with code %d\n",rv);
     }
   
   return rv;
}

int usqld_send_packet(int fd,XDR_tree* packet){

  fprintf(stderr,"about to send a %s packet\n",
	  usqld_find_packet_name(XDR_t_get_union_disc(XDR_TREE_COMPOUND(packet))));
   XDR_tree_dump(packet);
  return XDR_serialize_elem(usqld_get_protocol(),packet,fd);
}

int usqld_get_packet_type(XDR_tree*packet){
  return XDR_t_get_union_disc(XDR_TREE_COMPOUND(packet));
}

usqld_packet * usqld_error_packet(int errcode, const char * str){
  XDR_tree * p;
  XDR_tree * ep[2];
  ep[0] =XDR_tree_new_uint(errcode);
  ep[1] = XDR_tree_new_string(str);

  p = XDR_tree_new_union(PICKLE_ERROR,
			 XDR_tree_new_struct(2,ep));
  return p;
}
usqld_packet * usqld_ok_packet(){
  XDR_tree * p;
  p = XDR_tree_new_union(PICKLE_OK,XDR_tree_new_void());
  return p;
}
