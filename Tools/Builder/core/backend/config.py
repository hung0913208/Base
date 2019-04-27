from . import Backend
import platform

class Config(Backend):
    def __init__(self, **kwargs):
        super(Config, self).__init__()

        # @NOTE: update information of system
        self._kwargs = kwargs
        self._system = {
            'os': platform.system(),
            'cpu': platform.machine(),
            'architecture': platform.architecture()
        }

        # @NOTE: update information of cpu
        if self._system['os'] == 'Linux':
            self._system['distribute'] = platform.linux_distribution()[0]

            with open('/proc/cpuinfo') as fp:
                self._processors = []

                processor = None
                for line in fp.readlines():
                    if len(line) <= 1:
                        continue
                    key, value = line.split(':')

                    key = key.replace('\t', '')
                    value = value.replace('\n', '')

                    if key == 'processor':
                        if (not processor is None):
                            self._processors.append(processor)

                        processor = {}
                    else:
                        if key in ['flags', 'bugs']:
                            value = value.split(' ')
                        processor[key] = value

    @property
    def os(self):
        return self._system['os']

    def define(self):
        @self.rule
        def config_setting(name, values):
            def callback(**kwargs):
                is_okey = True

                # @NOTE: check system config with request from BUILD and
                # WORKSPACE
                for item in values:
                    key, value = item

                    if key in self._kwargs:
                        if value != self._kwargs[key]:
                            is_okey = False
                    elif key in self._system:
                        if value != self._kwargs[key]:
                            is_okey = False

                    if is_okey is False:
                        break

                # @NOTE: if everything is okey, we will turn flag 'name' on
                if is_okey is True:
                    self._manager.enable_flags(name)

            node = { 'callback': callback }
            self._manager.add_to_dependency_tree(name, node, None)

    def check(self):
        pass
