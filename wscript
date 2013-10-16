# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-
VERSION='0.1'
APPNAME='QT-Test'

from waflib import Configure

def options(opt):
    opt.load('compiler_c compiler_cxx boost protoc qt4')

    opt.load('tinyxml', tooldir=['waf-tools'])
    
def configure(conf):
    conf.load("compiler_c compiler_cxx boost protoc qt4 tinyxml")

    conf.add_supported_cxxflags (cxxflags = ['-O3', '-g'])

    conf.check_tinyxml(path=conf.options.tinyxml_dir)
    conf.check_cfg(package='libndn.cxx', args=['--cflags', '--libs'], uselib_store='NDNCXX', mandatory=True)
    conf.check_cfg(package='sqlite3', args=['--cflags', '--libs'], uselib_store='SQLITE3', mandatory=True)
    conf.check_cfg(package='liblog4cxx', args=['--cflags', '--libs'], uselib_store='LOG4CXX', mandatory=True)
    conf.define ("HAVE_LOG4CXX", 1)

    conf.check_boost(lib='system random thread filesystem')

    conf.write_config_header('config.h')

		
def build (bld):
    qt = bld (
        target = "Contacts",
        features = "qt4 cxx cxxprogram",
        defines = "WAF",
        source = bld.path.ant_glob(['src/*.cpp', 'src/*.ui', 'logging.cc']),
        includes = ".",
        use = "QTCORE QTGUI QTSQL SQLITE3 NDNCXX TINYXML BOOST BOOST_FILESYSTEM LOG4CXX",
        )   


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
