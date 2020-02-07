#define KINECT_AZURE_ENABLE_BODY_TRACKING = 1

#include <thread>
#include <mutex>
#include <napi.h>
#include <k4a/k4a.h>
#ifdef KINECT_AZURE_ENABLE_BODY_TRACKING
  #include <k4abt.h>
#endif // KINECT_AZURE_ENABLE_BODY_TRACKING
#include "structs.h"

k4a_device_t g_device = NULL;
k4a_device_configuration_t g_deviceConfig = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
CustomDeviceConfig g_customDeviceConfig;
k4a_calibration_t g_calibration;
k4a_transformation_t transformer = NULL;
bool flipToRGBA = false;

#ifdef KINECT_AZURE_ENABLE_BODY_TRACKING
k4abt_tracker_t g_tracker = NULL;
#endif // KINECT_AZURE_ENABLE_BODY_TRACKING

Napi::FunctionReference g_emit;
std::thread nativeThread;
std::mutex mtx;
Napi::ThreadSafeFunction tsfn;
std::mutex threadJoinedMutex;

bool is_listening = false;

Napi::Value MethodInit(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  if (info.Length() < 1) {
    Napi::TypeError::New(env, "Wrong number of arguments")
        .ThrowAsJavaScriptException();
    return env.Null();
  }
  // first argument is emit function
  Napi::Function emit = info[0].As<Napi::Function>();
  g_emit = Napi::Persistent(emit);
  return Napi::Boolean::New(env, true);
}

Napi::Number MethodGetInstalledCount(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  uint32_t count = k4a_device_get_installed_count();
  return Napi::Number::New(env, count);
}

Napi::Value MethodOpen(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  g_emit.Call({Napi::String::New(env, "log"), Napi::String::New(env, "open")});
  k4a_device_open(K4A_DEVICE_DEFAULT, &g_device);
  return Napi::Boolean::New(env, true);
}

Napi::Value MethodClose(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  // printf("[kinect_azure.cc] MethodClose\n");
  k4a_device_close(g_device);
  return Napi::Boolean::New(env, true);
}

Napi::Value MethodStartCameras(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  if (info.Length() < 1) {
    Napi::TypeError::New(env, "Wrong number of arguments")
        .ThrowAsJavaScriptException();
    return env.Null();
  }

  k4a_device_configuration_t deviceConfig = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
  // deviceConfig.synchronized_images_only = false;
  Napi::Object js_config =  info[0].As<Napi::Object>();
  Napi::Value js_camera_fps = js_config.Get("camera_fps");
  if (js_camera_fps.IsNumber())
  {
    deviceConfig.camera_fps = (k4a_fps_t) js_camera_fps.As<Napi::Number>().Int32Value();
  }

  Napi::Value js_color_format = js_config.Get("color_format");
  if (js_color_format.IsNumber())
  {
    deviceConfig.color_format = (k4a_image_format_t) js_color_format.As<Napi::Number>().Int32Value();
    // if color_format = K4A_IMAGE_FORMAT_COLOR_RGBA32, resign to K4A_IMAGE_FORMAT_COLOR_BGRA32
    if (deviceConfig.color_format == (k4a_image_format_t) 8) {
      deviceConfig.color_format = (k4a_image_format_t) 3;
      flipToRGBA = true;
    }
  }

  Napi::Value js_color_resolution = js_config.Get("color_resolution");
  if (js_color_resolution.IsNumber())
  {
    deviceConfig.color_resolution = (k4a_color_resolution_t) js_color_resolution.As<Napi::Number>().Int32Value();
  }

  Napi::Value js_depth_mode = js_config.Get("depth_mode");
  if (js_depth_mode.IsNumber())
  {
    deviceConfig.depth_mode = (k4a_depth_mode_t) js_depth_mode.As<Napi::Number>().Int32Value();
  }

  g_customDeviceConfig.reset();
  Napi::Value js_include_depth_to_color = js_config.Get("include_depth_to_color");
  if (js_include_depth_to_color.IsBoolean())
  {
    g_customDeviceConfig.include_depth_to_color = js_include_depth_to_color.As<Napi::Boolean>();
  }
  Napi::Value js_include_color_to_depth = js_config.Get("include_color_to_depth");
  if (js_include_color_to_depth.IsBoolean())
  {
    g_customDeviceConfig.include_color_to_depth = js_include_color_to_depth.As<Napi::Boolean>();
  }

  g_deviceConfig = deviceConfig;
  k4a_device_start_cameras(g_device, &g_deviceConfig);

  k4a_device_get_calibration(g_device, g_deviceConfig.depth_mode, g_deviceConfig.color_resolution, &g_calibration);

  return Napi::Boolean::New(env, true);
}

Napi::Value MethodStopCameras(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  k4a_device_stop_cameras(g_device);
  return Napi::Boolean::New(env, true);
}

#ifdef KINECT_AZURE_ENABLE_BODY_TRACKING
Napi::Value MethodCreateTracker(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  k4a_calibration_t sensor_calibration;
  k4a_device_get_calibration(g_device, g_deviceConfig.depth_mode, g_deviceConfig.color_resolution, &sensor_calibration);
  k4abt_tracker_configuration_t tracker_config = K4ABT_TRACKER_CONFIG_DEFAULT;
  k4abt_tracker_create(&sensor_calibration, tracker_config, &g_tracker);
  return Napi::Boolean::New(env, true);
}

Napi::Value MethodDestroyTracker(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  // printf("[kinect_azure.cc] MethodDestroyTracker\n");
  if (g_tracker != NULL) {
    k4abt_tracker_shutdown(g_tracker);
    k4abt_tracker_destroy(g_tracker);
  }
  g_tracker = NULL;
  return Napi::Boolean::New(env, true);
}
#endif // KINECT_AZURE_ENABLE_BODY_TRACKING

Napi::Value MethodStartListening(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  if (is_listening) {
    Napi::TypeError::New(env, "Kinect already listening")
        .ThrowAsJavaScriptException();
    return env.Null();
  }

  is_listening = true;

  if (info.Length() < 1) {
    Napi::TypeError::New(env, "Wrong number of arguments")
        .ThrowAsJavaScriptException();
    return env.Null();
  } else if (!info[0].IsFunction()){
    throw Napi::TypeError::New( env, "Expected first arg to be function" );
  }

  transformer = k4a_transformation_create( &g_calibration );
  tsfn = Napi::ThreadSafeFunction::New(
    env,
    info[0].As<Napi::Function>(),    // JavaScript function called asynchronously
    "Kinect Azure Listening",  // Name
    1,                         // 1 call in queue
    1,                         // Only one thread will use this initially
    []( Napi::Env ) {          // Finalizer used to clean threads up
      nativeThread.join();
      threadJoinedMutex.unlock();
    });

  threadJoinedMutex.lock();
  nativeThread = std::thread( [] {
    auto callback = []( Napi::Env env, Napi::Function jsCallback, JSFrame* jsFrameRef ) {
      if (!is_listening) {
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
          bodyFrame.Set(Napi::String::New(env, "numBodies"), Napi::Number::New(env, jsFrame.bodyFrame.numBodies));
          Napi::Array bodies = Napi::Array::New(env, jsFrame.bodyFrame.numBodies);
          for (size_t i = 0; i < jsFrame.bodyFrame.numBodies; i++) {
            Napi::Object body = Napi::Object::New(env);
            body.Set(Napi::String::New(env, "id"), Napi::Number::New(env, jsFrame.bodyFrame.bodies[i].id));
            Napi::Object skeleton = Napi::Object::New(env);
            {
              Napi::Array joints = Napi::Array::New(env, K4ABT_JOINT_COUNT);
              for (size_t j = 0; j < K4ABT_JOINT_COUNT; j++) {
                Napi::Object joint = Napi::Object::New(env);
                
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
      jsCallback.Call( { data } );
      // printf("[kinect_azure.cc] callback done\n");
      mtx.unlock();
    };

    uint8_t* rgba_data = NULL;
    JSFrame jsFrame;
    while(is_listening)
    {
      k4a_capture_t sensor_capture;
      k4a_wait_result_t get_capture_result = k4a_device_get_capture(g_device, &sensor_capture, 0);
      if (get_capture_result != K4A_WAIT_RESULT_SUCCEEDED)
      {
        // printf("[kinect_azure.cc] get_capture_result != K4A_WAIT_RESULT_SUCCEEDED\n");
        continue;
      }
      // Create new data
      mtx.lock();
      // printf("[kinect_azure.cc] jsFrame.reset\n");
      jsFrame.reset();
      k4a_image_t color_image = NULL;
      k4a_image_t depth_image = NULL;
      k4a_image_t ir_image = NULL;
      k4a_image_t depth_to_color_image = NULL;
      k4a_image_t color_to_depth_image = NULL;
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
            uint8_t* image_data = k4a_image_get_buffer(depth_image);
            memcpy(jsFrame.depthImageFrame.image_data, image_data, jsFrame.depthImageFrame.image_length);
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
          uint8_t* image_data = k4a_image_get_buffer(ir_image);
          memcpy(jsFrame.irImageFrame.image_data, image_data, jsFrame.irImageFrame.image_length);
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
          uint8_t* image_data = k4a_image_get_buffer(color_image);
          if (flipToRGBA != true){
            memcpy(jsFrame.colorImageFrame.image_data, image_data, jsFrame.colorImageFrame.image_length);
          } else {
            if (rgba_data == NULL)
              rgba_data = new uint8_t[jsFrame.colorImageFrame.image_length];
            
            for( int i = 0; i < jsFrame.colorImageFrame.image_length; i+=4 ) {
              rgba_data[i] = image_data[i+2];
              rgba_data[i+1] = image_data[i+1];
              rgba_data[i+2] = image_data[i];
              rgba_data[i+3] = image_data[i+3];
            }
            memcpy(jsFrame.colorImageFrame.image_data, rgba_data, jsFrame.colorImageFrame.image_length);
          }
        }
      }
      if (g_customDeviceConfig.include_depth_to_color)
      {
        if(k4a_image_create(K4A_IMAGE_FORMAT_DEPTH16, jsFrame.colorImageFrame.width, jsFrame.colorImageFrame.height, jsFrame.colorImageFrame.width * 2, &depth_to_color_image) == K4A_RESULT_SUCCEEDED)
        {
          if(k4a_transformation_depth_image_to_color_camera(transformer, depth_image, depth_to_color_image) == K4A_RESULT_SUCCEEDED)
          {
            jsFrame.depthToColorImageFrame.image_length = k4a_image_get_size(depth_to_color_image);
            jsFrame.depthToColorImageFrame.width = k4a_image_get_width_pixels(depth_to_color_image);
            jsFrame.depthToColorImageFrame.height = k4a_image_get_height_pixels(depth_to_color_image);
            jsFrame.depthToColorImageFrame.stride_bytes = k4a_image_get_stride_bytes(depth_to_color_image);
            jsFrame.depthToColorImageFrame.image_data = new uint8_t[jsFrame.depthToColorImageFrame.image_length];
            uint8_t* image_data = k4a_image_get_buffer(depth_to_color_image);
            memcpy(jsFrame.depthToColorImageFrame.image_data, image_data, jsFrame.depthToColorImageFrame.image_length);
          }
        }
      }
      if (g_customDeviceConfig.include_color_to_depth)
      {
        if(k4a_image_create(K4A_IMAGE_FORMAT_COLOR_BGRA32, jsFrame.depthImageFrame.width, jsFrame.depthImageFrame.height, jsFrame.depthImageFrame.width * 4, &color_to_depth_image) == K4A_RESULT_SUCCEEDED)
        {
          if(k4a_transformation_color_image_to_depth_camera(transformer, depth_image, color_image, color_to_depth_image) == K4A_RESULT_SUCCEEDED)
          {
            jsFrame.colorToDepthImageFrame.image_length = k4a_image_get_size(color_to_depth_image);
            jsFrame.colorToDepthImageFrame.width = k4a_image_get_width_pixels(color_to_depth_image);
            jsFrame.colorToDepthImageFrame.height = k4a_image_get_height_pixels(color_to_depth_image);
            jsFrame.colorToDepthImageFrame.stride_bytes = k4a_image_get_stride_bytes(color_to_depth_image);
            jsFrame.colorToDepthImageFrame.image_data = new uint8_t[jsFrame.colorToDepthImageFrame.image_length];
            uint8_t* image_data = k4a_image_get_buffer(color_to_depth_image);
            memcpy(jsFrame.colorToDepthImageFrame.image_data, image_data, jsFrame.colorToDepthImageFrame.image_length);
          }
        }
      }
      // release the image handles
      if (depth_image != NULL)
      {
        k4a_image_release(depth_image);
        depth_image = NULL;
      }
      if (color_image != NULL)
      {
        k4a_image_release(color_image);
        color_image = NULL;
      }
      if (ir_image != NULL)
      {
        k4a_image_release(ir_image);
        ir_image = NULL;
      }
      if (depth_to_color_image != NULL)
      {
        k4a_image_release(depth_to_color_image);
        depth_to_color_image = NULL;
      }
      if (color_to_depth_image != NULL)
      {
        k4a_image_release(color_to_depth_image);
        color_to_depth_image = NULL;
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
          jsFrame.bodyFrame.numBodies = num_bodies;
          jsFrame.bodyFrame.bodies = new JSBody[num_bodies];

          for (size_t i = 0; i < num_bodies; i++)
          {
            jsFrame.bodyFrame.bodies[i].id = k4abt_frame_get_body_id(body_frame, i);
            
            k4abt_skeleton_t skeleton;
            k4abt_frame_get_body_skeleton(body_frame, i, &skeleton);

            for (size_t j = 0; j < K4ABT_JOINT_COUNT; j++)
            {
              k4abt_joint_t joint = skeleton.joints[j];
              jsFrame.bodyFrame.bodies[i].skeleton.joints[j].cameraX = joint.position.xyz.x;
              jsFrame.bodyFrame.bodies[i].skeleton.joints[j].cameraY = joint.position.xyz.y;
              jsFrame.bodyFrame.bodies[i].skeleton.joints[j].cameraZ = joint.position.xyz.z;

              jsFrame.bodyFrame.bodies[i].skeleton.joints[j].orientationX = joint.orientation.wxyz.x;
              jsFrame.bodyFrame.bodies[i].skeleton.joints[j].orientationY = joint.orientation.wxyz.y;
              jsFrame.bodyFrame.bodies[i].skeleton.joints[j].orientationZ = joint.orientation.wxyz.z;
              jsFrame.bodyFrame.bodies[i].skeleton.joints[j].orientationW = joint.orientation.wxyz.w;

              k4a_float2_t point2d;
              int valid;

              k4a_calibration_3d_to_2d(&g_calibration, &joint.position, K4A_CALIBRATION_TYPE_DEPTH, K4A_CALIBRATION_TYPE_COLOR, &point2d, &valid);
              if (valid)
              {
                jsFrame.bodyFrame.bodies[i].skeleton.joints[j].colorX = point2d.xy.x;
                jsFrame.bodyFrame.bodies[i].skeleton.joints[j].colorY = point2d.xy.y;
              }
              k4a_calibration_3d_to_2d(&g_calibration, &joint.position, K4A_CALIBRATION_TYPE_DEPTH, K4A_CALIBRATION_TYPE_DEPTH, &point2d, &valid);
              if (valid)
              {
                jsFrame.bodyFrame.bodies[i].skeleton.joints[j].depthX = point2d.xy.x;
                jsFrame.bodyFrame.bodies[i].skeleton.joints[j].depthY = point2d.xy.y;
              }

              jsFrame.bodyFrame.bodies[i].skeleton.joints[j].confidence = joint.confidence_level;
            }
          }

          k4abt_frame_release(body_frame); // Remember to release the body frame once you finish using it
        }
      }
      #endif // KINECT_AZURE_ENABLE_BODY_TRACKING
      k4a_capture_release(sensor_capture);
     
      if (!is_listening)
      {
        mtx.unlock();
        break;
      }
      // Perform a blocking call
      // printf("[kinect_azure.cc] Perform a blocking call\n");
      mtx.unlock();
      napi_status status = tsfn.BlockingCall( &jsFrame, callback );
      if ( status != napi_ok )
      {
        // Handle error
        // printf("[kinect_azure.cc] BlockingCall Error!\n");
        break;
      } else {
        //  printf("[kinect_azure.cc] BlockingCall Done\n");
      }
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
  } );

  return Napi::Boolean::New(env, true);
}

class WaitForTheadJoinWorker : public Napi::AsyncWorker {
  public:
    WaitForTheadJoinWorker(Napi::Function& callback)
    : AsyncWorker(callback) {}

    ~WaitForTheadJoinWorker() {}
  // This code will be executed on the worker thread
  void Execute() override {
    threadJoinedMutex.lock();
  }

  void OnOK() override {
    threadJoinedMutex.unlock();
    Napi::HandleScope scope(Env());
    Callback().Call({Env().Null(), Napi::String::New(Env(), "OK")});
  }

  private:
    std::string echo;
};

Napi::Value MethodStopListening(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  if (!is_listening) {
    Napi::TypeError::New(env, "Kinect was not listening")
        .ThrowAsJavaScriptException();
    return env.Null();
  }
  // printf("[kinect_azure.cc] MethodStopListening\n");
  is_listening = false;
  Napi::Function cb = info[0].As<Napi::Function>();
  WaitForTheadJoinWorker* wk = new WaitForTheadJoinWorker(cb);
  wk->Queue();
  return info.Env().Undefined();
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  exports.Set(Napi::String::New(env, "init"),
    Napi::Function::New(env, MethodInit));
  exports.Set(Napi::String::New(env, "getInstalledCount"),
    Napi::Function::New(env, MethodGetInstalledCount));
  exports.Set(Napi::String::New(env, "open"),
    Napi::Function::New(env, MethodOpen));
  exports.Set(Napi::String::New(env, "close"),
    Napi::Function::New(env, MethodClose));
  exports.Set(Napi::String::New(env, "startCameras"),
    Napi::Function::New(env, MethodStartCameras));
  exports.Set(Napi::String::New(env, "stopCameras"),
    Napi::Function::New(env, MethodStopCameras));
  #ifdef KINECT_AZURE_ENABLE_BODY_TRACKING
  exports.Set(Napi::String::New(env, "createTracker"),
    Napi::Function::New(env, MethodCreateTracker));
  exports.Set(Napi::String::New(env, "destroyTracker"),
    Napi::Function::New(env, MethodDestroyTracker));
  #endif // KINECT_AZURE_ENABLE_BODY_TRACKING
  exports.Set(Napi::String::New(env, "startListening"),
    Napi::Function::New(env, MethodStartListening));
  exports.Set(Napi::String::New(env, "stopListening"),
    Napi::Function::New(env, MethodStopListening));
  return exports;
}

NODE_API_MODULE(hello, Init)