{
  'targets': [
    {
      'target_name': 'logger',
      'type': 'static_library',
      'include_dirs': [ 
        './include',
        '../../src',
        '../chakrashim/include',
      ],
      'sources': [ 
        'include/ILogger.h',
        'include/logger_wrap.h',
        'include/node_logger.h',
        'src/logger_wrap.cpp', 
        'src/node_logger.cpp', 
      ],
      'conditions': [
        ['node_engine=="chakra"', {
          'defines': [
            'BUILDING_CHAKRASHIM=1',  # other deps don't import chakrashim exports
          ],
        }],
      ],
    },
  ]
}
