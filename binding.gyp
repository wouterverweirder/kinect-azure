{
  'targets': [
    {
      "target_name": "kinectAzure",
      "cflags!": [ "-fno-exceptions" ],
      "cflags_cc!": [ "-fno-exceptions" ],
      "sources": [ "src/kinect_azure.cc" ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")",
        "<(module_root_dir)/sdk/sensor-sdk/build/native/include",
        "<(module_root_dir)/sdk/body-tracking-sdk/build/native/include"
      ],
      "libraries": [
        "<(module_root_dir)/sdk/sensor-sdk/lib/native/amd64/release/k4a.lib",
        "<(module_root_dir)/sdk/sensor-sdk/lib/native/amd64/release/k4arecord.lib",
        "<(module_root_dir)/sdk/body-tracking-sdk/lib/native/amd64/release/k4abt.lib"
      ],
      "copies": [
        {
          "destination": "<(module_root_dir)/build/Release",
          "files": [
            "<(module_root_dir)/sdk/sensor-sdk/lib/native/amd64/release/depthengine_2_0.dll",
            "<(module_root_dir)/sdk/sensor-sdk/lib/native/amd64/release/k4a.dll",
            "<(module_root_dir)/sdk/sensor-sdk/lib/native/amd64/release/k4arecord.dll",
            "<(module_root_dir)/sdk/body-tracking-sdk/lib/native/amd64/release/k4abt.dll"
          ]
        }
      ],
      'defines': [ 'NAPI_DISABLE_CPP_EXCEPTIONS' ],
    }
  ]
}
