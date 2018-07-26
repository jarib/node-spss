{
    'targets': [
        {
            'target_name': 'spss',
            'sources': [
                'src/spss.cc'
            ],

            'include_dirs': [
                'spssio/include'
            ],

            'link_settings': {
                'libraries': [
                    '-lspssdio'
                ]
            },

            'conditions': [
                ['OS=="mac"', {
                    'link_settings': {
                        'libraries': [
                            '-L<(module_root_dir)/spssio/macos',
                            '-Wl,-rpath,<(module_root_dir)/spssio/macos',
                        ]
                    }
                }],
                ['OS=="linux"', {
                    'link_settings': {
                        'libraries': [
                            '-L<(module_root_dir)/spssio/lin64',
                            '-Wl,-rpath,<(module_root_dir)/spssio/lin64',
                        ]
                    }
                }]
            ],
        }
    ]
}
