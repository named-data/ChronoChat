# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-
VERSION='0.5'
APPNAME='ChronoChat'

from waflib import Configure, Utils

def options(opt):
    opt.add_option('--debug',action='store_true',default=False,dest='debug',help='''debugging mode''')
    opt.add_option('--with-log4cxx',action='store_true',default=False,dest='log4cxx',help='''Enable log4cxx''')
    opt.add_option('--with-tests', action='store_true',default=False,dest='with_tests',help='''build unit tests''')
    opt.add_option('--without-security', action='store_false',default=True,dest='with_security',help='''Enable security''')

    opt.load('compiler_c compiler_cxx qt4')

    if Utils.unversioned_sys_platform () != "darwin":
        opt.load('gnu_dirs');

    opt.load('boost protoc', tooldir=['waf-tools'])

def configure(conf):
    conf.load("compiler_c compiler_cxx boost protoc qt4")

    if Utils.unversioned_sys_platform () != "darwin":
        conf.load('gnu_dirs');

    if conf.options.debug:
        conf.define ('_DEBUG', 1)
        conf.env['_DEBUG'] = True;
        flags = ['-O0',
                 '-Wall',
                 '-Wno-unused-variable',
                 '-g3',
                 '-Wno-unused-private-field', # only clang supports
                 '-fcolor-diagnostics',       # only clang supports
                 '-Qunused-arguments',        # only clang supports
                 '-Wno-deprecated-declarations',
                 '-Wno-unneeded-internal-declaration',
                 ]

        conf.add_supported_cxxflags (cxxflags = flags)
    else:
        flags = ['-O3', '-g', '-Wno-tautological-compare', '-Wno-unused-function', '-Wno-deprecated-declarations']
        conf.add_supported_cxxflags (cxxflags = flags)

    conf.check_cfg(package='libndn-cxx', args=['--cflags', '--libs'], uselib_store='NDN_CXX', mandatory=True)


    if conf.options.log4cxx:
        conf.check_cfg(package='liblog4cxx', args=['--cflags', '--libs'], uselib_store='LOG4CXX', mandatory=True)
        conf.define ("HAVE_LOG4CXX", 1)

    conf.check_cfg (package='ChronoSync', args=['ChronoSync >= 0.1', '--cflags', '--libs'], uselib_store='SYNC', mandatory=True)

    conf.check_boost(lib='system random thread filesystem unit_test_framework')

    if conf.options.with_tests:
        conf.env['WITH_TESTS'] = True
        conf.define('WITH_TESTS', 1)


    if conf.options.with_security:
        conf.define('WITH_SECURITY', 1)

    conf.write_config_header('src/config.h')

def build (bld):
    qt = bld (
        target = "ChronoChat",
        features = "qt4 cxx cxxprogram",
        # features= "qt4 cxx cxxshlib",
        defines = "WAF",
        source = bld.path.ant_glob(['src/*.cpp', 'src/*.ui', '*.qrc', 'logging.cc', 'src/*.proto']),
        includes = "src .",
        use = "QTCORE QTGUI QTWIDGETS QTSQL NDN_CXX BOOST LOG4CXX SYNC",
        )

    # Unit tests
    # if bld.env["WITH_TESTS"]:
    #   unittests = bld.program (
    #       target="unit-tests",
    #       source = bld.path.ant_glob(['test/**/*.cc']),
    #       features=['cxx', 'cxxprogram'],
    #       use = 'BOOST ChronoChat',
    #       includes = "src",
    #       install_path = None,
    #       )

    # Debug tools
    if bld.env["_DEBUG"]:
        for app in bld.path.ant_glob('debug-tools/*.cc'):
            bld(features=['cxx', 'cxxprogram'],
                target = '%s' % (str(app.change_ext('','.cc'))),
                source = app,
                use = 'NDN_CXX',
                install_path = None,
            )

      # Tmp disable
    if Utils.unversioned_sys_platform () == "darwin":
        app_plist = '''<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist SYSTEM "file://localhost/System/Library/DTDs/PropertyList.dtd">
<plist version="0.9">
<dict>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>CFBundleIconFile</key>
    <string>demo.icns</string>
    <key>CFBundleGetInfoString</key>
    <string>Created by Waf</string>
    <key>CFBundleIdentifier</key>
    <string>edu.ucla.cs.irl.ChronoChat</string>
    <key>CFBundleSignature</key>
    <string>????</string>
    <key>NOTE</key>
    <string>THIS IS A GENERATED FILE, DO NOT MODIFY</string>
    <key>CFBundleExecutable</key>
    <string>%s</string>
    <key>SUPublicDSAKeyFile</key>
    <string>dsa_pub.pem</string>
    <key>CFBundleIconFile</key>
    <string>demo.icns</string>
</dict>
</plist>'''

    # <key>LSUIElement</key>
    # <string>1</string>

        qt.mac_app = "ChronoChat.app"
        qt.mac_plist = app_plist % "ChronoChat"
        qt.mac_resources = 'demo.icns'
    else:
        bld (features = "subst",
             source = 'linux/chronochat.desktop.in',
             target = 'linux/chronochat.desktop',
             BINARY = "ChronoChat",
             install_path = "${DATAROOTDIR}/applications"
            )
        bld.install_files("${DATAROOTDIR}/chronochat",
                          bld.path.ant_glob(['linux/Resources/*']))


@Configure.conf
def add_supported_cxxflags(self, cxxflags):
    """
    Check which cxxflags are supported by compiler and add them to env.CXXFLAGS variable
    """
    self.start_msg('Checking allowed flags for c++ compiler')

    supportedFlags = []
    for flag in cxxflags:
        if self.check_cxx (cxxflags=[flag], mandatory=False):
            supportedFlags += [flag]

    self.end_msg (' '.join (supportedFlags))
    self.env.CXXFLAGS += supportedFlags
