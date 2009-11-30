/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains implementation of app supervisor object
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

#include <ctype.h>

#include "app_supervisor.h"
#include "../dbus/error.h"
#include "../dbus_constants.h"
#include "loader.h"

struct ladish_app
{
  struct list_head siblings;
  uint64_t id;
  char * name;
  char * commandline;
  pid_t pid;
};

struct ladish_app_supervisor
{
  char * name;
  char * opath;
  uint64_t version;
  uint64_t next_id;
  struct list_head applist;
};

bool ladish_app_supervisor_create(ladish_app_supervisor_handle * supervisor_handle_ptr, const char * opath, const char * name)
{
  struct ladish_app_supervisor * supervisor_ptr;

  supervisor_ptr = malloc(sizeof(struct ladish_app_supervisor));
  if (supervisor_ptr == NULL)
  {
    log_error("malloc() failed to allocate struct ladish_app_supervisor");
    return false;
  }

  supervisor_ptr->opath = strdup(opath);
  if (supervisor_ptr->opath == NULL)
  {
    log_error("strdup() failed for app supervisor opath");
    free(supervisor_ptr);
    return false;
  }

  supervisor_ptr->name = strdup(name);
  if (supervisor_ptr->name == NULL)
  {
    log_error("strdup() failed for app supervisor name");
    free(supervisor_ptr->opath);
    free(supervisor_ptr);
    return false;
  }

  supervisor_ptr->version = 0;
  supervisor_ptr->next_id = 1;

  INIT_LIST_HEAD(&supervisor_ptr->applist);

  *supervisor_handle_ptr = (ladish_app_supervisor_handle)supervisor_ptr;

  return true;
}

struct ladish_app * ladish_app_supervisor_find_app_by_name(struct ladish_app_supervisor * supervisor_ptr, const char * name)
{
  struct list_head * node_ptr;
  struct ladish_app * app_ptr;

  list_for_each(node_ptr, &supervisor_ptr->applist)
  {
    app_ptr = list_entry(node_ptr, struct ladish_app, siblings);
    if (strcmp(app_ptr->name, name) == 0)
    {
      return app_ptr;
    }
  }

  return NULL;
}

#define supervisor_ptr ((struct ladish_app_supervisor *)supervisor_handle)

void ladish_app_supervisor_destroy(ladish_app_supervisor_handle supervisor_handle)
{
  struct ladish_app * app_ptr;

  while (!list_empty(&supervisor_ptr->applist))
  {
    app_ptr = list_entry(supervisor_ptr->applist.next, struct ladish_app, siblings);
    list_del(&app_ptr->siblings);
    free(app_ptr->name);
    free(app_ptr->commandline);
    free(app_ptr);
  }

  free(supervisor_ptr->name);
  free(supervisor_ptr->opath);
  free(supervisor_ptr);
}

bool ladish_app_supervisor_child_exit(ladish_app_supervisor_handle supervisor_handle, pid_t pid)
{
  struct list_head * node_ptr;
  struct ladish_app * app_ptr;

  list_for_each(node_ptr, &supervisor_ptr->applist)
  {
    app_ptr = list_entry(node_ptr, struct ladish_app, siblings);
    if (app_ptr->pid == pid)
    {
      log_info("exit of studio child '%s' detected.", app_ptr->name);
      list_del(&app_ptr->siblings);
      free(app_ptr->name);
      free(app_ptr->commandline);
      free(app_ptr);
      return true;
    }
  }

  return false;
}

#undef supervisor_ptr
#define supervisor_ptr ((struct ladish_app_supervisor *)call_ptr->iface_context)

static void get_all(struct dbus_method_call * call_ptr)
{
  DBusMessageIter iter, array_iter, struct_iter;
  struct list_head * node_ptr;
  struct ladish_app * app_ptr;

  log_info("get_all called");

  call_ptr->reply = dbus_message_new_method_return(call_ptr->message);
  if (call_ptr->reply == NULL)
  {
    goto fail;
  }

  dbus_message_iter_init_append(call_ptr->reply, &iter);

  if (!dbus_message_iter_append_basic(&iter, DBUS_TYPE_UINT64, &supervisor_ptr->version))
  {
    goto fail_unref;
  }

  if (!dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "(ts)", &array_iter))
  {
    goto fail_unref;
  }

  list_for_each(node_ptr, &supervisor_ptr->applist)
  {
    app_ptr = list_entry(node_ptr, struct ladish_app, siblings);

    if (!dbus_message_iter_open_container (&array_iter, DBUS_TYPE_STRUCT, NULL, &struct_iter))
    {
      goto fail_unref;
    }

    if (!dbus_message_iter_append_basic(&struct_iter, DBUS_TYPE_UINT64, &app_ptr->id))
    {
      goto fail_unref;
    }

    log_info("app '%s' (%llu)", app_ptr->name, (unsigned long long)app_ptr->id);
    if (!dbus_message_iter_append_basic(&struct_iter, DBUS_TYPE_STRING, &app_ptr->name))
    {
      goto fail_unref;
    }

    if (!dbus_message_iter_close_container(&array_iter, &struct_iter))
    {
      goto fail_unref;
    }
  }

  if (!dbus_message_iter_close_container(&iter, &array_iter))
  {
    goto fail_unref;
  }

  return;

fail_unref:
  dbus_message_unref(call_ptr->reply);
  call_ptr->reply = NULL;

fail:
  log_error("Ran out of memory trying to construct method return");
}

static void run_custom(struct dbus_method_call * call_ptr)
{
  dbus_bool_t terminal;
  const char * commandline;
  const char * name_param;
  char * name;
  size_t len;
  char * end;
  unsigned int index;
  struct ladish_app * app_ptr;

  if (!dbus_message_get_args(
        call_ptr->message,
        &g_dbus_error,
        DBUS_TYPE_BOOLEAN, &terminal,
        DBUS_TYPE_STRING, &commandline,
        DBUS_TYPE_STRING, &name_param,
        DBUS_TYPE_INVALID))
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_INVALID_ARGS, "Invalid arguments to method \"%s\": %s",  call_ptr->method_name, g_dbus_error.message);
    dbus_error_free(&g_dbus_error);
    return;
  }

  log_info("run_custom('%s', %s, '%s') called", name_param, terminal ? "terminal" : "shell", commandline);

  if (*name_param)
  {
    /* allocate and copy app name */
    len = strlen(name_param);
    name = malloc(len + 100);
    if (name == NULL)
    {
      lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "malloc of app name failed");
      return;
    }

    strcpy(name, name_param);

    end = name + len;
  }
  else
  {
    /* allocate app name */
    len = strlen(commandline) + 100;
    name = malloc(len);
    if (name == NULL)
    {
      lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "malloc of app name failed");
      return;
    }

    strcpy(name, commandline);

    /* use first word as name */
    end = name;
    while (*end)
    {
      if (isspace(*end))
      {
        *end = 0;
        break;
      }

      end++;
    }
  }

  /* make the app name unique */
  index = 2;
  while (ladish_app_supervisor_find_app_by_name(supervisor_ptr, name) != NULL)
  {
    sprintf(end, "-%u", index);
    index++;
  }

  app_ptr = malloc(sizeof(struct ladish_app));
  if (app_ptr == NULL)
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "malloc of struct ladish_app failed");
    free(name);
    return;
  }

  app_ptr->id = supervisor_ptr->next_id++;
  app_ptr->name = name;
  app_ptr->commandline = strdup(commandline);
  app_ptr->pid = 0;
  if (app_ptr->commandline == NULL)
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "strdup() failed for commandline");
    free(app_ptr);
    free(name);
    return;
  }

  list_add_tail(&app_ptr->siblings, &supervisor_ptr->applist);

  if (!loader_execute(supervisor_ptr->name, name, "/", terminal, commandline, &app_ptr->pid))
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "Execution of '%s' failed",  commandline);
    list_del(&app_ptr->siblings);
    free(app_ptr->commandline);
    free(app_ptr);
    free(name);
    return;
  }

  log_info("%s pid is %lu", app_ptr->name, (unsigned long)app_ptr->pid);

  method_return_new_void(call_ptr);
}

#undef supervisor_ptr

METHOD_ARGS_BEGIN(GetAll, "Get list of running apps")
  METHOD_ARG_DESCRIBE_OUT("list_version", "t", "Version of the list")
  METHOD_ARG_DESCRIBE_OUT("apps_list", "a(ts)", "List of running apps")
METHOD_ARGS_END

METHOD_ARGS_BEGIN(RunCustom, "Start application by supplying commandline")
  METHOD_ARG_DESCRIBE_IN("terminal", "b", "Whether to run in terminal")
  METHOD_ARG_DESCRIBE_IN("commandline", "s", "Commandline")
  METHOD_ARG_DESCRIBE_IN("name", "s", "Name")
METHOD_ARGS_END

METHODS_BEGIN
  METHOD_DESCRIBE(GetAll, get_all)
  METHOD_DESCRIBE(RunCustom, run_custom)
METHODS_END

SIGNALS_BEGIN
SIGNALS_END

INTERFACE_BEGIN(g_iface_app_supervisor, IFACE_APP_SUPERVISOR)
  INTERFACE_DEFAULT_HANDLER
  INTERFACE_EXPOSE_METHODS
  INTERFACE_EXPOSE_SIGNALS
INTERFACE_END