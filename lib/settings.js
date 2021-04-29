const path = require('path');

module.exports = () => {
  const KINECT_SENSOR_SDK_URL = 'https://www.nuget.org/api/v2/package/Microsoft.Azure.Kinect.Sensor/1.4.1';
  
  const TARGET_SDK_DIR = path.resolve(__dirname, '../sdk');
  const TARGET_KINECT_SENSOR_SDK_ZIP = path.resolve(TARGET_SDK_DIR, 'sensor-sdk-1.4.1.zip');
  
  const TARGET_KINECT_SENSOR_SDK_DIR = path.resolve(TARGET_SDK_DIR, 'sensor-sdk-1.4.1');

  return {
    KINECT_SENSOR_SDK_URL,
    TARGET_SDK_DIR,
    TARGET_KINECT_SENSOR_SDK_ZIP,
    TARGET_KINECT_SENSOR_SDK_DIR
  }
};