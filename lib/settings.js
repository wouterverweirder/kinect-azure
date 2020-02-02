const path = require('path');

module.exports = () => {
  const KINECT_SENSOR_SDK_URL = 'https://www.nuget.org/api/v2/package/Microsoft.Azure.Kinect.Sensor/1.3.0';
  const KINECT_BODY_TRACKING_SDK_URL = 'https://www.nuget.org/api/v2/package/Microsoft.Azure.Kinect.BodyTracking/1.0.0';
  const KINECT_BODY_TRACKING_DEPENDENCIES_URL = 'https://www.nuget.org/api/v2/package/Microsoft.Azure.Kinect.BodyTracking.Dependencies/0.9.1';
  const KINECT_CUDNN_URL = 'https://www.nuget.org/api/v2/package/Microsoft.Azure.Kinect.BodyTracking.Dependencies.cuDNN/0.9.1';

  const TARGET_SDK_DIR = path.resolve(__dirname, '../sdk');
  const TARGET_KINECT_SENSOR_SDK_ZIP = path.resolve(TARGET_SDK_DIR, 'sensor-sdk.zip');
  const TARGET_KINECT_BODY_TRACKING_SDK_ZIP = path.resolve(TARGET_SDK_DIR, 'body-tracking-sdk.zip');
  const TARGET_KINECT_BODY_TRACKING_DEPENDENCIES_ZIP = path.resolve(TARGET_SDK_DIR, 'body-tracking-dependencies.zip');
  const TARGET_KINECT_CUDNN_ZIP = path.resolve(TARGET_SDK_DIR, 'cudnn.zip');

  const TARGET_KINECT_SENSOR_SDK_DIR = path.resolve(TARGET_SDK_DIR, 'sensor-sdk');
  const TARGET_KINECT_BODY_TRACKING_SDK_DIR = path.resolve(TARGET_SDK_DIR, 'body-tracking-sdk');
  const TARGET_KINECT_BODY_TRACKING_DEPENDENCIES_DIR = path.resolve(TARGET_SDK_DIR, 'body-tracking-dependencies');
  const TARGET_KINECT_CUDNN_DIR = path.resolve(TARGET_SDK_DIR, 'cudnn');


  const appRootDlls = {
    'onnxruntime.dll': path.resolve(TARGET_KINECT_BODY_TRACKING_SDK_DIR, 'lib/native/amd64/release/onnxruntime.dll'),
    'dnn_model_2_0.onnx': path.resolve(TARGET_KINECT_BODY_TRACKING_SDK_DIR, 'content/dnn_model_2_0.onnx'),
    'cublas64_100.dll': path.resolve(TARGET_KINECT_BODY_TRACKING_DEPENDENCIES_DIR, 'lib/native/amd64/release/cublas64_100.dll'),
    'cudart64_100.dll': path.resolve(TARGET_KINECT_BODY_TRACKING_DEPENDENCIES_DIR, 'lib/native/amd64/release/cudart64_100.dll'),
    'vcomp140.dll': path.resolve(TARGET_KINECT_BODY_TRACKING_DEPENDENCIES_DIR, 'lib/native/amd64/release/vcomp140.dll'),
    'cudnn64_7.dll': path.resolve(TARGET_KINECT_CUDNN_DIR, 'lib/native/amd64/release/cudnn64_7.dll'),
  };

  return {
    KINECT_SENSOR_SDK_URL,
    KINECT_BODY_TRACKING_SDK_URL,
    KINECT_BODY_TRACKING_DEPENDENCIES_URL,
    KINECT_CUDNN_URL,
    TARGET_SDK_DIR,
    TARGET_KINECT_SENSOR_SDK_ZIP,
    TARGET_KINECT_BODY_TRACKING_SDK_ZIP,
    TARGET_KINECT_BODY_TRACKING_DEPENDENCIES_ZIP,
    TARGET_KINECT_CUDNN_ZIP,
    TARGET_KINECT_SENSOR_SDK_DIR,
    TARGET_KINECT_BODY_TRACKING_SDK_DIR,
    TARGET_KINECT_BODY_TRACKING_DEPENDENCIES_DIR,
    TARGET_KINECT_CUDNN_DIR,
    appRootDlls
  }
};