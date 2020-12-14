# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

from waflib import Context, Logs, Utils
import os

VERSION = '0.5'
APPNAME = 'ChronoChat'

def options(opt):
    opt.load(['compiler_cxx', 'gnu_dirs'])
    opt.load(['default-compiler-flags',
              'boost', 'qt5_custom',
              'doxygen', 'sphinx_build'],
             tooldir=['.waf-tools'])

    optgrp = opt.add_option_group('ChronoChat Options')
    optgrp.add_option('--with-tests', action='store_true', default=False,
                      help='Build unit tests')

def configure(conf):
    conf.load(['compiler_cxx', 'gnu_dirs',
               'default-compiler-flags', 'boost', 'qt5_custom',
               'doxygen', 'sphinx_build'])

    conf.env.WITH_TESTS = conf.options.with_tests

    pkg_config_path = os.environ.get('PKG_CONFIG_PATH', '%s/pkgconfig' % conf.env.LIBDIR)
    conf.check_cfg(package='libndn-cxx', args=['--cflags', '--libs'], uselib_store='NDN_CXX',
                   pkg_config_path=pkg_config_path)
    conf.check_cfg(package='ChronoSync', args=['--cflags', '--libs'], uselib_store='SYNC',
                   pkg_config_path=pkg_config_path)

    boost_libs = ['system', 'random', 'thread', 'filesystem']
    if conf.env.WITH_TESTS:
        boost_libs.append('unit_test_framework')
        conf.define('WITH_TESTS', 1)

    conf.check_boost(lib=boost_libs, mt=True)
    if conf.env.BOOST_VERSION_NUMBER < 105800:
        conf.fatal('The minimum supported version of Boost is 1.65.1.\n'
                   'Please upgrade your distribution or manually install a newer version of Boost.\n'
                   'For more information, see https://redmine.named-data.net/projects/nfd/wiki/Boost')
    elif conf.env.BOOST_VERSION_NUMBER < 106501:
        Logs.warn('WARNING: Using a version of Boost older than 1.65.1 is not officially supported and may not work.\n'
                  'If you encounter any problems, please upgrade your distribution or manually install a newer version of Boost.\n'
                  'For more information, see https://redmine.named-data.net/projects/nfd/wiki/Boost')

    conf.check_compiler_flags()
    conf.write_config_header('src/config.h')

def build (bld):
    feature_list = 'qt5 cxx'
    if bld.env.WITH_TESTS:
        feature_list += ' cxxstlib'
    else:
        feature_list += ' cxxprogram'

    qt = bld(
        target = "ChronoChat",
        features = feature_list,
        defines = "WAF=1",
        source = bld.path.ant_glob(['src/*.cpp', 'src/*.ui', '*.qrc']),
        includes = "src .",
        use = "QT5CORE QT5GUI QT5WIDGETS QT5SQL NDN_CXX BOOST SYNC",
        )

    # Unit tests
    if bld.env.WITH_TESTS:
      unittests = bld.program(
          target="unit-tests",
          source = bld.path.ant_glob(['test/**/*.cpp']),
          features=['cxx', 'cxxprogram'],
          use = 'BOOST ChronoChat',
          includes = "src .",
          install_path = None,
          )

    # Debug tools
    if "_DEBUG" in bld.env["DEFINES"]:
        for app in bld.path.ant_glob('debug-tools/*.cpp'):
            bld(features=['cxx', 'cxxprogram'],
                target = '%s' % (str(app.change_ext('','.cpp'))),
                source = app,
                use = 'NDN_CXX',
                includes = "src .",
                install_path = None,
            )

    if not bld.env.WITH_TESTS:
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
