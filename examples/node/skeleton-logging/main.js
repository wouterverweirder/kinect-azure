const KinectAzure = require('kinect-azure');
const kinect = new KinectAzure();

if(kinect.open()) {
  console.log("Kinect Opened");
  
  kinect.startCameras({
    depth_mode: KinectAzure.K4A_DEPTH_MODE_NFOV_UNBINNED,
    color_resolution: KinectAzure.K4A_COLOR_RESOLUTION_720P
  });
  kinect.createTracker();
  kinect.startListening((data) => {
    if (data.bodyFrame.numBodies === 0) {
      return;
    }
    console.log(data.bodyFrame.bodies[0].skeleton.joints);
  });
}