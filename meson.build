project(
	'tcptest',
	'c',
	version: '0.1',
	default_options: [
		'warning_level=3',
		'c_std=c11',
		'b_pie=true',
		'b_lto=true',
	],
)

add_global_arguments('-D_POSIX_C_SOURCE=200809L', language: 'c')

includes = include_directories('include')

sources = ['main.c', 'tcpconn.c']
real_sources = []
foreach src : sources
	real_sources += 'src' / src
endforeach

executable(
	'tcptest',
	real_sources,
	include_directories: includes,
	install: true,
)
