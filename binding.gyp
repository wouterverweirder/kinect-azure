{
  'targets': [
    {
      "target_name": "kinectAzure",
      "cflags!": [ "-fno-exceptions" ],
      "cflags_cc!": [ "-fno-exceptions" ],
      "sources": [ "src/kinect_azure.cc" ],
      "variables": {
        "body_tracking_sdk_dir": "C:/Program Files/Azure Kinect Body Tracking SDK"
      },
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")",
        "<(module_root_dir)/sdk/sensor-sdk-1.4.1/build/native/include",
        "<(body_tracking_sdk_dir)/sdk/include"
      ],
      "libraries": [
        "<(module_root_dir)/sdk/sensor-sdk-1.4.1/lib/native/amd64/release/k4a.lib",
        "<(module_root_dir)/sdk/sensor-sdk-1.4.1/lib/native/amd64/release/k4arecord.lib",
        "<(body_tracking_sdk_dir)/sdk/windows-desktop/amd64/release/lib/k4abt.lib"
      ],
      "copies": [
        {
          "destination": "<(module_root_dir)/build/Release",
          "files": [
            "<(module_root_dir)/sdk/sensor-sdk-1.4.1/lib/native/amd64/release/depthengine_2_0.dll",
            "<(module_root_dir)/sdk/sensor-sdk-1.4.1/lib/native/amd64/release/k4a.dll",
            "<(module_root_dir)/sdk/sensor-sdk-1.4.1/lib/native/amd64/release/k4arecord.dll"
          ]
        }
      ],
      'defines': [
        'NAPI_DISABLE_CPP_EXCEPTIONS',
        'NAPI_VERSION=<(napi_build_version)'
      ]
    },
    {
      "target_name": "action_after_build",
      "type": "none",
      "dependencies": [ "<(module_name)" ],
      "copies": [
        {
          "files": [ "<(PRODUCT_DIR)/<(module_name).node" ],
          "destination": "<(module_path)"
        }
      ]
    }
  ]
}
