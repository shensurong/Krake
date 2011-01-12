/**
 * krk_config.c - functions related to configuration
 * 
 * Copyright (c) 2010 Yang Yang <paulyang.inf@gmail.com>
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <krk_core.h>
#include <krk_socket.h>
#include <krk_event.h>
#include <krk_connection.h>
#include <krk_config.h>
#include <krk_buffer.h>
#include <krk_monitor.h>
#include <checkers/krk_checker.h>

void krk_config_read(int sock, short type, void *arg);
void krk_config_write(int sock, short type, void *arg);

static inline void krk_config_show_content(struct krk_config *conf)
{
	fprintf(stderr, "config: \n");
	fprintf(stderr, "\tmonitor: %s\n", conf->monitor);
	fprintf(stderr, "\tchecker: %s\n", conf->checker);
	fprintf(stderr, "\tchecker_conf: %s\n", conf->checker_conf);
	fprintf(stderr, "\tchecker_conf_len: %lu\n", conf->checker_conf_len);
	fprintf(stderr, "\tinterval: %lu\n", conf->interval);
	fprintf(stderr, "\ttimeout: %lu\n", conf->timeout);
	fprintf(stderr, "\tthreshold: %lu\n", conf->threshold);
	fprintf(stderr, "\tnode: %s\n", conf->node);
	fprintf(stderr, "\tport: %u\n", conf->port);
}

/**
 * krk_config_check - check config
 * @conf: configuration to check
 *
 *
 * return: KRK_OK on success, KRK_ERROR on failed
 *
 * Finally I decide to do this check at the daemon
 * side instead of in krakectrl. This check could
 * make krake daemon more robust, although this could
 * impact the performance.
 */
static int krk_config_check(struct krk_config *conf)
{
	int ret = KRK_OK;
	return ret;
}

static int krk_config_parse(struct krk_config *conf)
{
	int ret = KRK_OK;
	struct krk_monitor *monitor = NULL;
	struct krk_node *node = NULL;

	if (conf->command != KRK_CONF_CMD_CREATE) {
		monitor = krk_monitor_find(conf->monitor);
		if (monitor == NULL) {
			ret = KRK_ERROR;
			goto out;
		}
	}

	switch (conf->command) {
		case KRK_CONF_CMD_CREATE:
			monitor = krk_monitor_create(conf->monitor);
			if (monitor == NULL) {
				ret = KRK_ERROR;
				goto out;
			}

			monitor->threshold = conf->threshold;
			/*TODO: initial timeout and interval*/
			break;
		case KRK_CONF_CMD_DESTROY:
			ret = krk_monitor_destroy(monitor);
			break;
		case KRK_CONF_CMD_ADD:
			node = krk_monitor_create_node(conf->node, conf->port);
			if (node == NULL) {
				ret = KRK_ERROR;
				goto out;
			}

			ret = krk_monitor_add_node(monitor, node);
			break;
		case KRK_CONF_CMD_REMOVE:
			node = krk_monitor_find_node(conf->node, conf->port, monitor);
			if (node == NULL) {
				ret = KRK_ERROR;
				goto out;
			}

			ret = krk_monitor_remove_node(monitor, node);
			break;
		case KRK_CONF_CMD_ENABLE:
			krk_monitor_enable(monitor);
			break;
		case KRK_CONF_CMD_DISABLE:
			krk_monitor_disable(monitor);
			break;
		default:
			ret = KRK_ERROR;
	}

out:
	return ret;
}

static int krk_config_process(struct krk_connection *conn)
{
	struct krk_config *conf = NULL;
	struct krk_event *rev = conn->rev;
	struct krk_event *wev = conn->wev;
	int buf_len, ret, i, n;
	struct krk_config_ret *retbuf;
	char *buf;
	struct krk_monitor* monitor = NULL;
	struct krk_monitor* monitors = NULL;
	struct krk_config_monitor *conf_monitor = NULL;
	struct krk_config_node *conf_node = NULL;
	struct krk_node *nodes = NULL;

	retbuf = malloc(sizeof(struct krk_config_ret));
	memset(retbuf, 0, sizeof(struct krk_config_ret));
	
	retbuf->retval = KRK_OK;
	
	buf_len = rev->buf->last - rev->buf->pos;
	if (buf_len < sizeof(struct krk_config)) {
		return KRK_AGAIN;
	}

	conf = (struct krk_config*)(rev->buf->pos);
	conf->checker_conf = conf->data;
	
	if (buf_len < 
			(sizeof(struct krk_config) + conf->checker_conf_len)) {
		return KRK_AGAIN;
	}

	if (buf_len >
			(sizeof(struct krk_config) + conf->checker_conf_len)) {
		return KRK_ERROR;
	}

#ifdef KRK_DEBUG
	krk_config_show_content(conf);
#endif
	
	ret = krk_config_check(conf);
	if (ret != KRK_OK) {
		retbuf->retval = KRK_ERROR;
	} else {
		if (conf->command == KRK_CONF_CMD_SHOW) {
			if (conf->monitor[0]) {
				/* buf = monitor + checker_conf + nodes */
				monitor = krk_monitor_find(conf->monitor);
				if (monitor == NULL) {
					retbuf->retval = KRK_ERROR;
					goto out;
				}
			
				retbuf->data_len = sizeof(struct krk_config_monitor)
					+ monitor->nr_nodes * sizeof(struct krk_config_node)
					+ monitor->checker_param_len;

				buf = malloc(retbuf->data_len);
				memset(buf, 0, retbuf->data_len);
				
				conf_monitor = (struct krk_config_monitor *)buf;
				
				strcpy(conf_monitor->monitor, monitor->name);
				conf_monitor->threshold = monitor->threshold;
				conf_monitor->interval = monitor->interval;
				conf_monitor->timeout = monitor->timeout;

				/* TODO: copy monitor->checker->name 
				 * to conf_monitor->checker 
				 */

				conf_monitor->checker_param_len = monitor->checker_param_len;
				if (monitor->checker_param_len) {
					memcpy(buf + sizeof(struct krk_config_monitor),
							monitor->checker_param, monitor->checker_param_len);
				}

				conf_node = malloc(monitor->nr_nodes * 
						sizeof(struct krk_config_node));
				if (conf_node == NULL) {
					retbuf->retval = KRK_ERROR;
					goto out;
				}
	
				nodes = malloc(monitor->nr_nodes * 
						sizeof(struct krk_node));
				if (nodes == NULL) {
					retbuf->retval = KRK_ERROR;
					goto out;
				}

				memset(conf_node, 0, monitor->nr_nodes * sizeof(struct krk_config_node));
				memset(nodes, 0, monitor->nr_nodes * sizeof(struct krk_node));

				if (monitor->nr_nodes) {
					ret = krk_monitor_get_all_nodes(monitor, nodes);
					if (ret < 0) {
						retbuf->retval = KRK_ERROR;
						goto out;
					}

					n = (ret > monitor->nr_nodes) ? monitor->nr_nodes : ret;

					for (i = 0; i < n; i++) {
						strcpy(conf_node[i].addr, nodes[i].addr);
						conf_node[i].port = nodes[i].port;
					}

					free(nodes);
					
					memcpy(buf + sizeof(struct krk_config_monitor) + 
							conf_monitor->checker_param_len, 
							conf_node, 
							monitor->nr_nodes * sizeof(struct krk_config_node));

					conf_monitor->nr_nodes = n;
#if 0
					fprintf(stderr, "conf_node[0].addr: %s, port: %u\n",
							conf_node[0].addr, conf_node[0].port);
#endif
					free(conf_node);
				}
			} else {
				monitors = malloc(sizeof(struct krk_monitor) * 64);
				if (monitors == NULL) {
					retbuf->retval = KRK_ERROR;
					goto out;
				}

				ret = krk_monitor_get_all_monitors(monitors);
				if (ret < 0) {
					retbuf->retval = KRK_ERROR;
					goto out;
				}

				if (ret > 0) {
					retbuf->data_len = ret * 64;
					buf = malloc(retbuf->data_len);

					for (i = 0; i < ret; i++) {
						strcpy(buf + i * 64, monitors[i].name);
					}
				}

				free(monitors);
			}
		
			if ((retbuf->data_len + sizeof(struct krk_config_ret))
					> (wev->buf->end - wev->buf->last)) {
				return KRK_ERROR;
			}
		} else {
			ret = krk_config_parse(conf);
			if (ret != KRK_OK) {
				retbuf->retval = KRK_ERROR;
			}
		}
	}

out:
	/* add return value */
	memcpy(wev->buf->pos, retbuf, sizeof(struct krk_config_ret));
	wev->buf->last += sizeof(struct krk_config_ret);
	
	/* append additional data */
	if (retbuf->data_len) {
		memcpy(wev->buf->last, buf, retbuf->data_len);
		wev->buf->last += retbuf->data_len;
		free(buf);
	}

	free(retbuf);

	krk_event_set_write(conn->sock, wev);
	krk_event_add(wev);
	
	return KRK_DONE;
}

void krk_config_read(int sock, short type, void *arg)
{
	int n, ret;
	struct krk_event *rev;
	struct krk_connection *conn;
	
	rev = arg;
	conn = rev->conn;
	
	n = recv(sock, rev->buf->last, rev->buf->end - rev->buf->last, 0);
	if (n == 0) {
		/* fprintf(stderr, "read config finished\n"); */
		krk_connection_destroy(conn);
		return;
	}

	if (n < 0) {
		/* fprintf(stderr, "read config error\n"); */
		krk_connection_destroy(conn);
		return;
	}

	rev->buf->last += n;
	ret = krk_config_process(conn);

	if (ret == KRK_AGAIN) {
		/* KRK_AGAIN means command not completed */
	}

	if (ret == KRK_DONE 
			|| ret == KRK_ERROR) {
		rev->buf->last = rev->buf->head;
	}

	krk_event_set_read(sock, rev);
	krk_event_add(rev);
}

void krk_config_write(int sock, short type, void *arg)
{
	int n;
	struct krk_event *wev;
	struct krk_connection *conn;

	wev = arg;
	conn = wev->conn;
	
	n = send(sock, wev->buf->pos, wev->buf->last - wev->buf->pos, 0);
	if (n < 0) {
		fprintf(stderr, "write config retval error\n");
		krk_connection_destroy(conn);
		return;
	}

	if (n == (wev->buf->last - wev->buf->pos)) {
		wev->buf->pos = wev->buf->last = wev->buf->head;
		return;
	}

	/* write busy, rearm */
	if (n < (wev->buf->last - wev->buf->pos)) {
		wev->buf->pos += n;
	}

	krk_event_set_write(sock, wev);
	krk_event_add(wev);
}
