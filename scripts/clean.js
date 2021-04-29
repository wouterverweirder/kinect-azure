const fs = require('fs-extra');
const path = require('path');
const settings = require('../lib/settings.js')();

const init = async () => {
  await fs.remove(settings.TARGET_SDK_DIR);
};

init();