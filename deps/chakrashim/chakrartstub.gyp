{
  'variables': {
    'target_arch%': 'ia32',
    'library%': 'static_library',   # build chakracore as static library or dll
    'component%': 'static_library', # link crt statically or dynamically
    'chakra_dir%': 'core',
    'icu_args%': '',
    'icu_include_path%': '',
    'linker_start_group%': '',
    'linker_end_group%': '',
    'chakra_libs_absolute%': '',

    # xplat (non-win32) only
    'chakra_config': '<(chakracore_build_config)',     # Debug, Release, Test

    'conditions': [
      ['target_arch=="ia32"', { 'Platform': 'x86' }],
      ['target_arch=="x64"', { 'Platform': 'x64' }],
      ['target_arch=="arm"', { 'Platform': 'arm',}],

      # xplat (non-win32) only
      ['chakracore_build_config=="Debug"', {
        'chakra_build_flags': [ '-d' ],
      }, 'chakracore_build_config=="Test"', {
        'chakra_build_flags': [ '-t' ],
      }, {
        'chakra_build_flags': [],
      }],
    ],
  },

  'targets': [
    {
      'target_name': 'chakrartstub',
      'toolsets': ['host'],
      'type': 'none',

      'variables': {
        'chakracore_header': [
          '<(chakra_dir)/lib/Common/ChakraCoreVersion.h',
          '<(chakra_dir)/lib/Jsrt/ChakraCore.h',
          '<(chakra_dir)/lib/Jsrt/ChakraCommon.h',
          '<(chakra_dir)/lib/Jsrt/ChakraCommonWindows.h',
          '<(chakra_dir)/lib/Jsrt/ChakraDebug.h',
        ],

        'chakracore_win_bin_dir':
          '<(chakra_dir)/build/vcbuild/bin/<(Platform)_<(chakracore_build_config)',
        'xplat_dir': '<(chakra_dir)/out/<(chakra_config)',
        'chakra_libs_absolute': '<(PRODUCT_DIR)/../../deps/chakrashim/<(xplat_dir)',

        'conditions': [
          ['OS=="win"', {
            'chakracore_input': '<(chakra_dir)/build/Chakra.Core.sln',
            'chakracore_binaries': [
              '<(chakracore_win_bin_dir)/chakracore.dll',
              '<(chakracore_win_bin_dir)/chakracore.pdb',
              '<(chakracore_win_bin_dir)/chakracore.lib',
            ]
          }],
        ],
      },

      'actions': [
        {
          'action_name': 'build_chakracore',
          'inputs': [
             '<(chakracore_input)',
          ],
          'outputs': [
             '<@(chakracore_binaries)',
          ],
          'conditions': [
            ['OS=="win"', {
              'action': [
                'msbuild',
                '/p:Platform=<(Platform)',
                '/p:Configuration=<(chakracore_build_config)',
                '/p:RuntimeLib=<(component)',
                '/p:AdditionalPreprocessorDefinitions=COMPILE_DISABLE_Simdjs=1',
                '/m',
                '<@(_inputs)',
              ],
            }],
          ],
        },
      ],

      'copies': [
        {
          'destination': 'include',
          'files': [ '<@(chakracore_header)' ],
        },
      ],

      'direct_dependent_settings': {
        'library_dirs': [ '<(PRODUCT_DIR)' ],
        'conditions': [
          ['OS=="win"', {
          }],
        ],
      },

    }, # end chakracore
  ],
}
