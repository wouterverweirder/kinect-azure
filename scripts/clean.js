const fs = require('fs-extra');
const path = require('path');
const settings = require('../lib/settings.js')();

const init = async () => {
  await fs.remove(settings.TARGET_SDK_DIR);

  const appRoot = process.cwd().split('/node_modules')[0];
  const { appRootDlls } = settings;
  for (const dllName in appRootDlls) {
    if (appRootDlls.hasOwnProperty(dllName)) {
      const dllTargetPath = path.resolve(appRoot, dllName);
      await fs.remove(dllTargetPath);
    }
  }
};

init();