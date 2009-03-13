#!/usr/bin/env python
# encoding: utf-8
# Ali Sabil, 2007


import Task, Utils
from TaskGen import taskgen, before, after, feature

@taskgen
def add_dbus_file(self, filename, prefix, mode):
	if not hasattr(self, 'dbus_lst'):
		self.dbus_lst = []
	self.meths.append('process_dbus')
	self.dbus_lst.append([filename, prefix, mode])

@taskgen
@before('apply_core')
def process_dbus(self):
	for filename, prefix, mode in getattr(self, 'dbus_lst', []):
		env = self.env.copy()
		node = self.path.find_resource(filename)

		if not node:
			raise Utils.WafError('file not found ' + filename)

		env['DBUS_BINDING_TOOL_PREFIX'] = prefix
		env['DBUS_BINDING_TOOL_MODE']   = mode

		task = self.create_task('dbus_binding_tool', env)
		task.set_inputs(node)
		task.set_outputs(node.change_ext('.h'))

Task.simple_task_type('dbus_binding_tool',
	'${DBUS_BINDING_TOOL} --prefix=${DBUS_BINDING_TOOL_PREFIX} --mode=${DBUS_BINDING_TOOL_MODE} --output=${TGT} ${SRC}',
	color='BLUE', before='cc')

def detect(conf):
	dbus_binding_tool = conf.find_program('dbus-binding-tool', var='DBUS_BINDING_TOOL')

