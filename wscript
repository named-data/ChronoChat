# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-
VERSION='1.0'
APPNAME='ChronoChat'

from waflib import Configure, Utils

def options(opt):
    opt.add_option('--debug',action='store_true',default=False,dest='debug',help='''debugging mode''')
    
    opt.load('compiler_c compiler_cxx boost protoc qt4')

    # opt.load('tinyxml', tooldir=['waf-tools'])
    opt.load('cryptopp', tooldir=['waf-tools'])
    
def configure(conf):
    conf.load("compiler_c compiler_cxx boost protoc qt4 cryptopp")

    if conf.options.debug:
        conf.define ('_DEBUG', 1)
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
        
    # conf.check_tinyxml(path=conf.options.tinyxml_dir)
    conf.check_cfg(package='libndn.cxx', args=['--cflags', '--libs'], uselib_store='NDNCXX', mandatory=True)
    conf.check_cfg(package='sqlite3', args=['--cflags', '--libs'], uselib_store='SQLITE3', mandatory=True)
    conf.check_cfg(package='liblog4cxx', args=['--cflags', '--libs'], uselib_store='LOG4CXX', mandatory=True)
    conf.check_cfg (package='ChronoSync', args=['ChronoSync >= 0.1', '--cflags', '--libs'], uselib_store='SYNC', mandatory=True)
    conf.define ("HAVE_LOG4CXX", 1)

    conf.check_boost(lib='system random thread filesystem')

    conf.write_config_header('config.h')

		
def build (bld):
    qt = bld (
        target = "ChronoChat",
        features = "qt4 cxx cxxprogram",
        defines = "WAF",
        source = bld.path.ant_glob(['src/*.cpp', 'src/*.ui', 'logging.cc', 'src/*.proto']),
        includes = ".",
        use = "QTCORE QTGUI QTSQL SQLITE3 NDNCXX BOOST BOOST_FILESYSTEM LOG4CXX CRYPTOPP SYNC",
        )

    cert_publish = bld (
        target = "CertPublish",
        features = "cxx cxxprogram",
        defines = "WAF",
        source = bld.path.ant_glob(['tmp/cert-publish.cpp']),
        includes = ".",
        use = "SQLITE3 NDNCXX BOOST BOOST_FILESYSTEM LOG4CXX",
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

from waflib.TaskGen import feature, before_method, after_method
@feature('cxx')
@after_method('process_source')
@before_method('apply_incpaths')
def add_includes_paths(self):
    incs = set(self.to_list(getattr(self, 'includes', '')))
    for x in self.compiled_tasks:
        incs.add(x.inputs[0].parent.path_from(self.path))
        self.includes = list(incs)
