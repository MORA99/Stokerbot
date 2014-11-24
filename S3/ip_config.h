/*********************************************
 * vim:sw=8:ts=8:si:et
 * To use the above modeline in vim you must have "set modeline" in your .vimrc
 * Author: Guido Socher 
 * Copyright: GPL V2
 *
 * This file can be used to decide which functionallity of the
 * TCP/IP stack shall be available.
 *
 *********************************************/
//@{
#ifndef IP_CONFIG_H
#define IP_CONFIG_H

#define ENC28J60_BROADCAST
#undef NTP_client
#define UDP_client
#define WWW_server
#undef PING_client
#define PINGPATTERN 0x42
#undef WOL_client
#define WWW_client
#define FROMDECODE_websrv_help
#define GRATARP
#undef URLENCODE_websrv_help

#endif /* IP_CONFIG_H */
//@}
