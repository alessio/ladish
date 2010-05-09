/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2010 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains implementation of the "delete room" command
 **************************************************************************
 *
 * LADI Session Handler is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * LADI Session Handler is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LADI Session Handler. If not, see <http://www.gnu.org/licenses/>
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "cmd.h"
#include "studio_internal.h"
#include "../dbus/error.h"

struct ladish_command_delete_room
{
  struct ladish_command command; /* must be the first member */
  char * name;
};

#define cmd_ptr ((struct ladish_command_delete_room *)context)

static bool run(void * context)
{
  struct list_head * node_ptr;
  ladish_room_handle room;
  uuid_t room_uuid;
  ladish_client_handle room_client;
  unsigned int running_app_count;

  ASSERT(cmd_ptr->command.state == LADISH_COMMAND_STATE_PENDING);

  log_info("Delete studio room request (%s)", cmd_ptr->name);

  list_for_each(node_ptr, &g_studio.rooms)
  {
    room = ladish_room_from_list_node(node_ptr);
    if (strcmp(ladish_room_get_name(room), cmd_ptr->name) == 0)
    {
      running_app_count = ladish_app_supervisor_get_running_app_count(ladish_room_get_app_supervisor(room));
      if (running_app_count != 0)
      {
        /* TODO: instead of rejecting the room deletion, use the command queue and wait for room apps to stop.
           This requires proper "project in room" implementation because project needs to be
           unloaded anyway and unloading project should initiate and wait apps termination */
        log_error("Cannot delete room \"%s\" because it has %u app(s) running", cmd_ptr->name, running_app_count);
        return false;
      }

      list_del(node_ptr);
      ladish_studio_emit_room_disappeared(room);

      ladish_room_get_uuid(room, room_uuid);
      room_client = ladish_graph_find_client_by_uuid(g_studio.studio_graph, room_uuid);
      ASSERT(room_client != NULL);
      ladish_graph_remove_client(g_studio.studio_graph, room_client);
      ladish_client_destroy(room_client);

      ladish_room_destroy(room);

      cmd_ptr->command.state = LADISH_COMMAND_STATE_DONE;
      return true;
    }
  }

  log_error("Cannot delete room with name \"%s\" because it is unknown", cmd_ptr->name);
  return false;
}

static void destructor(void * context)
{
  log_info("delete_room command destructor");
  free(cmd_ptr->name);
}

#undef cmd_ptr

bool ladish_command_delete_room(void * call_ptr, struct ladish_cqueue * queue_ptr, const char * room_name)
{
  struct ladish_command_delete_room * cmd_ptr;
  char * room_name_dup;

  room_name_dup = strdup(room_name);
  if (room_name_dup == NULL)
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "strdup('%s') failed.", room_name);
    goto fail;
  }

  cmd_ptr = ladish_command_new(sizeof(struct ladish_command_delete_room));
  if (cmd_ptr == NULL)
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "ladish_command_new() failed.");
    goto fail_free_name;
  }

  cmd_ptr->command.run = run;
  cmd_ptr->command.destructor = destructor;
  cmd_ptr->name = room_name_dup;

  if (!ladish_cqueue_add_command(queue_ptr, &cmd_ptr->command))
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "ladish_cqueue_add_command() failed.");
    goto fail_destroy_command;
  }

  return true;

fail_destroy_command:
  free(cmd_ptr);
fail_free_name:
  free(room_name_dup);
fail:
  return false;
}
