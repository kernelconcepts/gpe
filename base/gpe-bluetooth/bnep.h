/*
  BNEP protocol definition for Linux Bluetooth stack (BlueZ).
  Copyright (C) 2002 Maxim Krasnyansky <maxk@qualcomm.com>
	
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
*/

/*
 * $Id$
 */

#ifndef _BNEP_H
#define _BNEP_H

#include <net/ethernet.h>
#include <bluetooth/bluetooth.h>

// UUIDs
#define BNEP_BASE_UUID 0x0000000000001000800000805F9B34FB
#define BNEP_UUID16    0x02
#define BNEP_UUID32    0x04
#define BNEP_UUID128   0x16

#define BNEP_SVC_PANU  0x1115
#define BNEP_SVC_NAP   0x1116
#define BNEP_SVC_GN    0x1117

// Packet types
#define BNEP_GENERAL               0x00
#define BNEP_CONTROL               0x01
#define BNEP_COMPRESSED            0x02
#define BNEP_COMPRESSED_SRC_ONLY   0x03
#define BNEP_COMPRESSED_DST_ONLY   0x04

// Control types
#define BNEP_CMD_NOT_UNDERSTOOD    0x00
#define BNEP_SETUP_CONN_REQ        0x01
#define BNEP_SETUP_CONN_RSP        0x02
#define BNEP_FILTER_NET_TYPE_SET   0x03
#define BNEP_FILTER_NET_TYPE_RSP   0x04
#define BNEP_FILTER_MULT_ADDR_SET  0x05
#define BNEP_FILTER_MULT_ADDR_RSP  0x06

// Response messages 
#define BNEP_SUCCESS               0x00

#define BNEP_CONN_INVALID_DST      0x01
#define BNEP_CONN_INVALID_SRC      0x02
#define BNEP_CONN_INVALID_SVC      0x03
#define BNEP_CONN_NOT_ALLOWED      0x04

#define BNEP_FILTER_UNSUPPORTED_REQ    0x01
#define BNEP_FILTER_INVALID_RANGE      0x02
#define BNEP_FILTER_INVALID_MCADDR     0x02
#define BNEP_FILTER_LIMIT_REACHED      0x03
#define BNEP_FILTER_DENIED_SECURITY    0x04

// L2CAP settings
#define BNEP_MTU         1691
#define BNEP_FLUSH_TO    0xffff
#define BNEP_CONNECT_TO  15
#define BNEP_FILTER_TO   15

#ifndef BNEP_PSM
#define BNEP_PSM	 0x0f
#endif

// Headers 
#define BNEP_TYPE_MASK	 0x7f
#define BNEP_EXT_HEADER	 0x80

struct bnep_setup_conn_req {
	uint8_t  type;
	uint8_t  ctrl;
	uint8_t  uuid_size;
	uint8_t  service[0];
} __attribute__((packed));

struct bnep_set_filter_req {
	uint8_t  type;
	uint8_t  ctrl;
	uint16_t len;
	uint8_t  list[0];
} __attribute__((packed));

struct bnep_control_rsp {
	uint8_t  type;
	uint8_t  ctrl;
	uint16_t resp;
} __attribute__((packed));

struct bnep_ext_hdr {
	uint8_t  type;
	uint8_t  len;
	uint8_t  data[0];
} __attribute__((packed));

/* BNEP ioctl defines */
#define BNEPCONNADD	_IOW('B', 200, int)
#define BNEPCONNDEL	_IOW('B', 201, int)
#define BNEPGETCONNLIST	_IOR('B', 210, int)
#define BNEPGETCONNINFO	_IOR('B', 211, int)

struct bnep_connadd_req {
	int      sock;       // Connected socket
	uint32_t flags;
	uint16_t role;
	char     device[16]; // Name of the Ethernet device
};

struct bnep_conndel_req {
	uint32_t flags;
	uint8_t  dst[ETH_ALEN];
};

struct bnep_conninfo {
	uint32_t flags;
	uint16_t role;
	uint16_t state;	
	uint8_t  dst[ETH_ALEN];
	char     device[16];
};

struct bnep_connlist_req {
	uint32_t cnum;
	struct bnep_conninfo *ci;
};

#endif
