{
  'targets': [
    {
      "target_name": "kinectAzure",
      "cflags!": [ "-fno-exceptions" ],
      "cflags_cc!": [ "-fno-exceptions" ],
      "sources": [ "src/kinect_azure.cc" ],
      "conditions": [
        ['OS=="win"', {
          "include_dirs": [
            "<!@(node -p \"require('node-addon-api').include\")",
            "<(module_root_dir)/sdk/sensor-sdk-1.4.1/build/native/include",
            "<(module_root_dir)/sdk/bt-sdk-1.1.0/build/native/include"
          ],
          "libraries": [
            "<(module_root_dir)/sdk/sensor-sdk-1.4.1/lib/native/amd64/release/k4a.lib",
            "<(module_root_dir)/sdk/sensor-sdk-1.4.1/lib/native/amd64/release/k4arecord.lib",
            "<(module_root_dir)/sdk/bt-sdk-1.1.0/lib/native/amd64/release/k4abt.lib"
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
          ]
        }, { # 'OS!="win"'
          "include_dirs": [
            "<!@(node -p \"require('node-addon-api').include\")",
            "<(module_root_dir)/sdk/sensor-sdk-1.4.1/build/linux/include",
            "<(module_root_dir)/sdk/bt-sdk-1.1.0/build/linux/include"
          ],
          "libraries": [
            "<(module_root_dir)/sdk/sensor-sdk-1.4.1/lib/linux/amd64/release/libk4a.so.1.4.1",
            "<(module_root_dir)/sdk/sensor-sdk-1.4.1/lib/linux/amd64/release/libk4arecord.so.1.4.1",
            "<(module_root_dir)/sdk/bt-sdk-1.1.0/lib/linux/amd64/release/libk4abt.so.1.1.0"
          ],
          "copies": [
            {
              "destination": "<(module_root_dir)/build/Release",
              "files": [
                "<(module_root_dir)/sdk/sensor-sdk-1.4.1/lib/linux/amd64/release/libdepthengine.so.2.0",
                "<(module_root_dir)/sdk/sensor-sdk-1.4.1/lib/linux/amd64/release/libk4a.so.1.4.1",
                "<(module_root_dir)/sdk/sensor-sdk-1.4.1/lib/linux/amd64/release/libk4arecord.so.1.4.1"
              ]
            }
          ]
        }]
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
