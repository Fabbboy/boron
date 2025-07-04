#include "config.h"
#include "util.h"
#include <leif/color.h>
#include <leif/ez_api.h>
#include <leif/layout.h>
#include <leif/task.h>
#include <leif/widget.h>
#include <podvig/podvig.h>
#include <pthread.h>

static pthread_mutex_t sound_mutex = PTHREAD_MUTEX_INITIALIZER;

static void soundwidget(lf_ui_state_t* ui);

static void rerender_util_task(void* data);

static void rerender_snd_task(void* data);

typedef struct {
  lf_ui_state_t* ui;
} task_data_t;

void 
rerender_util_task(void* data) {
  lf_ui_state_t* ui = ((task_data_t*)data)->ui;
  lf_component_rerender(ui, uiutil);
  free(data);
}

void 
rerender_snd_task(void* data) {
  lf_ui_state_t* ui = ((task_data_t*)data)->ui;
  lf_component_rerender(ui, soundwidget);
  free(data);
}

void handlevolumelsider(lf_ui_state_t* ui, lf_widget_t* widget, float* val) {
  if(s.sound_data.volmuted) {
    runcmd("amixer sset Master toggle &");
    s.sound_data.volmuted = false;
  }
  int32_t valint = (int32_t)*val;
  if(valint % 5 == 0) {
    if (s.sndelem_master) {
      long min, max;
      snd_mixer_selem_get_playback_volume_range(s.sndelem_master, &min, &max);
      long vol = min + (max - min) * (*val / 100.0f);
      snd_mixer_selem_set_playback_volume_all(s.sndelem_master, vol);
    }
  }
    lf_component_rerender(s.sound_widget->ui, soundwidget);
    lf_component_rerender(s.ui, uiutil);
}

void volsliderclick(lf_ui_state_t* ui, lf_widget_t* widget) {
  if(s.sound_data.volmuted) {
    runcmd("amixer sset Master toggle &");
    s.sound_data.volmuted = false;
  }
  printf("Clicked.\n");
  if (s.sndelem_master) {
    long min, max;
    snd_mixer_selem_get_playback_volume_range(
      s.sndelem_master, &min, &max);
    float* val = ((lf_slider_t*)widget)->val;
    long vol = min + (max - min) * (*val / 100.0f);
    snd_mixer_selem_set_playback_volume_all(s.sndelem_master, vol);
  }
  lf_component_rerender(s.sound_widget->ui, soundwidget);
  lf_component_rerender(s.ui, uiutil);
}

void handlemicrophoneslider(lf_ui_state_t* ui, lf_widget_t* widget, float* val) {
  if(s.sound_data.micmuted) {
    runcmd("amixer sset Capture toggle &");
    s.sound_data.micmuted = false;
  }
  if (s.sndelem_capture) {
    long min, max;
    snd_mixer_selem_get_playback_volume_range(s.sndelem_capture, &min, &max);
    long vol = min + (max - min) * (*val / 100.0f);
    snd_mixer_selem_set_playback_volume_all(s.sndelem_capture, vol);
  }
  lf_component_rerender(s.sound_widget->ui, soundwidget);
}

static lf_slider_t* volumeslider(lf_ui_state_t* ui, float* val, char* icon){
  lf_slider_t* slider = lf_slider(ui, val, 0, 100); 
  if(!slider->_held) {
  lf_widget_set_sizing(lf_crnt(ui), LF_SIZING_GROW);
  slider->base._fixed_width = false;
  slider->base._fixed_height = false;
  slider->handle_props.color = lf_color_dim(lf_color_from_hex(barcolorforeground), 170.0f);
  slider->_initial_handle_props = slider->handle_props;
  lf_style_widget_prop_color(ui, lf_crnt(ui), color, lf_color_dim(lf_color_from_hex(barcolorforeground), 60));
  lf_style_widget_prop_color(ui, lf_crnt(ui), text_color, lf_color_from_hex(barcolorforeground));
  lf_style_widget_prop(ui, lf_crnt(ui), margin_bottom, 15); 
  slider->base.container.size.y = 15;
  slider->handle.size.x = 15;
  slider->handle.size.y = 15;
  slider->_initial_handle_props.corner_radius = 30 / 2.0f; 
  slider->_initial_handle_props.border_width = 0; 
  slider->_initial_handle_props.padding_left = 7.5; 
  slider->_initial_handle_props.padding_right = 7.5; 
  slider->_initial_handle_props.padding_top = 7.5; 
  slider->_initial_handle_props.padding_bottom = 7.5; 
  slider->handle_props = slider->_initial_handle_props;
  lf_widget_set_padding(ui, lf_crnt(ui), 7.5);
  }

  lf_text_t* text = lf_text_p(ui, icon);
  lf_widget_set_margin(ui, lf_crnt(ui), 0);
  lf_style_widget_prop(ui, lf_crnt(ui), margin_left, 5);
  lf_widget_set_padding(ui, lf_crnt(ui), 0);
  lf_style_widget_prop_color(ui, lf_crnt(ui), text_color, LF_BLACK);

  lf_slider_end(ui);

  return slider;

}


void mutemic(lf_ui_state_t* ui, lf_widget_t* widget) {
  s.sound_data.micmuted = !s.sound_data.micmuted;
  if(s.sound_data.micmuted) {
    s.sound_data.microphone_before = s.sound_data.microphone;
    s.sound_data.microphone = 0;
  } else {
    s.sound_data.microphone = s.sound_data.microphone_before;
  }
  char buf[32];
  sprintf(buf, "amixer sset Capture toggle"); 
  runcmd(buf);
  lf_component_rerender(s.sound_widget->ui, soundwidget); 
}

void mutevolume(lf_ui_state_t* ui, lf_widget_t* widget) {
  s.sound_data.volmuted = !s.sound_data.volmuted;
  if(s.sound_data.volmuted) {
    s.sound_data.volume_before = s.sound_data.volume;
    s.sound_data.volume = 0;
  } else {
    s.sound_data.volume = s.sound_data.volume_before;
  }
  char buf[32];
  sprintf(buf, "amixer sset Master toggle"); 
  runcmd(buf);
  lf_component_rerender(s.sound_widget->ui, soundwidget); 
}

lf_button_t*
soundbutton(lf_ui_state_t* ui, float val) {
  char* icon =  "";
  if(val  >= 50)    {  icon = ""; }
  else if(val > 0) {  icon = ""; } 
  else {  icon = ""; }
  lf_button_t* btn = lf_button(ui);
  lf_crnt(ui)->props = ui->theme->text_props;

  lf_widget_set_fixed_width(ui, lf_crnt(ui), 35);
  lf_widget_set_fixed_height(ui, lf_crnt(ui), 30);
  lf_style_widget_prop_color(ui, lf_crnt(ui), color, lf_color_dim(lf_color_from_hex(barcolorforeground), 15.0f)); 
  lf_style_widget_prop(ui, lf_crnt(ui), corner_radius_percent, 20.0f); 

  {
    lf_text_h4(ui, icon); 
    lf_widget_set_fixed_width(ui, lf_crnt(ui), 18);
    lf_widget_set_fixed_height(ui, lf_crnt(ui), 15);
    lf_style_widget_prop_color(ui, lf_crnt(ui), text_color, lf_color_from_hex(barcolorforeground));
  }

  lf_button_end(ui);

  return btn;
} 

void soundwidget(lf_ui_state_t* ui) {
  lf_div(ui)->base.scrollable = false;
  lf_style_widget_prop(ui, lf_crnt(ui), corner_radius_percent, 20); 
  lf_style_widget_prop_color(ui, lf_crnt(ui), border_color, lf_color_from_hex(0x1c1c1c)); 
  lf_style_widget_prop(ui, lf_crnt(ui), border_width, 2); 
  lf_style_widget_prop_color(ui, lf_crnt(ui), color, lf_color_from_hex(barcolorbackground));
  lf_widget_set_padding(ui, lf_crnt(ui), 15);

  lf_text_h4(ui, " System Volume");
  lf_style_widget_prop(ui, lf_crnt(ui), margin_bottom, -10);
  lf_widget_set_font_style(ui, lf_crnt(ui), LF_FONT_STYLE_BOLD);
  lf_div(ui);
  lf_widget_set_margin(ui, lf_crnt(ui), 0);
  lf_widget_set_padding(ui, lf_crnt(ui), 0);
  lf_widget_set_layout(lf_crnt(ui), LF_LAYOUT_HORIZONTAL);
  lf_widget_set_alignment(lf_crnt(ui), LF_ALIGN_CENTER_HORIZONTAL | LF_ALIGN_CENTER_VERTICAL);


  char* icon =  "";
  if(s.sound_data.volume >= 50)    {  icon = ""; }
  else if(s.sound_data.volume > 0) {  icon = ""; } 
  else {  icon = ""; }
  lf_slider_t* volslider = volumeslider(ui, &s.sound_data.volume, icon);
  volslider->on_slide = handlevolumelsider;
  volslider->on_click = volsliderclick;


  lf_div_end(ui);

  lf_text_h4(ui, " Microphone");
  lf_style_widget_prop(ui, lf_crnt(ui), margin_bottom, -10);
  lf_style_widget_prop(ui, lf_crnt(ui), margin_top, 10);
  lf_widget_set_font_style(ui, lf_crnt(ui), LF_FONT_STYLE_BOLD);
  lf_div(ui);
  lf_widget_set_margin(ui, lf_crnt(ui), 0);
  lf_widget_set_padding(ui, lf_crnt(ui), 0);
  lf_widget_set_layout(lf_crnt(ui), LF_LAYOUT_HORIZONTAL);
  lf_widget_set_alignment(lf_crnt(ui), LF_ALIGN_CENTER_HORIZONTAL | LF_ALIGN_CENTER_VERTICAL);
 
  icon =  "";
  if(s.sound_data.microphone >= 50)    {  icon = ""; }
  else if(s.sound_data.microphone > 0) {  icon = ""; } 
  else {  icon = ""; }
  printf("Mic: %f\n", s.sound_data.microphone);
  volumeslider(ui, &s.sound_data.microphone, icon)->on_slide = handlemicrophoneslider;

  lf_div_end(ui);


  lf_div_end(ui);
}

bool alsasetup(state_t* s) {
  const char* card = "default";
  const char* selem_name_master = "Master";
  const char* selem_name_capture = "Capture";

  if (snd_mixer_open(&s->sndhandle, 0) < 0) {
    fprintf(stderr, "boron: alsa: error opening mixer\n");
    return false;
  }

  if (snd_mixer_attach(s->sndhandle, card) < 0) {
    fprintf(stderr, "boron: alsa: error attaching mixer\n");
    snd_mixer_close(s->sndhandle);
    return false;
  }

  if (snd_mixer_selem_register(s->sndhandle, NULL, NULL) < 0) {
    fprintf(stderr, "boron: alsa: error registering mixer\n");
    snd_mixer_close(s->sndhandle);
    return false;
  }

  if (snd_mixer_load(s->sndhandle) < 0) {
    fprintf(stderr, "boron: alsa: error loading mixer elements\n");
    snd_mixer_close(s->sndhandle);
    return false;
  }

  // Master setup
  snd_mixer_selem_id_malloc(&s->sndsid_master);
  snd_mixer_selem_id_set_index(s->sndsid_master, 0);
  snd_mixer_selem_id_set_name(s->sndsid_master, selem_name_master);

  s->sndelem_master = snd_mixer_find_selem(s->sndhandle, s->sndsid_master);
  if (!s->sndelem_master) {
    fprintf(stderr, "boron: unable to find simple control '%s'\n", selem_name_master);
    snd_mixer_selem_id_free(s->sndsid_master);
    snd_mixer_close(s->sndhandle);
    return false;
  }

  // Capture setup
  snd_mixer_selem_id_malloc(&s->sndsid_capture);
  snd_mixer_selem_id_set_index(s->sndsid_capture, 0);
  snd_mixer_selem_id_set_name(s->sndsid_capture, selem_name_capture);

  s->sndelem_capture = snd_mixer_find_selem(s->sndhandle, s->sndsid_capture);
  if (!s->sndelem_capture) {
    fprintf(stderr, "boron: unable to find simple control '%s'\n", selem_name_capture);
    snd_mixer_selem_id_free(s->sndsid_master);
    snd_mixer_selem_id_free(s->sndsid_capture);
    snd_mixer_close(s->sndhandle);
    return false;
  }

  s->sndpollcount = snd_mixer_poll_descriptors_count(s->sndhandle);
  s->sndpfds = malloc(sizeof(*s->sndpfds) * s->sndpollcount);
  snd_mixer_poll_descriptors(s->sndhandle, s->sndpfds, s->sndpollcount);

  return true;
}

void 
alsahandleui(state_t* s) {
  {
    // --- MASTER VOLUME (Playback) ---
    long min, max, vol;
    int pswitch;
    snd_mixer_selem_get_playback_volume_range(s->sndelem_master, &min, &max);
    snd_mixer_selem_get_playback_volume(s->sndelem_master, SND_MIXER_SCHN_FRONT_LEFT, &vol);
    snd_mixer_selem_get_playback_switch(s->sndelem_master, SND_MIXER_SCHN_FRONT_LEFT, &pswitch);
    bool volmuted_before = s->sound_data.volmuted;
    s->sound_data.volmuted = pswitch == 0;
    if (s->sound_data.volmuted) {
      s->sound_data.volume_before = s->sound_data.volume;
      s->sound_data.volume = 0;
      if(s->ui) {
        task_data_t* task_data = malloc(sizeof(task_data_t));
        task_data->ui = s->ui;
        lf_task_enqueue(rerender_util_task, task_data);
      }
    }
    if (volmuted_before != s->sound_data.volmuted) {
      pthread_mutex_lock(&sound_mutex);
      if (s->sound_widget) {
        task_data_t* task_data = malloc(sizeof(task_data_t));
        task_data->ui = s->sound_widget->ui;
        lf_task_enqueue(rerender_snd_task, task_data);
      }
      pthread_mutex_unlock(&sound_mutex);
    } 
    if (max - min > 0 && pswitch != 0) {
      if (s->sound_widget) {
        lf_widget_t* active_widget = lf_widget_from_id(
          s->sound_widget->ui, 
          s->sound_widget->ui->root, 
          s->sound_widget->ui->active_widget_id);
        if (active_widget && active_widget->type == LF_WIDGET_TYPE_SLIDER) return;
      }
      int percent = (int)(((vol - min) * 100) / (max - min));
      pthread_mutex_lock(&sound_mutex);
      s->sound_data.volume = percent;
      if (s->sound_widget) {
        task_data_t* task_data = malloc(sizeof(task_data_t));
        task_data->ui = s->sound_widget->ui;
        lf_task_enqueue(rerender_snd_task, task_data);
      }
      if(s->ui) {
        task_data_t* task_data = malloc(sizeof(task_data_t));
        task_data->ui = s->ui;
        lf_task_enqueue(rerender_util_task, task_data);
      }
      pthread_mutex_unlock(&sound_mutex);
    }
  }

  {
    // --- CAPTURE VOLUME (Mic) ---
    long min, max, vol;
    int pswitch;
    snd_mixer_selem_get_capture_volume_range(s->sndelem_capture, &min, &max);
    snd_mixer_selem_get_capture_volume(s->sndelem_capture, SND_MIXER_SCHN_FRONT_LEFT, &vol);
    snd_mixer_selem_get_capture_switch(s->sndelem_capture, SND_MIXER_SCHN_FRONT_LEFT, &pswitch);
    bool micmuted_before = s->sound_data.micmuted;
    s->sound_data.micmuted = pswitch == 0;
    if (s->sound_data.micmuted) {
      s->sound_data.microphone_before = s->sound_data.microphone;
      s->sound_data.microphone = 0;
    }
    if (micmuted_before != s->sound_data.micmuted) {
      pthread_mutex_lock(&sound_mutex);
      if (s->sound_widget) {
        task_data_t* task_data = malloc(sizeof(task_data_t));
        task_data->ui = s->sound_widget->ui;
        lf_task_enqueue(rerender_snd_task, task_data);
      }
      pthread_mutex_unlock(&sound_mutex);
    } else if (max - min > 0 && pswitch != 0) {
      if (s->sound_widget) {
        lf_widget_t* active_widget = lf_widget_from_id(
          s->sound_widget->ui, 
          s->sound_widget->ui->root, 
          s->sound_widget->ui->active_widget_id);
        if (active_widget && active_widget->type == LF_WIDGET_TYPE_SLIDER) return;
      }
      int percent = (int)(((vol - min) * 100) / (max - min));
      pthread_mutex_lock(&sound_mutex);
      s->sound_data.microphone = percent;
      if (s->sound_widget) {
        task_data_t* task_data = malloc(sizeof(task_data_t));
        task_data->ui = s->sound_widget->ui;
        lf_task_enqueue(rerender_snd_task, task_data);
      }
      pthread_mutex_unlock(&sound_mutex);
    }
  }

}
void* alsalisten(void *arg) {
  state_t* s = (state_t *)arg;

  alsahandleui(s);
  while (1) {
    if (poll(s->sndpfds, s->sndpollcount, -1) > 0) {
      snd_mixer_handle_events(s->sndhandle);
      alsahandleui(s);
    }
  }

  return NULL;
}

bool 
sndsetup(void) {
  if(!alsasetup(&s)) return false;
  pthread_t listener_thread;
  if (pthread_create(&listener_thread, NULL, alsalisten, (void *)&s) != 0) {
    fprintf(stderr, "boron: alsa: error creating listener thread\n");
    snd_mixer_close(s.sndhandle);
    return false;
  }
  return true;
}

static void widgetclose(pv_widget_t* widget) {
  (void)widget;
  lf_component_rerender(s.ui, uiutil); 
}

bool 
sndcreatewidget(lf_window_t barwin) {
  pthread_mutex_lock(&sound_mutex);

  if(!s.pvstate)
    s.pvstate = pv_init();
  if(!s.pvstate) return false;
  s.sound_widget = pv_widget(
    s.pvstate, "boron_sound_popup", soundwidget,
    s.bararea.x + s.bararea.width - 300, 
    s.bararea.y + s.bararea.height + 10,
    300, 180);

  pthread_mutex_unlock(&sound_mutex);

  pv_widget_set_popup_of(s.pvstate, s.sound_widget, barwin);
  lf_widget_set_font_family(s.sound_widget->ui, s.sound_widget->ui->root, barfont);
  lf_widget_set_font_style(s.sound_widget->ui, s.sound_widget->ui->root, LF_FONT_STYLE_REGULAR);
  lf_style_widget_prop_color(s.sound_widget->ui, s.sound_widget->ui->root, color, LF_NO_COLOR); 

  pv_widget_hide(s.sound_widget);

  s.sound_widget->data.close_cb = widgetclose; 
  return true;
}
