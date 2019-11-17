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

#ifdef KINECT_AZURE_ENABLE_BODY_TRACKING
k4abt_tracker_t g_tracker = NULL;
#endif // KINECT_AZURE_ENABLE_BODY_TRACKING

Napi::FunctionReference g_emit;
std::thread nativeThread;
std::mutex mtx;
Napi::ThreadSafeFunction tsfn;

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
  if (js_camera_fps.IsNumber()) {
    // printf("[kinect_azure.cc] deviceConfig set camera_fps\n");
    deviceConfig.camera_fps = (k4a_fps_t) js_camera_fps.As<Napi::Number>().Int32Value();
  }

  Napi::Value js_color_format = js_config.Get("color_format");
  if (js_color_format.IsNumber()) {
    // printf("[kinect_azure.cc] deviceConfig set color_format\n");
    deviceConfig.color_format = (k4a_image_format_t) js_color_format.As<Napi::Number>().Int32Value();
  }

  Napi::Value js_color_resolution = js_config.Get("color_resolution");
  if (js_color_resolution.IsNumber()) {
    // printf("[kinect_azure.cc] deviceConfig set color_resolution\n");
    deviceConfig.color_resolution = (k4a_color_resolution_t) js_color_resolution.As<Napi::Number>().Int32Value();
  }

  Napi::Value js_depth_mode = js_config.Get("depth_mode");
  if (js_depth_mode.IsNumber()) {
    // printf("[kinect_azure.cc] deviceConfig set depth_mode\n");
    deviceConfig.depth_mode = (k4a_depth_mode_t) js_depth_mode.As<Napi::Number>().Int32Value();
  }
  g_deviceConfig = deviceConfig;
  k4a_device_start_cameras(g_device, &g_deviceConfig);

  return Napi::Boolean::New(env, true);
}

Napi::Value MethodStopCameras(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  // printf("[kinect_azure.cc] MethodStopCameras\n");
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
  k4abt_tracker_shutdown(g_tracker);
  k4abt_tracker_destroy(g_tracker);
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
  
  tsfn = Napi::ThreadSafeFunction::New(
    env,
    info[0].As<Napi::Function>(),    // JavaScript function called asynchronously
    "Kinect Azure Listening",  // Name
    1,                         // 1 call in queue
    1,                         // Only one thread will use this initially
    []( Napi::Env ) {          // Finalizer used to clean threads up
      // printf("[kinect_azure.cc] Joining nativeThread\n");
      nativeThread.join();
      // printf("[kinect_azure.cc] nativeThread Joined\n");
    });

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

    JSFrame jsFrame;
    while(is_listening)
    {
      k4a_capture_t sensor_capture;
      k4a_wait_result_t get_capture_result = k4a_device_get_capture(g_device, &sensor_capture, 100);
      if (get_capture_result != K4A_WAIT_RESULT_SUCCEEDED)
      {
        // printf("[kinect_azure.cc] get_capture_result != K4A_WAIT_RESULT_SUCCEEDED\n");

        continue;
      }
      // Create new data
      mtx.lock();
      // printf("[kinect_azure.cc] jsFrame.reset\n");
      jsFrame.reset();
      if (g_deviceConfig.depth_mode != K4A_DEPTH_MODE_OFF)
      {
        // printf("[kinect_azure.cc] k4a_capture_get_depth_image\n");
        k4a_image_t depth_image = k4a_capture_get_depth_image(sensor_capture);
        if (depth_image != NULL)
        {
          jsFrame.depthImageFrame.image_length = k4a_image_get_size(depth_image);
          jsFrame.depthImageFrame.width = k4a_image_get_width_pixels(depth_image);
          jsFrame.depthImageFrame.height = k4a_image_get_height_pixels(depth_image);
          jsFrame.depthImageFrame.stride_bytes = k4a_image_get_stride_bytes(depth_image);
          jsFrame.depthImageFrame.image_data = new uint8_t[jsFrame.depthImageFrame.image_length];
          uint8_t* image_data = k4a_image_get_buffer(depth_image);
          memcpy(jsFrame.depthImageFrame.image_data, image_data, jsFrame.depthImageFrame.image_length);
          k4a_image_release(depth_image);
        }
      }
      if (g_deviceConfig.color_resolution != K4A_COLOR_RESOLUTION_OFF)
      {
        // printf("[kinect_azure.cc] k4a_capture_get_color_image\n");
        k4a_image_t color_image = k4a_capture_get_color_image(sensor_capture);
        if (color_image != NULL)
        {
          jsFrame.colorImageFrame.image_length = k4a_image_get_size(color_image);
          jsFrame.colorImageFrame.width = k4a_image_get_width_pixels(color_image);
          jsFrame.colorImageFrame.height = k4a_image_get_height_pixels(color_image);
          jsFrame.colorImageFrame.stride_bytes = k4a_image_get_stride_bytes(color_image);
          jsFrame.colorImageFrame.image_data = new uint8_t[jsFrame.colorImageFrame.image_length];
          uint8_t* image_data = k4a_image_get_buffer(color_image);
          memcpy(jsFrame.colorImageFrame.image_data, image_data, jsFrame.colorImageFrame.image_length);
          k4a_image_release(color_image);
        }
      }
      #ifdef KINECT_AZURE_ENABLE_BODY_TRACKING
      // printf("[kinect_azure.cc] check tracker\n");
      if (g_tracker != NULL)
      {
        // printf("[kinect_azure.cc] k4abt_tracker_enqueue_capture\n");
        k4a_wait_result_t queue_capture_result = k4abt_tracker_enqueue_capture(g_tracker, sensor_capture, K4A_WAIT_INFINITE);
        if (queue_capture_result == K4A_WAIT_RESULT_TIMEOUT)
        {
          // It should never hit timeout when K4A_WAIT_INFINITE is set.
          // printf("[kinect_azure.cc] Error! Add capture to tracker process queue timeout!\n");
          break;
        }
        else if (queue_capture_result == K4A_WAIT_RESULT_FAILED)
        {
          // printf("[kinect_azure.cc] Error! Add capture to tracker process queue failed!\n");
          break;
        }

        k4abt_frame_t body_frame = NULL;
        k4a_wait_result_t pop_frame_result = k4abt_tracker_pop_result(g_tracker, &body_frame, K4A_WAIT_INFINITE);
        if (pop_frame_result == K4A_WAIT_RESULT_SUCCEEDED)
        {
          // Successfully popped the body tracking result. Start your processing
          size_t num_bodies = k4abt_frame_get_num_bodies(body_frame);

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

              jsFrame.bodyFrame.bodies[i].skeleton.joints[j].confidence = joint.confidence_level;
            }
          }

          k4abt_frame_release(body_frame); // Remember to release the body frame once you finish using it
        }
        else if (pop_frame_result == K4A_WAIT_RESULT_TIMEOUT)
        {
          //  It should never hit timeout when K4A_WAIT_INFINITE is set.
          // printf("[kinect_azure.cc] Error! Pop body frame result timeout!\n");
          break;
        }
        else
        {
          // printf("[kinect_azure.cc] Pop body frame result failed!\n");
          break;
        }
      }
      #endif // KINECT_AZURE_ENABLE_BODY_TRACKING
      k4a_capture_release(sensor_capture);
     
      if (!is_listening)
      {
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
    is_listening = false;
    // printf("[kinect_azure.cc] reset jsFrame\n");
    jsFrame.reset();
    // printf("[kinect_azure.cc] Release thread-safe function!\n");
    // Release the thread-safe function
    tsfn.Release();
  } );

  return Napi::Boolean::New(env, true);
}

Napi::Value MethodStopListening(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  if (!is_listening) {
    Napi::TypeError::New(env, "Kinect was not listening")
        .ThrowAsJavaScriptException();
    return env.Null();
  }
  // printf("[kinect_azure.cc] MethodStopListening\n");
  is_listening = false;
  return Napi::Boolean::New(env, true);
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