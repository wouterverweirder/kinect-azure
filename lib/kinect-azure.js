const kinect = require('bindings')('kinectAzure');
const EventEmitter = require('events').EventEmitter;
const path = require('path');
const fs = require('fs-extra');
const settings = require('./settings.js')();

class KinectAzure {

  constructor() {
    const emitter = new EventEmitter();
    kinect.init(emitter.emit.bind(emitter));
  }

  open() { return kinect.open() }

  startCameras(options) { return kinect.startCameras(options) }
  createTracker() { return kinect.createTracker() }
  startListening(cb) { return kinect.startListening(cb)}

  stopListening() { return kinect.stopListening() }
  destroyTracker() { return kinect.destroyTracker() }
  stopCameras() { return kinect.stopCameras() }
  close() { return kinect.close() }

  getDepthModeRange(depthMode) {
    switch (depthMode)
    {
      case KinectAzure.K4A_DEPTH_MODE_NFOV_2X2BINNED:
        return { min: 500, max: 5800 };
      case KinectAzure.K4A_DEPTH_MODE_NFOV_UNBINNED:
        return { min: 500, max: 4000 };
      case KinectAzure.K4A_DEPTH_MODE_WFOV_2X2BINNED:
        return { min: 250, max: 3000 };
      case KinectAzure.K4A_DEPTH_MODE_WFOV_UNBINNED:
        return { min: 250, max: 2500 };
      case KinectAzure.K4A_DEPTH_MODE_PASSIVE_IR:
      default:
        throw new Error("Invalid depth mode!");
    }
  }
}

{
  KinectAzure.K4A_DEPTH_MODE_OFF = 0;             /**< Depth sensor will be turned off with this setting. */
  KinectAzure.K4A_DEPTH_MODE_NFOV_2X2BINNED = 1;  /**< Depth captured at 320x288. Passive IR is also captured at 320x288. */
  KinectAzure.K4A_DEPTH_MODE_NFOV_UNBINNED = 2;   /**< Depth captured at 640x576. Passive IR is also captured at 640x576. */
  KinectAzure.K4A_DEPTH_MODE_WFOV_2X2BINNED = 3;  /**< Depth captured at 512x512. Passive IR is also captured at 512x512. */
  KinectAzure.K4A_DEPTH_MODE_WFOV_UNBINNED = 4;   /**< Depth captured at 1024x1024. Passive IR is also captured at 1024x1024. */
  KinectAzure.K4A_DEPTH_MODE_PASSIVE_IR = 5;      /**< Passive IR only, captured at 1024x1024. */
}
{
  KinectAzure.K4A_COLOR_RESOLUTION_OFF = 0;     /**< Color camera will be turned off with this setting */
  KinectAzure.K4A_COLOR_RESOLUTION_720P = 1;    /**< 1280 * 720  16:9 */
  KinectAzure.K4A_COLOR_RESOLUTION_1080P = 2;   /**< 1920 * 1080 16:9 */
  KinectAzure.K4A_COLOR_RESOLUTION_1440P = 3;   /**< 2560 * 1440 16:9 */
  KinectAzure.K4A_COLOR_RESOLUTION_1536P = 4;   /**< 2048 * 1536 4:3  */
  KinectAzure.K4A_COLOR_RESOLUTION_2160P = 5;   /**< 3840 * 2160 16:9 */
  KinectAzure.K4A_COLOR_RESOLUTION_3072P = 6;   /**< 4096 * 3072 4:3  */
}
{
  KinectAzure.K4A_IMAGE_FORMAT_COLOR_MJPG = 0;
  KinectAzure.K4A_IMAGE_FORMAT_COLOR_NV12 = 1;
  KinectAzure.K4A_IMAGE_FORMAT_COLOR_YUY2 = 2;
  KinectAzure.K4A_IMAGE_FORMAT_COLOR_BGRA32 = 3;
  KinectAzure.K4A_IMAGE_FORMAT_DEPTH16 = 4;
  KinectAzure.K4A_IMAGE_FORMAT_IR16 = 5;
  KinectAzure.K4A_IMAGE_FORMAT_CUSTOM8 = 6;
  KinectAzure.K4A_IMAGE_FORMAT_CUSTOM16 = 6;
  KinectAzure.K4A_IMAGE_FORMAT_CUSTOM = 7;
}
{
  KinectAzure.K4A_FRAMES_PER_SECOND_5 = 0;     /**< 5 FPS */
  KinectAzure.K4A_FRAMES_PER_SECOND_15 = 1;    /**< 15 FPS */
  KinectAzure.K4A_FRAMES_PER_SECOND_30 = 2;    /**< 30 FPS */
}

module.exports = KinectAzure;