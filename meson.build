project('sirEdit', ['cpp'],
        version: '1.0.0',
        meson_version: '>= 0.51.0'
)

cmake = import('cmake')
cpp = meson.get_compiler('cpp')
openmp = cpp.find_library('gomp', required : false)
cppCommon = cmake.subproject('cppCommon')
skillLib = cppCommon.dependency('skill_cpp_common')
gtest = dependency('gtest', main : true, required : false)

thread_dep = dependency('threads')
gtkmm = dependency('gtkmm-3.0', version: '>= 3.18')

# Sources of sir
src_sir = [
	'sir/BuildInformationFieldDeclarations.cpp',
	'sir/CommentFieldDeclarations.cpp',
	'sir/CommentTagFieldDeclarations.cpp',
	'sir/CustomFieldOptionFieldDeclarations.cpp',
	'sir/FieldLikeFieldDeclarations.cpp',
	'sir/FilePathFieldDeclarations.cpp',
	'sir/HintFieldDeclarations.cpp',
	'sir/IdentifierFieldDeclarations.cpp',
	'sir/RestrictionFieldDeclarations.cpp',
	'sir/ToolFieldDeclarations.cpp',
	'sir/ToolTypeCustomizationFieldDeclarations.cpp',
	'sir/TypeFieldDeclarations.cpp',
	'sir/File.cpp',
	'sir/BuildInformationPools.cpp',
	'sir/CommentPools.cpp',
	'sir/CommentTagPools.cpp',
	'sir/CustomFieldOptionPools.cpp',
	'sir/FieldLikePools.cpp',
	'sir/FilePathPools.cpp',
	'sir/HintPools.cpp',
	'sir/IdentifierPools.cpp',
	'sir/RestrictionPools.cpp',
	'sir/ToolPools.cpp',
	'sir/ToolTypeCustomizationPools.cpp',
	'sir/TypePools.cpp',
	'sir/TypesOfBuildInformation.cpp',
	'sir/TypesOfComment.cpp',
	'sir/TypesOfCommentTag.cpp',
	'sir/TypesOfCustomFieldOption.cpp',
	'sir/TypesOfFieldLike.cpp',
	'sir/TypesOfFilePath.cpp',
	'sir/TypesOfHint.cpp',
	'sir/TypesOfIdentifier.cpp',
	'sir/TypesOfRestriction.cpp',
	'sir/TypesOfTool.cpp',
	'sir/TypesOfToolTypeCustomization.cpp',
	'sir/TypesOfType.cpp'
]
inc_sir = include_directories('sir')

# SirEdit files
src_siredit = [
	'src/main.cpp',
	'src/runCodegen.cpp',
	'src/data/includeSearch.cpp',
	'src/data/serializeSIR.cpp',
	'src/gui/loadFile.cpp',
	'src/gui/mainWindow.cpp',
	'src/gui/overview.cpp',
	'src/gui/startupWindow.cpp',
	'src/gui/toolsEditor.cpp'
]
inc_siredit = include_directories('include')

# SirEdit tests
src_siredit_test = [
	'src/unittests/rules/rule1.cpp',
	'src/unittests/rules/rule2.cpp',
	'src/unittests/rules/rule3.cpp',
	'src/unittests/rules/rule4.cpp',
	'src/unittests/rules/rule5.cpp',
	'src/unittests/rules/rule6.cpp',
	'src/unittests/rules/rule7.cpp'
]

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
	              command: [resource_encoder, '@INPUT0@', '@OUTPUT0@', 'sirEdit_mainWindow_glade']),
	custom_target('codegen.jar',
	              input: ['data/codegen.jar'],
	              output: ['tmpres_sirEdit_codegen.cpp'],
	              command: [resource_encoder, '@INPUT0@', '@OUTPUT0@', 'sirEdit_codegen'])
]

executable('sirEdit', src_sir + src_siredit + src_resources, cpp_args: '-std=c++17', dependencies: [gtkmm, skillLib, openmp, thread_dep], include_directories: [inc_sir, inc_siredit], install: true)
sirEdit_test = executable('sirEdit_test', src_siredit_test, cpp_args: '-std=c++17', dependencies: [openmp, gtest], include_directories: [inc_siredit])
test('sirEdit tests', sirEdit_test)
