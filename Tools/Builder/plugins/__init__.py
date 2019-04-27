class Plugin(object):
    def __init__(self):
        super(Plugin, self).__init__()
        self._manager = None

    def derived(self):
        return ['Plugin']

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


from .git import Git
from .http import Http

__all__ = ['Plugin', 'Git', 'Http']
