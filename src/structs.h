#ifndef kinect_azure_structs_h
#define kinect_azure_structs_h

typedef struct _JSImageFrame
{
	uint8_t* image_data = NULL;
	size_t image_length = 0;
	int stride_bytes = 0;
	int width = 0;
	int height = 0;
	uint64_t device_timestamp = NULL;
	uint64_t system_timestamp = NULL;
	void reset() {
		if (image_data != NULL) {
			delete [] image_data;
			image_data = NULL;
		}
		image_length = 0;
		stride_bytes = 0;
		width = 0;
		height = 0;
		device_timestamp = NULL;
		system_timestamp = NULL;
	}
} JSImageFrame;

#ifdef KINECT_AZURE_ENABLE_BODY_TRACKING
typedef struct _JSJoint
{
	int index = 0;
	
	float cameraX = 0;
	float cameraY = 0;
	float cameraZ = 0;
	//
	float orientationX = 0;
	float orientationY = 0;
	float orientationZ = 0;
	float orientationW = 0;
	//
	float colorX = 0;
	float colorY = 0;
	//
	float depthX = 0;
	float depthY = 0;
	//
	int confidence = 0;
} JSJoint;

typedef struct _JSSkeleton
{
	JSJoint joints[K4ABT_JOINT_COUNT];
} JSSkeleton;

typedef struct _JSBody
{
	int id = 0;
  JSSkeleton skeleton;
} JSBody;

typedef struct _JSBodyFrame
{
	JSImageFrame bodyIndexMapImageFrame;
	JSImageFrame bodyIndexMapToColorImageFrame;
	JSBody* bodies = NULL;
  int numBodies = 0;
	void reset() {
		bodyIndexMapImageFrame.reset();
		bodyIndexMapToColorImageFrame.reset();
		if (bodies != NULL) {
			delete [] bodies;
			bodies = NULL;
		}
		numBodies = 0;
	}
} JSBodyFrame;
#endif // KINECT_AZURE_ENABLE_BODY_TRACKING

typedef struct _JSIMUSample
{
	float temperature = 0.0;
	float accX = 0.0;
	float accY = 0.0;
	float accZ = 0.0;
	uint64_t accTimestamp = 0;
	float gyroX = 0.0;
	float gyroY = 0.0;
	float gyroZ = 0.0;
	uint64_t gyroTimestamp = 0;
	void reset() {
		temperature = 0;
		accX = 0.0;
		accY = 0.0;
		accZ = 0.0;
		accTimestamp = 0;
		gyroX = 0.0;
		gyroY = 0.0;
		gyroZ = 0.0;
		gyroTimestamp = 0;
	}
} JSIMUSample;

typedef struct _JSFrame
{
	JSIMUSample imuSample;
	#ifdef KINECT_AZURE_ENABLE_BODY_TRACKING
	JSBodyFrame bodyFrame;
	#endif // KINECT_AZURE_ENABLE_BODY_TRACKING
	JSImageFrame colorImageFrame;
	JSImageFrame depthImageFrame;
	JSImageFrame irImageFrame;
	JSImageFrame depthToColorImageFrame;
	JSImageFrame colorToDepthImageFrame;
	void reset() {
		imuSample.reset();
		colorImageFrame.reset();
		depthImageFrame.reset();
		irImageFrame.reset();
		depthToColorImageFrame.reset();
		colorToDepthImageFrame.reset();
	}
	void resetBodyFrame() {
		#ifdef KINECT_AZURE_ENABLE_BODY_TRACKING
		bodyFrame.reset();
		#endif // KINECT_AZURE_ENABLE_BODY_TRACKING
	}
} JSFrame;

typedef struct _CustomDeviceConfig
{
	bool include_imu_sample = false;
	bool include_depth_to_color = false;
	bool include_color_to_depth = false;
	bool include_body_index_map = false;
	bool flip_BGRA_to_RGBA = false;
	bool apply_depth_to_alpha = false;
	bool depth_to_greyscale = false;
	bool depth_to_redblue = false;
	int min_depth = 100;
	int max_depth = 3000;
	
	void reset() {
		include_imu_sample = false;
		include_depth_to_color = false;
		include_color_to_depth = false;
		include_body_index_map = false;
		flip_BGRA_to_RGBA = false;
		apply_depth_to_alpha = false;
		depth_to_greyscale = false;
		depth_to_redblue = false;
		min_depth = 100;
		max_depth = 3000;
	}
} CustomDeviceConfig;

#endif

typedef struct _PlaybackProps
{
	uint64_t playback_fps = 15;
	uint64_t recording_length = 0;
	
	void reset() {
		playback_fps = 15;
		recording_length = 0;
	}
} PlaybackProps;