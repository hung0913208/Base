import platform

class System(object):
    _instance = None

    def __new__(self, **kwargs):
        if System._instance is None:
            System._instance = super(System, self).__new__(self)
        return System._instance

    def __init__(self, **kwargs):
        super(System, self).__init__()
        self._os = {}

    def select(self, name):
        if name in self._os[platform.system().lower()]:
            return self._os[platform.system().lower()][name]
        else:
            return None

    def config_with(self, os=None, apply=None):
        os = os.lower()

        if not os in self._os:
            self._os[os] = {}

        if isinstance(apply, dict):
            for key in apply:
                self._os[os][key] = apply[key]
        else:
            raise AssertionError('\'apply\' must be a dict')