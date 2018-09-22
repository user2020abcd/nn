# This file is part of Telegram Desktop,
# the official desktop application for the Telegram messaging service.
#
# For license and copyright information please follow this link:
# https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL

{
  'conditions': [
    [ 'build_linux', {
      'variables': {
        'linux_common_flags': [
          '-pipe',
          '-Wall',
          '-Werror',
          '-W',
          '-fPIC',
          '-Wno-unused-variable',
          '-Wno-unused-parameter',
          '-Wno-unused-function',
          '-Wno-switch',
          '-Wno-comment',
          '-Wno-unused-but-set-variable',
          '-Wno-missing-field-initializers',
          '-Wno-sign-compare',
        ],
      },
      'conditions': [
        [ '"<!(uname -p)" == "x86_64"', {
          'defines': [
            'Q_OS_LINUX64',
          ],
          'conditions': [
            [ '"<(official_build_target)" != "" and "<(official_build_target)" != "linux"', {
              'sources': [ '__Wrong_Official_Build_Target_<(official_build_target)_' ],
            }],
          ],
        }, {
          'defines': [
            'Q_OS_LINUX32',
          ],
          'conditions': [
            [ '"<(official_build_target)" != "" and "<(official_build_target)" != "linux32"', {
              'sources': [ '__Wrong_Official_Build_Target_<(official_build_target)_' ],
            }],
          ],
        }],
      ],
      'defines': [
        '_REENTRANT',
        'QT_STATICPLUGIN',
        'QT_PLUGIN',
      ],
      'cflags_c': [
        '<@(linux_common_flags)',
        '-std=gnu11',
      ],
      'cflags_cc': [
        '<@(linux_common_flags)',
        '-std=c++1z',
        '-Wno-register',
      ],
      'configurations': {
        'Debug': {
        },
      },
    }],
  ],
}
