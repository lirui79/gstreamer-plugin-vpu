/*
 * Simple GStreamer HEVC/H.265 video player.
 *
 * Copyright (c) 2014 struktur AG, Joachim Bauch <bauch@struktur.de>
 *
 * This file is part of gstreamer-libde265.
 *
 * libde265 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * libde265 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libde265.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <gst/gst.h>
#include <glib.h>
#ifdef G_OS_UNIX
#include <glib-unix.h>
#endif
#include <stdio.h>
#include <string.h>


void usage(char* cmd) {
    printf("Usage: %s [-h] -b boardid -i input -o output -s skip -k\n", cmd);
    printf("Options:\n");
    printf("  -h              Print this help message.\n");
    printf("  -b boardid      set decoder board-idx, which board decodes data.\n");
    printf("  -i input        set filesrc location.\n");
    printf("  -o output       set filssink location.\n");
    printf("  -s skip         set decoder skip-frames, save one frame by every skip frames, when there is I frame or IDR frame in the frames, it is saved, default 30.\n");
    printf("  -k              set decoder skip-frames is 0, IDR frames saved\n");
    printf("\n");
    printf("Examples:\n");
    printf("  shell>  %s -b -1 -i *.h264 -o *.yuv\n", cmd);
    printf("  shell>  %s -b 1 -i *.mp4 -o *.yuv -k\n", cmd);
    printf("  shell>  %s -b 0 -i *.h264 -o *.yuv -s 30\n", cmd);
}

static gboolean
bus_callback (GstBus * bus, GstMessage * msg, gpointer data)
{
  GMainLoop *loop = (GMainLoop *) data;

  switch (GST_MESSAGE_TYPE (msg)) {
    case GST_MESSAGE_EOS:
    {
      g_print ("End of stream\n");
      g_main_loop_quit (loop);
    }
      break;

    case GST_MESSAGE_ERROR:
    {
      gchar *debug;
      GError *error;

      gst_message_parse_error (msg, &error, &debug);
      g_free (debug);

      g_printerr ("Error: %s\n", error->message);
      g_error_free (error);

      g_main_loop_quit (loop);
    }
      break;

    case GST_MESSAGE_APPLICATION:
    {
      const GstStructure *s;

      s = gst_message_get_structure (msg);
      if (gst_structure_has_name (s, "SignalInterrupt")) {
        g_main_loop_quit (loop);
      }
    }
      break;

    default:
      break;
  }

  return TRUE;
}

#ifdef G_OS_UNIX
static gboolean
sigint_handler (gpointer data)
{
  GstElement *pipeline = (GstElement *) data;

  gst_element_post_message (GST_ELEMENT (pipeline),
      gst_message_new_application (GST_OBJECT (pipeline),
          gst_structure_new ("SignalInterrupt", "message", G_TYPE_STRING,
              "Pipeline interrupted", NULL)));

  return TRUE;
}
#endif

GstElement *init_pipeline(int argc, char *argv[]) {
    GstElement *source;
    GstElement *parse;
    GstElement *decoder;
    GstElement *sink;
    GstElement *pipeline;
    GError *error = NULL;
    const char *inputfile = NULL, *outputfile = NULL;
    gint skip_frame = -1, board_idx = -1;
    const char* optstr = "hkb:i:o:s:";
    guint64 val = 0;
    gint opt;
    while( (opt = getopt(argc, argv, optstr)) != -1 ) {
        switch (opt) {
        case 'h':
            usage(argv[0]);
            return NULL;
        case 'k':// 0x01
            val |= (0x1);
            //g_object_set (G_OBJECT (decoder), "skip-frames", 0, NULL);
            skip_frame = 0;
            break;
        case 'b':// 0x02
            val |= (0x1 << 1);
            //g_object_set (G_OBJECT (decoder), "board-idx", atoi(optarg), NULL);
            board_idx = atoi(optarg);
            break;
        case 'i':// 0x04
            val |= (0x1 << 2);
            //g_object_set (G_OBJECT (source), "location", optarg, NULL);
            inputfile = optarg;
            break;
        case 'o':// 0x08
            val |= (0x1 << 3);
            //g_object_set (G_OBJECT (sink), "location", optarg, NULL);
            outputfile = optarg;
            break;
        case 's':// 0x20
            val |= (0x1 << 4);
            //g_object_set (G_OBJECT (decoder), "skip-frames", atoi(optarg), NULL);
            skip_frame = atoi(optarg);
            break;
        default:
            usage(argv[0]);
            return NULL;
        }
    }

    if ((val & 0x04) != 0x04) {
        usage(argv[0]);
        return NULL;
    }

    if (outputfile == NULL) {
        pipeline = gst_parse_launch ("filesrc name=source ! parsebin name=parse ! amsh264dec name=decoder ! fakesink name=sink", &error);
    } else {
        pipeline = gst_parse_launch ("filesrc name=source ! parsebin name=parse ! amsh264dec name=decoder ! filesink name=sink", &error);
    }

    if (error) {
        gst_printerrln ("Could not construct pipeline, error: %s", error->message);
        return NULL;
    }

    source = gst_bin_get_by_name (GST_BIN (pipeline), "source");
    if (source == NULL) {
        g_printerr ("Could not create source element\n");
        gst_object_unref (GST_OBJECT (pipeline));
        return NULL;
    }

    parse = gst_bin_get_by_name (GST_BIN (pipeline), "parse");

    if (parse == NULL) {
        g_printerr ("Could not create parse element\n");
        gst_object_unref (GST_OBJECT (pipeline));
        return NULL;
    }

    decoder = gst_bin_get_by_name (GST_BIN (pipeline), "decoder");
    if (decoder == NULL) {
        g_printerr ("Could not create decoder element, please check your "
            "GStreamer plugin path.\n");
        gst_object_unref (GST_OBJECT (pipeline));
        return NULL;
    }

    sink = gst_bin_get_by_name (GST_BIN (pipeline), "sink");
    if (sink == NULL) {
        g_printerr ("Could not create sink element.\n");
        gst_object_unref (GST_OBJECT (pipeline));
        return NULL;
    }

    g_object_set (G_OBJECT (source), "location", inputfile, NULL);
    if (outputfile != NULL) {
        g_object_set (G_OBJECT (sink), "location", outputfile, NULL);
    }

    if (board_idx >= 0) {
        g_object_set (G_OBJECT (decoder), "board-idx", board_idx, NULL);
    }

    if (skip_frame >= 0) {
        g_object_set (G_OBJECT (decoder), "skip-frames", skip_frame, NULL);
    }
    return pipeline;
}

int main (int argc, char *argv[]) {
  GMainLoop *loop;
  GstElement *pipeline;
  GstBus *bus;
  guint bus_watch_id;
/* Initialize GStreamer */
  gst_init (&argc, &argv);
  loop = g_main_loop_new (NULL, FALSE);

  pipeline = init_pipeline(argc, argv);

  if (pipeline == NULL) {
    g_main_loop_unref (loop);
    gst_deinit ();
    return 1;
  }

  bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
  bus_watch_id = gst_bus_add_watch (bus, bus_callback, loop);
#ifdef G_OS_UNIX
  g_unix_signal_add (SIGINT, (GSourceFunc) sigint_handler, pipeline);
#endif

  gst_element_set_state (pipeline, GST_STATE_PLAYING);
  g_print ("Playing...\n");
  g_main_loop_run (loop);

  gst_element_set_state (pipeline, GST_STATE_NULL);
  g_source_remove (bus_watch_id);
  gst_bus_remove_watch (bus);
  gst_object_unref (bus);
  gst_object_unref (GST_OBJECT (pipeline));
  g_main_loop_unref (loop);
  gst_deinit ();
  return 0;
}
