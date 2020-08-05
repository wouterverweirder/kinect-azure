#define KINECT_AZURE_ENABLE_BODY_TRACKING = 1

#include <thread>
#include <mutex>
#include <napi.h>
#include <stdio.h>
#include <malloc.h>
#include <k4a/k4a.h>
#include <k4arecord/playback.h>
#ifdef KINECT_AZURE_ENABLE_BODY_TRACKING
#include <k4abt.h>
#endif // KINECT_AZURE_ENABLE_BODY_TRACKING
#include "structs.h"
#include <chrono>
#include <math.h>
#include "colorUtils.cc"
#include <algorithm>

k4a_device_t g_device = NULL;
k4a_device_configuration_t g_deviceConfig = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
CustomDeviceConfig g_customDeviceConfig;
k4a_calibration_t g_calibration;
k4a_transformation_t transformer = NULL;

#ifdef KINECT_AZURE_ENABLE_BODY_TRACKING
k4abt_tracker_t g_tracker = NULL;
#endif // KINECT_AZURE_ENABLE_BODY_TRACKING

//Napi::FunctionReference g_emit;
std::thread nativeThread;
std::mutex mtx;
Napi::ThreadSafeFunction tsfn;
std::mutex threadJoinedMutex;

bool is_open = false;
bool is_listening = false;
bool is_playbackFileOpen = false;
bool is_playing = false;
bool is_paused = false;
bool is_seeking = false;
double colorTimestamp = 0;

k4a_playback_t playback_handle = NULL;
k4a_record_configuration_t playback_config;
PlaybackProps g_playbackProps;

int depthPixelIndex = 0;
uint8_t *depth_to_color_data = NULL;
float combined = 0;
int normalizedValue = 0;
int rgb[3];

inline bool convertToBool(const char *key, Napi::Object js_config, bool currentValue)
{
  Napi::Value js_value = js_config.Get(key);
  if (js_value.IsBoolean())
  {
    return js_value.As<Napi::Boolean>();
  }
  return currentValue;
}

inline int convertToNumber(const char *key, Napi::Object js_config, int currentValue)
{
  Napi::Value js_value = js_config.Get(key);
  if (js_value.IsNumber())
  {
    return js_value.As<Napi::Number>();
  }
  return currentValue;
}

void copyCustomConfig(Napi::Object js_config)
{
  g_customDeviceConfig.reset();
  g_customDeviceConfig.include_imu_sample = convertToBool("include_imu_sample", js_config, g_customDeviceConfig.include_imu_sample);
  g_customDeviceConfig.include_color_to_depth = convertToBool("include_color_to_depth", js_config, g_customDeviceConfig.include_color_to_depth);
  g_customDeviceConfig.include_body_index_map = convertToBool("include_body_index_map", js_config, g_customDeviceConfig.include_body_index_map);
  g_customDeviceConfig.flip_BGRA_to_RGBA = convertToBool("flip_BGRA_to_RGBA", js_config, g_customDeviceConfig.flip_BGRA_to_RGBA);
  g_customDeviceConfig.apply_depth_to_alpha = convertToBool("apply_depth_to_alpha", js_config, g_customDeviceConfig.apply_depth_to_alpha);
  g_customDeviceConfig.depth_to_greyscale = convertToBool("depth_to_greyscale", js_config, g_customDeviceConfig.depth_to_greyscale);
  g_customDeviceConfig.depth_to_redblue = convertToBool("depth_to_redblue", js_config, g_customDeviceConfig.depth_to_redblue);
  g_customDeviceConfig.min_depth = convertToNumber("min_depth", js_config, g_customDeviceConfig.min_depth);
  g_customDeviceConfig.max_depth = convertToNumber("max_depth", js_config, g_customDeviceConfig.max_depth);
  g_customDeviceConfig.include_depth_to_color = convertToBool("include_depth_to_color", js_config, g_customDeviceConfig.include_depth_to_color) || g_customDeviceConfig.apply_depth_to_alpha == true || g_customDeviceConfig.depth_to_greyscale == true || g_customDeviceConfig.depth_to_redblue == true;
}

inline int map(float value, int inputMin, int inputMax, int outputMin, int outputMax)
{
  if (value > inputMax)
    value = inputMax;
  if (value < inputMin)
    value = inputMin;
  return (value - inputMin) * (outputMax - outputMin) / (inputMax - inputMin) + outputMin;
}

// Transform skeleton results from 3d depth space to 2d image space
inline bool transform_joint_from_depth_3d_to_2d(
    const k4a_calibration_t *calibration,
    k4a_float3_t joint_in_depth_space,
    k4a_float2_t &joint_in_color_2d,
    k4a_calibration_type_t target_space)
{
  int valid;
  k4a_calibration_3d_to_2d(
      calibration,
      &joint_in_depth_space,
      K4A_CALIBRATION_TYPE_DEPTH,
      target_space,
      &joint_in_color_2d,
      &valid);

  return valid != 0;
}

Napi::Value MethodInit(const Napi::CallbackInfo &info)
{
  Napi::Env env = info.Env();
  return Napi::Boolean::New(env, true);
}

Napi::Number MethodGetInstalledCount(const Napi::CallbackInfo &info)
{
  Napi::Env env = info.Env();
  uint32_t count = k4a_device_get_installed_count();
  return Napi::Number::New(env, count);
}

Napi::String MethodGetSerialNumber(const Napi::CallbackInfo &info)
{
  Napi::Env env = info.Env();

  //Get Serial number of currently open Kinect
  //check if Kinect open -> Get serial number -> return serial number
  char serial_number[256];

  if (g_device != nullptr)
  {
    size_t serial_number_size = sizeof(serial_number);
    k4a_device_get_serialnum(g_device, serial_number, &serial_number_size);
    /* printf("Serial Number: %s\n", serial_number); */
  }
  else
  {
    //Kinect g_device is not open
    //Return null empty char array
    /* printf("Device is NOT open. Can not return serial number"); */
    serial_number[0] = '\0';
  }

  return Napi::String::New(env, serial_number);
}

Napi::Value MethodOpen(const Napi::CallbackInfo &info)
{
  Napi::Env env = info.Env();

  // If index specified, look for specific Kinect.
  if (info.Length() == 1)
  {

    //Error check for invalid index
    int index = info[0].ToNumber().Int32Value();
    if (index >= k4a_device_get_installed_count())
    {
      Napi::TypeError::New(env, "Index passed is higher than amount of Kinect devices currently connected")
          .ThrowAsJavaScriptException();
      return Napi::String::New(env, "Error");
    }

    if (K4A_SUCCEEDED(k4a_device_open(index, &g_device)))
    {
      //Device opened
      /* printf("[kinect_azure.cc] MethodOpen - Opening device at index %u success\n", index); */
    }
    else
    {
      /* printf("[kinect_azure.cc] MethodOpen - Opening device at index %u failed\n", index); */
    }
  }
  else
  {
    // No device exists or no index number specified, open default Kinect.
    /* printf("[kinect_azure.cc] MethodOpen - Opening default device\n"); */
    k4a_device_open(K4A_DEVICE_DEFAULT, &g_device);
  }

  bool returnValue = (g_device == nullptr) ? false : true;
  is_open = returnValue;
  /* printf("[kinect_azure.cc] MethodOpen - returnValue: %u\n", returnValue); */
  return Napi::Boolean::New(env, returnValue);
}

Napi::Value MethodSerialOpen(const Napi::CallbackInfo &info)
{
  Napi::Env env = info.Env();

  // If serial number specified, look for specific Kinect.
  if (info.Length() == 1 && strcmp("undefined", info[0].ToString().Utf8Value().c_str()) != 0)
  {
    //Get serial number
    char serialNumber[256];
    strcpy(serialNumber, info[0].ToString().Utf8Value().c_str());
    /* printf("[kinect_azure.cc] MethodOpen - serial number: %s\n", info[0].ToString().Utf8Value().c_str()); */
    /* printf("[kinect_azure.cc] MethodOpen - copied serial number: %s\n", serialNumber); */

    uint32_t count = k4a_device_get_installed_count();
    /* printf("[kinect_azure.cc] MethodOpen - # of Kinects: %u\n", count); */
    for (uint8_t i = 0; i < count; i++)
    {
      /* printf("[kinect_azure.cc] MethodOpen - Value of i: %u\n", i); */

      if (K4A_SUCCEEDED(k4a_device_open(i, &g_device)))
      {
        //Device opened, check serial number
        /*printf("[kinect_azure.cc] MethodOpen - Opening device at index %u success\n", i); */
        //Check for matching serial number
        char serial_number[256];
        size_t serial_number_size = sizeof(serial_number);
        k4a_device_get_serialnum(g_device, serial_number, &serial_number_size);
        if (strcmp(serial_number, serialNumber) == 0)
        {
          //Match!
          /* printf("[kinect_azure.cc] MethodOpen - Match Found!\n"); */
          break; //device already opened; nothing more to do.
        }
        else
        {
          //Not a match; close device for next check
          /* printf("[kinect_azure.cc] MethodOpen - Not a match!\n"); */
          k4a_device_close(g_device);
          g_device = nullptr;
        }
      }
      else
      {
        /* printf("[kinect_azure.cc] MethodOpen - Opening device at index %u failed\n", i); */
      }
    }
  }
  else
  {
    // No device exists or no serial number specified, open default Kinect.
    /* printf("[kinect_azure.cc] MethodOpen - Opening default device\n"); */
    k4a_device_open(K4A_DEVICE_DEFAULT, &g_device);
  }

  bool returnValue = (g_device == nullptr) ? false : true;
  is_open = returnValue;
  /* printf("[kinect_azure.cc] MethodOpen - returnValue: %u\n", returnValue); */
  return Napi::Boolean::New(env, returnValue);
}

Napi::Value MethodClose(const Napi::CallbackInfo &info)
{
  Napi::Env env = info.Env();
  // printf("[kinect_azure.cc] MethodClose\n");
  k4a_device_close(g_device);
  bool returnValue = (g_device == nullptr) ? true : false;
  is_open = returnValue;
  return Napi::Boolean::New(env, returnValue);
}

Napi::Value MethodOpenPlayback(const Napi::CallbackInfo &info)
{

  Napi::Env env = info.Env();
  if (is_playbackFileOpen)
  {
    Napi::TypeError::New(env, "File already open")
        .ThrowAsJavaScriptException();
    return env.Null();
  }
  if (info.Length() < 1)
  {
    Napi::TypeError::New(env, "Wrong number of arguments")
        .ThrowAsJavaScriptException();
    return env.Null();
  }
  Napi::String js_path = info[0].As<Napi::String>();

  if (k4a_playback_open(js_path.Utf8Value().c_str(), &playback_handle) != K4A_RESULT_SUCCEEDED)
  {
    return Napi::Boolean::New(env, false);
  }

  if (k4a_playback_get_record_configuration(playback_handle, &playback_config) != K4A_RESULT_SUCCEEDED)
  {
    return Napi::Boolean::New(env, false);
  }

  g_playbackProps.recording_length = k4a_playback_get_recording_length_usec(playback_handle);

  is_playbackFileOpen = true;
  return Napi::Boolean::New(env, true);
}

Napi::Value MethodStartPlayback(const Napi::CallbackInfo &info)
{

  Napi::Env env = info.Env();
  if (!is_playbackFileOpen)
  {
    Napi::TypeError::New(env, "No playback file is open")
        .ThrowAsJavaScriptException();
    return env.Null();
  }
  if (info.Length() < 1)
  {
    Napi::TypeError::New(env, "Wrong number of arguments")
        .ThrowAsJavaScriptException();
    return env.Null();
  }

  is_playing = true;
  is_paused = false;

  Napi::Object js_config = info[0].As<Napi::Object>();
  Napi::Value js_color_format = js_config.Get("color_format");
  if (js_color_format.IsNumber())
  {
    k4a_image_format_t target_format = (k4a_image_format_t)js_color_format.As<Napi::Number>().Int32Value();
    if (k4a_playback_set_color_conversion(playback_handle, target_format) != K4A_RESULT_SUCCEEDED)
    {
      printf("Failed to read config\n");
      return Napi::Boolean::New(env, false);
    }
  }

  Napi::Value js_camera_fps = js_config.Get("camera_fps");
  if (js_camera_fps.IsNumber())
  {
    g_playbackProps.playback_fps = js_camera_fps.As<Napi::Number>().Int32Value();
  }
  else
  {
    g_playbackProps.playback_fps = playback_config.camera_fps;
  }

  if (g_playbackProps.playback_fps == 0)
    g_playbackProps.playback_fps = 5;
  else if (g_playbackProps.playback_fps == 1)
    g_playbackProps.playback_fps = 15;
  else if (g_playbackProps.playback_fps == 2)
    g_playbackProps.playback_fps = 30;

  k4a_device_configuration_t deviceConfig = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
  deviceConfig.color_resolution = (k4a_color_resolution_t)playback_config.color_resolution;
  deviceConfig.depth_mode = (k4a_depth_mode_t)playback_config.depth_mode;

  copyCustomConfig(js_config);

  g_deviceConfig = deviceConfig;

  k4a_playback_get_calibration(playback_handle, &g_calibration);

  // TODO: read other config values from file
  return Napi::Boolean::New(env, true);
}

Napi::Value MethodStopPlayback(const Napi::CallbackInfo &info)
{
  Napi::Env env = info.Env();
  k4a_playback_close(playback_handle);
  return Napi::Boolean::New(env, true);
}

Napi::Value MethodSeek(const Napi::CallbackInfo &info)
{
  Napi::Env env = info.Env();

  if (!is_playbackFileOpen)
  {
    Napi::TypeError::New(env, "No playback file is open")
        .ThrowAsJavaScriptException();
    return env.Null();
  }
  if (info.Length() < 1)
  {
    Napi::TypeError::New(env, "Wrong number of arguments")
        .ThrowAsJavaScriptException();
    return env.Null();
  }

  Napi::Value js_time = info[0].As<Napi::Value>();
  if (js_time.IsNumber())
  {
    int32_t time = (int32_t)js_time.As<Napi::Number>().Int32Value();
    k4a_playback_seek_timestamp(playback_handle, time * 1000000, K4A_PLAYBACK_SEEK_BEGIN);
    is_seeking = true;
  }
  return Napi::Boolean::New(env, true);
}

Napi::Value MethodPause(const Napi::CallbackInfo &info)
{
  Napi::Env env = info.Env();
  is_paused = true;
  return Napi::Boolean::New(env, is_paused);
}

Napi::Value MethodResume(const Napi::CallbackInfo &info)
{
  Napi::Env env = info.Env();
  is_paused = false;
  return Napi::Boolean::New(env, is_paused);
}

Napi::Value MethodTime(const Napi::CallbackInfo &info)
{
  Napi::Env env = info.Env();
  return Napi::Number::New(env, colorTimestamp / 1000000.0f);
}

Napi::Value MethodDuration(const Napi::CallbackInfo &info)
{
  Napi::Env env = info.Env();
  return Napi::Number::New(env, g_playbackProps.recording_length / 1000000.0f);
}

Napi::Value MethodStartCameras(const Napi::CallbackInfo &info)
{
  Napi::Env env = info.Env();
  if (info.Length() < 1)
  {
    Napi::TypeError::New(env, "Wrong number of arguments")
        .ThrowAsJavaScriptException();
    return env.Null();
  }

  k4a_device_configuration_t deviceConfig = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;

  Napi::Object js_config = info[0].As<Napi::Object>();
  Napi::Value js_camera_fps = js_config.Get("camera_fps");
  if (js_camera_fps.IsNumber())
  {
    deviceConfig.camera_fps = (k4a_fps_t)js_camera_fps.As<Napi::Number>().Int32Value();
  }

  Napi::Value js_color_format = js_config.Get("color_format");
  if (js_color_format.IsNumber())
  {
    deviceConfig.color_format = (k4a_image_format_t)js_color_format.As<Napi::Number>().Int32Value();
  }

  Napi::Value js_color_resolution = js_config.Get("color_resolution");
  if (js_color_resolution.IsNumber())
  {
    deviceConfig.color_resolution = (k4a_color_resolution_t)js_color_resolution.As<Napi::Number>().Int32Value();
  }

  Napi::Value js_depth_mode = js_config.Get("depth_mode");
  if (js_depth_mode.IsNumber())
  {
    deviceConfig.depth_mode = (k4a_depth_mode_t)js_depth_mode.As<Napi::Number>().Int32Value();
  }

  Napi::Value js_synchronized_images_only = js_config.Get("synchronized_images_only");
  if (js_synchronized_images_only.IsBoolean())
  {
    deviceConfig.synchronized_images_only = js_synchronized_images_only.As<Napi::Boolean>();
  }

  copyCustomConfig(js_config);

  g_deviceConfig = deviceConfig;

  k4a_device_start_cameras(g_device, &g_deviceConfig);

  if (g_customDeviceConfig.include_imu_sample)
  {
    k4a_device_start_imu(g_device);
  }

  k4a_device_get_calibration(g_device, g_deviceConfig.depth_mode, g_deviceConfig.color_resolution, &g_calibration);

  return Napi::Boolean::New(env, true);
}

Napi::Value MethodStopCameras(const Napi::CallbackInfo &info)
{
  Napi::Env env = info.Env();
  if (g_customDeviceConfig.include_imu_sample)
  {
    k4a_device_stop_imu(g_device);
  }
  k4a_device_stop_cameras(g_device);
  return Napi::Boolean::New(env, true);
}

#ifdef KINECT_AZURE_ENABLE_BODY_TRACKING
Napi::Value MethodCreateTracker(const Napi::CallbackInfo &info)
{
  Napi::Env env = info.Env();
  k4a_calibration_t sensor_calibration;
  k4a_device_get_calibration(g_device, g_deviceConfig.depth_mode, g_deviceConfig.color_resolution, &sensor_calibration);
  k4abt_tracker_configuration_t tracker_config = K4ABT_TRACKER_CONFIG_DEFAULT;

  if (info.Length() > 0)
  {
    Napi::Object js_config = info[0].As<Napi::Object>();
    Napi::Value js_sensor_orientation = js_config.Get("sensor_orientation");
    if (js_sensor_orientation.IsNumber())
    {
      tracker_config.sensor_orientation = (k4abt_sensor_orientation_t)js_sensor_orientation.As<Napi::Number>().Int32Value();
    }
    Napi::Value js_processing_mode = js_config.Get("processing_mode");
    if (js_processing_mode.IsNumber())
    {
      tracker_config.processing_mode = (k4abt_tracker_processing_mode_t)js_processing_mode.As<Napi::Number>().Int32Value();
    }
    Napi::Value js_gpu_device_id = js_config.Get("gpu_device_id");
    if (js_gpu_device_id.IsNumber())
    {
      tracker_config.gpu_device_id = (int32_t)js_gpu_device_id.As<Napi::Number>().Int32Value();
    }
  }

  k4abt_tracker_create(&sensor_calibration, tracker_config, &g_tracker);
  return Napi::Boolean::New(env, true);
}

Napi::Value MethodDestroyTracker(const Napi::CallbackInfo &info)
{
  Napi::Env env = info.Env();
  // printf("[kinect_azure.cc] MethodDestroyTracker\n");
  if (g_tracker != NULL)
  {
    k4abt_tracker_shutdown(g_tracker);
    k4abt_tracker_destroy(g_tracker);
  }
  g_tracker = NULL;
  return Napi::Boolean::New(env, true);
}
#endif // KINECT_AZURE_ENABLE_BODY_TRACKING

Napi::Value MethodSetColorControl(const Napi::CallbackInfo &info)
{
  Napi::Env env = info.Env();
  if (info.Length() < 0)
  {
    Napi::TypeError::New(env, "Wrong number of arguments")
        .ThrowAsJavaScriptException();
    return env.Null();
  }
  Napi::Object js_config = info[0].As<Napi::Object>();
  Napi::Value js_command = js_config.Get("command");
  if (!js_command.IsNumber())
  {
    Napi::TypeError::New(env, "missing command")
        .ThrowAsJavaScriptException();
    return env.Null();
  }
  k4a_color_control_command_t command = (k4a_color_control_command_t)js_command.As<Napi::Number>().Int32Value();

  Napi::Value js_mode = js_config.Get("mode");
  if (!js_mode.IsNumber())
  {
    Napi::TypeError::New(env, "missing mode")
        .ThrowAsJavaScriptException();
    return env.Null();
  }
  k4a_color_control_mode_t mode = (k4a_color_control_mode_t)js_mode.As<Napi::Number>().Int32Value();

  Napi::Value js_value = js_config.Get("value");
  if (!js_value.IsNumber())
  {
    Napi::TypeError::New(env, "missing value")
        .ThrowAsJavaScriptException();
    return env.Null();
  }
  int32_t value = (int32_t)js_value.As<Napi::Number>().Int32Value();
  k4a_device_set_color_control(g_device, command, mode, value);
  return info.Env().Undefined();
}

Napi::Value MethodStartListening(const Napi::CallbackInfo &info)
{
  Napi::Env env = info.Env();

  if (is_listening)
  {
    Napi::TypeError::New(env, "Kinect already listening")
        .ThrowAsJavaScriptException();
    return env.Null();
  }

  is_listening = true;
  if (info.Length() < 1)
  {
    Napi::TypeError::New(env, "Wrong number of arguments")
        .ThrowAsJavaScriptException();
    return env.Null();
  }
  else if (!info[0].IsFunction())
  {
    throw Napi::TypeError::New(env, "Expected first arg to be function");
  }

  transformer = k4a_transformation_create(&g_calibration);
  tsfn = Napi::ThreadSafeFunction::New(
      env,
      info[0].As<Napi::Function>(), // JavaScript function called asynchronously
      "Kinect Azure Listening",     // Name
      1,                            // 1 call in queue
      1,                            // Only one thread will use this initially
      [](Napi::Env) {               // Finalizer used to clean threads up
        nativeThread.join();
        threadJoinedMutex.unlock();
      });

  threadJoinedMutex.lock();
  nativeThread = std::thread([] {
    auto callback = [](Napi::Env env, Napi::Function jsCallback, JSFrame *jsFrameRef) {
      if (!is_listening || (is_paused && !is_seeking))
      {
        // printf("[kinect_azure.cc] callback not listening\n");
        return;
      }
      // printf("[kinect_azure.cc] construct data\n");
      // Transform native data into JS data, passing it to the provided
      // `jsCallback` -- the TSFN's JavaScript function.
      mtx.lock();
      JSFrame jsFrame = *jsFrameRef;

      Napi::Object data = Napi::Object::New(env);
      {
        Napi::Object imu = Napi::Object::New(env);
        imu.Set(Napi::String::New(env, "temperature"), Napi::Number::New(env, jsFrame.imuSample.temperature));
        imu.Set(Napi::String::New(env, "accX"), Napi::Number::New(env, jsFrame.imuSample.accX));
        imu.Set(Napi::String::New(env, "accY"), Napi::Number::New(env, jsFrame.imuSample.accY));
        imu.Set(Napi::String::New(env, "accZ"), Napi::Number::New(env, jsFrame.imuSample.accZ));
        imu.Set(Napi::String::New(env, "accTimestamp"), Napi::Number::New(env, jsFrame.imuSample.accTimestamp));
        imu.Set(Napi::String::New(env, "gyroX"), Napi::Number::New(env, jsFrame.imuSample.gyroX));
        imu.Set(Napi::String::New(env, "gyroY"), Napi::Number::New(env, jsFrame.imuSample.gyroY));
        imu.Set(Napi::String::New(env, "gyroZ"), Napi::Number::New(env, jsFrame.imuSample.gyroZ));
        imu.Set(Napi::String::New(env, "gyroTimestamp"), Napi::Number::New(env, jsFrame.imuSample.gyroTimestamp));
        data.Set(Napi::String::New(env, "imu"), imu);
      }
      {
        Napi::Object colorImageFrame = Napi::Object::New(env);
        Napi::Buffer<uint8_t> imageData = Napi::Buffer<uint8_t>::New(env, jsFrame.colorImageFrame.image_data, jsFrame.colorImageFrame.image_length);
        colorImageFrame.Set(Napi::String::New(env, "imageData"), imageData);
        colorImageFrame.Set(Napi::String::New(env, "imageLength"), Napi::Number::New(env, jsFrame.colorImageFrame.image_length));
        colorImageFrame.Set(Napi::String::New(env, "width"), Napi::Number::New(env, jsFrame.colorImageFrame.width));
        colorImageFrame.Set(Napi::String::New(env, "height"), Napi::Number::New(env, jsFrame.colorImageFrame.height));
        colorImageFrame.Set(Napi::String::New(env, "strideBytes"), Napi::Number::New(env, jsFrame.colorImageFrame.stride_bytes));
        data.Set(Napi::String::New(env, "colorImageFrame"), colorImageFrame);
      }
      {
        Napi::Object depthImageFrame = Napi::Object::New(env);
        Napi::Buffer<uint8_t> imageData = Napi::Buffer<uint8_t>::New(env, jsFrame.depthImageFrame.image_data, jsFrame.depthImageFrame.image_length);
        depthImageFrame.Set(Napi::String::New(env, "imageData"), imageData);
        depthImageFrame.Set(Napi::String::New(env, "imageLength"), Napi::Number::New(env, jsFrame.depthImageFrame.image_length));
        depthImageFrame.Set(Napi::String::New(env, "width"), Napi::Number::New(env, jsFrame.depthImageFrame.width));
        depthImageFrame.Set(Napi::String::New(env, "height"), Napi::Number::New(env, jsFrame.depthImageFrame.height));
        depthImageFrame.Set(Napi::String::New(env, "strideBytes"), Napi::Number::New(env, jsFrame.depthImageFrame.stride_bytes));
        data.Set(Napi::String::New(env, "depthImageFrame"), depthImageFrame);
      }
      {
        Napi::Object irImageFrame = Napi::Object::New(env);
        Napi::Buffer<uint8_t> imageData = Napi::Buffer<uint8_t>::New(env, jsFrame.irImageFrame.image_data, jsFrame.irImageFrame.image_length);
        irImageFrame.Set(Napi::String::New(env, "imageData"), imageData);
        irImageFrame.Set(Napi::String::New(env, "imageLength"), Napi::Number::New(env, jsFrame.irImageFrame.image_length));
        irImageFrame.Set(Napi::String::New(env, "width"), Napi::Number::New(env, jsFrame.irImageFrame.width));
        irImageFrame.Set(Napi::String::New(env, "height"), Napi::Number::New(env, jsFrame.irImageFrame.height));
        irImageFrame.Set(Napi::String::New(env, "strideBytes"), Napi::Number::New(env, jsFrame.irImageFrame.stride_bytes));
        data.Set(Napi::String::New(env, "irImageFrame"), irImageFrame);
      }
      {
        Napi::Object depthToColorImageFrame = Napi::Object::New(env);
        Napi::Buffer<uint8_t> imageData = Napi::Buffer<uint8_t>::New(env, jsFrame.depthToColorImageFrame.image_data, jsFrame.depthToColorImageFrame.image_length);
        depthToColorImageFrame.Set(Napi::String::New(env, "imageData"), imageData);
        depthToColorImageFrame.Set(Napi::String::New(env, "imageLength"), Napi::Number::New(env, jsFrame.depthToColorImageFrame.image_length));
        depthToColorImageFrame.Set(Napi::String::New(env, "width"), Napi::Number::New(env, jsFrame.depthToColorImageFrame.width));
        depthToColorImageFrame.Set(Napi::String::New(env, "height"), Napi::Number::New(env, jsFrame.depthToColorImageFrame.height));
        depthToColorImageFrame.Set(Napi::String::New(env, "strideBytes"), Napi::Number::New(env, jsFrame.depthToColorImageFrame.stride_bytes));
        data.Set(Napi::String::New(env, "depthToColorImageFrame"), depthToColorImageFrame);
      }
      {
        Napi::Object colorToDepthImageFrame = Napi::Object::New(env);
        Napi::Buffer<uint8_t> imageData = Napi::Buffer<uint8_t>::New(env, jsFrame.colorToDepthImageFrame.image_data, jsFrame.colorToDepthImageFrame.image_length);
        colorToDepthImageFrame.Set(Napi::String::New(env, "imageData"), imageData);
        colorToDepthImageFrame.Set(Napi::String::New(env, "imageLength"), Napi::Number::New(env, jsFrame.colorToDepthImageFrame.image_length));
        colorToDepthImageFrame.Set(Napi::String::New(env, "width"), Napi::Number::New(env, jsFrame.colorToDepthImageFrame.width));
        colorToDepthImageFrame.Set(Napi::String::New(env, "height"), Napi::Number::New(env, jsFrame.colorToDepthImageFrame.height));
        colorToDepthImageFrame.Set(Napi::String::New(env, "strideBytes"), Napi::Number::New(env, jsFrame.colorToDepthImageFrame.stride_bytes));
        data.Set(Napi::String::New(env, "colorToDepthImageFrame"), colorToDepthImageFrame);
      }
      {
#ifdef KINECT_AZURE_ENABLE_BODY_TRACKING
        Napi::Object bodyFrame = Napi::Object::New(env);
        {
          {
            Napi::Object bodyIndexMapImageFrame = Napi::Object::New(env);
            Napi::Buffer<uint8_t> imageData = Napi::Buffer<uint8_t>::New(env, jsFrame.bodyFrame.bodyIndexMapImageFrame.image_data, jsFrame.bodyFrame.bodyIndexMapImageFrame.image_length);
            bodyIndexMapImageFrame.Set(Napi::String::New(env, "imageData"), imageData);
            bodyIndexMapImageFrame.Set(Napi::String::New(env, "imageLength"), Napi::Number::New(env, jsFrame.bodyFrame.bodyIndexMapImageFrame.image_length));
            bodyIndexMapImageFrame.Set(Napi::String::New(env, "width"), Napi::Number::New(env, jsFrame.bodyFrame.bodyIndexMapImageFrame.width));
            bodyIndexMapImageFrame.Set(Napi::String::New(env, "height"), Napi::Number::New(env, jsFrame.bodyFrame.bodyIndexMapImageFrame.height));
            bodyIndexMapImageFrame.Set(Napi::String::New(env, "strideBytes"), Napi::Number::New(env, jsFrame.bodyFrame.bodyIndexMapImageFrame.stride_bytes));
            bodyFrame.Set(Napi::String::New(env, "bodyIndexMapImageFrame"), bodyIndexMapImageFrame);
          }
          {
            Napi::Object bodyIndexMapToColorImageFrame = Napi::Object::New(env);
            Napi::Buffer<uint8_t> imageData = Napi::Buffer<uint8_t>::New(env, jsFrame.bodyFrame.bodyIndexMapToColorImageFrame.image_data, jsFrame.bodyFrame.bodyIndexMapToColorImageFrame.image_length);
            bodyIndexMapToColorImageFrame.Set(Napi::String::New(env, "imageData"), imageData);
            bodyIndexMapToColorImageFrame.Set(Napi::String::New(env, "imageLength"), Napi::Number::New(env, jsFrame.bodyFrame.bodyIndexMapToColorImageFrame.image_length));
            bodyIndexMapToColorImageFrame.Set(Napi::String::New(env, "width"), Napi::Number::New(env, jsFrame.bodyFrame.bodyIndexMapToColorImageFrame.width));
            bodyIndexMapToColorImageFrame.Set(Napi::String::New(env, "height"), Napi::Number::New(env, jsFrame.bodyFrame.bodyIndexMapToColorImageFrame.height));
            bodyIndexMapToColorImageFrame.Set(Napi::String::New(env, "strideBytes"), Napi::Number::New(env, jsFrame.bodyFrame.bodyIndexMapToColorImageFrame.stride_bytes));
            bodyFrame.Set(Napi::String::New(env, "bodyIndexMapToColorImageFrame"), bodyIndexMapToColorImageFrame);
          }
          bodyFrame.Set(Napi::String::New(env, "numBodies"), Napi::Number::New(env, jsFrame.bodyFrame.numBodies));
          Napi::Array bodies = Napi::Array::New(env, jsFrame.bodyFrame.numBodies);
          for (size_t i = 0; i < jsFrame.bodyFrame.numBodies; i++)
          {
            Napi::Object body = Napi::Object::New(env);
            body.Set(Napi::String::New(env, "id"), Napi::Number::New(env, jsFrame.bodyFrame.bodies[i].id));
            Napi::Object skeleton = Napi::Object::New(env);
            {
              Napi::Array joints = Napi::Array::New(env, K4ABT_JOINT_COUNT);
              for (size_t j = 0; j < K4ABT_JOINT_COUNT; j++)
              {
                Napi::Object joint = Napi::Object::New(env);

                joint.Set(Napi::String::New(env, "index"), Napi::Number::New(env, jsFrame.bodyFrame.bodies[i].skeleton.joints[j].index));

                joint.Set(Napi::String::New(env, "cameraX"), Napi::Number::New(env, jsFrame.bodyFrame.bodies[i].skeleton.joints[j].cameraX));
                joint.Set(Napi::String::New(env, "cameraY"), Napi::Number::New(env, jsFrame.bodyFrame.bodies[i].skeleton.joints[j].cameraY));
                joint.Set(Napi::String::New(env, "cameraZ"), Napi::Number::New(env, jsFrame.bodyFrame.bodies[i].skeleton.joints[j].cameraZ));

                joint.Set(Napi::String::New(env, "orientationX"), Napi::Number::New(env, jsFrame.bodyFrame.bodies[i].skeleton.joints[j].orientationX));
                joint.Set(Napi::String::New(env, "orientationY"), Napi::Number::New(env, jsFrame.bodyFrame.bodies[i].skeleton.joints[j].orientationY));
                joint.Set(Napi::String::New(env, "orientationZ"), Napi::Number::New(env, jsFrame.bodyFrame.bodies[i].skeleton.joints[j].orientationZ));
                joint.Set(Napi::String::New(env, "orientationW"), Napi::Number::New(env, jsFrame.bodyFrame.bodies[i].skeleton.joints[j].orientationW));

                joint.Set(Napi::String::New(env, "colorX"), Napi::Number::New(env, jsFrame.bodyFrame.bodies[i].skeleton.joints[j].colorX));
                joint.Set(Napi::String::New(env, "colorY"), Napi::Number::New(env, jsFrame.bodyFrame.bodies[i].skeleton.joints[j].colorY));

                joint.Set(Napi::String::New(env, "depthX"), Napi::Number::New(env, jsFrame.bodyFrame.bodies[i].skeleton.joints[j].depthX));
                joint.Set(Napi::String::New(env, "depthY"), Napi::Number::New(env, jsFrame.bodyFrame.bodies[i].skeleton.joints[j].depthY));

                joint.Set(Napi::String::New(env, "confidence"), Napi::Number::New(env, jsFrame.bodyFrame.bodies[i].skeleton.joints[j].confidence));

                joints.Set(Napi::Number::New(env, j), joint);
              }
              skeleton.Set(Napi::String::New(env, "joints"), joints);
            }
            body.Set(Napi::String::New(env, "skeleton"), skeleton);
            bodies.Set(Napi::Number::New(env, i), body);
          }
          bodyFrame.Set(Napi::String::New(env, "bodies"), bodies);
        }
        data.Set(Napi::String::New(env, "bodyFrame"), bodyFrame);
#endif // KINECT_AZURE_ENABLE_BODY_TRACKING
      }

      // printf("[kinect_azure.cc] jsCallback.Call\n");
      jsCallback.Call({data});
      // printf("[kinect_azure.cc] callback done\n");
      mtx.unlock();
    };

    uint8_t *processed_color_data = NULL;
    uint8_t *processed_depth_data = NULL;

    JSFrame jsFrame;
    while (is_listening)
    {
      k4a_capture_t sensor_capture;
      if (is_playing)
      {
        if (is_paused && !is_seeking)
          continue;

        is_seeking = false;
        k4a_stream_result_t get_stream_result = k4a_playback_get_next_capture(playback_handle, &sensor_capture);
        if (get_stream_result == K4A_STREAM_RESULT_SUCCEEDED)
        {
          int milliseconds = round(1000 / g_playbackProps.playback_fps);
          std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
        }
        else if (get_stream_result == K4A_STREAM_RESULT_EOF)
        {
          // End of file reached
          k4a_playback_seek_timestamp(playback_handle, 0, K4A_PLAYBACK_SEEK_BEGIN);
          continue;
        }
        else
        {
          continue;
        }
      }
      else
      {
        k4a_wait_result_t get_capture_result = k4a_device_get_capture(g_device, &sensor_capture, 0);
        if (get_capture_result != K4A_WAIT_RESULT_SUCCEEDED)
        {
          // printf("[kinect_azure.cc] get_capture_result != K4A_WAIT_RESULT_SUCCEEDED\n");
          continue;
        }
      }

      // Create new data
      mtx.lock();
      // printf("[kinect_azure.cc] jsFrame.reset\n");
      jsFrame.reset();
      k4a_imu_sample_t imu_sample;
      k4a_image_t color_image = NULL;
      k4a_image_t depth_image = NULL;
      k4a_image_t ir_image = NULL;
      k4a_image_t depth_to_color_image = NULL;
      k4a_image_t color_to_depth_image = NULL;
      if (g_customDeviceConfig.include_imu_sample)
      {
        while (k4a_device_get_imu_sample(g_device, &imu_sample, 0) == K4A_WAIT_RESULT_SUCCEEDED)
        {
          jsFrame.imuSample.temperature = imu_sample.temperature;
          jsFrame.imuSample.accX = imu_sample.acc_sample.xyz.x;
          jsFrame.imuSample.accY = imu_sample.acc_sample.xyz.y;
          jsFrame.imuSample.accZ = imu_sample.acc_sample.xyz.z;
          jsFrame.imuSample.accTimestamp = imu_sample.acc_timestamp_usec;
          jsFrame.imuSample.gyroX = imu_sample.gyro_sample.xyz.x;
          jsFrame.imuSample.gyroY = imu_sample.gyro_sample.xyz.y;
          jsFrame.imuSample.gyroZ = imu_sample.gyro_sample.xyz.z;
          jsFrame.imuSample.gyroTimestamp = imu_sample.gyro_timestamp_usec;
        }
      }
      if (g_deviceConfig.depth_mode != K4A_DEPTH_MODE_OFF)
      {
        // printf("[kinect_azure.cc] k4a_capture_get_depth_image\n");
        // if it is not passive IR, capture the depth image
        if (g_deviceConfig.depth_mode != K4A_DEPTH_MODE_PASSIVE_IR)
        {
          depth_image = k4a_capture_get_depth_image(sensor_capture);
          if (depth_image != NULL)
          {
            jsFrame.depthImageFrame.image_length = k4a_image_get_size(depth_image);
            jsFrame.depthImageFrame.width = k4a_image_get_width_pixels(depth_image);
            jsFrame.depthImageFrame.height = k4a_image_get_height_pixels(depth_image);
            jsFrame.depthImageFrame.stride_bytes = k4a_image_get_stride_bytes(depth_image);
            jsFrame.depthImageFrame.image_data = new uint8_t[jsFrame.depthImageFrame.image_length];
          }
        }
        // capture the IR image
        ir_image = k4a_capture_get_ir_image(sensor_capture);
        if (ir_image != NULL)
        {
          jsFrame.irImageFrame.image_length = k4a_image_get_size(ir_image);
          jsFrame.irImageFrame.width = k4a_image_get_width_pixels(ir_image);
          jsFrame.irImageFrame.height = k4a_image_get_height_pixels(ir_image);
          jsFrame.irImageFrame.stride_bytes = k4a_image_get_stride_bytes(ir_image);
          jsFrame.irImageFrame.image_data = new uint8_t[jsFrame.irImageFrame.image_length];
        }
      }
      if (g_deviceConfig.color_resolution != K4A_COLOR_RESOLUTION_OFF)
      {
        // printf("[kinect_azure.cc] k4a_capture_get_color_image\n");
        color_image = k4a_capture_get_color_image(sensor_capture);
        if (color_image != NULL)
        {
          jsFrame.colorImageFrame.image_length = k4a_image_get_size(color_image);
          jsFrame.colorImageFrame.width = k4a_image_get_width_pixels(color_image);
          jsFrame.colorImageFrame.height = k4a_image_get_height_pixels(color_image);
          jsFrame.colorImageFrame.stride_bytes = k4a_image_get_stride_bytes(color_image);
          jsFrame.colorImageFrame.image_data = new uint8_t[jsFrame.colorImageFrame.image_length];
        }
        colorTimestamp = (double)k4a_image_get_device_timestamp_usec(color_image);
      }

      if (g_customDeviceConfig.include_depth_to_color)
      {

        if (
            k4a_image_create(
                K4A_IMAGE_FORMAT_DEPTH16,
                jsFrame.colorImageFrame.width, jsFrame.colorImageFrame.height,
                jsFrame.colorImageFrame.width * 2,
                &depth_to_color_image) == K4A_RESULT_SUCCEEDED)
        {
          jsFrame.depthToColorImageFrame.height = 10;

          if (
              k4a_transformation_depth_image_to_color_camera(transformer, depth_image, depth_to_color_image) == K4A_RESULT_SUCCEEDED)
          {
            jsFrame.depthToColorImageFrame.width = 10;

            jsFrame.depthToColorImageFrame.width = k4a_image_get_width_pixels(depth_to_color_image);
            jsFrame.depthToColorImageFrame.height = k4a_image_get_height_pixels(depth_to_color_image);

            if (g_customDeviceConfig.depth_to_greyscale == true || g_customDeviceConfig.depth_to_redblue == true)
            {
              jsFrame.depthToColorImageFrame.image_length = k4a_image_get_size(color_image);
              jsFrame.depthToColorImageFrame.stride_bytes = k4a_image_get_stride_bytes(color_image);
              jsFrame.depthToColorImageFrame.image_data = new uint8_t[jsFrame.colorImageFrame.image_length];
            }
            else
            {
              jsFrame.depthToColorImageFrame.image_length = k4a_image_get_size(depth_to_color_image);
              jsFrame.depthToColorImageFrame.stride_bytes = k4a_image_get_stride_bytes(depth_to_color_image);
              jsFrame.depthToColorImageFrame.image_data = new uint8_t[jsFrame.depthToColorImageFrame.image_length];
            }
          }
          else
          {
            jsFrame.depthToColorImageFrame.width = 1;
          }
        }
      }
      if (g_customDeviceConfig.include_color_to_depth)
      {
        if (k4a_image_create(K4A_IMAGE_FORMAT_COLOR_BGRA32, jsFrame.depthImageFrame.width, jsFrame.depthImageFrame.height, jsFrame.depthImageFrame.width * 4, &color_to_depth_image) == K4A_RESULT_SUCCEEDED)
        {
          if (k4a_transformation_color_image_to_depth_camera(transformer, depth_image, color_image, color_to_depth_image) == K4A_RESULT_SUCCEEDED)
          {
            jsFrame.colorToDepthImageFrame.image_length = k4a_image_get_size(color_to_depth_image);
            jsFrame.colorToDepthImageFrame.width = k4a_image_get_width_pixels(color_to_depth_image);
            jsFrame.colorToDepthImageFrame.height = k4a_image_get_height_pixels(color_to_depth_image);
            jsFrame.colorToDepthImageFrame.stride_bytes = k4a_image_get_stride_bytes(color_to_depth_image);
            jsFrame.colorToDepthImageFrame.image_data = new uint8_t[jsFrame.colorToDepthImageFrame.image_length];
          }
        }
      }

      if (depth_image != NULL)
      {
        uint8_t *image_data = k4a_image_get_buffer(depth_image);
        memcpy(jsFrame.depthImageFrame.image_data, image_data, jsFrame.depthImageFrame.image_length);
      }

      if (ir_image != NULL)
      {
        uint8_t *image_data = k4a_image_get_buffer(ir_image);
        memcpy(jsFrame.irImageFrame.image_data, image_data, jsFrame.irImageFrame.image_length);
      }

      if (color_image != NULL)
      {
        uint8_t *image_data = k4a_image_get_buffer(color_image);
        if (processed_color_data == NULL)
          processed_color_data = new uint8_t[jsFrame.colorImageFrame.image_length];

        if (g_customDeviceConfig.flip_BGRA_to_RGBA == true || g_customDeviceConfig.apply_depth_to_alpha == true)
        {
          if (g_customDeviceConfig.flip_BGRA_to_RGBA == true)
          {
            for (int i = 0; i < jsFrame.colorImageFrame.image_length; i += 4)
            {
              processed_color_data[i] = image_data[i + 2];
              processed_color_data[i + 1] = image_data[i + 1];
              processed_color_data[i + 2] = image_data[i];
              processed_color_data[i + 3] = image_data[i + 3];
            }
          }
          else
          {
            memcpy(processed_color_data, image_data, jsFrame.colorImageFrame.image_length);
          }
          if (g_customDeviceConfig.apply_depth_to_alpha == true)
          {
            depthPixelIndex = 0;
            depth_to_color_data = k4a_image_get_buffer(depth_to_color_image);

            for (int i = 0; i < jsFrame.colorImageFrame.image_length; i += 4)
            {
              combined = (depth_to_color_data[depthPixelIndex + 1] << 8) | (depth_to_color_data[depthPixelIndex] & 0xff);
              normalizedValue = map(combined, g_customDeviceConfig.min_depth, g_customDeviceConfig.max_depth, 255, 0);
              if (normalizedValue >= 0xF0)
                normalizedValue = 0;
              processed_color_data[i + 3] = normalizedValue;
              depthPixelIndex += 2;
            }
          }
          memcpy(jsFrame.colorImageFrame.image_data, processed_color_data, jsFrame.colorImageFrame.image_length);
        }
        else
        {
          memcpy(jsFrame.colorImageFrame.image_data, image_data, jsFrame.colorImageFrame.image_length);
        }
      }

      if (depth_to_color_image != NULL)
      {
        uint8_t *image_data = k4a_image_get_buffer(depth_to_color_image);
        if (g_customDeviceConfig.depth_to_greyscale == true || g_customDeviceConfig.depth_to_redblue == true)
        {

          if (processed_depth_data == NULL)
            processed_depth_data = new uint8_t[jsFrame.colorImageFrame.image_length];

          depthPixelIndex = 0;
          if (g_customDeviceConfig.depth_to_greyscale == true)
          {

            for (int i = 0; i < jsFrame.depthToColorImageFrame.image_length; i += 4)
            {
              combined = (image_data[depthPixelIndex + 1] << 8) | (image_data[depthPixelIndex] & 0xff);
              normalizedValue = map(combined, g_customDeviceConfig.min_depth, g_customDeviceConfig.max_depth, 255, 0);
              if (normalizedValue >= 0xF0)
                normalizedValue = 0;
              processed_depth_data[i] = normalizedValue;
              processed_depth_data[i + 1] = normalizedValue;
              processed_depth_data[i + 2] = normalizedValue;
              processed_depth_data[i + 3] = 0xFF;
              depthPixelIndex += 2;
            }
          }
          else if (g_customDeviceConfig.depth_to_redblue == true)
          {
            for (int i = 0; i < jsFrame.depthToColorImageFrame.image_length; i += 4)
            {
              combined = std::min(g_customDeviceConfig.max_depth, std::max(g_customDeviceConfig.min_depth, image_data[depthPixelIndex + 1] << 8 | image_data[depthPixelIndex]));
              float hue = ((float)(combined - g_customDeviceConfig.min_depth) / (float)(g_customDeviceConfig.max_depth - g_customDeviceConfig.min_depth));
              hue = 1 - hue;
              colorUtils::hsvToRgb(hue * 0xFF, 1.0f, 1.0f, rgb);
              processed_depth_data[i] = rgb[0];
              processed_depth_data[i + 1] = rgb[1];
              processed_depth_data[i + 2] = rgb[2];
              processed_depth_data[i + 3] = 0xFF;
              depthPixelIndex += 2;
            }
          }
          memcpy(jsFrame.depthToColorImageFrame.image_data, processed_depth_data, jsFrame.depthToColorImageFrame.image_length);
        }
        else
        {
          memcpy(jsFrame.depthToColorImageFrame.image_data, image_data, jsFrame.depthToColorImageFrame.image_length);
        }
      }
      if (color_to_depth_image != NULL)
      {
        uint8_t *image_data = k4a_image_get_buffer(color_to_depth_image);
        memcpy(jsFrame.colorToDepthImageFrame.image_data, image_data, jsFrame.colorToDepthImageFrame.image_length);
      }

#ifdef KINECT_AZURE_ENABLE_BODY_TRACKING
      // printf("[kinect_azure.cc] check tracker\n");
      if (g_tracker != NULL)
      {
        // printf("[kinect_azure.cc] k4abt_tracker_enqueue_capture\n");
        k4a_wait_result_t queue_capture_result = k4abt_tracker_enqueue_capture(g_tracker, sensor_capture, 0);
        if (queue_capture_result == K4A_WAIT_RESULT_FAILED)
        {
          k4a_capture_release(sensor_capture);
          mtx.unlock();
          break;
        }

        k4abt_frame_t body_frame = NULL;
        k4a_wait_result_t pop_frame_result = k4abt_tracker_pop_result(g_tracker, &body_frame, 0);
        if (pop_frame_result == K4A_WAIT_RESULT_SUCCEEDED)
        {
          // Successfully popped the body tracking result. Start your processing
          size_t num_bodies = k4abt_frame_get_num_bodies(body_frame);

          jsFrame.resetBodyFrame();

          // body index map?
          k4a_image_t body_index_map_image = NULL;
          k4a_image_t body_index_map_color_image = NULL;
          if (g_customDeviceConfig.include_body_index_map)
          {
            body_index_map_image = k4abt_frame_get_body_index_map(body_frame);
            if (body_index_map_image != NULL)
            {
              jsFrame.bodyFrame.bodyIndexMapImageFrame.image_length = k4a_image_get_size(body_index_map_image);
              jsFrame.bodyFrame.bodyIndexMapImageFrame.width = k4a_image_get_width_pixels(body_index_map_image);
              jsFrame.bodyFrame.bodyIndexMapImageFrame.height = k4a_image_get_height_pixels(body_index_map_image);
              jsFrame.bodyFrame.bodyIndexMapImageFrame.stride_bytes = k4a_image_get_stride_bytes(body_index_map_image);
              jsFrame.bodyFrame.bodyIndexMapImageFrame.image_data = new uint8_t[jsFrame.bodyFrame.bodyIndexMapImageFrame.image_length];
              uint8_t *image_data = k4a_image_get_buffer(body_index_map_image);
              memcpy(jsFrame.bodyFrame.bodyIndexMapImageFrame.image_data, image_data, jsFrame.bodyFrame.bodyIndexMapImageFrame.image_length);
            }
            // transform to color space as well?
            if (g_customDeviceConfig.include_depth_to_color)
            {
              if (
                  k4a_image_create(
                      K4A_IMAGE_FORMAT_CUSTOM8,
                      jsFrame.colorImageFrame.width, jsFrame.colorImageFrame.height,
                      jsFrame.colorImageFrame.width * (int)sizeof(uint8_t),
                      &body_index_map_color_image) == K4A_RESULT_SUCCEEDED)
              {
                if (k4a_transformation_depth_image_to_color_camera_custom(transformer, depth_image, body_index_map_image, depth_to_color_image, body_index_map_color_image, K4A_TRANSFORMATION_INTERPOLATION_TYPE_NEAREST, K4ABT_BODY_INDEX_MAP_BACKGROUND) == K4A_RESULT_SUCCEEDED)
                {
                  jsFrame.bodyFrame.bodyIndexMapToColorImageFrame.image_length = k4a_image_get_size(body_index_map_color_image);
                  jsFrame.bodyFrame.bodyIndexMapToColorImageFrame.width = k4a_image_get_width_pixels(body_index_map_color_image);
                  jsFrame.bodyFrame.bodyIndexMapToColorImageFrame.height = k4a_image_get_height_pixels(body_index_map_color_image);
                  jsFrame.bodyFrame.bodyIndexMapToColorImageFrame.stride_bytes = k4a_image_get_stride_bytes(body_index_map_color_image);
                  jsFrame.bodyFrame.bodyIndexMapToColorImageFrame.image_data = new uint8_t[jsFrame.bodyFrame.bodyIndexMapToColorImageFrame.image_length];

                  uint8_t *image_data = k4a_image_get_buffer(body_index_map_color_image);
                  memcpy(jsFrame.bodyFrame.bodyIndexMapToColorImageFrame.image_data, image_data, jsFrame.bodyFrame.bodyIndexMapToColorImageFrame.image_length);
                }
              }
            }
          }

          jsFrame.bodyFrame.numBodies = num_bodies;
          jsFrame.bodyFrame.bodies = new JSBody[num_bodies];

          for (size_t i = 0; i < num_bodies; i++)
          {
            jsFrame.bodyFrame.bodies[i].id = k4abt_frame_get_body_id(body_frame, i);

            k4abt_skeleton_t skeleton;
            k4abt_frame_get_body_skeleton(body_frame, i, &skeleton);

            for (int j = 0; j < K4ABT_JOINT_COUNT; j++)
            {
              k4abt_joint_t joint = skeleton.joints[j];
              jsFrame.bodyFrame.bodies[i].skeleton.joints[j].index = j;

              jsFrame.bodyFrame.bodies[i].skeleton.joints[j].cameraX = joint.position.xyz.x;
              jsFrame.bodyFrame.bodies[i].skeleton.joints[j].cameraY = joint.position.xyz.y;
              jsFrame.bodyFrame.bodies[i].skeleton.joints[j].cameraZ = joint.position.xyz.z;

              jsFrame.bodyFrame.bodies[i].skeleton.joints[j].orientationX = joint.orientation.wxyz.x;
              jsFrame.bodyFrame.bodies[i].skeleton.joints[j].orientationY = joint.orientation.wxyz.y;
              jsFrame.bodyFrame.bodies[i].skeleton.joints[j].orientationZ = joint.orientation.wxyz.z;
              jsFrame.bodyFrame.bodies[i].skeleton.joints[j].orientationW = joint.orientation.wxyz.w;

              k4a_float2_t point2d;
              bool valid;
              valid = transform_joint_from_depth_3d_to_2d(
                  &g_calibration,
                  skeleton.joints[j].position,
                  point2d,
                  K4A_CALIBRATION_TYPE_DEPTH);

              if (valid)
              {
                jsFrame.bodyFrame.bodies[i].skeleton.joints[j].depthX = point2d.xy.x;
                jsFrame.bodyFrame.bodies[i].skeleton.joints[j].depthY = point2d.xy.y;
              }

              if (g_deviceConfig.color_resolution != K4A_COLOR_RESOLUTION_OFF)
              {
                valid = transform_joint_from_depth_3d_to_2d(
                    &g_calibration,
                    skeleton.joints[j].position,
                    point2d,
                    K4A_CALIBRATION_TYPE_COLOR);

                if (valid)
                {
                  jsFrame.bodyFrame.bodies[i].skeleton.joints[j].colorX = point2d.xy.x;
                  jsFrame.bodyFrame.bodies[i].skeleton.joints[j].colorY = point2d.xy.y;
                }
              }

              jsFrame.bodyFrame.bodies[i].skeleton.joints[j].confidence = joint.confidence_level;
            }
          }

          if (body_index_map_image != NULL)
          {
            k4a_image_release(body_index_map_image);
            body_index_map_image = NULL;
          }

          if (body_index_map_color_image != NULL)
          {
            k4a_image_release(body_index_map_color_image);
            body_index_map_color_image = NULL;
          }

          if (body_index_map_color_image != NULL)
          {
            k4a_image_release(body_index_map_color_image);
            body_index_map_color_image = NULL;
          }

          k4abt_frame_release(body_frame);
          body_frame = NULL;
        }
      }
#endif // KINECT_AZURE_ENABLE_BODY_TRACKING

      // release images
      if (depth_image != NULL)
      {
        k4a_image_release(depth_image);
        depth_image = NULL;
      }
      if (ir_image != NULL)
      {
        k4a_image_release(ir_image);
        ir_image = NULL;
      }
      if (color_image != NULL)
      {
        k4a_image_release(color_image);
        color_image = NULL;
      }
      if (color_to_depth_image != NULL)
      {
        k4a_image_release(color_to_depth_image);
        color_to_depth_image = NULL;
      }
      if (depth_to_color_image != NULL)
      {
        k4a_image_release(depth_to_color_image);
        depth_to_color_image = NULL;
      }

      k4a_capture_release(sensor_capture);

      if (!is_listening)
      {
        mtx.unlock();
        break;
      }
      // Perform a blocking call
      mtx.unlock();
      napi_status status = tsfn.BlockingCall(&jsFrame, callback);
      if (status != napi_ok)
      {
        break;
      }
    }

    if (processed_color_data != NULL)
    {
      delete[] processed_color_data;
      processed_color_data = NULL;
    }

    if (processed_depth_data != NULL)
    {
      delete[] processed_depth_data;
      processed_depth_data = NULL;
    }

    if (transformer != NULL)
    {
      k4a_transformation_destroy(transformer);
    }

    is_listening = false;
    // printf("[kinect_azure.cc] reset jsFrame\n");
    jsFrame.reset();
    // printf("[kinect_azure.cc] Release thread-safe function!\n");
    // Release the thread-safe function
    tsfn.Release();
    // printf("[kinect_azure.cc] Thread-safe function released!\n");
  });

  return Napi::Boolean::New(env, true);
}

class WaitForTheadJoinWorker : public Napi::AsyncWorker
{
public:
  WaitForTheadJoinWorker(Napi::Function &callback)
      : AsyncWorker(callback) {}

  ~WaitForTheadJoinWorker() {}
  // This code will be executed on the worker thread
  void Execute() override
  {
    threadJoinedMutex.lock();
  }

  void OnOK() override
  {
    threadJoinedMutex.unlock();
    Napi::HandleScope scope(Env());
    Callback().Call({Env().Null(), Napi::String::New(Env(), "OK")});
  }

private:
  std::string echo;
};

Napi::Value MethodStopListening(const Napi::CallbackInfo &info)
{
  Napi::Env env = info.Env();
  if (!is_listening)
  {
    Napi::TypeError::New(env, "Kinect was not listening")
        .ThrowAsJavaScriptException();
    return env.Null();
  }
  // printf("[kinect_azure.cc] MethodStopListening\n");
  is_listening = false;
  Napi::Function cb = info[0].As<Napi::Function>();
  WaitForTheadJoinWorker *wk = new WaitForTheadJoinWorker(cb);
  wk->Queue();
  return info.Env().Undefined();
}

Napi::Object Init(Napi::Env env, Napi::Object exports)
{
  exports.Set(Napi::String::New(env, "init"), Napi::Function::New(env, MethodInit));
  exports.Set(Napi::String::New(env, "getInstalledCount"), Napi::Function::New(env, MethodGetInstalledCount));
  exports.Set(Napi::String::New(env, "getSerialNumber"), Napi::Function::New(env, MethodGetSerialNumber));
  exports.Set(Napi::String::New(env, "openPlayback"), Napi::Function::New(env, MethodOpenPlayback));
  exports.Set(Napi::String::New(env, "startPlayback"), Napi::Function::New(env, MethodStartPlayback));
  exports.Set(Napi::String::New(env, "stopPlayback"), Napi::Function::New(env, MethodStopPlayback));
  exports.Set(Napi::String::New(env, "pause"), Napi::Function::New(env, MethodPause));
  exports.Set(Napi::String::New(env, "resume"), Napi::Function::New(env, MethodResume));
  exports.Set(Napi::String::New(env, "seek"), Napi::Function::New(env, MethodSeek));
  exports.Set(Napi::String::New(env, "time"), Napi::Function::New(env, MethodTime));
  exports.Set(Napi::String::New(env, "duration"), Napi::Function::New(env, MethodDuration));
  exports.Set(Napi::String::New(env, "open"), Napi::Function::New(env, MethodOpen));
  exports.Set(Napi::String::New(env, "serialOpen"), Napi::Function::New(env, MethodSerialOpen));
  exports.Set(Napi::String::New(env, "close"), Napi::Function::New(env, MethodClose));
  exports.Set(Napi::String::New(env, "startCameras"), Napi::Function::New(env, MethodStartCameras));
  exports.Set(Napi::String::New(env, "stopCameras"), Napi::Function::New(env, MethodStopCameras));
#ifdef KINECT_AZURE_ENABLE_BODY_TRACKING
  exports.Set(Napi::String::New(env, "createTracker"), Napi::Function::New(env, MethodCreateTracker));
  exports.Set(Napi::String::New(env, "destroyTracker"), Napi::Function::New(env, MethodDestroyTracker));
#endif // KINECT_AZURE_ENABLE_BODY_TRACKING
  exports.Set(Napi::String::New(env, "setColorControl"), Napi::Function::New(env, MethodSetColorControl));
  exports.Set(Napi::String::New(env, "startListening"), Napi::Function::New(env, MethodStartListening));
  exports.Set(Napi::String::New(env, "stopListening"), Napi::Function::New(env, MethodStopListening));
  return exports;
}

NODE_API_MODULE(hello, Init)