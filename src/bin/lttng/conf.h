/*
 * Copyright (C) 2011 - David Goulet <david.goulet@polymtl.ca>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; only version 2
 * of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef _LTTNG_CONFIG_H
#define _LTTNG_CONFIG_H

#define CONFIG_FILENAME ".lttngrc"

void config_destroy(char *path);
int config_init(char *path);
int config_add_session_name(char *path, char *name);
char *config_get_default_path(void);

/* Must free() the return pointer */
char *config_read_session_name(char *path);
char *config_get_file_path(char *path);

#endif /* _LTTNG_CONFIG_H */
