#!/usr/bin/env python
# encoding: utf-8
# Thomas Nagy, 2005 (ita)

"""Environment representation

There is one gotcha: getitem returns [] if the contents evals to False
This means env['foo'] = {}; print env['foo'] will print [] not {}
"""

import os, copy, re
import Logs, Options
from Constants import *
re_imp = re.compile('^(#)*?([^#=]*?)\ =\ (.*?)$', re.M)

class Environment(object):
	"""A safe-to-use dictionary, but do not attach functions to it please (break cPickle)
	An environment instance can be stored into a file and loaded easily
	"""
	__slots__ = ("table", "parent")
	def __init__(self, filename=None):
		self.table={}
		#self.parent = None <- set only if necessary

		if Options.commands['configure']:
			# set the prefix once and for everybody on creation (configuration)
			self.table['PREFIX'] = os.path.abspath(os.path.expanduser(Options.options.prefix))

		if filename:
			self.load(filename)

	def __contains__(self, key):
		if key in self.table: return True
		try: return self.parent.__contains__(key)
		except AttributeError: return False # parent may not exist

	def __str__(self):
		keys = set()
		cur = self
		while cur:
			keys.update(cur.table.keys())
			cur = getattr(cur, 'parent', None)
		keys = list(keys)
		keys.sort()
		return "\n".join(["%r %r" % (x, self.__getitem__(x)) for x in keys])

	def set_variant(self, name):
		self.table[VARIANT] = name

	def variant(self):
		env = self
		while 1:
			try:
				return env.table[VARIANT]
			except KeyError:
				try: env = env.parent
				except AttributeError: return DEFAULT

	def copy(self):
		newenv = Environment()
		if Options.commands['configure']:
			if self['PREFIX']: del newenv.table['PREFIX']
		newenv.parent = self
		return newenv

	def __getitem__(self, key):
		x = self.table.get(key, None)
		if not x is None: return x
		try:
			u = self.parent
		except AttributeError:
			return []
		else:
			return u[key]

	def __setitem__(self, key, value):
		self.table[key] = value

	def get_flat(self, key):
		s = self[key]
		if not s: return ''
		elif isinstance(s, list): return ' '.join(s)
		else: return s

	def _get_list_value_for_modification(self, key):
		"""Gets a value that must be a list for further modification.  The
		list may be modified inplace and there is no need to
		"self.table[var] = value" afterwards.
		"""
		try:
			value = self.table[key]
		except KeyError:
			try: value = self.parent[key]
			except AttributeError: value = []
			if isinstance(value, list):
				value = copy.copy(value)
			else:
				value = [value]
		else:
			if not isinstance(value, list):
				value = [value]
		self.table[key] = value
		return value

	def append_value(self, var, value):
		current_value = self._get_list_value_for_modification(var)

		if isinstance(value, list):
			current_value.extend(value)
		else:
			current_value.append(value)

	def prepend_value(self, var, value):
		current_value = self._get_list_value_for_modification(var)

		if isinstance(value, list):
			current_value = value + current_value
			# a new list: update the dictionary entry
			self.table[var] = current_value
		else:
			current_value.insert(0, value)

	# prepend unique would be ambiguous
	def append_unique(self, var, value):
		current_value = self._get_list_value_for_modification(var)

		if isinstance(value, list):
			for value_item in value:
				if value_item not in current_value:
					current_value.append(value_item)
		else:
			if value not in current_value:
				current_value.append(value)

	def store(self, filename):
		"Write the variables into a file"
		file = open(filename, 'w')

		# compute a merged table
		table_list = []
		env = self
		while 1:
			table_list.insert(0, env.table)
			try: env = env.parent
			except AttributeError: break
		merged_table = {}
		for table in table_list:
			merged_table.update(table)

		keys = merged_table.keys()
		keys.sort()
		for k in keys: file.write('%s = %r\n' % (k, merged_table[k]))
		file.close()

	def load(self, filename):
		"Retrieve the variables from a file"
		tbl = self.table
		file = open(filename, 'r')
		code = file.read()
		file.close()
		for m in re_imp.finditer(code):
			g = m.group
			tbl[g(2)] = eval(g(3))
		Logs.debug('env: %s' % str(self.table))

	def get_destdir(self):
		"return the destdir, useful for installing"
		if self.__getitem__('NOINSTALL'): return ''
		return Options.options.destdir

