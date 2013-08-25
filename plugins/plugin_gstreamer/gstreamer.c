/*
 *
 *  MyPlayer
 *
 *  Copyright © 2013 Sébastien Bianti
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <gst/gst.h>
#include <gst/pbutils/pbutils.h>

#include <myp_plugin.h>
#include <print.h>

static char *current_uri;

static void myp_gst_init(int argc, char *argv[])
{
  gst_init(&argc, &argv);
}

static void myp_gst_quit()
{
  
}

static gboolean myp_seturi(char *uri)
{
  if (g_strcmp0(uri, current_uri) == 0)
    return TRUE;
  else
    g_free(current_uri);

  /* Need to check the validity of file://file-path because */
  /* gst_uri_is_valid returns always true in that case */
  if (g_str_has_prefix(uri, "file://"))
    uri = uri+7;

  if (gst_uri_is_valid(uri)) {
    current_uri = g_strdup(uri);
    return TRUE;
  }

  if (g_file_test(uri, G_FILE_TEST_IS_REGULAR) == TRUE) {
    current_uri = g_strdup_printf("file://%s", uri);
    return TRUE;
  }

  if (uri[0] != '~')
    goto error;

  current_uri = g_strdup_printf("file://%s%s", g_get_home_dir(), uri+1);
  if (g_file_test(current_uri + 7, G_FILE_TEST_IS_REGULAR) == FALSE) {
    g_free(current_uri);
    goto error;
  }

  return TRUE;

 error:
  current_uri = NULL;
  return FALSE;
}

static void finished_cb(GstDiscoverer *discoverer, void *null_data) {
  printl("¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯");
  gst_discoverer_stop(discoverer);
}

static void prv_print_tag_foreach(const GstTagList *tags, const gchar *tag,
				  gpointer user_data) {
  GValue val = { 0, };
  gchar *str;
  gint depth = GPOINTER_TO_INT(user_data);

  gst_tag_list_copy_value(&val, tags, tag);

  if (G_VALUE_HOLDS_STRING(&val))
    str = g_value_dup_string(&val);
  else
    str = gst_value_serialize(&val);

  printl("%*s%s: %s", 2 * depth, " ", gst_tag_get_nick(tag), str);
  g_free(str);

  g_value_unset(&val);
}

static void prv_print_stream_info(GstDiscovererStreamInfo *info, gint depth) {
  gchar *desc = NULL;
  GstCaps *caps;
  const GstTagList *tags;

  caps = gst_discoverer_stream_info_get_caps(info);

  if (caps) {
    if (gst_caps_is_fixed(caps))
      desc = gst_pb_utils_get_codec_description(caps);
    else
      desc = gst_caps_to_string(caps);
    gst_caps_unref(caps);
  }

  printl("%*s%s: %s", 2 * depth, " ",
	 gst_discoverer_stream_info_get_stream_type_nick(info),
	 (desc ? desc : ""));

  if (desc) {
    g_free(desc);
    desc = NULL;
  }

  tags = gst_discoverer_stream_info_get_tags(info);
  if (tags) {
    printl("%*sTags:", 2 * (depth + 1), " ");
    gst_tag_list_foreach(tags, prv_print_tag_foreach,
			 GINT_TO_POINTER(depth + 2));
    puts("");
  }
}

static void prv_print_topology(GstDiscovererStreamInfo *info, gint depth) {
  GstDiscovererStreamInfo *next;

  if (!info)
    return;

  prv_print_stream_info(info, depth);

  next = gst_discoverer_stream_info_get_next(info);
  if (next) {
    prv_print_topology(next, depth + 1);
    gst_discoverer_stream_info_unref(next);
  } else if (GST_IS_DISCOVERER_CONTAINER_INFO(info)) {
    GList *tmp, *streams;

    streams = gst_discoverer_container_info_get_streams
      (GST_DISCOVERER_CONTAINER_INFO(info));

    for (tmp = streams; tmp; tmp = tmp->next) {
      GstDiscovererStreamInfo *tmpinf = (GstDiscovererStreamInfo *) tmp->data;
      prv_print_topology(tmpinf, depth + 1);
    }
    gst_discoverer_stream_info_list_free(streams);
  }
}

static void discovered_cb(GstDiscoverer *discoverer, GstDiscovererInfo *info,
			  GError *err, void *null_data) {
  GstDiscovererResult result;
  const gchar *uri;
  const GstTagList *tags;
  GstDiscovererStreamInfo *sinfo;

  uri = gst_discoverer_info_get_uri(info);
  result = gst_discoverer_info_get_result(info);
  switch (result) {
  case GST_DISCOVERER_URI_INVALID:
    printerrl("Invalid URI '%s'", uri);
    break;
  case GST_DISCOVERER_ERROR:
    printerrl("Discoverer error: %s", err->message);
    break;
  case GST_DISCOVERER_TIMEOUT:
    printerrl("Timeout");
    break;
  case GST_DISCOVERER_BUSY:
    printerrl("Busy\n");
    break;
  case GST_DISCOVERER_MISSING_PLUGINS:{
    const GstStructure *s = gst_discoverer_info_get_misc(info);
    gchar *str = gst_structure_to_string(s);

    printerrl("Missing plugins: %s", str);
    g_free(str);
    break;
  }
  case GST_DISCOVERER_OK:
    break;
  }

  if (result != GST_DISCOVERER_OK) {
    printerrl("<%s> cannot be played", uri);
    return;
  }

  tags = gst_discoverer_info_get_tags(info);
  if (tags) {
    printl("Clip info:");
    gst_tag_list_foreach(tags, prv_print_tag_foreach, GINT_TO_POINTER(1));
  }

  printl("Seekable: %s",
	 gst_discoverer_info_get_seekable(info) ? "yes" : "no");

  sinfo = gst_discoverer_info_get_stream_info(info);
  if (!sinfo)
    return;

  printl("Stream information:");
  prv_print_topology(sinfo, 0);

  gst_discoverer_stream_info_unref(sinfo);

  printl("Duration: %" GST_TIME_FORMAT "",
  	  GST_TIME_ARGS(gst_discoverer_info_get_duration(info)));

}

static gboolean prv_discover(char *uri, GError **err)
{
  GstDiscoverer *discoverer = gst_discoverer_new(5*GST_SECOND, err);

  if (discoverer == NULL)
    return FALSE;

  g_signal_connect(discoverer, "discovered", G_CALLBACK(discovered_cb), NULL);
  g_signal_connect(discoverer, "finished", G_CALLBACK(finished_cb), NULL);

  gst_discoverer_start(discoverer);

  if (gst_discoverer_discover_uri_async(discoverer, uri) == FALSE) {
    printerrl("Failed to start discovering URI '%s'", uri);
    return FALSE;
  }

  return TRUE;
}

static gboolean myp_play()
{
  GError *err = NULL;

  printl("playing %s\n", current_uri);
  if (prv_discover(current_uri, &err) == FALSE) {
    printerrl("Error discovering uri %s:\n%s", current_uri, err->message);
    g_clear_error(&err);
  }
}

static gboolean myp_play_pause()
{

}

static gboolean myp_stop()
{

}

static gboolean myp_set_prop()
{

}

static enum myp_plugin_status_t myp_status()
{
  return STATUS_NULL;
}

static char *myp_plugin_name()
{

}

static char *myp_plugin_version()
{

}

static char *myp_plugin_info()
{

}

myp_plugin_t prepare_plugin()
{
  myp_plugin_t gst_plugin = MYP_PLUGIN_NEW();

  gst_plugin->init = myp_gst_init;
  gst_plugin->quit = myp_gst_quit;
  gst_plugin->seturi = myp_seturi;
  gst_plugin->play = myp_play;
  gst_plugin->play_pause = myp_play_pause;
  gst_plugin->stop = myp_stop;
  gst_plugin->set_prop = myp_set_prop;
  gst_plugin->status = myp_status;
  gst_plugin->plugin_name = myp_plugin_name;
  gst_plugin->plugin_version = myp_plugin_version;
  gst_plugin->plugin_info = myp_plugin_info;

  return gst_plugin;
}

