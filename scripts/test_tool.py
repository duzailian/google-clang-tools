#!/usr/bin/env python
# Copyright (c) 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Test harness for chromium clang tools."""

import difflib
import glob
import json
import os
import os.path
import shutil
import subprocess
import sys


def _RunGit(args):
  if sys.platform == 'win32':
    args = ['git.bat'] + args
  else:
    args = ['git'] + args
  subprocess.check_call(args)


def _GenerateCompileCommands(files, include_paths):
  """Returns a JSON string containing a compilation database for the input."""
  # Note: in theory, backslashes in the compile DB should work but the tools
  # that write compile DBs and the tools that read them don't agree on the
  # escaping convention: https://llvm.org/bugs/show_bug.cgi?id=19687
  files = [f.replace('\\', '/') for f in files]
  include_path_flags = ' '.join('-I %s' % include_path.replace('\\', '/')
                                for include_path in include_paths)
  return json.dumps([{'directory': '.',
                      'command': 'clang++ -std=c++11 -fsyntax-only %s -c %s' % (
                          include_path_flags, f),
                      'file': f} for f in files], indent=2)


def _NumberOfTestsToString(tests):
  """Returns an English describing the number of tests."""
  return '%d test%s' % (tests, 's' if tests != 1 else '')


def _RunToolAndApplyEdits(tools_clang_scripts_directory,
                          tool_to_test,
                          test_directory_for_tool,
                          actual_files):
  try:
    # Stage the test files in the git index. If they aren't staged, then
    # run_tool.py will skip them when applying replacements.
    args = ['add']
    args.extend(actual_files)
    _RunGit(args)

    # Launch the following pipeline:
    #     run_tool.py ... | extract_edits.py | apply_edits.py ...
    args = ['python',
            os.path.join(tools_clang_scripts_directory, 'run_tool.py'),
            tool_to_test,
            test_directory_for_tool]
    args.extend(actual_files)
    run_tool = subprocess.Popen(args, stdout=subprocess.PIPE)

    args = ['python',
            os.path.join(tools_clang_scripts_directory, 'extract_edits.py')]
    extract_edits = subprocess.Popen(args, stdin=run_tool.stdout,
                                     stdout=subprocess.PIPE)

    args = ['python',
            os.path.join(tools_clang_scripts_directory, 'apply_edits.py'),
            test_directory_for_tool]
    apply_edits = subprocess.Popen(args, stdin=extract_edits.stdout,
                                   stdout=subprocess.PIPE)

    # Wait for the pipeline to finish running + check exit codes.
    stdout, _ = apply_edits.communicate()
    for process in [run_tool, extract_edits, apply_edits]:
      process.wait()
      if process.returncode != 0:
        print "Failure while running the tool."
        return process.returncode

    # Reformat the resulting edits via: git cl format.
    args = ['cl', 'format']
    args.extend(actual_files)
    _RunGit(args)

    return 0

  finally:
    # No matter what, unstage the git changes we made earlier to avoid polluting
    # the index.
    args = ['reset', '--quiet', 'HEAD']
    args.extend(actual_files)
    _RunGit(args)


def main(argv):
  if len(argv) < 1:
    print 'Usage: test_tool.py <clang tool>'
    print '  <clang tool> is the clang tool to be tested.'
    sys.exit(1)

  tool_to_test = argv[0]
  print '\nTesting %s\n' % tool_to_test
  tools_clang_scripts_directory = os.path.dirname(os.path.realpath(__file__))
  tools_clang_directory = os.path.dirname(tools_clang_scripts_directory)
  test_directory_for_tool = os.path.join(
      tools_clang_directory, tool_to_test, 'tests')
  compile_database = os.path.join(test_directory_for_tool,
                                  'compile_commands.json')
  source_files = glob.glob(os.path.join(test_directory_for_tool,
                                        '*-original.cc'))
  actual_files = ['-'.join([source_file.rsplit('-', 1)[0], 'actual.cc'])
                  for source_file in source_files]
  expected_files = ['-'.join([source_file.rsplit('-', 1)[0], 'expected.cc'])
                    for source_file in source_files]
  include_paths = []
  include_paths.append(
      os.path.realpath(os.path.join(tools_clang_directory, '../..')))
  # Many gtest headers expect to have testing/gtest/include in the include
  # search path.
  include_paths.append(
      os.path.realpath(os.path.join(tools_clang_directory,
                                    '../..',
                                    'testing/gtest/include')))

  if len(actual_files) == 0:
    print 'Tool "%s" does not have compatible test files.' % tool_to_test
    return 1

  # Set up the test environment.
  for source, actual in zip(source_files, actual_files):
    shutil.copyfile(source, actual)
  # Generate a temporary compilation database to run the tool over.
  with open(compile_database, 'w') as f:
    f.write(_GenerateCompileCommands(actual_files, include_paths))

  # Run the tool.
  exitcode = _RunToolAndApplyEdits(tools_clang_scripts_directory, tool_to_test,
                                   test_directory_for_tool, actual_files)
  if (exitcode != 0):
    return exitcode

  # Compare actual-vs-expected results.
  passed = 0
  failed = 0
  for expected, actual in zip(expected_files, actual_files):
    print '[ RUN      ] %s' % os.path.relpath(actual)
    expected_output = actual_output = None
    with open(expected, 'r') as f:
      expected_output = f.readlines()
    with open(actual, 'r') as f:
      actual_output = f.readlines()
    if actual_output != expected_output:
      failed += 1
      for line in difflib.unified_diff(expected_output, actual_output,
                                       fromfile=os.path.relpath(expected),
                                       tofile=os.path.relpath(actual)):
        sys.stdout.write(line)
      print '[  FAILED  ] %s' % os.path.relpath(actual)
      # Don't clean up the file on failure, so the results can be referenced
      # more easily.
      continue
    print '[       OK ] %s' % os.path.relpath(actual)
    passed += 1
    os.remove(actual)

  if failed == 0:
    os.remove(compile_database)

  print '[==========] %s ran.' % _NumberOfTestsToString(len(source_files))
  if passed > 0:
    print '[  PASSED  ] %s.' % _NumberOfTestsToString(passed)
  if failed > 0:
    print '[  FAILED  ] %s.' % _NumberOfTestsToString(failed)
    return 1


if __name__ == '__main__':
  sys.exit(main(sys.argv[1:]))
