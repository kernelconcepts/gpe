#ifndef USQLD_PROTOCOL_H
#define USQLD_PROTOCOL_H

#include <xdr.h>

#define PICKLE_CONNECT 1
#define PICKLE_DISCONNECT 2
#define PICKLE_QUERY 3
#define PICKLE_ERROR 4
#define PICKLE_OK 5
#define PICKLE_STARTROWS 6
#define PICKLE_ROW 7
#define PICKLE_EOF 8
#define PICKLE_MAX 9

#define USQLD_PROTOCOL_VERSION "USQLD_0.1.0"

#define USQLD_SERVER_PORT 8322

#define USQLD_OK 0 
#define USQLD_ERRBASE 1024
#define USQLD_ALREADY_OPEN (USQLD_ERRBASE + 1)
#define USQLD_NOT_OPEN (USQLD_ERRBASE + 2)
#define USQLD_VERSION_MISMATCH (USQLD_ERRBASE + 3)
#define USQLD_UNSUPPORTED  (USQLD_ERRBASE + 4)
#define USQLD_PROTOCOL_ERROR  (USQLD_ERRBASE + 5)

typedef XDR_tree usqld_packet;

XDR_schema * usqld_get_protocol();
int usqld_recv_packet(int fd,usqld_packet ** packet);
int usqld_send_packet(int fd,usqld_packet* packet);
int usqld_get_packet_type(usqld_packet *packet);

usqld_packet * usqld_error_packet(int errcode, const char * str);
usqld_packet * usqld_ok_packet();


#endif
