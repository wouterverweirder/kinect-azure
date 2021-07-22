const KinectAzure = require('.');
const kinect = new KinectAzure();

console.log("num installed kinects:" + kinect.getInstalledCount());

if(kinect.open()) {
  console.log("Kinect Opened");
  console.log("serial nr: " + kinect.getSerialNumber());
  kinect.startCameras({
    depth_mode: KinectAzure.K4A_DEPTH_MODE_NFOV_UNBINNED,
    color_resolution: KinectAzure.K4A_COLOR_RESOLUTION_720P
  });
  kinect.startListening((data) => {
    if (!(data.colorImageFrame && data.colorImageFrame.width > 0 && data.depthImageFrame && data.depthImageFrame.width > 0)) {
      return;
    }
    console.log('we received a frame');
    console.log(`color: [${data.colorImageFrame.deviceTimestamp}:${data.colorImageFrame.systemTimestamp}] ${data.colorImageFrame.width}x${data.colorImageFrame.height}`);
    console.log(`depth: [${data.depthImageFrame.deviceTimestamp}:${data.depthImageFrame.systemTimestamp}] ${data.depthImageFrame.width}x${data.depthImageFrame.height}`);
    console.log('stop listening');
    kinect.stopListening().then(() => {
      console.log("stopped listening");
      kinect.stopCameras();
      console.log("stopped cameras");
    });
  });
} else {
  console.log("no kinect connected");
}