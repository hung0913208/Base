import os
import shutil
import subprocess
import tempfile


class Language(object):
    def __init__(self):
        super(Language, self).__init__()
        self._manager = None
        self._performer = None

    def derived(self):
        return ['Language']

    def mount(self, mount_point):
        if mount_point.type == 'Manager':
            self._manager = mount_point

            if self.check() is False:
                raise AssertionError('there was an error when perform `check()`')
        elif mount_point.type == 'Performer':
            self._performer = mount_point
        else:
            raise AssertionError('only support \'Performer\' and \'Manager\'')

    def make(self, **kwargs):
        raise AssertionError('this is a virtual class')

    def check(self):
        raise AssertionError('this is a virtual class')

    def define(self):
        raise AssertionError('this is a virtual class')

    def rule(self, function):
        def wrapping(*args, **kwargs):
            return function(*args, **kwargs)

        self._manager.add_rule(self, function)
        return wrapping

    def add_to_dependencies(self, name, node, deps):
        if not deps is None:
            if isinstance(deps, list):
                if self._manager.add_to_dependency_tree(name, node, deps) is False:
                    # @TODO: error when adding a dependency since we require
                    # tree must be created layer to layer, step by step
                    return False
                else:
                    pass
            return True
        else:
            return self._manager.create_new_node(name, node)

    @staticmethod
    def test_compile(format, compile, input_suffix, flags, source, output):
        if output is None or compile is None or source is None or format is None:
            return False
        elif flags is None:
            flags = ""
        elif isinstance(flags, list):
            flags = ' '.join(flags)

        # @NOTE: prepare source code for testing
        fd, input = tempfile.mkstemp(suffix=input_suffix)
        os.write(fd, source.encode())
        os.fsync(fd)
        os.close(fd)

        # @NOTE: prepare command for testing
        cmd = []
        for item in (format % (compile, flags, input, output)).split(' '):
            if len(item) > 0:
                cmd.append(item)

        if len(cmd) == 0:
            return False
        build = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

        error_console = build.stderr.read()
        output_console = build.stdout.read()

        os.remove(input)
        if not build.returncode is None and build.returncode != 0:
            return False
        elif len(error_console) > 0:
            return False
        elif os.path.exists(output) is False:
            return False
        else:
            test = subprocess.Popen(output)

            error_console = build.stderr.read()
            output_console = build.stdout.read()

            # @NOTE: okey, everything is done now, remove input and decide testing
            # is okey or not
            os.remove(output)
            if test.returncode is None or test.returncode == 0:
                return True
            else:
                return False

    @staticmethod
    def __which__(file_name):
        for path in os.environ["PATH"].split(os.pathsep):
            full_path = os.path.join(path, file_name)
            if os.path.exists(full_path) and os.access(full_path, os.X_OK):
                return full_path
        return None

    @staticmethod
    def which(names, verify=None, count=False):
        if isinstance(names, list):
            result = {}
            count_result = 0
            for name in names:
                result[name] = Language.__which__(name)

                if not result[name] is None:
                    count_result += 1

            if verify is None or not callable(verify):
                return count_result if count else result
            else:
                for name in names:
                    if result[name] is None:
                        continue
                    elif verify(result[name]) is True:
                        return result[name]
                else:
                    return None
        else:
            result = Language.__which__(names)

            if verify is None or not callable(names):
                return result
            elif verify(names, result) is True:
                return result
            else:
                return None


from .c import C
from .d import D

__all__ = ['Language', 'C', 'D']
