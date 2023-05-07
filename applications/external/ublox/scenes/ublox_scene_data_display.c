#include "../ublox_i.h"
#include "../ublox_worker_i.h"

#define TAG "ublox_scene_data_display"

const NotificationSequence sequence_new_reading = {
  //&message_vibro_on,
  &message_green_255,
  &message_delay_100,
  &message_green_0,
  //&message_vibro_off,
  NULL,
};

void ublox_scene_data_display_worker_callback(UbloxWorkerEvent event, void* context) {
  Ublox* ublox = context;

  view_dispatcher_send_custom_event(ublox->view_dispatcher, event);
}

void ublox_scene_data_display_view_callback(void* context, InputKey key) {
  Ublox* ublox = context;

  // just reuse generic events
  if (key == InputKeyLeft) {
    view_dispatcher_send_custom_event(ublox->view_dispatcher, GuiButtonTypeLeft);
  } else if (key == InputKeyOk) {
    view_dispatcher_send_custom_event(ublox->view_dispatcher, GuiButtonTypeCenter);
  }
}

static void timer_callback(void* context) {
  Ublox* ublox = context;
  //FURI_LOG_I(TAG, "timer callback");
  // every time, try to set the view back to a GPS-found mode
  ublox_worker_start(ublox->worker, UbloxWorkerStateRead,
		     ublox_scene_data_display_worker_callback, ublox);

}

void ublox_scene_data_display_on_enter(void* context) {
  Ublox* ublox = context;

  data_display_set_callback(ublox->data_display, ublox_scene_data_display_view_callback, ublox);
  if ((ublox->data_display_state).view_mode == UbloxDataDisplayViewModeHandheld) {
    data_display_set_state(ublox->data_display, DataDisplayHandheldMode);
  } else if ((ublox->data_display_state).view_mode == UbloxDataDisplayViewModeCar) {
    data_display_set_state(ublox->data_display, DataDisplayCarMode);
  }
  
  view_dispatcher_switch_to_view(ublox->view_dispatcher, UbloxViewDataDisplay);
  
  ublox->timer = furi_timer_alloc(timer_callback, FuriTimerTypePeriodic, ublox);

  // convert from seconds to milliseconds
  furi_timer_start(ublox->timer, furi_ms_to_ticks((ublox->data_display_state).refresh_rate * 1000));

  ublox_worker_start(ublox->worker, UbloxWorkerStateRead,
		     ublox_scene_data_display_worker_callback, ublox);
}

bool ublox_scene_data_display_on_event(void* context, SceneManagerEvent event) {
  Ublox* ublox = context;
  bool consumed = false;

  if (event.type == SceneManagerEventTypeCustom) {
    if (event.event == GuiButtonTypeLeft) {
      FURI_LOG_I(TAG, "left button pressed");
      scene_manager_next_scene(ublox->scene_manager, UbloxSceneDataDisplayConfig);
      consumed = true;
    } else if (event.event == GuiButtonTypeCenter) {
      furi_timer_stop(ublox->timer);
      // MUST stop the worker first! (idk why though)
      ublox_worker_stop(ublox->worker);
      FURI_LOG_I(TAG, "reset odometer");
      ublox_worker_start(ublox->worker, UbloxWorkerStateResetOdometer,
			 ublox_scene_data_display_worker_callback, ublox);
      furi_timer_start(ublox->timer, furi_ms_to_ticks((ublox->data_display_state).refresh_rate * 1000));
      
    } else if (event.event == UbloxWorkerEventDataReady) {
      if ((ublox->data_display_state).notify_mode == UbloxDataDisplayNotifyOn) {
	notification_message(ublox->notifications, &sequence_new_reading);
      }
      
      data_display_set_nav_pvt_message(ublox->data_display, ublox->nav_pvt);
      data_display_set_nav_odo_message(ublox->data_display, ublox->nav_odo);
      if ((ublox->data_display_state).view_mode == UbloxDataDisplayViewModeHandheld) {
	data_display_set_state(ublox->data_display, DataDisplayHandheldMode);
      } else if ((ublox->data_display_state).view_mode == UbloxDataDisplayViewModeCar) {
	data_display_set_state(ublox->data_display, DataDisplayCarMode);
      }
      
    } else if (event.event == UbloxWorkerEventFailed) {
      FURI_LOG_I(TAG, "UbloxWorkerEventFailed");
      data_display_set_state(ublox->data_display, DataDisplayGPSNotFound);
    }
  }

  return consumed;
}

void ublox_scene_data_display_on_exit(void* context) {
  Ublox* ublox = context;

  furi_timer_stop(ublox->timer);
  furi_timer_free(ublox->timer);

  ublox_worker_stop(ublox->worker);
  
  data_display_reset(ublox->data_display);
  FURI_LOG_I(TAG, "leaving data display scene");
}
    
  
