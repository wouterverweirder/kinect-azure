# Azure Kinect Library for Node / Electron

[![Donate](https://img.shields.io/badge/Donate-PayPal-green.svg)](https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=NUZP3U3QZEQV2&currency_code=EUR&source=url)

This library enables you to use the Azure Kinect in your nodejs or electron apps.

![screenshot of skeleton 2d demo](examples/screenshots/screenshot-demos-body-tracking-2d.png)

Features:

- get rgb camera feed
- get depth feed
- get ir feed
- transform between color and depth feed
- point cloud (greyscale and colored)
- get skeleton joints (2d and 3d)
- body index / user masking
- get temperature / accelerometer / gyroscope data (IMU data)

## Installation

Make sure you have the Azure Kinect drivers and the [Azure Kinect Body SDK](https://docs.microsoft.com/en-us/azure/kinect-dk/body-sdk-download) installed.

The Body Tracking SDK relies on quite a few dll & onnx files, which you can find in the `Azure Kinect Body SDK\tools` directory. Either add that folder (eg C:\Program Files\Azure Kinect Body Tracking SDK\tools) to your PATH or copy the dll and onnx files into the root of your project.

Just npm install like you would do with any regular module. 

```
$ npm install kinect-azure
```

There are no precompiled binaries yet, so you need to have [node-gyp installed on your system](https://github.com/nodejs/node-gyp).

### Potential issues

You might run into the error below when trying to install this module:

> Error: spawn C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\MSBuild\15.0\Bin\MSBuild.exe ENOENT

To fix this, make sure to install the latest version of node-gyp globally:

```
$ npm install -g node-gyp
```

## Examples

There are nodejs and electron examples in the examples/ folder of this repo. To run them, execute npm install and npm start:

```
$ cd examples/electron
$ npm install
$ npm start
```

The electron examples have the javascript code inside the html files. You can find these html files in [examples/electron/renderer/demos](examples/electron/renderer/demos).

## Donate

Like this library? Always welcome to buy me a beer 🍺

[![Donate](https://img.shields.io/badge/Donate-PayPal-green.svg)](https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=NUZP3U3QZEQV2&currency_code=EUR&source=url)