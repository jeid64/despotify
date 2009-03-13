#!/usr/bin/env python
# encoding: utf-8
# Sebastian Schlingmann, 2008
# Thomas Nagy, 2008 (ita)

import TaskGen
from TaskGen import taskgen, feature
from Constants import *

TaskGen.declare_chain(
	name = 'luac',
	rule = '${LUAC} -s -o ${TGT} ${SRC}',
	ext_in = '.lua',
	ext_out = '.luac',
	reentrant = 0,
	install = 'LUADIR', # env variable
)

@taskgen
@feature('lua')
def init_lua(self):
	self.default_chmod = O755

def detect(conf):
	conf.find_program('luac', var='LUAC', mandatory = True)

