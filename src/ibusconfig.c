/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* ibus - The Input Bus
 * Copyright (C) 2008-2010 Peng Huang <shawn.p.huang@gmail.com>
 * Copyright (C) 2008-2010 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include "ibusinternal.h"
#include "ibusmarshalers.h"
#include "ibusshare.h"
#include "ibusconfig.h"

#define IBUS_CONFIG_GET_PRIVATE(o)  \
   (G_TYPE_INSTANCE_GET_PRIVATE ((o), IBUS_TYPE_CONFIG, IBusConfigPrivate))

enum {
    VALUE_CHANGED,
    LAST_SIGNAL,
};


/* IBusConfigPriv */
struct _IBusConfigPrivate {
    gpointer pad;
};
typedef struct _IBusConfigPrivate IBusConfigPrivate;

static guint    config_signals[LAST_SIGNAL] = { 0 };

static void      ibus_config_class_init     (IBusConfigClass    *class);
static void      ibus_config_init           (IBusConfig         *config);
static void      ibus_config_real_destroy   (IBusProxy          *proxy);

static void      ibus_config_g_signal       (GDBusProxy         *proxy,
                                             const gchar        *sender_name,
                                             const gchar        *signal_name,
                                             GVariant           *parameters);

G_DEFINE_TYPE (IBusConfig, ibus_config, IBUS_TYPE_PROXY)

static void
ibus_config_class_init (IBusConfigClass *class)
{
    GDBusProxyClass *dbus_proxy_class = G_DBUS_PROXY_CLASS (class);
    IBusProxyClass *proxy_class = IBUS_PROXY_CLASS (class);

    g_type_class_add_private (class, sizeof (IBusConfigPrivate));

    dbus_proxy_class->g_signal = ibus_config_g_signal;
    proxy_class->destroy = ibus_config_real_destroy;


    /* install signals */
    /**
     * IBusConfig::value-changed:
     * @config: An IBusConfig.
     * @section: Section name.
     * @name: Name of the property.
     * @value: Value.
     *
     * Emitted when configuration value is changed.
     * <note><para>Argument @user_data is ignored in this function.</para></note>
     */
    config_signals[VALUE_CHANGED] =
        g_signal_new (I_("value-changed"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            _ibus_marshal_VOID__STRING_STRING_VARIANT,
            G_TYPE_NONE,
            3,
            G_TYPE_STRING,
            G_TYPE_STRING,
            G_TYPE_VARIANT | G_SIGNAL_TYPE_STATIC_SCOPE);
}

static void
ibus_config_init (IBusConfig *config)
{
}

static void
ibus_config_real_destroy (IBusProxy *proxy)
{
    IBUS_PROXY_CLASS(ibus_config_parent_class)->destroy (proxy);
}


static void
ibus_config_g_signal (GDBusProxy  *proxy,
                      const gchar *sender_name,
                      const gchar *signal_name,
                      GVariant    *parameters)
{
    if (g_strcmp0 (signal_name, "ValueChanged") == 0) {
        const gchar *section = NULL;
        const gchar *name = NULL;
        GVariant *value = NULL;

        g_variant_get (parameters, "(&s&sv)", &section, &name, &value);

        g_signal_emit (proxy,
                       config_signals[VALUE_CHANGED],
                       0,
                       section,
                       name,
                       value);
        g_variant_unref (value);
        return;
    }

    g_return_if_reached ();
}

IBusConfig *
ibus_config_new (GDBusConnection  *connection,
                 GCancellable     *cancellable,
                 GError          **error)
{
    g_assert (G_IS_DBUS_CONNECTION (connection));

    GInitable *initable;

    GDBusProxyFlags flags = G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START |
                            G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES;

    initable = g_initable_new (IBUS_TYPE_CONFIG,
                               cancellable,
                               error,
                               "g-connection",      connection,
                               "g-flags",           flags,
                               "g-name",            IBUS_SERVICE_CONFIG,
                               "g-interface-name",  IBUS_INTERFACE_CONFIG,
                               "g-object-path",     IBUS_PATH_CONFIG,
                               "g-default-timeout", ibus_get_timeout (),
                               NULL);
    if (initable == NULL)
        return NULL;

    if (g_dbus_proxy_get_name_owner (G_DBUS_PROXY (initable)) == NULL) {
        /* The configuration daemon, which is usually ibus-gconf, is not started yet. */
        g_object_unref (initable);
        return NULL;
    }

    /* clients should not destroy the config service. */
    IBUS_PROXY (initable)->own = FALSE;

    return IBUS_CONFIG (initable);
}

void
ibus_config_new_async (GDBusConnection     *connection,
                       GCancellable        *cancellable,
                       GAsyncReadyCallback  callback,
                       gpointer             user_data)
{
    g_assert (G_IS_DBUS_CONNECTION (connection));
    g_assert (callback != NULL);

    GDBusProxyFlags flags = G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START |
                            G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES;

    g_async_initable_new_async (IBUS_TYPE_CONFIG,
                                G_PRIORITY_DEFAULT,
                                cancellable,
                                callback,
                                user_data,
                                "g-connection",      connection,
                                "g-flags",           flags,
                                "g-name",            IBUS_SERVICE_CONFIG,
                                "g-interface-name",  IBUS_INTERFACE_CONFIG,
                                "g-object-path",     IBUS_PATH_CONFIG,
                                "g-default-timeout", ibus_get_timeout (),
                                NULL);
}

IBusConfig *
ibus_config_new_async_finish (GAsyncResult  *res,
                              GError       **error)
{
    g_assert (G_IS_ASYNC_RESULT (res));
    g_assert (error == NULL || *error == NULL);

    GObject *object = NULL;
    GObject *source_object = NULL;

    source_object = g_async_result_get_source_object (res);
    g_assert (source_object != NULL);

    object = g_async_initable_new_finish (G_ASYNC_INITABLE (source_object),
                                          res,
                                          error);
    g_object_unref (source_object);

    if (object != NULL) {
        if (g_dbus_proxy_get_name_owner (G_DBUS_PROXY (object)) == NULL) {
            /* The configuration daemon, which is usually ibus-gconf, 
             * is not started yet. */
            if (error != NULL) {
                *error = g_error_new (G_DBUS_ERROR,
                                      G_DBUS_ERROR_FAILED,
                                      IBUS_SERVICE_CONFIG " does not exist.");
            }
            g_object_unref (object);
            return NULL;
        }
        /* clients should not destroy the config service. */
        IBUS_PROXY (object)->own = FALSE;
        return IBUS_CONFIG (object);
    }
    else {
        return NULL;
    }
}

GVariant *
ibus_config_get_value (IBusConfig  *config,
                       const gchar *section,
                       const gchar *name)
{
    g_assert (IBUS_IS_CONFIG (config));
    g_assert (section != NULL);
    g_assert (name != NULL);

    GError *error = NULL;
    GVariant *result;
    result = g_dbus_proxy_call_sync ((GDBusProxy *) config,
                                     "GetValue",                /* method_name */
                                     g_variant_new ("(ss)",
                                        section, name),         /* parameters */
                                     G_DBUS_CALL_FLAGS_NONE,    /* flags */
                                     -1,                        /* timeout */
                                     NULL,                      /* cancellable */
                                     &error                     /* error */
                                     );
    if (result == NULL) {
        g_warning ("%s.GetValue: %s", IBUS_INTERFACE_CONFIG, error->message);
        g_error_free (error);
        return NULL;
    }

    GVariant *value = NULL;
    g_variant_get (result, "(v)", &value);
    g_variant_unref (result);

    return value;
}

void
ibus_config_get_value_async (IBusConfig         *config,
                             const gchar        *section,
                             const gchar        *name,
                             gint                timeout_ms,
                             GCancellable       *cancellable,
                             GAsyncReadyCallback callback,
                             gpointer            user_data)
{
    g_assert (IBUS_IS_CONFIG (config));
    g_assert (section != NULL);
    g_assert (name != NULL);

    g_dbus_proxy_call ((GDBusProxy *)config,
                       "GetValue",
                       g_variant_new ("(ss)", section, name),
                       G_DBUS_CALL_FLAGS_NONE,
                       timeout_ms,
                       cancellable,
                       callback,
                       user_data);
}

GVariant *
ibus_config_get_value_async_finish (IBusConfig    *config,
                                    GAsyncResult  *result,
                                    GError       **error)
{
    g_assert (IBUS_IS_CONFIG (config));
    g_assert (G_IS_ASYNC_RESULT (result));
    g_assert (error == NULL || *error == NULL);

    GVariant *value = NULL;
    GVariant *retval = g_dbus_proxy_call_finish ((GDBusProxy *)config,
                                                 result,
                                                 error);
    if (retval != NULL) {
        g_variant_get (retval, "(v)", &value);
        g_variant_unref (retval);
    }

    return value;
}

GVariant *
ibus_config_get_values (IBusConfig  *config,
                        const gchar *section)
{
    g_assert (IBUS_IS_CONFIG (config));
    g_assert (section != NULL);

    GError *error = NULL;
    GVariant *result;
    result = g_dbus_proxy_call_sync ((GDBusProxy *) config,
                                     "GetValues",
                                     g_variant_new ("(s)", section),
                                     G_DBUS_CALL_FLAGS_NONE,
                                     -1,
                                     NULL,
                                     &error);
    if (result == NULL) {
        g_warning ("%s.GetValues: %s", IBUS_INTERFACE_CONFIG, error->message);
        g_error_free (error);
        return NULL;
    }

    GVariant *value = NULL;
    g_variant_get (result, "(@a{sv})", &value);
    g_variant_unref (result);

    return value;
}

void
ibus_config_get_values_async (IBusConfig         *config,
                              const gchar        *section,
                              gint                timeout_ms,
                              GCancellable       *cancellable,
                              GAsyncReadyCallback callback,
                              gpointer            user_data)
{
    g_assert (IBUS_IS_CONFIG (config));
    g_assert (section != NULL);

    g_dbus_proxy_call ((GDBusProxy *)config,
                       "GetValues",
                       g_variant_new ("(s)", section),
                       G_DBUS_CALL_FLAGS_NONE,
                       timeout_ms,
                       cancellable,
                       callback,
                       user_data);
}

GVariant *
ibus_config_get_values_async_finish (IBusConfig    *config,
                                     GAsyncResult  *result,
                                     GError       **error)
{
    g_assert (IBUS_IS_CONFIG (config));
    g_assert (G_IS_ASYNC_RESULT (result));
    g_assert (error == NULL || *error == NULL);

    GVariant *value = NULL;
    GVariant *retval = g_dbus_proxy_call_finish ((GDBusProxy *)config,
                                                 result,
                                                 error);
    if (retval != NULL) {
        g_variant_get (retval, "(@a{sv})", &value);
        g_variant_unref (retval);
    }

    return value;
}
    
gboolean
ibus_config_set_value (IBusConfig   *config,
                       const gchar  *section,
                       const gchar  *name,
                       GVariant     *value)
{
    g_assert (IBUS_IS_CONFIG (config));
    g_assert (section != NULL);
    g_assert (name != NULL);
    g_assert (value != NULL);

    GError *error = NULL;
    GVariant *result;
    result = g_dbus_proxy_call_sync ((GDBusProxy *) config,
                                     "SetValue",                /* method_name */
                                     g_variant_new ("(ssv)",
                                        section, name, value),  /* parameters */
                                     G_DBUS_CALL_FLAGS_NONE,    /* flags */
                                     -1,                        /* timeout */
                                     NULL,                      /* cancellable */
                                     &error                     /* error */
                                     );
    if (result == NULL) {
        g_warning ("%s.SetValue: %s", IBUS_INTERFACE_CONFIG, error->message);
        g_error_free (error);
        return FALSE;
    }
    g_variant_unref (result);
    return TRUE;
}

void
ibus_config_set_value_async (IBusConfig         *config,
                             const gchar        *section,
                             const gchar        *name,
                             GVariant           *value,
                             gint                timeout_ms,
                             GCancellable       *cancellable,
                             GAsyncReadyCallback callback,
                             gpointer            user_data)
{
    g_assert (IBUS_IS_CONFIG (config));
    g_assert (section != NULL);
    g_assert (name != NULL);
    g_assert (value != NULL);

    g_dbus_proxy_call ((GDBusProxy *) config,
                       "SetValue",                /* method_name */
                       g_variant_new ("(ssv)",
                                      section, name, value),  /* parameters */
                       G_DBUS_CALL_FLAGS_NONE,    /* flags */
                       timeout_ms,
                       cancellable,
                       callback,
                       user_data);
}

gboolean
ibus_config_set_value_async_finish (IBusConfig         *config,
                                    GAsyncResult       *result,
                                    GError            **error)
{
    g_assert (IBUS_IS_CONFIG (config));
    g_assert (G_IS_ASYNC_RESULT (result));
    g_assert (error == NULL || *error == NULL);

    GVariant *retval = g_dbus_proxy_call_finish ((GDBusProxy *)config,
                                                 result,
                                                 error);
    if (retval != NULL) {
        g_variant_unref (retval);
        return TRUE;
    }

    return FALSE;
}

gboolean
ibus_config_unset (IBusConfig   *config,
                   const gchar  *section,
                   const gchar  *name)
{
    g_assert (IBUS_IS_CONFIG (config));
    g_assert (section != NULL);
    g_assert (name != NULL);

    GError *error = NULL;
    GVariant *result;
    result = g_dbus_proxy_call_sync ((GDBusProxy *) config,
                                     "UnsetValue",              /* method_name */
                                     g_variant_new ("(ss)",
                                        section, name),         /* parameters */
                                     G_DBUS_CALL_FLAGS_NONE,    /* flags */
                                     -1,                        /* timeout */
                                     NULL,                      /* cancellable */
                                     &error                     /* error */
                                     );
    if (result == NULL) {
        g_warning ("%s.UnsetValue: %s", IBUS_INTERFACE_CONFIG, error->message);
        g_error_free (error);
        return FALSE;
    }
    g_variant_unref (result);
    return TRUE;
}
