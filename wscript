# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-
VERSION='0.3'
APPNAME='ChronoChat'

from waflib import Build, Logs, Utils, Task, TaskGen, Configure

def options(opt):
    opt.add_option('--debug',action='store_true',default=False,dest='debug',help='''debugging mode''')
    # opt.add_option('--test', action='store_true',default=False,dest='_test',help='''build unit tests''')
    # opt.add_option('--log4cxx', action='store_true',default=False,dest='log4cxx',help='''Compile with log4cxx logging support''')

    # if Utils.unversioned_sys_platform () == "darwin":
    #     opt.add_option('--auto-update', action='store_true',default=False,dest='autoupdate',help='''(OSX) Download sparkle framework and enable autoupdate feature''')

    opt.load('compiler_c compiler_cxx boost protoc qt4')

def configure(conf):
    conf.load("compiler_c compiler_cxx")

    if conf.options.debug:
        conf.define ('_DEBUG', 1)
        conf.add_supported_cxxflags (cxxflags = ['-O0',
                                                 '-Wall',
                                                 '-Wno-unused-variable',
                                                 '-g3',
                                                 '-Wno-unused-private-field', # only clang supports
                                                 '-fcolor-diagnostics',       # only clang supports
                                                 '-Qunused-arguments'         # only clang supports
                                                 ])
    else:
        conf.add_supported_cxxflags (cxxflags = ['-O3', '-g'])

    conf.check_cfg (package='libsync', args=['--cflags', '--libs'], uselib_store='SYNC', mandatory=True)

    conf.define ("CHRONOCHAT_VERSION", VERSION)

    # if Utils.unversioned_sys_platform () == "darwin":
    #     conf.check_cxx(framework_name='Foundation', uselib_store='OSX_FOUNDATION', mandatory=False, compile_filename='test.mm')
    #     conf.check_cxx(framework_name='AppKit',     uselib_store='OSX_APPKIT',     mandatory=False, compile_filename='test.mm')
    #     conf.check_cxx(framework_name='CoreWLAN',   uselib_store='OSX_COREWLAN',   define_name='HAVE_COREWLAN',
    #                    use="OSX_FOUNDATION", mandatory=False, compile_filename='test.mm')

    #     if conf.options.autoupdate:
    #         def check_sparkle(**kwargs):
    #           conf.check_cxx (framework_name="Sparkle", header_name=["Foundation/Foundation.h", "AppKit/AppKit.h"],
    #                           uselib_store='OSX_SPARKLE', define_name='HAVE_SPARKLE', mandatory=True,
    #                           compile_filename='test.mm', use="OSX_FOUNDATION OSX_APPKIT",
    #                           **kwargs
    #                           )
    #         try:
    #             # Try standard paths first
    #             check_sparkle()
    #         except:
    #             try:
    #                 # Try local path
    #                 Logs.info ("Check local version of Sparkle framework")
    #                 check_sparkle(cxxflags="-F%s/osx/Frameworks/" % conf.path.abspath(),
    #                               linkflags="-F%s/osx/Frameworks/" % conf.path.abspath())
    #                 conf.env.HAVE_LOCAL_SPARKLE = 1
    #             except:
    #                 import urllib, subprocess, os, shutil
    #                 if not os.path.exists('osx/Frameworks/Sparkle.framework'):
    #                     # Download to local path and retry
    #                     Logs.info ("Sparkle framework not found, trying to download it to 'build/'")

    #                     urllib.urlretrieve ("http://sparkle.andymatuschak.org/files/Sparkle%201.5b6.zip", "build/Sparkle.zip")
    #                     if os.path.exists('build/Sparkle.zip'):
    #                         try:
    #                             subprocess.check_call (['unzip', '-qq', 'build/Sparkle.zip', '-d', 'build/Sparkle'])
    #                             os.remove ("build/Sparkle.zip")
    #                             if not os.path.exists("osx/Frameworks"):
    #                                 os.mkdir ("osx/Frameworks")
    #                             os.rename ("build/Sparkle/Sparkle.framework", "osx/Frameworks/Sparkle.framework")
    #                             shutil.rmtree("build/Sparkle", ignore_errors=True)

    #                             check_sparkle(cxxflags="-F%s/osx/Frameworks/" % conf.path.abspath(),
    #                                           linkflags="-F%s/osx/Frameworks/" % conf.path.abspath())
    #                             conf.env.HAVE_LOCAL_SPARKLE = 1
    #                         except subprocess.CalledProcessError as e:
    #                             conf.fatal("Cannot find Sparkle framework. Auto download failed: '%s' returned %s" % (' '.join(e.cmd), e.returncode))
    #                         except:
    #                             conf.fatal("Unknown Error happened when auto downloading Sparkle framework")

    #         if conf.is_defined('HAVE_SPARKLE'):
    #             conf.env.HAVE_SPARKLE = 1 # small cheat for wscript

    # if conf.options.log4cxx:
    #     conf.check_cfg(package='liblog4cxx', args=['--cflags', '--libs'], uselib_store='LOG4CXX', mandatory=True)
    #     conf.define ("HAVE_LOG4CXX", 1)

    conf.load('protoc')

    conf.load('qt4')

    conf.load('boost')

    conf.check_boost(lib='system random')

    boost_version = conf.env.BOOST_VERSION.split('_')
    if int(boost_version[0]) < 1 or int(boost_version[1]) < 48:
        Logs.error ("Minumum required boost version is 1.48")
        return

    # if conf.options._test:
    #     conf.define ('_TESTS', 1)
    #     conf.env.TEST = 1

    conf.write_config_header('config.h')

def build (bld):
    # # Unit tests
    # if bld.env['TEST']:
    #   unittests = bld.program (
    #       target="unit-tests",
    #       features = "qt4 cxx cxxprogram",
    #       defines = "WAF",
    #       source = bld.path.ant_glob(['test/*.cc']),
    #       use = 'BOOST_TEST BOOST_FILESYSTEM BOOST_DATE_TIME LOG4CXX SQLITE3 QTCORE QTGUI ccnx database fs_watcher chronoshare',
    #       includes = "ccnx scheduler src executor gui fs-watcher",
    #       install_prefix = None,
    #       )

    qt = bld (
        target = "ChronoChat",
        features = "qt4 cxx cxxprogram",
        defines = "WAF",
        source = bld.path.ant_glob(['*.cpp', '*.ui', 'demo.qrc', '*.proto']),
        includes = " . ",
        use = "BOOST BOOST_THREAD QTCORE QTGUI SYNC",
        )

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
    <key>LSUIElement</key>
    <string>1</string>
    <key>SUPublicDSAKeyFile</key>
    <string>dsa_pub.pem</string>
    <key>CFBundleIconFile</key>
    <string>demo.icns</string>
</dict>
</plist>'''

        qt.mac_app = "ChronoChat.app"
        qt.mac_plist = app_plist % "ChronoChat"
        qt.mac_resources = 'demo.icns'
        # qt.use += " OSX_FOUNDATION OSX_COREWLAN adhoc"

        # if bld.env['HAVE_SPARKLE']:
        #     qt.use += " OSX_SPARKLE"
        #     qt.source += ["osx/auto-update/sparkle-auto-update.mm"]
        #     qt.includes += " osx/auto-update"
        #     if bld.env['HAVE_LOCAL_SPARKLE']:
        #         qt.mac_frameworks = "osx/Frameworks/Sparkle.framework"

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
