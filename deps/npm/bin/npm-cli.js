#!/usr/bin/env node
;(function () { // wrapper in case we're in module_context mode
  // windows: running "npm blah" in this folder will invoke WSH, not node.
  /*global WScript*/
  if (typeof WScript !== 'undefined') {
    WScript.echo(
      'npm does not work when run\n' +
        'with the Windows Scripting Host\n\n' +
        "'cd' to a different directory,\n" +
        "or type 'npm.cmd <args>',\n" +
        "or type 'node npm <args>'."
    )
    WScript.quit(1)
    return
  }

  if (!process.env.NPM_CONFIG_NODEDIR) {
    if (process.platform === 'win32') {
      process.env.NPM_CONFIG_NODEDIR = require('path').join(process.execPath, '..', 'sdk')
    } else {
      process.env.NPM_CONFIG_NODEDIR = require('path').join(process.execPath, '..', '..')
    }
  }

  process.title = 'npm'

  var unsupported = require('../lib/utils/unsupported.js')
  unsupported.checkForBrokenNode()

  var log = require('npmlog')
  log.pause() // will be unpaused when config is loaded.
  log.info('it worked if it ends with', 'ok')

  unsupported.checkForUnsupportedNode()

  if (!unsupported.checkVersion(process.version).unsupported) {
    var updater = require('update-notifier')
    var pkg = require('../package.json')
    updater({pkg: pkg}).notify({defer: true})
  }

  var path = require('path')
  var npm = require('../lib/npm.js')
  var npmconf = require('../lib/config/core.js')
  var errorHandler = require('../lib/utils/error-handler.js')
  var parseJSON = require('../lib/utils/parse-json.js')
  var fs = require('fs')
  var output = require('../lib/utils/output.js')

  var configDefs = npmconf.defs
  var shorthands = configDefs.shorthands
  var types = configDefs.types
  var nopt = require('nopt')

  // if npm is called as "npmg" or "npm_g", then
  // run in global mode.
  if (path.basename(process.argv[1]).slice(-1) === 'g') {
    process.argv.splice(1, 1, 'npm', '-g')
  }

  log.verbose('cli', process.argv)

  var conf = nopt(types, shorthands)
  npm.argv = conf.argv.remain
  if (npm.deref(npm.argv[0])) npm.command = npm.argv.shift()
  else conf.usage = true

  if (conf.version) {
    console.log(npm.version)
    return errorHandler.exit(0)
  }

  if (conf.versions) {
    npm.command = 'version'
    conf.usage = false
    npm.argv = []
  }

  log.info('using', 'npm@%s', npm.version)
  log.info('using', 'node@%s', process.version)

  process.on('uncaughtException', errorHandler)

  if (conf.usage && npm.command !== 'help') {
    npm.argv.unshift(npm.command)
    npm.command = 'help'
  }
  
  // check package.json to see if the package to install
  // needs uwp and target architecture options.
  if('install' == npm.command) {
    var localJsonPath = path.join(process.cwd(), '\package.json');
    if(fs.existsSync(localJsonPath)) {
      var j = parseJSON(fs.readFileSync(localJsonPath + ''));
      
      if(j.target_arch) {
      	process.argv.push('--target_arch=' + j.target_arch);
      }
      
      if(j.platform == 'uwp') {
      	process.argv.push('--node_uwp_dll');
      } 
    }
  }

  // now actually fire up npm and run the command.
  // this is how to use npm programmatically:
  conf._exit = true
  npm.load(conf, function (er) {
    if (er) return errorHandler(er)
    npm.commands[npm.command](npm.argv, function (err) {
      // https://www.youtube.com/watch?v=7nfPu8qTiQU
      if (!err && npm.config.get('ham-it-up') && !npm.config.get('json') && !npm.config.get('parseable') && npm.command !== 'completion') {
        output('\n 🎵 I Have the Honour to Be Your Obedient Servant,🎵 ~ npm 📜🖋\n')
      }
      errorHandler.apply(this, arguments)
    })
  })
})()
