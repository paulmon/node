'use strict';
const common = require('../common');
const assert = require('assert');

const expected_keys = ['ares', 'http_parser', 'modules', 'node',
                       'uv', 'zlib'];
expected_keys.push(process.jsEngine);

if (common.hasCrypto) {
  expected_keys.push('openssl');
}

if (common.hasIntl) {
  expected_keys.push('icu');
  expected_keys.push('cldr');
  expected_keys.push('tz');
  expected_keys.push('unicode');
}

expected_keys.sort();
const actual_keys = Object.keys(process.versions).sort();

assert.deepStrictEqual(actual_keys, expected_keys);

const commonTemplate = /^\d+\.\d+\.\d+(?:-.*)?$/;

assert(commonTemplate.test(process.versions.ares));
assert(commonTemplate.test(process.versions.http_parser));
assert(commonTemplate.test(process.versions.node));
assert(commonTemplate.test(process.versions.uv));
assert(commonTemplate.test(process.versions.zlib));

assert(/^\d+\.\d+\.\d+(?:\.\d+)?(?: \(candidate\))?$/.test(process.versions[process.jsEngine || 'v8']));
assert(/^\d+$/.test(process.versions.modules));
