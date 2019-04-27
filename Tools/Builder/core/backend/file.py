from . import Backend
import glob as globimp
import os

def is_file_existed(file_name, specific_paths=[]):
    for path in os.environ["PATH"].split(os.pathsep):
        full_path = os.path.join(path, file_name)

        if os.path.exists(full_path) and os.access(full_path, os.X_OK):
            return full_path

    for path in specific_paths:
        full_path = os.path.join(path, file_name)

        if os.path.exists(full_path) and os.access(full_path, os.X_OK):
            return full_path
    return None


class File(Backend):
    """ File is a class to manage file on project
    """

    def __init__(self, **kwargs):
        super(File, self).__init__()
        self._path = kwargs.get('path') or []

    def scan(self, name):
        result = is_file_existed(name, self._path)

        if not result is None:
            return result

    def define(self):
        @self.rule
        def glob(root, output, variables, simple_path=False, **kwargs):
            # @NOTE: since we intend to use glob like an interface of glob.glob
            # we will parse everything from scratch now and don't provide any
            # rule because it's our system call
            result = []

            # @NOTE: check if we have more than 1 input
            if len(variables) > 1:
                raise AssertionError('\'glob\' only have 1 input')

            # @NOTE: expand template -> filepath
            for template in variables[0]:
                for path in globimp.glob('%s/%s' % (root, template)):
                    if simple_path is True:
                        result.append(path[len(root) + 1:])
                    else:
                        result.append(path)
            return result

    def check(self):
        pass