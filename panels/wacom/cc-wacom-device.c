/*
 * Copyright © 2016 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Carlos Garnacho <carlosg@gnome.org>
 *
 */

#include "config.h"

#include <string.h>
#include "cc-wacom-device.h"

enum {
	PROP_0,
	PROP_DEVICE,
	N_PROPS
};

GParamSpec *props[N_PROPS] = { 0 };

typedef struct _CcWacomDevice CcWacomDevice;

struct _CcWacomDevice {
	GObject parent_instance;

	GsdDevice *device;
	WacomDevice *wdevice;
};

static void cc_wacom_device_initable_iface_init (GInitableIface *iface);

G_DEFINE_TYPE_WITH_CODE (CcWacomDevice, cc_wacom_device, G_TYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE,
						cc_wacom_device_initable_iface_init))

WacomDeviceDatabase *
cc_wacom_device_database_get (void)
{
	static WacomDeviceDatabase *db = NULL;

	if (g_once_init_enter (&db)) {
		gpointer p = libwacom_database_new ();
		g_once_init_leave (&db, p);
	}

	return db;
}

static void
cc_wacom_device_init (CcWacomDevice *device)
{
}

static void
cc_wacom_device_set_property (GObject      *object,
			      guint         prop_id,
			      const GValue *value,
			      GParamSpec   *pspec)
{
	CcWacomDevice *device = CC_WACOM_DEVICE (object);

	switch (prop_id) {
	case PROP_DEVICE:
		device->device = g_value_get_object (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
cc_wacom_device_get_property (GObject    *object,
			      guint       prop_id,
			      GValue     *value,
			      GParamSpec *pspec)
{
	CcWacomDevice *device = CC_WACOM_DEVICE (object);

	switch (prop_id) {
	case PROP_DEVICE:
		g_value_set_object (value, device->device);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
cc_wacom_device_finalize (GObject *object)
{
	CcWacomDevice *device = CC_WACOM_DEVICE (object);

	g_clear_pointer (&device->wdevice, libwacom_destroy);

	G_OBJECT_CLASS (cc_wacom_device_parent_class)->finalize (object);
}

static void
cc_wacom_device_class_init (CcWacomDeviceClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->set_property = cc_wacom_device_set_property;
	object_class->get_property = cc_wacom_device_get_property;
	object_class->finalize = cc_wacom_device_finalize;

	props[PROP_DEVICE] =
		g_param_spec_object ("device",
				     "device",
				     "device",
				     GSD_TYPE_DEVICE,
				     G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

	g_object_class_install_properties (object_class, N_PROPS, props);
}

static gboolean
cc_wacom_device_initable_init (GInitable     *initable,
			       GCancellable  *cancellable,
			       GError       **error)
{
	CcWacomDevice *device = CC_WACOM_DEVICE (initable);
	WacomDeviceDatabase *wacom_db;
	const gchar *node_path;

	wacom_db = cc_wacom_device_database_get ();
	node_path = gsd_device_get_device_file (device->device);
	device->wdevice = libwacom_new_from_path (wacom_db, node_path, FALSE, NULL);

	if (!device->wdevice) {
		g_set_error (error, 0, 0, "Tablet description not found");
		return FALSE;
	}

	return TRUE;
}

static void
cc_wacom_device_initable_iface_init (GInitableIface *iface)
{
	iface->init = cc_wacom_device_initable_init;
}

CcWacomDevice *
cc_wacom_device_new (GsdDevice *device)
{
	return g_initable_new (CC_TYPE_WACOM_DEVICE,
			       NULL, NULL,
			       "device", device,
			       NULL);
}

const gchar *
cc_wacom_device_get_name (CcWacomDevice *device)
{
	g_return_val_if_fail (CC_IS_WACOM_DEVICE (device), NULL);

	return libwacom_get_name (device->wdevice);
}

const gchar *
cc_wacom_device_get_icon_name (CcWacomDevice *device)
{
	WacomIntegrationFlags integration_flags;

	g_return_val_if_fail (CC_IS_WACOM_DEVICE (device), NULL);

	integration_flags = libwacom_get_integration_flags (device->wdevice);

	if (integration_flags & WACOM_DEVICE_INTEGRATED_SYSTEM) {
		return "wacom-tablet-pc";
	} else if (integration_flags & WACOM_DEVICE_INTEGRATED_DISPLAY) {
		return "wacom-tablet-cintiq";
	} else {
		return "wacom-tablet";
	}
}

gboolean
cc_wacom_device_is_reversible (CcWacomDevice *device)
{
	g_return_val_if_fail (CC_IS_WACOM_DEVICE (device), FALSE);

	return libwacom_is_reversible (device->wdevice);
}

WacomIntegrationFlags
cc_wacom_device_get_integration_flags (CcWacomDevice *device)
{
	g_return_val_if_fail (CC_IS_WACOM_DEVICE (device), 0);

	return libwacom_get_integration_flags (device->wdevice);
}

GsdDevice *
cc_wacom_device_get_device (CcWacomDevice *device)
{
	g_return_val_if_fail (CC_IS_WACOM_DEVICE (device), NULL);

	return device->device;
}

GSettings *
cc_wacom_device_get_settings (CcWacomDevice *device)
{
	g_return_val_if_fail (CC_IS_WACOM_DEVICE (device), NULL);

	return gsd_device_get_settings (device->device);
}

const gint *
cc_wacom_device_get_supported_tools (CcWacomDevice *device,
				     gint          *n_tools)
{
	*n_tools = 0;

	g_return_val_if_fail (CC_IS_WACOM_DEVICE (device), NULL);

	return libwacom_get_supported_styli (device->wdevice, n_tools);
}

static GnomeRROutput *
find_output_by_edid (GnomeRRScreen *rr_screen,
		     const gchar   *vendor,
		     const gchar   *product,
		     const gchar   *serial)
{
	GnomeRROutput **rr_outputs;
	GnomeRROutput *retval = NULL;
	guint i;

	rr_outputs = gnome_rr_screen_list_outputs (rr_screen);

	for (i = 0; rr_outputs[i] != NULL; i++) {
		gchar *o_vendor, *o_product, *o_serial;
		gboolean match;

		gnome_rr_output_get_ids_from_edid (rr_outputs[i],
						   &o_vendor,
						   &o_product,
						   &o_serial);

		g_debug ("Checking for match between '%s','%s','%s' and '%s','%s','%s'", \
		         vendor, product, serial, o_vendor, o_product, o_serial);

		match = (g_strcmp0 (vendor,  o_vendor)  == 0) && \
		        (g_strcmp0 (product, o_product) == 0) && \
		        (g_strcmp0 (serial,  o_serial)  == 0);

		g_free (o_vendor);
		g_free (o_product);
		g_free (o_serial);

		if (match) {
			retval = rr_outputs[i];
			break;
		}
	}

	if (retval == NULL)
		g_debug ("Did not find a matching output for EDID '%s,%s,%s'",
			 vendor, product, serial);

	return retval;
}

static GnomeRROutput *
find_output (GnomeRRScreen *rr_screen,
	     CcWacomDevice *device)
{
	GSettings *settings;
	GVariant *variant;
	const gchar **edid;
	GnomeRROutput *ret = NULL;
	gsize n;

	settings = cc_wacom_device_get_settings (device);
	variant = g_settings_get_value (settings, "display");
	edid = g_variant_get_strv (variant, &n);

	if (n != 3) {
		g_critical ("Expected 'display' key to store %d values; got %"G_GSIZE_FORMAT".", 3, n);
		goto out;
	}

	if (strlen (edid[0]) == 0 || strlen (edid[1]) == 0 || strlen (edid[2]) == 0)
		goto out;

	ret = find_output_by_edid (rr_screen, edid[0], edid[1], edid[2]);

out:
	g_free (edid);
	g_variant_unref (variant);
	g_object_unref (settings);

	return ret;
}

GnomeRROutput *
cc_wacom_device_get_output (CcWacomDevice *device,
			    GnomeRRScreen *rr_screen)
{
	GnomeRROutput *rr_output;
	GnomeRRCrtc *crtc;

        g_return_val_if_fail (CC_IS_WACOM_DEVICE (device), NULL);
        g_return_val_if_fail (GNOME_IS_RR_SCREEN (rr_screen), NULL);

	rr_output = find_output (rr_screen, device);
	if (rr_output == NULL) {
		return NULL;
	}

	crtc = gnome_rr_output_get_crtc (rr_output);

	if (!crtc || gnome_rr_crtc_get_current_mode (crtc) == NULL) {
		g_debug ("Output is not active.");
		return NULL;
	}

	return rr_output;
}

void
cc_wacom_device_set_output (CcWacomDevice *device,
			    GnomeRROutput *output)
{
	GSettings *settings;
	gchar *vendor, *product, *serial;
	const gchar *values[] = { "", "", "", NULL };

        g_return_if_fail (CC_IS_WACOM_DEVICE (device));

	vendor = product = serial = NULL;
	settings = cc_wacom_device_get_settings (device);

	if (output != NULL) {
		gnome_rr_output_get_ids_from_edid (output,
						   &vendor,
						   &product,
						   &serial);
		values[0] = vendor;
		values[1] = product;
		values[2] = serial;
	}

	g_settings_set_strv (settings, "display", values);

	g_free (vendor);
	g_free (product);
	g_free (serial);
	g_object_unref (settings);
}