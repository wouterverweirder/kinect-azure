#ifndef kinect_azure_structs_h
#define kinect_azure_structs_h

#ifdef KINECT_AZURE_ENABLE_BODY_TRACKING
typedef struct _JSJoint
{
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
	JSBody* bodies = NULL;
  int numBodies = 0;
	void reset() {
		if (bodies != NULL) {
			delete [] bodies;
			bodies = NULL;
		}
		numBodies = 0;
	}
} JSBodyFrame;
#endif // KINECT_AZURE_ENABLE_BODY_TRACKING

typedef struct _JSImageFrame
{
	uint8_t* image_data = NULL;
	size_t image_length = 0;
	int stride_bytes = 0;
	int width = 0;
	int height = 0;
	void reset() {
		if (image_data != NULL) {
			delete [] image_data;
			image_data = NULL;
		}
		image_length = 0;
		stride_bytes = 0;
		width = 0;
		height = 0;
	}
} JSImageFrame;

typedef struct _JSFrame
{
	#ifdef KINECT_AZURE_ENABLE_BODY_TRACKING
	JSBodyFrame bodyFrame;
	#endif // KINECT_AZURE_ENABLE_BODY_TRACKING
	JSImageFrame colorImageFrame;
	JSImageFrame depthImageFrame;
	JSImageFrame irImageFrame;
	JSImageFrame depthToColorImageFrame;
	JSImageFrame colorToDepthImageFrame;
	void reset() {
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
	bool include_depth_to_color = false;
	bool include_color_to_depth = false;
	int apply_depth_to_alpha = false;
	int min_depth = 100;
	int max_depth = 3000;
	
	void reset() {
		include_depth_to_color = false;
		include_color_to_depth = false;
		apply_depth_to_alpha = false;
		min_depth = 100;
		max_depth = 3000;
	}
} CustomDeviceConfig;

#endif
