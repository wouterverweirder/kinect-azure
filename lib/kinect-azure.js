const kinect = require("bindings")("kinectAzure");

class KinectAzure {
	constructor() {
		kinect.init();
	}
	/**
	 * open a playback mkv stream
	 * @param {string} path The path to the mkv recording
	 * @param {function} playback_handle callback
	 */
	openPlayback(path, playback_handle) {
		return kinect.openPlayback(path, playback_handle);
	}

	/**
	 * Starts the kinect cameras
	 * @param {Object} options The configuration for the cameras
	 * @param {number} options.camera_fps The number of frames per second, if not set will default to recording fps
	 * @param {number} options.color_format The color format
	 * @param {bool} options.include_depth_to_color generate depth to color image
	 * @param {bool} options.include_color_to_depth generate color to depth image
	 * @param {bool} options.flip_BGRA_to_RGBA flip blue and red channels
	 * @param {bool} options.apply_depth_to_alpha apply the depth data to the alpha channel of the color image
	 * min and max depth are used to normalize depth data
	 * @param {number} options.min_depth min depth distance in mm
	 * @param {number} options.max_depth max depth distance in mm
	 */
	startPlayback(options) {
		return kinect.startPlayback(options);
	}
	stopPlayback() {
		return kinect.stopPlayback();
	}
	resume() {
		return kinect.resume();
	}
	pause() {
		return kinect.pause();
	}
	seek(time) {
		return kinect.seek(time);
	}
	get time() {
		return kinect.time();
	}
	get duration() {
		return kinect.duration();
	}

	/**
	 * Opens the Kinect device at the specified index (defaults to 0 index)
	 * @param {number} index
	 */
	open(index) {
		return kinect.open(index);
	}

	/**
	 * Opens Kinect device with specified serial number
	 * @param {string} Kinect serial number
	 */
	serialOpen(serial) {
		return kinect.serialOpen(serial);
	}

	/**
	 * Get serial number of the open device.
	 * @return {string} - If device closed; returns 0
	 */
	getSerialNumber() {
		return kinect.getSerialNumber();
	}

	getInstalledCount() {
		return kinect.getInstalledCount();
	}

	/**
	 * Starts the kinect cameras
	 * @param {Object} options The configuration for the cameras
	 * @param {number} options.camera_fps The number of frames per second
	 * @param {number} options.color_format The color format
	 * @param {number} options.color_resolution The color resolution
	 * @param {number} options.depth_mode The depth mode
	 * @param {number} options.point_cloud_mode The point cloud mode
	 * @param {bool} options.synchronized_images_only Only produce capture objects if they contain synchronized color and depth images
	 * @param {bool} options.include_depth_to_color generate depth to color image
	 * @param {bool} options.include_color_to_depth generate color to depth image
	 * @param {bool} options.include_body_index_map include body index map
	 * @param {bool} options.include_imu_sample include imu data (temperature, accelerometer, gyroscope)
	 * @param {bool} options.flip_BGRA_to_RGBA flip blue and red channels
	 * @param {bool} options.apply_depth_to_alpha apply the depth data to the alpha channel of the color image
	 * @param {bool} options.depth_to_greyscale converts depth_to_color 16bit image to RGBA 32bit greyscale image
	 * @param {bool} options.depth_to_redblue converts depth_to_color 16bit image to RGBA 32bit color image
	 * min and max depth are used to normalize depth data when converting within cpp
	 * @param {number} options.min_depth min depth distance in mm
	 * @param {number} options.max_depth max depth distance in mm
	 */
	startCameras(options) {
		return kinect.startCameras(options);
	}
	/**
	 * Creates a body tracker
	 * @param {Object} options The configuration for the body tracker
	 * @param {number} options.sensor_orientation (KinectAzure.K4ABT_SENSOR_ORIENTATION_DEFAULT, KinectAzure.K4ABT_SENSOR_ORIENTATION_CLOCKWISE90, KinectAzure.K4ABT_SENSOR_ORIENTATION_COUNTERCLOCKWISE90 or KinectAzure.K4ABT_SENSOR_ORIENTATION_FLIP180)
	 * @param {number} options.processing_mode (KinectAzure.K4ABT_TRACKER_PROCESSING_MODE_GPU or KinectAzure.K4ABT_TRACKER_PROCESSING_MODE_CPU)
	 * @param {number} options.gpu_device_id The id of the GPU to use
	 */
	createTracker(options = {}) {
		return kinect.createTracker(options);
	}
	startListening(cb) {
		return kinect.startListening(cb);
	}

	stopListening(cb) {
		return new Promise((resolve, reject) => {
			kinect.stopListening((err, result) => {
				if (err) {
					reject(err);
				} else {
					resolve(result);
					if (cb) {
						cb(err, result);
					}
				}
			});
		});
	}
	destroyTracker() {
		return kinect.destroyTracker();
	}
	stopCameras() {
		return kinect.stopCameras();
	}
	close() {
		return kinect.close();
	}

	getDepthModeRange(depthMode) {
		switch (depthMode) {
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

	/**
	 * Set the Azure Kinect color sensor control value.
	 * @param {Object} options The configuration for the color control
	 * @param {number} options.command Color sensor control command. (eg KinectAzure. K4A_COLOR_CONTROL_EXPOSURE_TIME_ABSOLUTE, ...)
	 * @param {number} options.mode Color sensor control mode to set. This mode represents whether the command is in automatic or manual mode. (eg KinectAzure. K4A_COLOR_CONTROL_MODE_AUTO, KinectAzure.K4A_COLOR_CONTROL_MODE_MANUAL)
	 * @param {number} options.value Value to set the color sensor's control to. The value is only valid if mode is set to K4A_COLOR_CONTROL_MODE_MANUAL, and is otherwise ignored.
	 */
	setColorControl(options) {
		return kinect.setColorControl(options);
	}

	setAutoExposure() {
		this.setColorControl({
			command: KinectAzure.K4A_COLOR_CONTROL_EXPOSURE_TIME_ABSOLUTE,
			mode: KinectAzure.K4A_COLOR_CONTROL_MODE_AUTO,
			value: 0,
		});
	}

	setExposure(value) {
		this.setColorControl({
			command: KinectAzure.K4A_COLOR_CONTROL_EXPOSURE_TIME_ABSOLUTE,
			mode: KinectAzure.K4A_COLOR_CONTROL_MODE_MANUAL,
			value: value,
		});
	}

	/**
	 * Brightness setting
	 * @param {number} value valid range is 0 to 255, default value = 128
	 */
	setBrightness(value = 128) {
		this.setColorControl({
			command: KinectAzure.K4A_COLOR_CONTROL_BRIGHTNESS,
			mode: KinectAzure.K4A_COLOR_CONTROL_MODE_MANUAL,
			value: value,
		});
	}

	/**
	 * Contrast setting
	 * @param {number} value value range is 0 to 10
	 */
	setContrast(value) {
		this.setColorControl({
			command: KinectAzure.K4A_COLOR_CONTROL_CONTRAST,
			mode: KinectAzure.K4A_COLOR_CONTROL_MODE_MANUAL,
			value: value,
		});
	}

	/**
	 * Saturation setting
	 * @param {number} value value range is 0 to 50
	 */
	setSaturation(value) {
		this.setColorControl({
			command: KinectAzure.K4A_COLOR_CONTROL_SATURATION,
			mode: KinectAzure.K4A_COLOR_CONTROL_MODE_MANUAL,
			value: value,
		});
	}

	/**
	 * Sharpness setting
	 * @param {number} value value
	 */
	setSharpness(value) {
		this.setColorControl({
			command: KinectAzure.K4A_COLOR_CONTROL_SHARPNESS,
			mode: KinectAzure.K4A_COLOR_CONTROL_MODE_MANUAL,
			value: value,
		});
	}

	/**
	 * Whitebalance setting
	 * @param {number} value unit is degrees Kelvin. Must be divisible by 10.
	 */
	setWhiteBalance(value) {
		this.setColorControl({
			command: KinectAzure.K4A_COLOR_CONTROL_WHITEBALANCE,
			mode: KinectAzure.K4A_COLOR_CONTROL_MODE_MANUAL,
			value: value,
		});
	}

	/**
	 * Backlight compensation setting
	 * @param {number} value 0 means backlight compensation is disabled, 1 means it is enabled
	 */
	setBacklightCompensation(value) {
		this.setColorControl({
			command: KinectAzure.K4A_COLOR_CONTROL_BACKLIGHT_COMPENSATION,
			mode: KinectAzure.K4A_COLOR_CONTROL_MODE_MANUAL,
			value: value,
		});
	}

	/**
	 * Gain setting
	 * @param {number} value value
	 */
	setGain(value) {
		this.setColorControl({
			command: KinectAzure.K4A_COLOR_CONTROL_GAIN,
			mode: KinectAzure.K4A_COLOR_CONTROL_MODE_MANUAL,
			value: value,
		});
	}

	/**
	 * Powerline compensation setting
	 * @param {number} value 1 sets it to 50Hz, 2 sets it to 60 Hz
	 */
	setPowerlineFrequency(value) {
		this.setColorControl({
			command: KinectAzure.K4A_COLOR_CONTROL_POWERLINE_FREQUENCY,
			mode: KinectAzure.K4A_COLOR_CONTROL_MODE_MANUAL,
			value: value,
		});
	}
}

{
	KinectAzure.K4A_DEPTH_MODE_OFF = 0; /**< Depth sensor will be turned off with this setting. */
	KinectAzure.K4A_DEPTH_MODE_NFOV_2X2BINNED = 1; /**< Depth captured at 320x288. Passive IR is also captured at 320x288. */
	KinectAzure.K4A_DEPTH_MODE_NFOV_UNBINNED = 2; /**< Depth captured at 640x576. Passive IR is also captured at 640x576. */
	KinectAzure.K4A_DEPTH_MODE_WFOV_2X2BINNED = 3; /**< Depth captured at 512x512. Passive IR is also captured at 512x512. */
	KinectAzure.K4A_DEPTH_MODE_WFOV_UNBINNED = 4; /**< Depth captured at 1024x1024. Passive IR is also captured at 1024x1024. */
	KinectAzure.K4A_DEPTH_MODE_PASSIVE_IR = 5; /**< Passive IR only, captured at 1024x1024. */
}

{
	KinectAzure.K4A_COLOR_RESOLUTION_OFF = 0; /**< Color camera will be turned off with this setting */
	KinectAzure.K4A_COLOR_RESOLUTION_720P = 1; /**< 1280 * 720  16:9 */
	KinectAzure.K4A_COLOR_RESOLUTION_1080P = 2; /**< 1920 * 1080 16:9 */
	KinectAzure.K4A_COLOR_RESOLUTION_1440P = 3; /**< 2560 * 1440 16:9 */
	KinectAzure.K4A_COLOR_RESOLUTION_1536P = 4; /**< 2048 * 1536 4:3  */
	KinectAzure.K4A_COLOR_RESOLUTION_2160P = 5; /**< 3840 * 2160 16:9 */
	KinectAzure.K4A_COLOR_RESOLUTION_3072P = 6; /**< 4096 * 3072 4:3  */
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
	KinectAzure.K4A_FRAMES_PER_SECOND_5 = 0; /**< 5 FPS */
	KinectAzure.K4A_FRAMES_PER_SECOND_15 = 1; /**< 15 FPS */
	KinectAzure.K4A_FRAMES_PER_SECOND_30 = 2; /**< 30 FPS */
}

{
	KinectAzure.K4ABT_BODY_INDEX_MAP_BACKGROUND = 255;
}

{
	KinectAzure.K4ABT_JOINT_PELVIS = 0;
	KinectAzure.K4ABT_JOINT_SPINE_NAVEL = 1;
	KinectAzure.K4ABT_JOINT_SPINE_CHEST = 2;
	KinectAzure.K4ABT_JOINT_NECK = 3;
	KinectAzure.K4ABT_JOINT_CLAVICLE_LEFT = 4;
	KinectAzure.K4ABT_JOINT_SHOULDER_LEFT = 5;
	KinectAzure.K4ABT_JOINT_ELBOW_LEFT = 6;
	KinectAzure.K4ABT_JOINT_WRIST_LEFT = 7;
	KinectAzure.K4ABT_JOINT_HAND_LEFT = 8;
	KinectAzure.K4ABT_JOINT_HANDTIP_LEFT = 9;
	KinectAzure.K4ABT_JOINT_THUMB_LEFT = 10;
	KinectAzure.K4ABT_JOINT_CLAVICLE_RIGHT = 11;
	KinectAzure.K4ABT_JOINT_SHOULDER_RIGHT = 12;
	KinectAzure.K4ABT_JOINT_ELBOW_RIGHT = 13;
	KinectAzure.K4ABT_JOINT_WRIST_RIGHT = 14;
	KinectAzure.K4ABT_JOINT_HAND_RIGHT = 15;
	KinectAzure.K4ABT_JOINT_HANDTIP_RIGHT = 16;
	KinectAzure.K4ABT_JOINT_THUMB_RIGHT = 17;
	KinectAzure.K4ABT_JOINT_HIP_LEFT = 18;
	KinectAzure.K4ABT_JOINT_KNEE_LEFT = 19;
	KinectAzure.K4ABT_JOINT_ANKLE_LEFT = 20;
	KinectAzure.K4ABT_JOINT_FOOT_LEFT = 21;
	KinectAzure.K4ABT_JOINT_HIP_RIGHT = 22;
	KinectAzure.K4ABT_JOINT_KNEE_RIGHT = 23;
	KinectAzure.K4ABT_JOINT_ANKLE_RIGHT = 24;
	KinectAzure.K4ABT_JOINT_FOOT_RIGHT = 25;
	KinectAzure.K4ABT_JOINT_HEAD = 26;
	KinectAzure.K4ABT_JOINT_NOSE = 27;
	KinectAzure.K4ABT_JOINT_EYE_LEFT = 28;
	KinectAzure.K4ABT_JOINT_EAR_LEFT = 29;
	KinectAzure.K4ABT_JOINT_EYE_RIGHT = 30;
	KinectAzure.K4ABT_JOINT_EAR_RIGHT = 31;
	KinectAzure.K4ABT_JOINT_COUNT = 32;
}

{
	KinectAzure.K4ABT_SENSOR_ORIENTATION_DEFAULT = 0; /**< Mount the sensor at its default orientation */
	KinectAzure.K4ABT_SENSOR_ORIENTATION_CLOCKWISE90 = 1; /**< Clockwisely rotate the sensor 90 degree */
	KinectAzure.K4ABT_SENSOR_ORIENTATION_COUNTERCLOCKWISE90 = 2; /**< Counter-clockwisely rotate the sensor 90 degrees */
	KinectAzure.K4ABT_SENSOR_ORIENTATION_FLIP180 = 3; /**< Mount the sensor upside-down */
}

{
	KinectAzure.K4ABT_TRACKER_PROCESSING_MODE_GPU = 0; /**< SDK will use GPU mode to run the tracker */
	KinectAzure.K4ABT_TRACKER_PROCESSING_MODE_CPU = 1; /**< SDK will use CPU only mode to run the tracker */
}

{
	/** Exposure time setting.
	 *
	 * \details
	 * May be set to ::K4A_COLOR_CONTROL_MODE_AUTO or ::K4A_COLOR_CONTROL_MODE_MANUAL.
	 *
	 * \details
	 * The Azure Kinect supports a limited number of fixed expsore settings. When setting this, expect the exposure to
	 * be rounded up to the nearest setting. Exceptions are 1) The last value in the table is the upper limit, so a
	 * value larger than this will be overridden to the largest entry in the table. 2) The exposure time cannot be
	 * larger than the equivelent FPS. So expect 100ms exposure time to be reduced to 30ms or 33.33ms when the camera is
	 * started. The most recent copy of the table 'device_exposure_mapping' is in
	 * https://github.com/microsoft/Azure-Kinect-Sensor-SDK/blob/develop/src/color/color_priv.h
	 *
	 * \details
	 * Exposure time is measured in microseconds.
	 */
	KinectAzure.K4A_COLOR_CONTROL_EXPOSURE_TIME_ABSOLUTE = 0;

	/** Exposure or Framerate priority setting.
	 *
	 * \details
	 * May only be set to ::K4A_COLOR_CONTROL_MODE_MANUAL.
	 *
	 * \details
	 * Value of 0 means framerate priority. Value of 1 means exposure priority.
	 *
	 * \details
	 * Using exposure priority may impact the framerate of both the color and depth cameras.
	 *
	 * \details
	 * Deprecated starting in 1.2.0. Please discontinue usage, firmware does not support this.
	 */
	KinectAzure.K4A_COLOR_CONTROL_AUTO_EXPOSURE_PRIORITY = 1;

	/** Brightness setting.
	 *
	 * \details
	 * May only be set to ::K4A_COLOR_CONTROL_MODE_MANUAL.
	 *
	 * \details
	 * The valid range is 0 to 255. The default value is 128.
	 */
	KinectAzure.K4A_COLOR_CONTROL_BRIGHTNESS = 2;

	/** Contrast setting.
	 *
	 * \details
	 * May only be set to ::K4A_COLOR_CONTROL_MODE_MANUAL.
	 */
	KinectAzure.K4A_COLOR_CONTROL_CONTRAST = 3;

	/** Saturation setting.
	 *
	 * \details
	 * May only be set to ::K4A_COLOR_CONTROL_MODE_MANUAL.
	 */
	KinectAzure.K4A_COLOR_CONTROL_SATURATION = 4;

	/** Sharpness setting.
	 *
	 * \details
	 * May only be set to ::K4A_COLOR_CONTROL_MODE_MANUAL.
	 */
	KinectAzure.K4A_COLOR_CONTROL_SHARPNESS = 5;

	/** White balance setting.
	 *
	 * \details
	 * May be set to ::K4A_COLOR_CONTROL_MODE_AUTO or ::K4A_COLOR_CONTROL_MODE_MANUAL.
	 *
	 * \details
	 * The unit is degrees Kelvin. The setting must be set to a value evenly divisible by 10 degrees.
	 */
	KinectAzure.K4A_COLOR_CONTROL_WHITEBALANCE = 6;

	/** Backlight compensation setting.
	 *
	 * \details
	 * May only be set to ::K4A_COLOR_CONTROL_MODE_MANUAL.
	 *
	 * \details
	 * Value of 0 means backlight compensation is disabled. Value of 1 means backlight compensation is enabled.
	 */
	KinectAzure.K4A_COLOR_CONTROL_BACKLIGHT_COMPENSATION = 7;

	/** Gain setting.
	 *
	 * \details
	 * May only be set to ::K4A_COLOR_CONTROL_MODE_MANUAL.
	 */
	KinectAzure.K4A_COLOR_CONTROL_GAIN = 8;

	/** Powerline frequency setting.
	 *
	 * \details
	 * May only be set to ::K4A_COLOR_CONTROL_MODE_MANUAL.
	 *
	 * \details
	 * Value of 1 sets the powerline compensation to 50 Hz. Value of 2 sets the powerline compensation to 60 Hz.
	 */
	KinectAzure.K4A_COLOR_CONTROL_POWERLINE_FREQUENCY = 9;
}

{
	KinectAzure.K4A_COLOR_CONTROL_MODE_AUTO = 0; /**< set the associated k4a_color_control_command_t to auto*/
	KinectAzure.K4A_COLOR_CONTROL_MODE_MANUAL = 1; /**< set the associated k4a_color_control_command_t to manual*/
}

module.exports = KinectAzure;
