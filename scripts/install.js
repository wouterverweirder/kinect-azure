const fs = require('fs-extra');
const path = require('path');
const os = require('os');
const download = require('download');
const extract = require('extract-zip');
const settings = require('../lib/settings.js')();
const exec = require('child_process').exec;

const init = async (callback) => {
  await fs.ensureDir(settings.TARGET_SDK_DIR);
  if (!(await fs.pathExists(settings.TARGET_KINECT_SENSOR_SDK_ZIP))) {
    console.log('downloading sensor sdk');
    await fs.writeFile(settings.TARGET_KINECT_SENSOR_SDK_ZIP, await download(settings.KINECT_SENSOR_SDK_URL));
  }
  if (!(await fs.pathExists(settings.TARGET_KINECT_SENSOR_SDK_DIR))) {
    console.log('extracting sensor sdk');
    await extractFile(settings.TARGET_KINECT_SENSOR_SDK_ZIP, settings.TARGET_KINECT_SENSOR_SDK_DIR);
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

init();