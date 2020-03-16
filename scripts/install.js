const fs = require('fs-extra');
const path = require('path');
const os = require('os');
const download = require('download');
const extract = require('extract-zip');
const settings = require('../lib/settings.js')();

const init = async () => {
  await fs.ensureDir(settings.TARGET_SDK_DIR);
  if (!(await fs.pathExists(settings.TARGET_KINECT_SENSOR_SDK_ZIP))) {
    console.log('downloading sensor sdk');
    await fs.writeFile(settings.TARGET_KINECT_SENSOR_SDK_ZIP, await download(settings.KINECT_SENSOR_SDK_URL));
  }
  if (!(await fs.pathExists(settings.TARGET_KINECT_BODY_TRACKING_SDK_ZIP))) {
    console.log('downloading body tracking sdk');
    await fs.writeFile(settings.TARGET_KINECT_BODY_TRACKING_SDK_ZIP, await download(settings.KINECT_BODY_TRACKING_SDK_URL));
  }
  if (!(await fs.pathExists(settings.TARGET_KINECT_BODY_TRACKING_DEPENDENCIES_ZIP))) {
    console.log('downloading body tracking dependencies');
    await fs.writeFile(settings.TARGET_KINECT_BODY_TRACKING_DEPENDENCIES_ZIP, await download(settings.KINECT_BODY_TRACKING_DEPENDENCIES_URL));
  }
  if (!(await fs.pathExists(settings.TARGET_KINECT_CUDNN_ZIP))) {
    console.log('downloading cudnn');
    await fs.writeFile(settings.TARGET_KINECT_CUDNN_ZIP, await download(settings.KINECT_CUDNN_URL));
  }
  if (!(await fs.pathExists(settings.TARGET_KINECT_SENSOR_SDK_DIR))) {
    console.log('extracting sensor sdk');
    await extractFile(settings.TARGET_KINECT_SENSOR_SDK_ZIP, settings.TARGET_KINECT_SENSOR_SDK_DIR);
  }
  if (!(await fs.pathExists(settings.TARGET_KINECT_BODY_TRACKING_SDK_DIR))) {
    console.log('extracting body tracking sdk');
    await extractFile(settings.TARGET_KINECT_BODY_TRACKING_SDK_ZIP, settings.TARGET_KINECT_BODY_TRACKING_SDK_DIR);
  }
  if (!(await fs.pathExists(settings.TARGET_KINECT_BODY_TRACKING_DEPENDENCIES_DIR))) {
    console.log('extracting body tracking dependencies');
    await extractFile(settings.TARGET_KINECT_BODY_TRACKING_DEPENDENCIES_ZIP, settings.TARGET_KINECT_BODY_TRACKING_DEPENDENCIES_DIR);
  }
  if (!(await fs.pathExists(settings.TARGET_KINECT_CUDNN_DIR))) {
    console.log('extracting cudnn');
    await extractFile(settings.TARGET_KINECT_CUDNN_ZIP, settings.TARGET_KINECT_CUDNN_DIR);
  }

  const appRoot = process.cwd().split('/node_modules')[0];
  const { appRootDlls } = settings;
  for (const dllName in appRootDlls) {
    if (appRootDlls.hasOwnProperty(dllName)) {
      const dllTargetPath = path.resolve(appRoot, dllName);
      const dllSourcePath = appRootDlls[dllName];
      if (!(await fs.pathExists(dllTargetPath))) {
        console.log(`copying ${dllSourcePath} to ${dllTargetPath}`);
        await fs.copyFile(dllSourcePath, dllTargetPath);
      }
    }
  }
};

const extractFile = (zipPath, dir) => {
  return new Promise((resolve, reject) => {
    extract(zipPath, { dir: dir }, err => {
      if (err) {
        return reject(err);
      }
      resolve();
    });
  });
};

if (os.type() === 'Windows_NT') {
  init();
} else {
  console.log('Platform not supported');
}
