# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-
VERSION='0.1'
APPNAME='QT-Test'

def options(opt):
    opt.load('compiler_c compiler_cxx boost protoc qt4')
    
def configure(conf):
    conf.load("compiler_c compiler_cxx")

#    conf.add_supported_cxxflags (cxxflags = ['-O3', '-g'])

    conf.load('protoc')

    conf.load('qt4')

    conf.load('boost')

    conf.check_boost(lib='system random thread')
