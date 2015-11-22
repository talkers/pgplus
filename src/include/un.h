/*
 * Playground+ - un.h
 * Header file from Linux 1.2.*.* to avoid any problems
 * -----------------------------------------------------------------------
 */

#ifndef _LINUX_UN_H
#define _LINUX_UN_H

#define UNIX_PATH_MAX   108

struct sockaddr_un
{
  unsigned short sun_family; 
  char sun_path[UNIX_PATH_MAX];
};

#endif

