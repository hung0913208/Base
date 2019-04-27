class Backend(object):
    def __init__(self):
        super(Backend, self).__init__()
        self._manager = None

    def mount(self, manager):
        self._manager = manager

    def define(self):
        raise AssertionError('this is virtual class')

    def check(self):
        raise AssertionError('this is virtual class')

    def rule(self, function):
        def wrapping(*args, **kwargs):
            return function(*args, **kwargs)

        self._manager.add_rule(self, function, use_on_workspace=True)
        return wrapping


from .package import Package
from .config import Config
from .hook import Hook
from .file import File

__all__ = ['Backend', 'Package', 'Config', 'File', 'Hook']