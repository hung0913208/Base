from ..utils import bash_iterate, run_on_parallel
from ..logger import Logger
from . import Backend

class Hook(Backend):
    def __init__(self, **kwargs):
        super(Hook, self).__init__()
        self._timeout = kwargs.get('timeout') or 300
        self._failing_callbacks = {
            'compiling': [],
            'linking': [],
            'testing': [],
            'archiving': [],
            'invoking': [],
        }

        self._passing_callbacks = {
            'compiling': [],
            'linking': [],
            'testing': [],
            'archiving': [],
            'invoking': [],
        }

    def define(self):
        # @NOTE: we will define every rule from here

        @self.rule
        def on_compiling_pass(name, deps=None, cmd=None, script=None, exclude=None,
                              on_parallel=True, timeout=None):
            def wrapper(root, workspace, **kwargs):
                if not cmd is None and script is None:
                    self._passing_callbacks['compiling'].append({
                        cmd[0]: cmd[1],
                        'name': name,
                        'timeout': timeout or self._timeout,
                        'exclude': exclude,
                        'parallel': True if on_parallel != 0 else False
                    })
                elif cmd is None and not script is None:
                    self._passing_callbacks['compiling'].append({
                        'python': script,
                        'name': name,
                        'timeout': timeout or self._timeout,
                        'exclude': exclude,
                        'parallel': True if on_parallel != 0 else False
                    })
                elif cmd is None and script is None:
                    raise AssertionError('`cmd` and `script` mustn\'t be None '
                                         'at the same time')
                else:
                    raise AssertionError('`cmd` and `script` must be used one '
                                         'at the same time')

            node = { 'callback': wrapper }
            self._manager.add_to_dependency_tree(name, node, None)


        @self.rule
        def on_testing_pass(name, deps=None, cmd=None, script=None, exclude=None,
                             on_parallel=True, timeout=None):
            def wrapper(root, workspace, **kwargs):
                if not cmd is None and script is None:
                    self._passing_callbacks['testing'].append({
                        cmd[0]: cmd[1],
                        'name': name,
                        'timeout': timeout or self._timeout,
                        'exclude': exclude,
                        'parallel': True if on_parallel != 0 else False
                    })
                elif cmd is None and not script is None:
                    self._passing_callbacks['testing'].append({
                        'python': script,
                        'name': name,
                        'timeout': timeout or self._timeout,
                        'exclude': exclude,
                        'parallel': True if on_parallel != 0 else False
                    })
                elif cmd is None and script is None:
                    raise AssertionError('`cmd` and `script` mustn\'t be None '
                                         'at the same time')
                else:
                    raise AssertionError('`cmd` and `script` must be used one '
                                         'at the same time')

            node = { 'callback': wrapper }
            self._manager.add_to_dependency_tree(name, node, None)

        @self.rule
        def on_linking_pass(name, deps=None, cmd=None, script=None, exclude=None,
                            on_parallel=True, timeout=None):
            def wrapper(root, workspace, **kwargs):
                if not cmd is None and script is None:
                    self._passing_callbacks['linking'].append({
                        cmd[0]: cmd[1],
                        'name': name,
                        'timeout': timeout or self._timeout,
                        'exclude': exclude,
                        'parallel': True if on_parallel != 0 else False
                    })
                elif cmd is None and not script is None:
                    self._passing_callbacks['linking'].append({
                        'python': script,
                        'name': name,
                        'timeout': timeout or self._timeout,
                        'exclude': exclude,
                        'parallel': True if on_parallel != 0 else False
                    })
                elif cmd is None and script is None:
                    raise AssertionError('`cmd` and `script` mustn\'t be None '
                                         'at the same time')
                else:
                    raise AssertionError('`cmd` and `script` must be used one '
                                         'at the same time')

            node = { 'callback': wrapper }
            self._manager.add_to_dependency_tree(name, node, None)

        @self.rule
        def on_archiving_pass(name, deps=None, cmd=None, script=None, exclude=None,
                            on_parallel=True, timeout=None):
            def wrapper(root, workspace, **kwargs):
                if not cmd is None and script is None:
                    self._passing_callbacks['archiving'].append({
                        cmd[0]: cmd[1],
                        'name': name,
                        'timeout': timeout or self._timeout,
                        'exclude': exclude,
                        'parallel': True if on_parallel != 0 else False
                    })
                elif cmd is None and not script is None:
                    self._passing_callbacks['archiving'].append({
                        'python': script,
                        'name': name,
                        'timeout': timeout or self._timeout,
                        'exclude': exclude,
                        'parallel': True if on_parallel != 0 else False
                    })
                elif cmd is None and script is None:
                    raise AssertionError('`cmd` and `script` mustn\'t be None '
                                         'at the same time')
                else:
                    raise AssertionError('`cmd` and `script` must be used one '
                                         'at the same time')

            node = { 'callback': wrapper }
            self._manager.add_to_dependency_tree(name, node, None)

        @self.rule
        def on_invoking_pass(name, deps=None, cmd=None, script=None, exclude=None,
                             on_parallel=True, timeout=None):
            def wrapper(root, workspace, **kwargs):
                if not cmd is None and script is None:
                    self._passing_callbacks['invoking'].append({
                        cmd[0]: cmd[1],
                        'name': name,
                        'timeout': timeout or self._timeout,
                        'exclude': exclude,
                        'parallel': True if on_parallel != 0 else False
                    })
                elif cmd is None and not script is None:
                    self._passing_callbacks['invoking'].append({
                        'python': script,
                        'name': name,
                        'timeout': timeout or self._timeout,
                        'exclude': exclude,
                        'parallel': True if on_parallel != 0 else False
                    })
                elif cmd is None and script is None:
                    raise AssertionError('`cmd` and `script` mustn\'t be None '
                                         'at the same time')
                else:
                    raise AssertionError('`cmd` and `script` must be used one '
                                         'at the same time')

            node = { 'callback': wrapper }
            self._manager.add_to_dependency_tree(name, node, None)

        @self.rule
        def on_compiling_fail(name, deps=None, cmd=None, script=None, exclude=None,
                              on_parallel=True, timeout=None):
            def wrapper(root, workspace, **kwargs):
                if not cmd is None and script is None:
                    self._failing_callbacks['compiling'].append({
                        cmd[0]: cmd[1],
                        'name': name,
                        'timeout': timeout or self._timeout,
                        'exclude': exclude,
                        'parallel': True if on_parallel != 0 else False
                    })
                elif cmd is None and not script is None:
                    self._failing_callbacks['compiling'].append({
                        'python': script,
                        'timeout': timeout or self._timeout,
                        'name': name,
                        'exclude': exclude,
                        'parallel': True if on_parallel != 0 else False
                    })
                elif cmd is None and script is None:
                    raise AssertionError('`cmd` and `script` mustn\'t be None '
                                         'at the same time')
                else:
                    raise AssertionError('`cmd` and `script` must be used one '
                                         'at the same time')

            node = { 'callback': wrapper }
            self._manager.add_to_dependency_tree(name, node, None)

        @self.rule
        def on_testing_fail(name, deps=None, cmd=None, script=None, exclude=None,
                            on_parallel=True, timeout=None):
            def wrapper(root, workspace, **kwargs):
                if not cmd is None and script is None:
                    self._failing_callbacks['testing'].append({
                        cmd[0]: cmd[1],
                        'name': name,
                        'timeout': timeout or self._timeout,
                        'exclude': exclude,
                        'parallel': True if on_parallel != 0 else False
                    })
                elif cmd is None and not script is None:
                    self._failing_callbacks['testing'].append({
                        'python': script,
                        'name': name,
                        'timeout': timeout or self._timeout,
                        'exclude': exclude,
                        'parallel': True if on_parallel != 0 else False
                    })
                elif cmd is None and script is None:
                    raise AssertionError('`cmd` and `script` mustn\'t be None '
                                         'at the same time')
                else:
                    raise AssertionError('`cmd` and `script` must be used one '
                                         'at the same time')

            node = { 'callback': wrapper }
            self._manager.add_to_dependency_tree(name, node, None)

        @self.rule
        def on_linking_fail(name, deps=None, cmd=None, script=None, exclude=None,
                            on_parallel=True, timeout=None):
            def wrapper(root, workspace, **kwargs):
                if not cmd is None and script is None:
                    self._failing_callbacks['linking'].append({
                        cmd[0]: cmd[1],
                        'name': name,
                        'timeout': timeout or self._timeout,
                        'exclude': exclude,
                        'parallel': True if on_parallel != 0 else False
                    })
                elif cmd is None and not script is None:
                    self._failing_callbacks['linking'].append({
                        'python': script,
                        'name': name,
                        'timeout': timeout or self._timeout,
                        'exclude': exclude,
                        'parallel': True if on_parallel != 0 else False
                    })
                elif cmd is None and script is None:
                    raise AssertionError('`cmd` and `script` mustn\'t be None '
                                         'at the same time')
                else:
                    raise AssertionError('`cmd` and `script` must be used one '
                                         'at the same time')

            node = { 'callback': wrapper }
            self._manager.add_to_dependency_tree(name, node, None)

        @self.rule
        def on_archiving_fail(name, deps=None, cmd=None, script=None, exclude=None,
                              on_parallel=True, timeout=None):
            def wrapper(root, workspace, **kwargs):
                if not cmd is None and script is None:
                    self._failing_callbacks['archiving'].append({
                        cmd[0]: cmd[1],
                        'name': name,
                        'timeout': timeout or self._timeout,
                        'exclude': exclude,
                        'parallel': True if on_parallel != 0 else False
                    })
                elif cmd is None and not script is None:
                    self._failing_callbacks['archiving'].append({
                        'python': script,
                        'name': name,
                        'timeout': timeout or self._timeout,
                        'exclude': exclude,
                        'parallel': True if on_parallel != 0 else False
                    })
                elif cmd is None and script is None:
                    raise AssertionError('`cmd` and `script` mustn\'t be None '
                                         'at the same time')
                else:
                    raise AssertionError('`cmd` and `script` must be used one '
                                         'at the same time')

            node = { 'callback': wrapper }
            self._manager.add_to_dependency_tree(name, node, None)

        @self.rule
        def on_invoking_fail(name, deps=None, cmd=None, script=None, exclude=None,
                             on_parallel=True, timeout=None):
            def wrapper(root, workspace, **kwargs):
                if not cmd is None and script is None:
                    self._failing_callbacks['invoking'].append({
                        cmd[0]: cmd[1],
                        'name': name,
                        'timeout': timeout or self._timeout,
                        'exclude': exclude,
                        'parallel': True if on_parallel != 0 else False
                    })
                elif cmd is None and not script is None:
                    self._failing_callbacks['invoking'].append({
                        'python': script,
                        'name': name,
                        'timeout': timeout or self._timeout,
                        'exclude': exclude,
                        'parallel': True if on_parallel != 0 else False
                    })
                elif cmd is None and script is None:
                    raise AssertionError('`cmd` and `script` mustn\'t be None '
                                         'at the same time')
                else:
                    raise AssertionError('`cmd` and `script` must be used one '
                                         'at the same time')

            node = { 'callback': wrapper }
            self._manager.add_to_dependency_tree(name, node, None)

        # @NOTE: we will define every hook from here
        def run_bash_script(name, target, script, code=0, background=False,
                            timeout=None):
            Logger.debug("perform bash script from rule `%s`" % name)

            ret = bash_iterate(script, name=name, background=background,
                               workspace=self._manager._output,
                               root=self._manager._root,
                               error_code=code,
                               timeout=timeout,
                               target=target)
            if ret is False:
                self._manager.found_bug('Error when run script `%s` '
                                       'with target `%s`' % (name, target))
            else:
                Logger.debug("done bash script from rule `%s`" % name)

        @self._manager.hook('compiling', False)
        def perform_when_compiling_fail(output, code):
            for callback in self._failing_callbacks['compiling']:
                if 'bash' in callback:
                    if callback.get('parallel') is True:
                        run_on_parallel(run_bash_script,
                                        target=output,
                                        code=code,
                                        script=callback['bash'],
                                        name=callback['name'],
                                        timeout=callback['timeout'],
                                        background=True)
                    else:
                        run_bash_script(target=output,
                                        code=code,
                                        name=callback['name'],
                                        timeout=callback['timeout'],
                                        script=callback['bash'])
                elif 'python' in callback:
                    pass
                else:
                    continue

        @self._manager.hook('compiling', True)
        def perform_when_compiling_pass(output):
            for callback in self._passing_callbacks['compiling']:
                if 'bash' in callback:
                    if callback.get('parallel') is True:
                        run_on_parallel(run_bash_script,
                                        target=output,
                                        script=callback['bash'],
                                        name=callback['name'],
                                        timeout=callback['timeout'],
                                        background=True)
                    else:
                        run_bash_script(target=output,
                                        name=callback['name'],
                                        timeout=callback['timeout'],
                                        script=callback['bash'])
                elif 'python' in callback:
                    pass
                else:
                    continue

        @self._manager.hook('linking', False)
        def perform_when_linking_fail(output, code):
            for callback in self._failing_callbacks['linking']:
                if 'bash' in callback:
                    if callback.get('parallel') is True:
                        run_on_parallel(run_bash_script,
                                        target=output,
                                        code=code,
                                        script=callback['bash'],
                                        name=callback['name'],
                                        timeout=callback['timeout'],
                                        background=True)
                    else:
                        run_bash_script(target=output,
                                        code=code,
                                        name=callback['name'],
                                        timeout=callback['timeout'],
                                        script=callback['bash'])
                elif 'python' in callback:
                    pass
                else:
                    continue

        @self._manager.hook('linking', True)
        def perform_when_linking_pass(output):
            for callback in self._passing_callbacks['linking']:
                if 'bash' in callback:
                    if callback.get('parallel') is True:
                        run_on_parallel(run_bash_script,
                                        target=output,
                                        timeout=callback['timeout'],
                                        script=callback['bash'])
                    else:
                        run_bash_script(output, script=callback['bash'],
                                        timeout=callback['timeout'])
                elif 'python' in callback:
                    pass
                else:
                    continue

        @self._manager.hook('archiving', False)
        def perform_when_archiving_fail(output, code):
            for callback in self._failing_callbacks['archiving']:
                if 'bash' in callback:
                    if callback.get('parallel') is True:
                        run_on_parallel(run_bash_script,
                                        target=output,
                                        code=code,
                                        script=callback['bash'],
                                        name=callback['name'],
                                        timeout=callback['timeout'],
                                        background=True)
                    else:
                        run_bash_script(target=output,
                                        code=code,
                                        name=callback['name'],
                                        timeout=callback['timeout'],
                                        script=callback['bash'])
                elif 'python' in callback:
                    pass
                else:
                    continue

        @self._manager.hook('archiving', True)
        def perform_when_archiving_pass(output):
            for callback in self._passing_callbacks['archiving']:
                if 'bash' in callback:
                    if callback.get('parallel') is True:
                        run_on_parallel(run_bash_script,
                                        target=output,
                                        script=callback['bash'],
                                        name=callback['name'],
                                        timeout=callback['timeout'],
                                        background=True)
                    else:
                        run_bash_script(target=output,
                                        name=callback['name'],
                                        timeout=callback['timeout'],
                                        script=callback['bash'])
                elif 'python' in callback:
                    pass
                else:
                    continue

        @self._manager.hook('testing', False)
        def perform_when_testing_fail(output, code):
            for callback in self._failing_callbacks['testing']:
                if 'bash' in callback:
                    if callback.get('parallel') is True:
                        run_on_parallel(run_bash_script,
                                        target=output,
                                        code=code,
                                        script=callback['bash'],
                                        timeout=callback['timeout'],
                                        background=True)
                    else:
                        run_bash_script(target=output,
                                        code=code,
                                        name=callback['name'],
                                        timeout=callback['timeout'],
                                        script=callback['bash'])
                elif 'python' in callback:
                    pass
                else:
                    continue

        @self._manager.hook('testing', True)
        def perform_when_tesing_pass(output):
            for callback in self._passing_callbacks['testing']:
                if 'bash' in callback:
                    if callback.get('parallel') is True:
                        run_on_parallel(run_bash_script,
                                        target=output,
                                        script=callback['bash'],
                                        name=callback['name'],
                                        timeout=callback['timeout'],
                                        background=True)
                    else:
                        run_bash_script(target=output,
                                        name=callback['name'],
                                        timeout=callback['timeout'],
                                        script=callback['bash'])
                elif 'python' in callback:
                    pass
                else:
                    continue

        @self._manager.hook('invoking', False)
        def perform_when_invoking_fail(output, code):
            for callback in self._failing_callbacks['invoking']:
                if 'bash' in callback:
                    if callback.get('parallel') is True:
                        run_on_parallel(run_bash_script,
                                        target=output,
                                        code=code,
                                        script=callback['bash'],
                                        name=callback['name'],
                                        timeout=callback['timeout'],
                                        background=True)
                    else:
                        run_bash_script(target=output,
                                        code=code,
                                        name=callback['name'],
                                        timeout=callback['timeout'],
                                        script=callback['bash'])
                elif 'python' in callback:
                    pass
                else:
                    continue

        @self._manager.hook('invoking', True)
        def perform_when_invoking_pass(output):
            for callback in self._passing_callbacks['invoking']:
                if 'bash' in callback:
                    if callback.get('parallel') is True:
                        run_on_parallel(run_bash_script,
                                        target=output,
                                        script=callback['bash'],
                                        name=callback['name'],
                                        timeout=callback['timeout'],
                                        background=True)
                    else:
                        run_bash_script(target=output,
                                        name=callback['name'],
                                        timeout=callback['timeout'],
                                        script=callback['bash'])
                elif 'python' in callback:
                    pass
                else:
                    continue

    def check(self):
        if self._manager is None:
            raise AssertionError('self._manager must not be None')
        elif self._manager.backends.get('package') is None:
            raise AssertionError('require backend `Package` is installed')
