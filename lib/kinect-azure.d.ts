declare module "kinect-azure" {
	export default KinectAzure;

	export interface ImageFrame {
		imageData: Buffer;
		imageLength: number;
		width: number;
		height: number;
		strideBytes: number;
	}

	export interface TimeStamp {
		deviceTimeStamp: number;
		systemTimeStamp: number;
	}

	export interface Body {
		id: number;
		skeleton: Skeleton;
	}

	export interface Skeleton {
		joints: Joint[];
	}

	export interface Joint {
		index: number;

		cameraX: number;
		cameraY: number;
		cameraZ: number;

		orientationX: number;
		orientationY: number;
		orientationZ: number;
		orientationW: number;

		colorX: number;
		colorY: number;

		depthX: number;
		depthY: number;

		confidence: number;
	}

	export interface KinectData {
		imu: {
			temperature: number;
			accX: number;
			accY: number;
			accZ: number;
			accTimestamp: number;
			gyroX: number;
			gyroY: number;
			gyroZ: number;
			gyroTimestamp: number;
		};
		colorImageFrame: ImageFrame & TimeStamp;
		depthImageFrame: ImageFrame & TimeStamp;
		irImageFrame: ImageFrame & TimeStamp;
		depthToColorImageFrame: ImageFrame;
		colorToDepthImageFrame: ImageFrame;
		bodyFrame: {
			bodyIndexMapImageFrame: ImageFrame;
			bodyIndexMapToColorImageFrame: ImageFrame;
			numBodies: number;
			bodies: Body[];
		};
	}

	class KinectAzure {
		get time(): number;
		get duration(): number;

		constructor();

		close(): boolean;

		/**
		 * Creates a body tracker
		 * @param options The configuration for the body tracker
		 */
		createTracker(
			options: Partial<{
				sensor_orientation: K4ABT_SENSOR_ORIENTATION;
				processing_mode: K4ABT_TRACKER_PROCESSING_MODE;
				/** The id of the GPU to use */
				gpu_device_id: number;
				/** Specify the model file name and location used by the tracker. */
				model_path: string;
			}> = {}
		): boolean;

		destroyTracker(): number;

		getDepthModeRange(depthMode: K4A_DEPTH_MODE): { min: number; max: number };

		open(): boolean;

		/** open a playback mkv stream */
		openPlayback(path: String, playback_handle: Function): boolean;

		pause(): boolean;

		resume(): boolean;

		seek(time: number): boolean;

		setAutoExposure(): void;

		/**
		 * Backlight compensation setting
		 * @param {number} value 0 means backlight compensation is disabled, 1 means it is enabled
		 */
		setBacklightCompensation(value: number): void;

		/**
		 * Brightness setting
		 * @param {number} value valid range is 0 to 255, default value = 128
		 */
		setBrightness(value: number = 128): void;

		/**
		 * Set the Azure Kinect color sensor control value.
		 * @param {Object} options The configuration for the color control
		 * @param {number} options.command Color sensor control command. (eg KinectAzure. K4A_COLOR_CONTROL_EXPOSURE_TIME_ABSOLUTE, ...)
		 * @param {number} options.mode Color sensor control mode to set. This mode represents whether the command is in automatic or manual mode. (eg KinectAzure. K4A_COLOR_CONTROL_MODE_AUTO, KinectAzure.K4A_COLOR_CONTROL_MODE_MANUAL)
		 * @param {number} options.value Value to set the color sensor's control to. The value is only valid if mode is set to K4A_COLOR_CONTROL_MODE_MANUAL, and is otherwise ignored.
		 */
		setColorControl(options: {
			command: K4A_COLOR_CONTROL;
			mode: K4A_COLOR_CONTROL_MODE;
			value: number;
		}): void;

		/**
		 * Contrast setting
		 * @param {number} value value range is 0 to 10
		 */
		setContrast(value: number): void;

		setExposure(value: number): void;

		/**
		 * Gain setting
		 * @param {number} value value
		 */
		setGain(value: number): void;

		/**
		 * Powerline compensation setting
		 * @param {number} value 1 sets it to 50Hz, 2 sets it to 60 Hz
		 */
		setPowerlineFrequency(value: number): void;

		/**
		 * Saturation setting
		 * @param {number} value value range is 0 to 50
		 */
		setSaturation(value: number): void;

		/**
		 * Sharpness setting
		 * @param {number} value value
		 */
		setSharpness(value: number): void;

		/**
		 * Whitebalance setting
		 * @param {number} value unit is degrees Kelvin. Must be divisible by 10.
		 */
		setWhiteBalance(value: number): void;

		/**
		 * Starts the kinect cameras
		 *
		 * min and max depth are used to normalize depth data.
		 * @param {Object} options The configuration for the cameras
		 */
		startCameras(options?: CameraOptions): boolean;

		startListening(cb: Function): void;

		/**
		 * Starts the kinect cameras.
		 *
		 * min and max depth are used to normalize depth data
		 */
		startPlayback(
			options?: Partial<{
				/** The number of frames per second, if not set will default to recording fps */
				camera_fps: K4A_FRAMES_PER_SECOND;
				/** The color format */
				color_format: K4A_IMAGE_FORMAT;
				/** generate depth to color image */
				include_depth_to_color: bool;
				/** generate color to depth image */
				include_color_to_depth: bool;
				/** flip blue and red channels */
				flip_BGRA_to_RGBA: bool;
				/** apply the depth data to the alpha channel of the color image */
				apply_depth_to_alpha: bool;
				/** min depth distance in mm */
				min_depth: number;
				/** max depth distance in mm */
				max_depth: number;
			}>
		): boolean;

		stopCameras(): void;

		stopListening(cb?: (err, result) => void): Promise<void>;

		stopPlayback(): void;

		static K4ABT_BODY_INDEX_MAP_BACKGROUND: number;

		static K4ABT_JOINT_ANKLE_LEFT: number;

		static K4ABT_JOINT_ANKLE_RIGHT: number;

		static K4ABT_JOINT_CLAVICLE_LEFT: number;

		static K4ABT_JOINT_CLAVICLE_RIGHT: number;

		static K4ABT_JOINT_COUNT: number;

		static K4ABT_JOINT_EAR_LEFT: number;

		static K4ABT_JOINT_EAR_RIGHT: number;

		static K4ABT_JOINT_ELBOW_LEFT: number;

		static K4ABT_JOINT_ELBOW_RIGHT: number;

		static K4ABT_JOINT_EYE_LEFT: number;

		static K4ABT_JOINT_EYE_RIGHT: number;

		static K4ABT_JOINT_FOOT_LEFT: number;

		static K4ABT_JOINT_FOOT_RIGHT: number;

		static K4ABT_JOINT_HANDTIP_LEFT: number;

		static K4ABT_JOINT_HANDTIP_RIGHT: number;

		static K4ABT_JOINT_HAND_LEFT: number;

		static K4ABT_JOINT_HAND_RIGHT: number;

		static K4ABT_JOINT_HEAD: number;

		static K4ABT_JOINT_HIP_LEFT: number;

		static K4ABT_JOINT_HIP_RIGHT: number;

		static K4ABT_JOINT_KNEE_LEFT: number;

		static K4ABT_JOINT_KNEE_RIGHT: number;

		static K4ABT_JOINT_NECK: number;

		static K4ABT_JOINT_NOSE: number;

		static K4ABT_JOINT_PELVIS: number;

		static K4ABT_JOINT_SHOULDER_LEFT: number;

		static K4ABT_JOINT_SHOULDER_RIGHT: number;

		static K4ABT_JOINT_SPINE_CHEST: number;

		static K4ABT_JOINT_SPINE_NAVEL: number;

		static K4ABT_JOINT_THUMB_LEFT: number;

		static K4ABT_JOINT_THUMB_RIGHT: number;

		static K4ABT_JOINT_WRIST_LEFT: number;

		static K4ABT_JOINT_WRIST_RIGHT: number;

		static K4ABT_SENSOR_ORIENTATION_CLOCKWISE90: number;

		static K4ABT_SENSOR_ORIENTATION_COUNTERCLOCKWISE90: number;

		static K4ABT_SENSOR_ORIENTATION_DEFAULT: number;

		static K4ABT_SENSOR_ORIENTATION_FLIP180: number;

		static K4ABT_TRACKER_PROCESSING_MODE_CPU: number;

		static K4ABT_TRACKER_PROCESSING_MODE_GPU: number;

		static K4ABT_TRACKER_PROCESSING_MODE_GPU_CUDA: number;

		static K4ABT_TRACKER_PROCESSING_MODE_GPU_DIRECTML: number;

		static K4ABT_TRACKER_PROCESSING_MODE_GPU_TENSORRT: number;

		static K4A_COLOR_CONTROL_AUTO_EXPOSURE_PRIORITY: number;

		static K4A_COLOR_CONTROL_BACKLIGHT_COMPENSATION: number;

		static K4A_COLOR_CONTROL_BRIGHTNESS: number;

		static K4A_COLOR_CONTROL_CONTRAST: number;

		static K4A_COLOR_CONTROL_EXPOSURE_TIME_ABSOLUTE: number;

		static K4A_COLOR_CONTROL_GAIN: number;

		static K4A_COLOR_CONTROL_MODE_AUTO: number;

		static K4A_COLOR_CONTROL_MODE_MANUAL: number;

		static K4A_COLOR_CONTROL_POWERLINE_FREQUENCY: number;

		static K4A_COLOR_CONTROL_SATURATION: number;

		static K4A_COLOR_CONTROL_SHARPNESS: number;

		static K4A_COLOR_CONTROL_WHITEBALANCE: number;

		static K4A_COLOR_RESOLUTION_1080P: number;

		static K4A_COLOR_RESOLUTION_1440P: number;

		static K4A_COLOR_RESOLUTION_1536P: number;

		static K4A_COLOR_RESOLUTION_2160P: number;

		static K4A_COLOR_RESOLUTION_3072P: number;

		static K4A_COLOR_RESOLUTION_720P: number;

		static K4A_COLOR_RESOLUTION_OFF: number;

		static K4A_DEPTH_MODE_NFOV_2X2BINNED: number;

		static K4A_DEPTH_MODE_NFOV_UNBINNED: number;

		static K4A_DEPTH_MODE_OFF: number;

		static K4A_DEPTH_MODE_PASSIVE_IR: number;

		static K4A_DEPTH_MODE_WFOV_2X2BINNED: number;

		static K4A_DEPTH_MODE_WFOV_UNBINNED: number;

		static K4A_FRAMES_PER_SECOND_15: number;

		static K4A_FRAMES_PER_SECOND_30: number;

		static K4A_FRAMES_PER_SECOND_5: number;

		static K4A_IMAGE_FORMAT_COLOR_BGRA32: number;

		static K4A_IMAGE_FORMAT_COLOR_MJPG: number;

		static K4A_IMAGE_FORMAT_COLOR_NV12: number;

		static K4A_IMAGE_FORMAT_COLOR_YUY2: number;

		static K4A_IMAGE_FORMAT_CUSTOM: number;

		static K4A_IMAGE_FORMAT_CUSTOM16: number;

		static K4A_IMAGE_FORMAT_CUSTOM8: number;

		static K4A_IMAGE_FORMAT_DEPTH16: number;

		static K4A_IMAGE_FORMAT_IR16: number;
	}

	export interface CameraOptions
		extends Partial<{
			/** The number of frames per second */
			camera_fps: K4A_FRAMES_PER_SECOND;
			/** The color format */
			color_format: K4A_IMAGE_FORMAT;
			/** The color resolution */
			color_resolution: K4A_COLOR_RESOLUTION;
			/** The depth mode */
			depth_mode: K4A_DEPTH_MODE;
			/** The point cloud mode */
			point_cloud_mode: number;
			/** Only produce capture objects if they contain synchronized color and depth images */
			synchronized_image_only: boolean;
			/** generate depth to color image */
			include_depth_to_color: boolean;
			/** generate color to depth image */
			include_color_to_depth: boolean;
			/** include body index map */
			include_body_index_map: boolean;
			/** include imu data (temperature, accelerometer, gyroscope) */
			include_imu_sample: boolean;
			/** flip blue and red channels */
			flip_BGRA_to_RGBA: boolean;
			/** apply the depth data to the alpha channel of the color image */
			apply_depth_to_alpha: boolean;
			/** converts depth_to_color 16bit image to RGBA 32bit greyscale image */
			depth_to_greyscale: boolean;
			/** converts depth_to_color 16bit image to RGBA 32bit color image */
			depth_to_redblue: boolean;
			/** min depth distance in mm */
			min_depth: number;
			/** max depth distance in mm */
			max_depth: number;
		}> {}

	export const enum K4A_DEPTH_MODE {
		/**Depth sensor will be turned off with this setting. */
		OFF,
		/**Depth captured at 320x288. Passive IR is also captured at 320x288. */
		NFOV_2X2BINNED,
		/**Depth captured at 640x576. Passive IR is also captured at 640x576. */
		NFOV_UNBINNED,
		/**Depth captured at 512x512. Passive IR is also captured at 512x512. */
		WFOV_2X2BINNED,
		/**Depth captured at 1024x1024. Passive IR is also captured at 1024x1024. */
		WFOV_UNBINNED,
		/**Passive IR only, captured at 1024x1024. */
		PASSIVE_IR,
	}

	export const enum K4A_COLOR_RESOLUTION {
		/**Color camera will be turned off with this setting */
		RESOLUTION_OFF,
		/**1280 * 720  16:9 */
		_720P,
		/**1920 * 1080 16:9 */
		_1080P,
		/**2560 * 1440 16:9 */
		_1440P,
		/**2048 * 1536 4:3  */
		_1536P,
		/**3840 * 2160 16:9 */
		_2160P,
		/**4096 * 3072 4:3  */
		_3072P,
	}

	export const enum K4A_IMAGE_FORMAT {
		COLOR_MJPG = 0,
		COLOR_NV12 = 1,
		COLOR_YUY2 = 2,
		COLOR_BGRA32 = 3,
		DEPTH16 = 4,
		IR16 = 5,
		CUSTOM8 = 6,
		CUSTOM16 = 6,
		CUSTOM = 7,
	}

	export const enum K4A_FRAMES_PER_SECOND {
		_5 = 0,
		_15 = 1,
		_30 = 2,
	}

	export const enum K4ABT_SENSOR_ORIENTATION {
		/**< Mount the sensor at its default orientation */
		DEFAULT = 0,
		/**< Clockwisely rotate the sensor 90 degree */
		CLOCKWISE90,
		/**< Counter-clockwisely rotate the sensor 90 degrees */
		COUNTERCLOCKWISE90,
		/**< Mount the sensor upside-down */
		FLIP180,
	}

	export const enum K4ABT_JOINT {
		PELVIS,
		SPINE_NAVEL,
		SPINE_CHEST,
		NECK,
		CLAVICLE_LEFT,
		SHOULDER_LEFT,
		ELBOW_LEFT,
		WRIST_LEFT,
		HAND_LEFT,
		HANDTIP_LEFT,
		THUMB_LEFT,
		CLAVICLE_RIGHT,
		SHOULDER_RIGHT,
		ELBOW_RIGHT,
		WRIST_RIGHT,
		HAND_RIGHT,
		HANDTIP_RIGHT,
		THUMB_RIGHT,
		HIP_LEFT,
		KNEE_LEFT,
		ANKLE_LEFT,
		FOOT_LEFT,
		HIP_RIGHT,
		KNEE_RIGHT,
		ANKLE_RIGHT,
		FOOT_RIGHT,
		HEAD,
		NOSE,
		EYE_LEFT,
		EAR_LEFT,
		EYE_RIGHT,
		EAR_RIGHT,
		COUNT,
	}

	export const enum K4ABT_TRACKER_PROCESSING_MODE {
		/**< SDK will use the most appropriate GPU mode for the operating system to run the tracker */
		GPU = 0,
		/**< SDK will use CPU only mode to run the tracker */
		CPU = 1,

		// Currently this is ONNX DirectML EP for Windows and ONNX Cuda EP for Linux. ONNX TensorRT EP is experimental

		/**< SDK will use ONNX Cuda EP to run the tracker */
		GPU_CUDA = 2,
		/**< SDK will use ONNX TensorRT EP to run the tracker */
		GPU_TENSORRT = 3,
		/**< SDK will use ONNX DirectML EP to run the tracker (Windows only) */
		GPU_DIRECTML = 4,
	}

	export const enum K4A_COLOR_CONTROL {
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
		EXPOSURE_TIME_ABSOLUTE,

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
		AUTO_EXPOSURE_PRIORITY,

		/** Brightness setting.
		 *
		 * \details
		 * May only be set to ::K4A_COLOR_CONTROL_MODE_MANUAL.
		 *
		 * \details
		 * The valid range is 0 to 255. The default value is 128.
		 */
		BRIGHTNESS,

		/** Contrast setting.
		 *
		 * \details
		 * May only be set to ::K4A_COLOR_CONTROL_MODE_MANUAL.
		 */
		CONTRAST,

		/** Saturation setting.
		 *
		 * \details
		 * May only be set to ::K4A_COLOR_CONTROL_MODE_MANUAL.
		 */
		SATURATION,

		/** Sharpness setting.
		 *
		 * \details
		 * May only be set to ::K4A_COLOR_CONTROL_MODE_MANUAL.
		 */
		SHARPNESS,

		/** White balance setting.
		 *
		 * \details
		 * May be set to ::K4A_COLOR_CONTROL_MODE_AUTO or ::K4A_COLOR_CONTROL_MODE_MANUAL.
		 *
		 * \details
		 * The unit is degrees Kelvin. The setting must be set to a value evenly divisible by 10 degrees.
		 */
		WHITEBALANCE,

		/** Backlight compensation setting.
		 *
		 * \details
		 * May only be set to ::K4A_COLOR_CONTROL_MODE_MANUAL.
		 *
		 * \details
		 * Value of 0 means backlight compensation is disabled. Value of 1 means backlight compensation is enabled.
		 */
		BACKLIGHT_COMPENSATION,

		/** Gain setting.
		 *
		 * \details
		 * May only be set to ::K4A_COLOR_CONTROL_MODE_MANUAL.
		 */
		GAIN,

		/** Powerline frequency setting.
		 *
		 * \details
		 * May only be set to ::K4A_COLOR_CONTROL_MODE_MANUAL.
		 *
		 * \details
		 * Value of 1 sets the powerline compensation to 50 Hz. Value of 2 sets the powerline compensation to 60 Hz.
		 */
		POWERLINE_FREQUENCY,
	}

	export const enum K4A_COLOR_CONTROL_MODE {
		/**< set the associated k4a_color_control_command_t to auto*/
		AUTO = 0,
		/**< set the associated k4a_color_control_command_t to manual*/
		MANUAL = 1,
	}
}
