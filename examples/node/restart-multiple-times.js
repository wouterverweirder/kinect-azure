const KinectAzure = require('kinect-azure');
const kinect = new KinectAzure();

// execute as an async function, so we can use the await keyword
const main = async () => {
  if(kinect.open()) {
    console.log("Kinect Opened");
    kinect.startCameras({
      depth_mode: KinectAzure.K4A_DEPTH_MODE_NFOV_UNBINNED,
      color_resolution: KinectAzure.K4A_COLOR_RESOLUTION_720P
    });
    console.log("cameras started");
    kinect.createTracker();
    console.log("tracker created");
    kinect.startListening((data) => {
    });
    console.log("started listening");
    // stop kinect - use the await keyword to wait for the promise to resolve
    await kinect.stopListening();
    console.log("stopped listening");
    kinect.destroyTracker();
    console.log("destroyed tracking");
    kinect.stopCameras();
    console.log("stopped cameras");
    // start again
    kinect.startCameras({
      depth_mode: KinectAzure.K4A_DEPTH_MODE_NFOV_UNBINNED,
      color_resolution: KinectAzure.K4A_COLOR_RESOLUTION_720P
    });
    console.log("cameras started");
    kinect.createTracker();
    console.log("tracker created");
    kinect.startListening((data) => {
    });
    // you can also use the .then() method to wait for the stopListening() method to finish it's work
    kinect.stopListening().then(() => {
      console.log("stopped listening");
      kinect.destroyTracker();
      console.log("destroyed tracking");
      kinect.stopCameras();
      console.log("stopped cameras");
      // start again
      kinect.startCameras({
        depth_mode: KinectAzure.K4A_DEPTH_MODE_NFOV_UNBINNED,
        color_resolution: KinectAzure.K4A_COLOR_RESOLUTION_720P
      });
      console.log("cameras started");
      kinect.createTracker();
      console.log("tracker created");
      kinect.startListening((data) => {
      });
      // or use an old-school callback to wait for the stopListening() method to finish it's work
      kinect.stopListening((err, result) => {
        console.log("stopped listening");
        kinect.destroyTracker();
        console.log("destroyed tracking");
        kinect.stopCameras();
        console.log("stopped cameras");
        // start again
        kinect.startCameras({
          depth_mode: KinectAzure.K4A_DEPTH_MODE_NFOV_UNBINNED,
          color_resolution: KinectAzure.K4A_COLOR_RESOLUTION_720P
        });
        console.log("cameras started");
        kinect.createTracker();
        console.log("tracker created");
        kinect.startListening((data) => {
          console.log("num bodies: " + data.bodyFrame.numBodies);
        });
      });
    });
  }
};

main();
