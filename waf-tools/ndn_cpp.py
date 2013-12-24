#! /usr/bin/env python
# encoding: utf-8

'''

When using this tool, the wscript will look like:

	def options(opt):
	        opt.tool_options('ndn_cpp', tooldir=["waf-tools"])

	def configure(conf):
		conf.load('compiler_cxx ndn_cpp')

	def build(bld):
		bld(source='main.cpp', target='app', use='NDNCPP')

Options are generated, in order to specify the location of ndn-cpp includes/libraries.


'''

import sys
import re
from waflib import Utils,Logs,Errors
from waflib.Configure import conf
NDNCPP_DIR=['/usr/local/ndn', '/usr', '/usr/local', '/opt/local','/sw']
NDNCPP_VERSION_FILE='ndn-cpp-config.h'
NDNCPP_VERSION_CODE='''
#include <iostream>
#include <ndn-cpp/ndn-cpp-config.h>
int main() { std::cout << NDN_CPP_PACKAGE_VERSION; }
'''

def options(opt):
	opt.add_option('--ndn-cpp',type='string',default='/usr/local/ndn',dest='ndn_cpp_dir',help='''path to where ndn-cpp is installed, e.g. /usr/local/ndn''')
@conf
def __ndncpp_get_version_file(self,dir):
	try:
		return self.root.find_dir(dir).find_node('%s/%s' % ('include/ndn-cpp', NDNCPP_VERSION_FILE))
	except:
		return None
@conf
def ndncpp_get_version(self,dir):
	val=self.check_cxx(fragment=NDNCPP_VERSION_CODE,includes=['%s/%s' % (dir, 'include')], execute=True, define_ret = True, mandatory=True)
	return val
@conf
def ndncpp_get_root(self,*k,**kw):
	root=k and k[0]or kw.get('path',None)
	# Logs.pprint ('RED', '   %s' %root)
	if root and self.__ndncpp_get_version_file(root):
		return root
	for dir in NDNCPP_DIR:
		if self.__ndncpp_get_version_file(dir):
			return dir
	if root:
		self.fatal('NDNCPP not found in %s'%root)
	else:
		self.fatal('NDNCPP not found, please provide a --ndn-cpp argument (see help)')
@conf
def check_ndncpp(self,*k,**kw):
	if not self.env['CXX']:
		self.fatal('load a c++ compiler first, conf.load("compiler_cxx")')

	var=kw.get('uselib_store','NDNCPP')
	self.start_msg('Checking ndn-cpp')
	root = self.ndncpp_get_root(*k,**kw)
	self.env.NDNCPP_VERSION=self.ndncpp_get_version(root)

	self.env['INCLUDES_%s'%var]= '%s/%s' % (root, "include")
	self.env['LIB_%s'%var] = "ndn-cpp"
	self.env['LIBPATH_%s'%var] = '%s/%s' % (root, "lib")

	self.end_msg(self.env.NDNCPP_VERSION)
	if Logs.verbose:
		Logs.pprint('CYAN','	NDNCPP include : %s'%self.env['INCLUDES_%s'%var])
		Logs.pprint('CYAN','	NDNCPP lib     : %s'%self.env['LIB_%s'%var])
		Logs.pprint('CYAN','	NDNCPP libpath : %s'%self.env['LIBPATH_%s'%var])


