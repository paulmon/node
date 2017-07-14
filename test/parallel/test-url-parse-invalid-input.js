'use strict';
const common = require('../common');
const assert = require('assert');
const url = require('url');

// https://github.com/joyent/node/issues/568
[
  undefined,
  null,
  true,
  false,
  0.0,
  0,
  [],
  {},
  () => {},
  Symbol('foo')
].forEach((val) => {
  assert.throws(() => { url.parse(val); },
                /^TypeError: Parameter "url" must be a string, not (undefined|boolean|number|object|function|symbol)$/);
});

const engineSpecificMalformedUrlError =
    common.engineSpecificMessage({
      v8: /^URIError: URI malformed$/,
      chakracore: /^URIError: The URI to be decoded is not a valid encoding$/
    });

assert.throws(() => { url.parse('http://%E0%A4%A@fail'); }, engineSpecificMalformedUrlError);
