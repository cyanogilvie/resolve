pandoc = find_program('pandoc', native: true, required: false)

if pandoc.found()
  include_filter = meson.project_source_root() / 'tools/include-code.lua'
  doc_source_dir = meson.project_source_root() / 'doc'

  doc_md = configure_file(
    input:          'resolve.md.in',
    output:         '@BASENAME@',
    configuration:  teaconf,
  )

  doc_examples = files()

  doc_targets = []

  doc_targets += custom_target('manpage',
    input:    doc_md,
    output:   meson.project_name() + '.n',
    command:  [pandoc, '--standalone', '--from', 'markdown', '--to', 'man',
      '@INPUT@', '--output', '@OUTPUT@'],
    install_dir:      get_option('mandir'),
    build_by_default: true,
  )
  doc_targets += custom_target('readme',
    input:    [doc_md, doc_examples],
    output:   'README.md',
    command:  [pandoc,
      '--lua-filter', include_filter, '--resource-path', doc_source_dir,
      '--from', 'markdown', '--to', 'gfm',
      '@INPUT0@', '--output', '@OUTPUT@'],
    install_dir:      meson.project_source_root(),
    install_tag:      'source',
    build_by_default: false,
  )

  alias_target('doc', doc_targets)
endif
