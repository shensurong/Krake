/**
 * krk_monitor.h - Monitor log
 * 
 * Copyright (c) 2011 Yang Yang <paulyang.inf@gmail.com>
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */


#ifndef __KRK_MONITOR_H__
#define __KRK_MONITOR_H__

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>

#include <krk_core.h>
#include <krk_list.h>
#include <krk_event.h>
#include <krk_config.h>
#include <krk_connection.h>
#include <krk_ssl.h>
#include <checkers/krk_checker.h>

#define KRK_MONITOR_FLAG_ENABLED 0x1

#define KRK_MONITOR_MAX_NR 64
#define KRK_NODE_MAX_NUM 10000

struct krk_monitor {
    char name[KRK_NAME_LEN];
    unsigned char id;

    struct list_head list;

    unsigned long interval;
    unsigned long timeout;
    unsigned long failure_threshold;
    unsigned long success_threshold;

    struct krk_checker *checker;
    void *checker_param;
    unsigned int checker_param_len; /* size of void *checker_param */
    void *parsed_checker_param; /* points to checker-specific data structure */

    struct list_head node_list;
    unsigned long nr_nodes;

    struct krk_event *tmout_ev;	

    char notify_script[KRK_NAME_LEN];
    char notify_script_name[KRK_NAME_LEN];

    struct krk_ssl *ssl;

    unsigned int enabled:1;
    unsigned int ssl_flag:1;
};

struct krk_monitor_info {
    char name[KRK_NAME_LEN];
    char checker[KRK_NAME_LEN];
    unsigned long nr_nodes;
    unsigned int enabled:1;
};

struct krk_node {
    char addr[KRK_IPADDR_LEN];
    unsigned int port;
    unsigned int nr_fail;
    unsigned int nr_success;
    unsigned char id;

    union {
        struct sockaddr_in inaddr;
        struct sockaddr_in6 in6addr;
    };

    struct list_head list;
    struct krk_monitor *parent;

    struct krk_connection *conn;
    struct list_head connection_list;
    unsigned long nr_connections;

    void *checker_data;    /* initialed in <checker>_init_node */

    struct krk_buffer *buf;

    unsigned int ipv6:1;
    unsigned int down:1;
    unsigned int ready:1;
};

struct krk_node_info {
    char addr[KRK_IPADDR_LEN];
    unsigned int port;
    unsigned int nr_fail;
    unsigned int nr_success;
    unsigned int ipv6:1;
    unsigned int down:1;
    unsigned int ready:1;
};


extern struct krk_monitor* krk_monitor_find(const char *name);
extern struct krk_monitor* krk_monitor_create(const char *name);
extern int krk_monitor_destroy(struct krk_monitor *monitor);
extern int krk_remove_unused_monitor(struct krk_config *conf);
extern int krk_monitor_init(void);
extern int krk_all_monitors_destroy(void);
extern int krk_monitor_exit(void);
extern int krk_monitor_add_node(struct krk_monitor *monitor, 
        struct krk_node *node);
extern int krk_monitor_remove_node(struct krk_monitor *monitor, 
        struct krk_node *node);
extern int krk_remove_unused_node(struct krk_config_monitor *conf_monitor, struct krk_monitor *monitor);
extern void krk_monitor_enable(struct krk_monitor *monitor);
extern void krk_monitor_disable(struct krk_monitor *monitor);
extern struct krk_node* krk_monitor_create_node(const char *addr, unsigned short port);
extern int krk_monitor_destroy_node(struct krk_node *node);
extern struct krk_node* krk_monitor_find_node(const char *addr, 
        const unsigned short port, struct krk_monitor *monitor);
extern int krk_monitor_get_all_nodes(struct krk_monitor *monitor, 
        struct krk_node *node); 
extern int krk_monitors_destroy_all_nodes(struct krk_monitor *monitor);
extern void krk_monitor_notify(struct krk_monitor *monitor, 
        struct krk_node *node);
extern int krk_monitor_add_node_connection(struct krk_node *node, struct krk_connection *conn);
extern int krk_monitor_remove_node_connection(struct krk_node *node, struct krk_connection *conn);
extern int krk_monitor_get_nodes_by_addr(const char *addr, 
        struct krk_node *nodes);
extern int krk_monitor_get_all_nodes(struct krk_monitor *monitor, 
        struct krk_node *nodes);
extern struct krk_node* krk_monitor_find_node_by_id(const unsigned char id, 
        struct krk_monitor *monitor);
extern int krk_monitor_set_node_status(struct krk_monitor *monitor, 
        unsigned char id, int status);
extern void krk_monitor_show(void);
extern int krk_monitor_get_all_monitors(struct krk_monitor *monitors);
extern int krk_get_all_monitor_name(char *buf);
extern int krk_monitor_destroy_all_nodes(struct krk_monitor *monitor);

extern void krk_get_monitor_info(struct krk_monitor_info *info, struct krk_monitor *monitor);
extern void krk_show_monitor_info(struct krk_monitor_info *info);
extern void krk_show_node_info(struct krk_node_info *info);
extern size_t krk_info_buffer_size(void);
extern int krk_monitor_init_node_ssl(struct krk_node *node);

extern void krk_monitor_node_failure_inc(struct krk_monitor *, struct krk_node *);
extern void krk_monitor_node_success_inc(struct krk_monitor *, struct krk_node *);

#endif
