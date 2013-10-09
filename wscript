# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-
VERSION='0.1'
APPNAME='QT-Test'

from waflib import Configure

def options(opt):
    opt.load('compiler_c compiler_cxx boost protoc qt4')
    
def configure(conf):
    conf.load("compiler_c compiler_cxx")

    conf.add_supported_cxxflags (cxxflags = ['-O3', '-g'])

    conf.load('protoc')

    conf.load('qt4')

    conf.load('boost')

    conf.check_boost(lib='system random thread')

    conf.write_config_header('config.h')

		
def build (bld):
    qt = bld (
        target = "ChronoChat",
        features = "qt4 cxx cxxprogram",
        defines = "WAF",
        source = bld.path.ant_glob(['src/*.cpp', 'src/*.ui']),
        includes = ".",
        use = "BOOST BOOST_THREAD QTCORE QTGUI",
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
