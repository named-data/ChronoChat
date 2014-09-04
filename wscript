# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-
VERSION='0.5'
APPNAME='ChronoChat'

from waflib import Configure, Utils, Logs, Context
import os

def options(opt):

    opt.load(['compiler_c', 'compiler_cxx', 'qt4', 'gnu_dirs'])

    opt.load(['default-compiler-flags', 'boost', 'protoc',
              'doxygen', 'sphinx_build'],
              tooldir=['waf-tools'])

    opt = opt.add_option_group('ChronotChat Options')

    opt.add_option('--with-tests', action='store_true', default=False, dest='with_tests',
                   help='''build unit tests''')

    opt.add_option('--with-log4cxx', action='store_true', default=False, dest='log4cxx',
                   help='''Enable log4cxx''')

def configure(conf):
    conf.load(['compiler_c', 'compiler_cxx', 'qt4',
               'default-compiler-flags', 'boost', 'protoc', 'gnu_dirs',
               'doxygen', 'sphinx_build'])

    conf.check_cfg(package='libndn-cxx', args=['--cflags', '--libs'],
                   uselib_store='NDN_CXX', mandatory=True)

    if conf.options.log4cxx:
        conf.check_cfg(package='liblog4cxx', args=['--cflags', '--libs'],
                       uselib_store='LOG4CXX', mandatory=True)
        conf.define("HAVE_LOG4CXX", 1)

    conf.check_cfg (package='ChronoSync', args=['ChronoSync >= 0.1', '--cflags', '--libs'],
                    uselib_store='SYNC', mandatory=True)

    boost_libs = 'system random thread filesystem'
    if conf.options.with_tests:
        conf.env['WITH_TESTS'] = 1
        conf.define('WITH_TESTS', 1);
        boost_libs += ' unit_test_framework'

    conf.check_boost(lib=boost_libs)
    if conf.env.BOOST_VERSION_NUMBER < 104800:
        Logs.error("Minimum required boost version is 1.48.0")
        Logs.error("Please upgrade your distribution or install custom boost libraries" +
                   " (http://redmine.named-data.net/projects/nfd/wiki/Boost_FAQ)")
        return

    conf.write_config_header('src/config.h')

def build (bld):
    feature_list = 'qt4 cxx'
    if bld.env["WITH_TESTS"]:
        feature_list += ' cxxshlib'
    else:
        feature_list += ' cxxprogram'

    qt = bld (
        target = "ChronoChat",
        features = feature_list,
        defines = "WAF=1",
        source = bld.path.ant_glob(['src/*.cpp', 'src/*.ui', '*.qrc', 'logging.cc', 'src/*.proto']),
        includes = "src .",
        use = "QTCORE QTGUI QTWIDGETS QTSQL NDN_CXX BOOST LOG4CXX SYNC",
        )

    # Unit tests
    if bld.env["WITH_TESTS"]:
      unittests = bld.program (
          target="unit-tests",
          source = bld.path.ant_glob(['test/**/*.cpp']),
          features=['cxx', 'cxxprogram'],
          use = 'BOOST ChronoChat',
          includes = "src .",
          install_path = None,
          )

    # Debug tools
    if bld.env["_DEBUG"]:
        for app in bld.path.ant_glob('debug-tools/*.cc'):
            bld(features=['cxx', 'cxxprogram'],
                target = '%s' % (str(app.change_ext('','.cc'))),
                source = app,
                use = 'NDN_CXX',
                includes = "src .",
                install_path = None,
            )

    if not bld.env["WITH_TESTS"]:
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
            bld(features = "subst",
                source = 'linux/chronochat.desktop.in',
                target = 'linux/chronochat.desktop',
                BINARY = "ChronoChat",
                install_path = "${DATAROOTDIR}/applications"
                )
            bld.install_files("${DATAROOTDIR}/chronochat",
                              bld.path.ant_glob(['linux/Resources/*']))


# docs
def docs(bld):
    from waflib import Options
    Options.commands = ['doxygen', 'sphinx'] + Options.commands

def doxygen(bld):
    version(bld)

    if not bld.env.DOXYGEN:
        Logs.error("ERROR: cannot build documentation (`doxygen' is not found in $PATH)")
    else:
        bld(features="subst",
            name="doxygen-conf",
            source=["docs/doxygen.conf.in",
                    "docs/named_data_theme/named_data_footer-with-analytics.html.in"],
            target=["docs/doxygen.conf",
                    "docs/named_data_theme/named_data_footer-with-analytics.html"],
            VERSION=VERSION,
            HTML_FOOTER="../build/docs/named_data_theme/named_data_footer-with-analytics.html" \
                          if os.getenv('GOOGLE_ANALYTICS', None) \
                          else "../docs/named_data_theme/named_data_footer.html",
            GOOGLE_ANALYTICS=os.getenv('GOOGLE_ANALYTICS', ""),
            )

        bld(features="doxygen",
            doxyfile='docs/doxygen.conf',
            use="doxygen-conf")

def sphinx(bld):
    version(bld)

    if not bld.env.SPHINX_BUILD:
        bld.fatal("ERROR: cannot build documentation (`sphinx-build' is not found in $PATH)")
    else:
        bld(features="sphinx",
            outdir="docs",
            source=bld.path.ant_glob("docs/**/*.rst"),
            config="docs/conf.py",
            VERSION=VERSION)

def version(ctx):
    if getattr(Context.g_module, 'VERSION_BASE', None):
        return

    Context.g_module.VERSION_BASE = Context.g_module.VERSION
    Context.g_module.VERSION_SPLIT = [v for v in VERSION_BASE.split('.')]

    try:
        cmd = ['git', 'describe', '--match', 'ChronoChat-*']
        p = Utils.subprocess.Popen(cmd, stdout=Utils.subprocess.PIPE,
                                   stderr=None, stdin=None)
        out = p.communicate()[0].strip()
        if p.returncode == 0 and out != "":
            Context.g_module.VERSION = out[11:]
    except:
        pass
