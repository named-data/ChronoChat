# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-
VERSION='0.4'
APPNAME='ChronoChat'

from waflib import Configure, Utils

def options(opt):
    opt.add_option('--debug',action='store_true',default=False,dest='debug',help='''debugging mode''')
    opt.add_option('--log4cxx',action='store_true',default=False,dest='log4cxx',help='''Enable log4cxx''')
    opt.add_option('--test', action='store_true',default=False,dest='_test',help='''build unit tests''')
    
    opt.load('compiler_c compiler_cxx qt4')

    if Utils.unversioned_sys_platform () != "darwin":
        opt.load('gnu_dirs');

    opt.load('boost protoc cryptopp ndn_cpp', tooldir=['waf-tools'])
    
def configure(conf):
    conf.load("compiler_c compiler_cxx boost protoc qt4 cryptopp ndn_cpp")

    if Utils.unversioned_sys_platform () != "darwin":
        conf.load('gnu_dirs');

    if conf.options.debug:
        conf.define ('_DEBUG', 1)
        conf.env.DEBUG = 1
        conf.add_supported_cxxflags (cxxflags = ['-O0',
                                                 '-Wall',
                                                 '-Wno-unused-variable',
                                                 '-g3',
                                                 '-Wno-unused-private-field', # only clang supports
                                                 '-fcolor-diagnostics',       # only clang supports
                                                 '-Qunused-arguments',        # only clang supports
                                                 ])
    else:
        conf.add_supported_cxxflags (cxxflags = ['-O3', '-g', '-Wno-tautological-compare', '-Wno-unused-function'])
        
    conf.check_ndncpp (path=conf.options.ndn_cpp_dir)
    conf.check_cfg(package='libndn-cpp-et', args=['--cflags', '--libs'], uselib_store='NDN-CPP-ET', mandatory=True)
    
    conf.check_cfg(package='sqlite3', args=['--cflags', '--libs'], uselib_store='SQLITE3', mandatory=True)
    if conf.options.log4cxx:
        conf.check_cfg(package='liblog4cxx', args=['--cflags', '--libs'], uselib_store='LOG4CXX', mandatory=True)
    conf.check_cfg (package='ChronoSync', args=['ChronoSync >= 0.1', '--cflags', '--libs'], uselib_store='SYNC', mandatory=True)

    conf.check_cryptopp(path=conf.options.cryptopp_dir)

    conf.check_boost(lib='system random thread filesystem test')

    conf.write_config_header('config.h')

    if conf.options._test:
      conf.define('_TEST', 1)
		
def build (bld):
    qt = bld (
        target = "ChronoChat",
        # features = "qt4 cxx cxxprogram",
        features= "qt4 cxx cxxshlib",
        defines = "WAF",
        source = bld.path.ant_glob(['src/*.cpp', 'src/*.ui', '*.qrc', 'logging.cc', 'src/*.proto']),
        includes = "src .",
        use = "QTCORE QTGUI QTWIDGETS QTSQL SQLITE3 NDNCPP NDN-CPP-ET BOOST BOOST_FILESYSTEM LOG4CXX CRYPTOPP SYNC",
        )

    # Unit tests
    if bld.get_define("_TEST"):
      unittests = bld.program (
          target="unit-tests",
          source = bld.path.ant_glob(['test/**/*.cc']),
          features=['cxx', 'cxxprogram'],
          use = 'BOOST_TEST ChronoChat',
          includes = ['src'],
          )
      # Tmp disable
#     if Utils.unversioned_sys_platform () == "darwin":
#         app_plist = '''<?xml version="1.0" encoding="UTF-8"?>
# <!DOCTYPE plist SYSTEM "file://localhost/System/Library/DTDs/PropertyList.dtd">
# <plist version="0.9">
# <dict>
#     <key>CFBundlePackageType</key>
#     <string>APPL</string>
#     <key>CFBundleIconFile</key>
#     <string>demo.icns</string>
#     <key>CFBundleGetInfoString</key>
#     <string>Created by Waf</string>
#     <key>CFBundleIdentifier</key>
#     <string>edu.ucla.cs.irl.ChronoChat</string>
#     <key>CFBundleSignature</key>
#     <string>????</string>
#     <key>NOTE</key>
#     <string>THIS IS A GENERATED FILE, DO NOT MODIFY</string>
#     <key>CFBundleExecutable</key>
#     <string>%s</string>
#     <key>SUPublicDSAKeyFile</key>
#     <string>dsa_pub.pem</string>
#     <key>CFBundleIconFile</key>
#     <string>demo.icns</string>
# </dict>
# </plist>'''

#     # <key>LSUIElement</key>
#     # <string>1</string>

#         qt.mac_app = "ChronoChat.app"
#         qt.mac_plist = app_plist % "ChronoChat"
#         qt.mac_resources = 'demo.icns'
#     else:
#         bld (features = "subst",
#              source = 'linux/chronochat.desktop.in',
#              target = 'linux/chronochat.desktop',
#              BINARY = "ChronoChat",
#              install_path = "${DATAROOTDIR}/applications"
#             )
#         bld.install_files("${DATAROOTDIR}/chronochat",
#                           bld.path.ant_glob(['linux/Resources/*']))


from waflib import TaskGen
@TaskGen.extension('.mm')
def m_hook(self, node):
    """Alias .mm files to be compiled the same as .cc files, gcc/clang will do the right thing."""
    return self.create_compiled_task('cxx', node)

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
