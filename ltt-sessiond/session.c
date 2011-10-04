/*
 * Copyright (C)  2011 - David Goulet <david.goulet@polymtl.ca>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; only version 2
 * of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#define _GNU_SOURCE
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <lttng-sessiond-comm.h>
#include <lttngerr.h>

#include "session.h"

/*
 * NOTES:
 *
 * No ltt_session.lock is taken here because those data structure are widely
 * spread across the lttng-tools code base so before caling functions below
 * that can read/write a session, the caller MUST acquire the session lock
 * using session_lock() and session_unlock().
 */

/*
 * Init tracing session list.
 *
 * Please see session.h for more explanation and correct usage of the list.
 */
static struct ltt_session_list ltt_session_list = {
	.head = CDS_LIST_HEAD_INIT(ltt_session_list.head),
	.lock = PTHREAD_MUTEX_INITIALIZER,
	.count = 0,
};

/*
 * Add a ltt_session structure to the global list.
 *
 * The caller MUST acquire the session list lock before.
 */
static void add_session_list(struct ltt_session *ls)
{
	cds_list_add(&ls->list, &ltt_session_list.head);
	ltt_session_list.count++;
}

/*
 * Delete a ltt_session structure to the global list.
 *
 * The caller MUST acquire the session list lock before.
 */
static void del_session_list(struct ltt_session *ls)
{
	cds_list_del(&ls->list);
	/* Sanity check */
	if (ltt_session_list.count > 0) {
		ltt_session_list.count--;
	}
}

/*
 * Return a pointer to the session list.
 */
struct ltt_session_list *session_get_list(void)
{
	return &ltt_session_list;
}

/*
 * Acquire session list lock
 */
void session_lock_list(void)
{
	pthread_mutex_lock(&ltt_session_list.lock);
}

/*
 * Release session list lock
 */
void session_unlock_list(void)
{
	pthread_mutex_unlock(&ltt_session_list.lock);
}

/*
 * Acquire session lock
 */
void session_lock(struct ltt_session *session)
{
	pthread_mutex_lock(&session->lock);
}

/*
 * Release session lock
 */
void session_unlock(struct ltt_session *session)
{
	pthread_mutex_unlock(&session->lock);
}

/*
 * Return a ltt_session structure ptr that matches name.
 * If no session found, NULL is returned.
 */
struct ltt_session *session_find_by_name(char *name)
{
	int found = 0;
	struct ltt_session *iter;

	DBG2("Trying to find session by name %s", name);

	session_lock_list();
	cds_list_for_each_entry(iter, &ltt_session_list.head, list) {
		if (strncmp(iter->name, name, NAME_MAX) == 0) {
			found = 1;
			break;
		}
	}
	session_unlock_list();

	if (!found) {
		iter = NULL;
	}

	return iter;
}

/*
 * Delete session from the session list and free the memory.
 *
 * Return -1 if no session is found.  On success, return 1;
 */
int session_destroy(struct ltt_session *session)
{
	/* Safety check */
	if (session == NULL) {
		ERR("Session pointer was null on session destroy");
		return LTTCOMM_OK;
	}

	DBG("Destroying session %s", session->name);
	del_session_list(session);
	free(session->name);
	free(session->path);
	pthread_mutex_destroy(&session->lock);
	free(session);

	return LTTCOMM_OK;
}

/*
 * Create a brand new session and add it to the session list.
 */
int session_create(char *name, char *path)
{
	int ret;
	struct ltt_session *new_session;

	new_session = session_find_by_name(name);
	if (new_session != NULL) {
		ret = LTTCOMM_EXIST_SESS;
		goto error_exist;
	}

	/* Allocate session data structure */
	new_session = malloc(sizeof(struct ltt_session));
	if (new_session == NULL) {
		perror("malloc");
		ret = LTTCOMM_FATAL;
		goto error_malloc;
	}

	/* Define session name */
	if (name != NULL) {
		if (asprintf(&new_session->name, "%s", name) < 0) {
			ret = LTTCOMM_FATAL;
			goto error_asprintf;
		}
	} else {
		ERR("No session name given");
		ret = LTTCOMM_FATAL;
		goto error;
	}

	/* Define session system path */
	if (path != NULL) {
		if (asprintf(&new_session->path, "%s", path) < 0) {
			ret = LTTCOMM_FATAL;
			goto error_asprintf;
		}
	} else {
		ERR("No session path given");
		ret = LTTCOMM_FATAL;
		goto error;
	}

	/* Init kernel session */
	new_session->kernel_session = NULL;

	/* Init UST session list */
	CDS_INIT_LIST_HEAD(&new_session->ust_session_list.head);

	/* Init lock */
	pthread_mutex_init(&new_session->lock, NULL);

	/* Add new session to the session list */
	session_lock_list();
	add_session_list(new_session);
	session_unlock_list();

	DBG("Tracing session %s created in %s", name, path);

	return LTTCOMM_OK;

error:
error_asprintf:
	if (new_session != NULL) {
		free(new_session);
	}

error_exist:
error_malloc:
	return ret;
}
