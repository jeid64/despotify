#!/usr/bin/env python
# encoding: utf-8
# Carlos Rafael Giani, 2006

"""
Unit tests run in the shutdown() method, and for c/c++ programs

One should NOT have to give parameters to programs to execute

In the shutdown method, add the following code:

	>>> def shutdown():
	...	ut = UnitTest.unit_test()
	...	ut.run()
	...	ut.print_results()


Each object to use as a unit test must be a program and must have X{obj.unit_test=1}
"""
import os, sys
import Build, TaskGen, Utils, Options, Logs
import pproc

class unit_test(object):
	"Unit test representation"
	def __init__(self):
		self.returncode_ok = 0		# Unit test returncode considered OK. All returncodes differing from this one
						# will cause the unit test to be marked as "FAILED".

		# The following variables are filled with data by run().

		# print_results() uses these for printing the unit test summary,
		# but if there is need for direct access to the results,
		# they can be retrieved here, after calling run().

		self.num_tests_ok = 0		# Number of successful unit tests
		self.num_tests_failed = 0	# Number of failed unit tests
		self.num_tests_err = 0		# Tests that have not even run
		self.total_num_tests = 0	# Total amount of unit tests
		self.max_label_length = 0	# Maximum label length (pretty-print the output)

		self.unit_tests = {}		# Unit test dictionary. Key: the label (unit test filename relative
						# to the build dir), value: unit test filename with absolute path
		self.unit_test_results = {}	# Dictionary containing the unit test results.
						# Key: the label, value: result (true = success false = failure)
		self.unit_test_erroneous = {}	# Dictionary indicating erroneous unit tests.
						# Key: the label, value: true = unit test has an error  false = unit test is ok
		self.change_to_testfile_dir = False #True if the test file needs to be executed from the same dir
		self.want_to_see_test_output = False #True to see the stdout from the testfile (for example check suites)
		self.want_to_see_test_error = False #True to see the stderr from the testfile (for example check suites)
		self.run_if_waf_does = 'check' #build was the old default

	def run(self):
		"Run the unit tests and gather results (note: no output here)"

		self.num_tests_ok = 0
		self.num_tests_failed = 0
		self.num_tests_err = 0
		self.total_num_tests = 0
		self.max_label_length = 0

		self.unit_tests = {}
		self.unit_test_results = {}
		self.unit_test_erroneous = {}

		# If waf is not building, don't run anything
		if not Options.commands[self.run_if_waf_does]: return

		# Gather unit tests to call
		for obj in Build.bld.all_task_gen:
			unit_test = getattr(obj,'unit_test', '')
			if unit_test and 'program' in obj.features:
				try:
					output = obj.path
					filename = os.path.join(output.abspath(obj.env), obj.target)
					srcdir = output.abspath()
					label = os.path.join(output.bldpath(obj.env), obj.target)
					self.max_label_length = max(self.max_label_length, len(label))
					self.unit_tests[label] = (filename, srcdir)
				except KeyError:
					pass
		self.total_num_tests = len(self.unit_tests)
		# Now run the unit tests
		Utils.pprint('GREEN', 'Running the unit tests')
		count = 0
		result = 1

		for label, file_and_src in self.unit_tests.iteritems():
			filename = file_and_src[0]
			srcdir = file_and_src[1]
			count += 1
			line = Build.bld.progress_line(count, self.total_num_tests, Logs.colors.GREEN, Logs.colors.NORMAL)
			if Options.options.progress_bar and line:
				sys.stdout.write(line)
				sys.stdout.flush()
			try:
				kwargs = {}
				if self.change_to_testfile_dir:
					kwargs['cwd'] = srcdir
				if not self.want_to_see_test_output:
					kwargs['stdout'] = pproc.PIPE  # PIPE for ignoring output
				if not self.want_to_see_test_error:
					kwargs['stderr'] = pproc.PIPE  # PIPE for ignoring output
				pp = pproc.Popen(filename, **kwargs)
				pp.wait()

				result = int(pp.returncode == self.returncode_ok)

				if result:
					self.num_tests_ok += 1
				else:
					self.num_tests_failed += 1

				self.unit_test_results[label] = result
				self.unit_test_erroneous[label] = 0
			except OSError:
				self.unit_test_erroneous[label] = 1
				self.num_tests_err += 1
			except KeyboardInterrupt:
				pass
		if Options.options.progress_bar: sys.stdout.write(Logs.colors.cursor_on)

	def print_results(self):
		"Pretty-prints a summary of all unit tests, along with some statistics"

		# If waf is not building, don't output anything
		if not Options.commands[self.run_if_waf_does]: return

		p = Utils.pprint
		# Early quit if no tests were performed
		if self.total_num_tests == 0:
			p('YELLOW', 'No unit tests present')
			return
		p('GREEN', 'Running unit tests')
		print

		for label, filename in self.unit_tests.iteritems():
			err = 0
			result = 0

			try: err = self.unit_test_erroneous[label]
			except KeyError: pass

			try: result = self.unit_test_results[label]
			except KeyError: pass

			n = self.max_label_length - len(label)
			if err: n += 4
			elif result: n += 7
			else: n += 3

			line = '%s %s' % (label, '.' * n)

			print line,
			if err: p('RED', 'ERROR')
			elif result: p('GREEN', 'OK')
			else: p('YELLOW', 'FAILED')

		percentage_ok = float(self.num_tests_ok) / float(self.total_num_tests) * 100.0
		percentage_failed = float(self.num_tests_failed) / float(self.total_num_tests) * 100.0
		percentage_erroneous = float(self.num_tests_err) / float(self.total_num_tests) * 100.0

		print '''
Successful tests:      %i (%.1f%%)
Failed tests:          %i (%.1f%%)
Erroneous tests:       %i (%.1f%%)

Total number of tests: %i
''' % (self.num_tests_ok, percentage_ok, self.num_tests_failed, percentage_failed,
		self.num_tests_err, percentage_erroneous, self.total_num_tests)
		p('GREEN', 'Unit tests finished')

