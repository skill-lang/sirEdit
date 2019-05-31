project('sirEdit', ['cpp'],
        version: '1.0.0',
        meson_version: '>= 0.51.0'
)

cmake = import('cmake')
cpp = meson.get_compiler('cpp')
openmp = cpp.find_library('gomp')
cppCommon = cmake.subproject('cppCommon')
skillLib = cppCommon.dependency('skill_cpp_common')

thread_dep = dependency('threads')
gtkmm = dependency('gtkmm-3.0', version: '>= 3.18')

# Sources of sir
src_sir = [
	'sir/CommentFieldDeclarations.cpp',
	'sir/CommentTagFieldDeclarations.cpp',
	'sir/CustomFieldOptionFieldDeclarations.cpp',
	'sir/FieldLikeFieldDeclarations.cpp',
	'sir/HintFieldDeclarations.cpp',
	'sir/IdentifierFieldDeclarations.cpp',
	'sir/RestrictionFieldDeclarations.cpp',
	'sir/ToolFieldDeclarations.cpp',
	'sir/TypeFieldDeclarations.cpp',
	'sir/File.cpp',
	'sir/CommentPools.cpp',
	'sir/CommentTagPools.cpp',
	'sir/CustomFieldOptionPools.cpp',
	'sir/FieldLikePools.cpp',
	'sir/HintPools.cpp',
	'sir/IdentifierPools.cpp',
	'sir/RestrictionPools.cpp',
	'sir/ToolPools.cpp',
	'sir/TypePools.cpp',
	'sir/TypesOfComment.cpp',
	'sir/TypesOfCommentTag.cpp',
	'sir/TypesOfCustomFieldOption.cpp',
	'sir/TypesOfFieldLike.cpp',
	'sir/TypesOfHint.cpp',
	'sir/TypesOfIdentifier.cpp',
	'sir/TypesOfRestriction.cpp',
	'sir/TypesOfTool.cpp',
	'sir/TypesOfType.cpp'
]
inc_sir = include_directories('sir')

# SirEdit files
src_siredit = [
	'src/main.cpp',
	'src/data/serializeSIR.cpp',
	'src/gui/loadFile.cpp',
	'src/gui/mainWindow.cpp',
	'src/gui/overview.cpp',
	'src/gui/startupWindow.cpp',
	'src/gui/toolsEditor.cpp'
]
inc_siredit = include_directories('include')

# Resource encoding
resource_encoder = executable('resource_encoder', ['resource_encoder.cpp'], native: true)
src_resources = [
	custom_target('startupWindow.glade',
	              input: ['data/gui/startupWindow.glade'],
	              output: ['tmpres_startupWindow_glade.cpp'],
	              command: [resource_encoder, '@INPUT0@', '@OUTPUT0@', 'sirEdit_startupWindow_glade']),
	custom_target('mainWindow.glade',
	              input: ['data/gui/mainWindow.glade'],
	              output: ['tmpres_mainWindow_glade.cpp'],
	              command: [resource_encoder, '@INPUT0@', '@OUTPUT0@', 'sirEdit_mainWindow_glade'])
]

executable('sirEdit', src_sir + src_siredit + src_resources, cpp_args: '-std=c++17', dependencies: [gtkmm, skillLib, openmp, thread_dep], include_directories: [inc_sir, inc_siredit], install: true)