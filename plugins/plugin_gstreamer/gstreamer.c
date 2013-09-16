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
#include <gst/video/gstvideosink.h>
#include <gst/video/videooverlay.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#if defined (GDK_WINDOWING_X11)
#include <gdk/gdkx.h>
#endif

#include <stdio.h>

#include <myp_plugin.h>
#include <print.h>

#define TIME_FORMAT "u:%02u:%2.1f"
#define A_TIME_FORMAT "A: %"TIME_FORMAT
#define V_TIME_FORMAT "V: %"TIME_FORMAT
#define AV_TIME_FORMAT A_TIME_FORMAT" "V_TIME_FORMAT" A-V: %.3f"
#define SPEED_FORMAT "speed: %.1f"

#define HOURS(nanosec) (guint) (nanosec / (3600 * GST_SECOND))
#define MINUTES(nanosec) (guint) ((nanosec / (60 * GST_SECOND)) % 60)
#define SECONDS(nanosec) ((nanosec / 1000000) % 60000) / 1000.0
#define TIME_ARGS(nanosec) HOURS(nanosec), MINUTES(nanosec), SECONDS(nanosec)

static char *current_uri;
static GstElement *pipeline;
static GstBus *bus;
static GstState state;
static char *plugin_info;

static gboolean current_stream_is_seekable;
static gint64 current_duration;
static gdouble current_speed;
static gboolean stream_initiated;
static GtkWidget *video_window;
static GstElement *video_sink;
static GstElement *audio_sink;
static gint native_height;
static gint native_width;

struct {
  gboolean timeline_visible;
  gboolean fullscreen;
} prop;

enum {
  CONTAINS_VIDEO = 0x1,
  CONTAINS_AUDIO = 0x2
} stream_types;

#define PLUGIN_NAME "MypGStreamer"
#define MYP_GST_VERSION "0.0.1"

static void myp_gst_init(int argc, char *argv[])
{
  guint major, minor, micro, nano;

  gst_init(&argc, &argv);
  gtk_init(&argc, &argv);

  gst_version(&major, &minor, &micro, &nano);
  plugin_info = g_strdup_printf("Plugin GStreamer for MyPlayer based on "
				"GStreamer-%d.%d.%d.%d",
				major, minor, micro, nano);

  prop.timeline_visible = TRUE;
  prop.fullscreen = FALSE;
  stream_types = 0;
  video_window = NULL;
}

static gboolean myp_stop()
{
  if (GST_IS_ELEMENT(pipeline) == FALSE)
    return FALSE;

  gst_object_unref(bus);
  gst_element_set_state(pipeline, GST_STATE_NULL);
  gst_object_unref(pipeline);

  stream_initiated = FALSE;
  current_stream_is_seekable = FALSE;
  video_sink = NULL;
  audio_sink = NULL;
  state = GST_STATE_NULL;
  prop.fullscreen = FALSE;
  stream_types = 0;

  if (video_window) {
    gtk_widget_destroy(video_window);
    video_window = NULL;
  }

  return TRUE;
}

static void myp_gst_quit()
{
  printl("\n%s says bye bye!", PLUGIN_NAME);

  if (GST_IS_ELEMENT(pipeline))
    myp_stop();

  g_free(plugin_info);

  gst_deinit();
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

  if (g_strcmp0(gst_discoverer_stream_info_get_stream_type_nick(info),
		"video") == 0)
    stream_types |= CONTAINS_VIDEO;

  if (g_strcmp0(gst_discoverer_stream_info_get_stream_type_nick(info),
		"audio") == 0)
    stream_types |= CONTAINS_AUDIO;

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

  printl("Duration: %" TIME_FORMAT "",
  	  TIME_ARGS(gst_discoverer_info_get_duration(info)));

}

static gboolean prv_discover(char *uri, GError **err)
{
  GstDiscoverer *discoverer = gst_discoverer_new(5*GST_SECOND, err);
  GstDiscovererInfo *info;

  if (discoverer == NULL)
    return FALSE;

  info = gst_discoverer_discover_uri(discoverer, uri, NULL);
  if (info == FALSE) {
    printerrl("Failed to start discovering URI '%s'", uri);
    return FALSE;
  }

  discovered_cb(discoverer, info, NULL, NULL);

  return TRUE;
}

static gboolean myp_set_speed(gboolean relative, gdouble val)
{
  GstEvent *seek_event;

  if (relative)
    current_speed = current_speed * val;
  else
    current_speed = val;

  seek_event =
    gst_event_new_seek(current_speed, GST_FORMAT_TIME,
		       GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE,
		       GST_SEEK_TYPE_NONE, -1, GST_SEEK_TYPE_NONE, -1);

  if (GST_IS_ELEMENT(video_sink))
    gst_element_send_event(video_sink, seek_event);
  else
    gst_element_send_event(audio_sink, seek_event);

  return TRUE;
}

static gboolean myp_step(int n_frame)
{
  if (GST_IS_ELEMENT(video_sink) == FALSE)
    return FALSE;

  gst_element_set_state(pipeline, GST_STATE_PAUSED);

  gst_element_send_event(video_sink,
			 gst_event_new_step(GST_FORMAT_BUFFERS, n_frame,
					    current_speed, TRUE, FALSE));

  return TRUE;
}

static gint64 get_position(GstElement *element)
{
  gint64 current_pos = 0;

  if (!gst_element_query_position(element, GST_FORMAT_TIME, &current_pos))
    return -1;

  return current_pos;
}

static gboolean prv_refresh_ui(void *null_data)
{
  gint64 a_pos, v_pos;

  if (state == GST_STATE_PAUSED)
    return TRUE;
  if (state < GST_STATE_PAUSED) {
    puts("");
    return FALSE;
  }

  switch (stream_types) {
  case CONTAINS_VIDEO:
    print(V_TIME_FORMAT" "SPEED_FORMAT"\r",
	  TIME_ARGS(get_position(video_sink)), current_speed);
    break;
  case CONTAINS_AUDIO:
    print(A_TIME_FORMAT" "SPEED_FORMAT"\r",
    	  TIME_ARGS(get_position(audio_sink)), current_speed);
    break;
  case CONTAINS_VIDEO | CONTAINS_AUDIO:
    a_pos = get_position(audio_sink);
    v_pos = get_position(video_sink);
    print(AV_TIME_FORMAT" "SPEED_FORMAT"\r", TIME_ARGS(a_pos), TIME_ARGS(v_pos),
	  (float)(a_pos - v_pos) / GST_SECOND, current_speed);
    break;
  }

  fflush(stdout);

  return TRUE;
}

static gboolean prv_toggle_fullscreen()
{
  prop.fullscreen = !prop.fullscreen;
  printl("%s", prop.fullscreen ? "Fullscreen":"Native Definition");
  if (prop.fullscreen)
    gtk_window_fullscreen(GTK_WINDOW(video_window));
  else
    gtk_window_unfullscreen(GTK_WINDOW(video_window));
}

static void realize_cb(GtkWidget *widget, void *null_data)
{
  guintptr window_handle;
  GdkWindow *window = gtk_widget_get_window(widget);

  if (!gdk_window_ensure_native(window))
    printerrl("Couldn't create native window needed for GstVideoOverlay!");

#if defined (GDK_WINDOWING_X11)
  window_handle = GDK_WINDOW_XID(window);
#endif

  gst_video_overlay_set_window_handle(GST_VIDEO_OVERLAY(pipeline),
				      window_handle);
}

static gboolean draw_cb(GtkWidget *widget, GdkEventExpose *event,
			void *null_data)
{
  GtkAllocation allocation;
  GdkWindow *window;
  cairo_t *cr;

  if (state > GST_STATE_READY)
    return FALSE;

  window = gtk_widget_get_window(widget);

  gtk_widget_get_allocation(widget, &allocation);

  cr = gdk_cairo_create(window);

  cairo_set_source_rgb(cr, 0, 0, 0);

  cairo_rectangle(cr, 0, 0, allocation.width, allocation.height);

  cairo_fill(cr);

  cairo_destroy(cr);

  return FALSE;
}

static void create_window()
{
  GtkWidget *inner_window = gtk_drawing_area_new();

  video_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  gtk_widget_set_double_buffered(inner_window, FALSE);

  g_signal_connect(inner_window, "realize", G_CALLBACK(realize_cb), NULL);
#if GTK_MAJOR_VERSION == 2
  g_signal_connect(inner_window, "expose_event", G_CALLBACK(draw_cb), NULL);
#else
  g_signal_connect(inner_window, "draw", G_CALLBACK(draw_cb), NULL);
#endif

  gtk_container_add(GTK_CONTAINER(video_window), inner_window);

  gtk_widget_show_all(video_window);
}

static void prv_init_stream_attributes()
{
  GstQuery *query = gst_query_new_seeking(GST_FORMAT_TIME);
  gint64 start, end;

  if (gst_element_query(pipeline, query)) {
    gst_query_parse_seeking(query, NULL, &current_stream_is_seekable,
			    &start, &end);

    if (current_stream_is_seekable == FALSE)
      printl("This stream is not seekable");
  }

  if (!gst_element_query_duration(pipeline, GST_FORMAT_TIME,
				  &current_duration))
    printerrl("/!\\ Could not query current duration. /!\\");

  if (current_speed != 1.0)
    myp_set_speed(FALSE, current_speed);

  stream_types = 0;

  g_object_get(pipeline, "video-sink", &video_sink, NULL);
  g_object_get(pipeline, "audio-sink", &audio_sink, NULL);

  if (GST_IS_ELEMENT(video_sink)) {
    stream_types |= CONTAINS_VIDEO;

    printl("video sink: %s", GST_OBJECT_NAME(video_sink));

    native_height = GST_VIDEO_SINK_HEIGHT(video_sink);
    native_width = GST_VIDEO_SINK_WIDTH(video_sink);

    printl("definition is %d×%d", native_width, native_height);

    gtk_widget_set_size_request(video_window, native_width, native_height);
  }

  if (GST_IS_ELEMENT(audio_sink)) {
    stream_types |= CONTAINS_AUDIO;
    printl("audio sink: %s", GST_OBJECT_NAME(audio_sink));
  }

  if (prop.timeline_visible)
    g_timeout_add(100, (GSourceFunc)prv_refresh_ui, NULL);

  if (prop.fullscreen)
    gtk_window_fullscreen(GTK_WINDOW(video_window));

  stream_initiated = TRUE;
}

static void pipeline_message_cb(GstBus *bus, GstMessage *msg, void *null_data)
{
  switch (GST_MESSAGE_TYPE(msg)) {
  case GST_MESSAGE_ERROR: {
    GError *err;
    gchar *debug;

    gst_message_parse_error(msg, &err, &debug);
    printerrl("\nError: %s", err->message);
    g_error_free(err);
    g_free(debug);

    gst_element_set_state(pipeline, GST_STATE_READY);
    myp_stop();
    break;
  }
  case GST_MESSAGE_STATE_CHANGED:
    if (GST_MESSAGE_SRC(msg) != GST_OBJECT(pipeline))
      break;

    gst_message_parse_state_changed(msg, NULL, &state, NULL);

    if (stream_initiated == FALSE && state == GST_STATE_PAUSED)
	prv_init_stream_attributes();
    break;
  case GST_MESSAGE_EOS:
    gst_element_set_state(pipeline, GST_STATE_READY);
    myp_stop();
    break;
  default:
    break;
  }
}

static gboolean myp_play(gdouble speed, gboolean fullscreen)
{
  GError *err = NULL;
  char *pipeline_str;

  if (state > GST_STATE_READY)
    myp_stop();

  printl("playing %s\n", current_uri);
  if (prv_discover(current_uri, &err) == FALSE) {
    printerrl("Error discovering uri %s:\n%s", current_uri, err->message);
    g_clear_error(&err);
  }

  pipeline_str = g_strdup_printf("playbin uri=%s", current_uri);

  pipeline = gst_parse_launch(pipeline_str, NULL);
  bus = gst_element_get_bus(pipeline);

  g_free(pipeline_str);

  current_speed = speed;
  prop.fullscreen = fullscreen;

  if (gst_element_set_state(pipeline, GST_STATE_PLAYING) ==
      GST_STATE_CHANGE_FAILURE) {
    printerrl("Unable to set pipeline to playing state !");

    gst_object_unref(bus);
    gst_object_unref(pipeline);

    return FALSE;
  }

  gst_bus_add_signal_watch(bus);
  g_signal_connect(bus, "message", G_CALLBACK(pipeline_message_cb), NULL);

  if (stream_types & CONTAINS_VIDEO)
    create_window();

  return TRUE;
}

static gboolean myp_play_pause()
{
  switch (state) {
  case GST_STATE_PLAYING:
    gst_element_set_state(pipeline, GST_STATE_PAUSED);
    break;
  case GST_STATE_PAUSED:
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    break;
  default:
    return FALSE;
  }

  return TRUE;
}

static gboolean myp_seek(gint64 seek)
{
  gint64 current_pos = 0;
  gint64 new_pos;

  if (current_stream_is_seekable == FALSE)
    return FALSE;

  current_pos = get_position(pipeline);
  if (current_pos == -1)
    printerrl("/!\\ Could not query current position. /!\\");

  new_pos = seek * GST_SECOND + current_pos;
  if (new_pos < 0)
    new_pos = 0;
  else if (new_pos > current_duration)
    new_pos = current_duration;

  return gst_element_seek_simple(pipeline, GST_FORMAT_TIME,
				 GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT,
				 new_pos);
}

static gboolean myp_set_pos(gint64 pos)
{
  if (pos == -1)
    pos = current_duration;

  return gst_element_seek_simple(pipeline, GST_FORMAT_TIME,
				 GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT,
				 pos);
}

static gboolean myp_set_prop(const char *name, gboolean activate)
{
  gboolean ret = TRUE;

  if (g_strcmp0(name, PROP_TIMELINE_VISIBLE) == 0)
    prop.timeline_visible = activate;
  else
    ret = FALSE;

  return ret;
}

static enum myp_plugin_status_t myp_status()
{
  switch (state) {
  case GST_STATE_VOID_PENDING:
  case GST_STATE_NULL:
  case GST_STATE_READY:
    return STATUS_NULL;

  case GST_STATE_PAUSED:
    return STATUS_PAUSED;

  case GST_STATE_PLAYING:
    return STATUS_PLAYING;
  }
}

static const char *myp_plugin_name()
{
  return PLUGIN_NAME;
}

static const char *myp_plugin_version()
{
  return MYP_GST_VERSION;
}

static const char *myp_plugin_info()
{
  return plugin_info;
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
  gst_plugin->seek = myp_seek;
  gst_plugin->set_pos = myp_set_pos;
  gst_plugin->set_speed = myp_set_speed;
  gst_plugin->step = myp_step;

  gst_plugin->set_prop = myp_set_prop;
  gst_plugin->toggle_fullscreen = prv_toggle_fullscreen;
  gst_plugin->status = myp_status;

  gst_plugin->plugin_name = myp_plugin_name;
  gst_plugin->plugin_version = myp_plugin_version;
  gst_plugin->plugin_info = myp_plugin_info;

  return gst_plugin;
}
