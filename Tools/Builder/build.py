#!/usr/bin/python3
#
# Project: build
# Description: this is a very simple build tool which imitates from Bazel
#

import subprocess
import argparse
import shutil
import glob
import sys
import os

from core import *
from languages import *
from plugins import *


class Build(Plugin):
    def __init__(self, root, rebuild=False, **kwargs):
        super(Build, self).__init__()
        if root[0] != '/':
            root = '%s/%s' % (os.getcwd(), root)

        # @NOTE: load optional parameters
        self._output = kwargs.get('build') or ('%s/build' % root)
        self._root = root

        # @NOTE: force Builder to remove and build again
        if rebuild is False and os.path.exists(self._output):
            shutil.rmtree(self._output)

        # @NOTE: load our builder's objection
        self._manager = Manager(root, **kwargs)
        self._manager.install([
            self,
            Git(**kwargs),
            Http(**kwargs)
        ])
        self._manager.support([
            C(**kwargs),
            D(**kwargs)
        ])

    def prepare(self):
        """ prepare everything before building this repository
        """
        workspace = '%s/.workspace' % self._root

        try:
            if os.path.exists(workspace):
                Logger.debug("found .workspace file %s -> going to parse this file now" % workspace)

                if self.parse_workspace_file(workspace) is False:
                    return False
                else:
                    return self._manager.perform(root=self._root, output=self._output)
            else:
                return False
        except Exception as error:
            Logger.error('Got an exception: %s -> going to teardown this project' % str(error))
            Logger.exception()
            self._manager.teardown(self._root)

    def derived(self):
        """ list derived classes of Build
        """
        result = super(Build, self).derived()

        if not result is None:
            result.append('Build')
        return result

    def define(self):
        pass

    @staticmethod
    def run(command):
        try:
            cmd = subprocess.Popen(command.split(' '),
                                   stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE)
            error_console = cmd.stderr.read()
            output_console = cmd.stdout.read()

            cmd.communicate()
            cmd.wait()

            return True
        except Exception as error:
            Logger.error('Error when perform %s: %s' % (command, str(error)))
            return False

    def analyze(self, path=None):
        """ analyze a repository
        """
        path = self._root if path is None else path
        need_performing = False

        try:
            for path in glob.glob('%s/*' % path):
                if os.path.isdir(path):
                    exclusive = '%s/.excluse' % path
                    build = '%s/.build' % path

                    if os.path.exists(exclusive):
                        Logger.debug("found .excluse file %s -> going to run it now" % exclusive)

                        if Build.run(exclusive) is False:
                            return False
                        else:
                            continue
                    elif os.path.exists(build) and not os.path.exists(exclusive):
                        Logger.debug("found .build file %s -> going to parse this file now" % build)

                        if self.parse_build_file(build) is False:
                            return False
                        elif self.analyze(path) is False:
                            return False
                    elif self.analyze(path) is False:
                        return False
            else:
                return True
        except Exception as error:

            # @NOTE: got an exception teardown now
            Logger.error('Got an exception: %s -> going to teardown this project' % str(error))
            Logger.exception()
            return False

    def build(self):
        """ build a repository
        """
        return self._manager.perform(root=self._root, output=self._output)

    def release(self):
        self._manager.teardown(self._root, self._output)

    def parse_workspace_file(self, workspace_file):
        """ parse file .workspace
        """

        # @NOTE: we must use dir that contains the 'workspace_file' since .workspace
        # usually define its resouce with this dir
        self._manager.set_current_dir('workspace', os.path.dirname(workspace_file))
        with open(workspace_file) as fp:
            source = fp.read()

        for item in iter_function(source):
            function = self._manager.find_function(item['function'], 'workspace')
            variables = {}

            if 'variables' in item:
                for var in item['variables']:
                    if isinstance(var, dict):
                        variables[list(var.keys())[0]] = list(var.values())[0]

            if function is None:
                raise AssertionError('can\'t determine %s' % item['function'])
            else:
                function(**variables)

        return True

    def parse_build_file(self, build_file):
        """ parse file .build
        """

        # @NOTE: we must use dir that contains the 'build_file' since .build
        # usually define its resouce with this dir
        self._manager.set_current_dir('build', os.path.dirname(build_file))

        with open(build_file) as fp:
            source = fp.read()

        for item in iter_function(source):
            function = self._manager.find_function(item['function'], 'build')
            variables = {}

            if 'variables' in item:
                for var in item['variables']:
                    if isinstance(var, dict):
                        variables[list(var.keys())[0]] = list(var.values())[0]

            if function is None:
                Logger.warning('can\'t determine %s -> ignore it now' % item['function'])
                continue
            else:
                function(**variables)
        return True


class Serving(Plugin):
    def __init__(self, **kwargs):
        super(Serving, self).__init__()
        self._error = False

    @property
    def error(self):
        return self._error

    def define(self):
        pass

    def check(self):
        pass


def parse():
    parser = argparse.ArgumentParser()
    parser.add_argument('--rebuild', type=int, default=1,
                        help='build everything from scratch')
    parser.add_argument('--silence', type=int, default=0,
                        help='make Builder more quieted')
    parser.add_argument('--root', type=str, default=os.getcwd(),
                        help='where project is defined')
    parser.add_argument('--debug', type=int, default=0,
                        help='enable debug info')
    parser.add_argument('--stacktrace', type=str, default=None,
                        help='enable stacktrace info')
    parser.add_argument('--use_package_management', type=int, default=1,
                        help='enable using package management')
    parser.add_argument('--auto_update_packages', type=int,
                        default=0, help='enable auto update packages')
    parser.add_argument('--on_serving', type=int, default=0,
                        help='use Builder on serving mode when they receive '
                             'tasks from afar')
    parser.add_argument('--mode', type=int, default=0,
                        help='select mode of this process if on_serving is on')
    return parser.parse_args()

if __name__ == '__main__':
    flags = parse()

    if flags.debug != 0 and flags.silence == 0:
        # @NOTE: by default we only use showing stacktrace if flag debug is on

        Logger.set_level(DEBUG)
        if not flags.stacktrace is None:
            if flags.stacktrace.lower() == 'debug':
                Logger.set_stacktrace(DEBUG)
            elif flags.stacktrace.lower() == 'warning':
                Logger.set_stacktrace(WARN)
            elif flags.stacktrace.lower() == 'error':
                Logger.set_stacktrace(FATAL)

    Logger.silence(flags.silence == 1)

    if flags.on_serving == 0:
        builder = Build(flags.root,
                        auto_update_packages=flags.auto_update_packages==1,
                        use_package_management=flags.use_package_management==1,
                        silence=(flags.silence == 1),
                        rebuild=(flags.rebuild == 1))
        code = 255

        if builder.prepare() is False:
            Logger.debug('prepare fail -> exit with code 255')
        elif builder.analyze() is False:
            Logger.debug('build fail -> exit with code 255')
        elif builder.build() is False:
            Logger.debug('build fail -> exit with code 255')
        else:
            code = 0

        builder.release()
        sys.exit(code)
    else:
        recepter = Serving(root=flags.root,
                           auto_update_packages=flags.auto_update_packages==1,
                           use_package_management=flags.use_package_management==1,
                           silence=(flags.silence == 1))
        sys.exit(255 if recepter.error is True else 0)
