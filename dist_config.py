import os

package = 'libt3widget'
excludesrc = '/(Makefile|TODO.*|SciTE.*|run\.sh|test\.c|signals.h)$'
auxsources= [ 'src/widget_api.h' ]
extrabuilddirs = [ 'doc' ]
auxfiles = [ 'doc/API' ]

versioninfo = '1:0:0'


def get_replacements(mkdist):
	return [
		{
			'tag': '<VERSION>',
			'replacement': mkdist.version
		},
		{
			'tag': '^#define T3_WIDGET_VERSION .*',
			'replacement': '#define T3_WIDGET_VERSION ' + mkdist.get_version_bin(),
			'files': [ 'src/main.h' ],
			'regex': True
		},
		{
			'tag': '<OBJECTS>',
			'replacement': " ".join(mkdist.sources_to_objects(mkdist.exclude_by_regex(mkdist.sources, '^src/x11.cc'), '\.cc$', '.lo')),
			'files': [ 'Makefile.in' ]
		},
		{
			'tag': '<VERSIONINFO>',
			'replacement': versioninfo,
			'files': [ 'Makefile.in' ]
		},
		{
			'tag': '<LIBVERSION>',
			'replacement': versioninfo.split(':', 2)[0],
			'files': [ 'Makefile.in', 'config.pkg' ]
		}
	]

def finalize(mkdist):
	os.symlink('.', os.path.join(mkdist.topdir, 'src', 't3widget'))
