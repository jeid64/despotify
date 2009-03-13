#!/usr/bin/env python
# encoding: utf-8
# andersg at 0x63.nu 2007

import os
import pproc
import Task, Options, Utils
from Configure import conf
from TaskGen import extension, taskgen, feature, before

xsubpp_str = '${PERL} ${XSUBPP} -noprototypes -typemap ${EXTUTILS_TYPEMAP} ${SRC} > ${TGT}'
EXT_XS = ['.xs']

@taskgen
@before('apply_incpaths')
@feature('perlext')
def init_perlext(self):
	self.uselib = self.to_list(getattr(self, 'uselib', ''))
	if not 'PERL' in self.uselib: self.uselib.append('PERL')
	if not 'PERLEXT' in self.uselib: self.uselib.append('PERLEXT')
	self.env['shlib_PATTERN'] = self.env['perlext_PATTERN']


@extension(EXT_XS)
def xsubpp_file(self, node):
	gentask = self.create_task('xsubpp')
	gentask.set_inputs(node)
	outnode = node.change_ext('.c')
	gentask.set_outputs(outnode)

	self.allnodes.append(outnode)

Task.simple_task_type('xsubpp', xsubpp_str, color='BLUE', before="cc cxx")

@conf
def check_perl_version(conf, minver=None):
	"""
	Checks if perl is installed.

	If installed the variable PERL will be set in environment.

	Perl binary can be overridden by --with-perl-binary config variable

	"""
	res = True

	if not getattr(Options.options, 'perlbinary', None):
		perl = conf.find_program("perl", var="PERL")
		if not perl:
			return False
	else:
		perl = Options.options.perlbinary
		conf.env['PERL'] = perl

	version = Utils.cmd_output(perl + " -e'printf \"%vd\", $^V'")
	if not version:
		res = False
		version = "Unknown"
	elif not minver is None:
		ver = tuple(map(int, version.split(".")))
		if ver < minver:
			res = False

	if minver is None:
		cver = ""
	else:
		cver = ".".join(map(str,minver))
	conf.check_message("perl", cver, res, version)
	return res

@conf
def check_perl_module(conf, module):
	"""
	Check if specified perlmodule is installed.

	Minimum version can be specified by specifying it after modulename
	like this:

	conf.check_perl_module("Some::Module 2.92")
	"""
	cmd = [conf.env['PERL'], '-e', 'use %s' % module]
	r = pproc.call(cmd, stdout=pproc.PIPE, stderr=pproc.PIPE) == 0
	conf.check_message("perl module %s" % module, "", r)
	return r

@conf
def check_perl_ext_devel(conf):
	"""
	Check for configuration needed to build perl extensions.

	Sets different xxx_PERLEXT variables in the environment.

	Also sets the ARCHDIR_PERL variable useful as installation path,
	which can be overridden by --with-perl-archdir option.
	"""
	if not conf.env['PERL']:
		return False

	perl = conf.env['PERL']

	conf.env["LINKFLAGS_PERLEXT"] = Utils.cmd_output(perl + " -MConfig -e'print $Config{lddlflags}'")
	conf.env["CPPPATH_PERLEXT"] = Utils.cmd_output(perl + " -MConfig -e'print \"$Config{archlib}/CORE\"'")
	conf.env["CCFLAGS_PERLEXT"] = Utils.cmd_output(perl + " -MConfig -e'print \"$Config{ccflags} $Config{cccdlflags}\"'")

	conf.env["XSUBPP"] = Utils.cmd_output(perl + " -MConfig -e'print \"$Config{privlib}/ExtUtils/xsubpp$Config{exe_ext}\"'")
	conf.env["EXTUTILS_TYPEMAP"] = Utils.cmd_output(perl + " -MConfig -e'print \"$Config{privlib}/ExtUtils/typemap\"'")

	if not getattr(Options.options, 'perlarchdir', None):
		conf.env["ARCHDIR_PERL"] = Utils.cmd_output(perl + " -MConfig -e'print $Config{sitearch}'")
	else:
		conf.env["ARCHDIR_PERL"] = getattr(Options.options, 'perlarchdir')

	conf.env['perlext_PATTERN'] = '%s.' + Utils.cmd_output(perl + " -MConfig -e'print $Config{dlext}'")

	return True

def detect(conf):
	pass

def set_options(opt):
	opt.add_option("--with-perl-binary", type="string", dest="perlbinary", help = 'Specify alternate perl binary', default=None)
	opt.add_option("--with-perl-archdir", type="string", dest="perlarchdir", help = 'Specify directory where to install arch specific files', default=None)

