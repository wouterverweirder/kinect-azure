const { ipcRenderer } = require('electron');
ipcRenderer.on('close-kinect', (event, message) => {
  if (window.kinect) {
    // properly close the kinect and send a message back to the renderer
    console.log('close the kinect');
    window.kinect.stopListening().then(() => {
      console.log('kinect closed');
      window.kinect.destroyTracker();
      window.kinect.stopCameras();
      window.kinect.close();
      event.sender.send('kinect closed');
    });
  } else {
    event.sender.send('kinect closed');
  }
});