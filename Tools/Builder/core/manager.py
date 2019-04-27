from threading import Thread, Lock
from time import sleep

try:
    from queue import Queue, Empty
except ImportError:
    from Queue import Queue, Empty

from .performer import Performer
from .stacktracer import *
from .backend import *
from .logger import *
from .utils import *

class Manager(object):
    def __init__(self, root, **kwargs):
        super(Manager, self).__init__()

        self._performer = Performer(self, **kwargs)
        self._lock = Lock()
        self._state = Lock()

        # @NOTE: these parameters are used to tracking down system status
        self._use_parallel = None
        self._current_dir_path = None
        self._current_dir_type = None
        self._count_payload = 0
        self._on_going = 0
        self._output = None
        self._root = None
        self._error = False
        self._keep = True

        # @NOTE: define our slot callbacks
        self._departure_callbacks = []

        # @NOTE: queue task of build and teardown stage
        self._build = Queue()
        self._teardown = Queue()

        # @NOTE: eveything about a real workspace will be represented here
        self._count_rules = 0
        self._running = 0
        self._await = []
        self._rules = {}
        self._functions = {}

        # @NOTE: share resouce between objects
        self._languages = {}
        self._plugins = {}
        self._backends = {
            'package': Package(**kwargs),
            'config': Config(**kwargs),
            'file': File(**kwargs),
            'hook': Hook(**kwargs)
        }

        # @NOTE: okey now, load backends
        for name in self._backends:
            self._backends[name].mount(self)
            self._backends[name].check()
            self._backends[name].define()

    @property
    def root(self):
        return self._root

    @property
    def backends(self):
        return self._backends

    @property
    def languages(self):
        return self._languages

    @property
    def plugins(self):
        return self._plugins

    @property
    def type(self):
        return 'Manager'

    @property
    def error(self):
        return self._error

    @property
    def count_rules(self):
        return is_okey_to_continue(self._lock, lambda: self._count_rules)

    @property
    def count_payload(self):
        return is_okey_to_continue(self._lock, lambda: self._count_payload)

    def __enter_payload(self):
        def update():
            self._count_payload += 1
        update_variable_safety(self._state, update)

    def __exit_payload(self):
        def update():
            self._count_payload -= 1
        update_variable_safety(self._state, update)

    def package_update(self, passed=False):
        return self._backends['package'].update(passed)

    def package_install(self, packages):
        if isinstance(packages, list):
            for package in packages:
                if self._backends['package'].install(package, True) is True:
                    return True
            else:
                return False
        else:
            return self._backends['package'].install(packages, True)

    def support(self, languages):
        if isinstance(languages, list):
            for language in languages:
                self._languages[Manager.derived(language)] = language
        elif isinstance(language, Language):
            self._languages[Manager.derived(language)] = language
        elif not languages is None:
            raise AssertionError('\'languages\' must be a list')
        else:
            self._languages = {}

        for name in self._languages:
            self._languages[name].mount(self._performer)

        for name in self._languages:
            try:
                self._languages[name].mount(self)
            except Exception as error:
                Logger.error(str(error))
                self._languages[name] = None

        for name in self._languages:
            if self._languages[name] is None:
                continue

            try:
                self._languages[name].define()
            except Exception as error:
                Logger.error(str(error))
                self._languages[name] = None

    def install(self, plugins):
        if isinstance(plugins, list):
            for plugin in plugins:
                self._plugins[Manager.derived(plugin)] = plugin
        elif isinstance(plugin, Plugin):
            self._plugins[Manager.derived(plugin)] = plugin
        elif not plugins is None:
            raise AssertionError('\'plugins\' must be a list')
        else:
            self._plugins = []

        for name in self._plugins:
            self._plugins[name].mount(self)

        for name in self._plugins:
            self._plugins[name].define()

    def set_current_dir(self, type, dir_path):
        if dir_path != self._current_dir_path and (not self._current_dir_path is None):
            for callback in self._departure_callbacks:
                callback(mode=self._current_dir_type)

        if type.lower() == 'workspace':
            self._root = dir_path

        self._current_dir_path = dir_path
        self._current_dir_type = type
        self._on_going = 0

    def create_new_node(self, name, node):
        name = self.convert_name_to_absolute_name(name)

        if name in self._rules:
            return False

        try:
            self._rules[name] = {
                'dir': self._current_dir_path,
                'define': node,
                'depends_on': [],
                'depends_by': []
            }
            self._build.put(name)
            self._count_rules += 1
            self._on_going += 1
            return True
        except Queue.Full:
            return False

    def read_value_from_node(self, dep, key):
        # @NOTE: this helper function helps to read value from a node of our
        # build tree
        if dep[0] == ':' or dep[0:2] == '//':
            name = self.convert_dep_to_absolute_name(dep)
        else:
            name = dep

        if not name in self._rules:
            return None
        elif key in ['define', 'depends_on', 'depends_by']:
            return None
        else:
            return self._rules[name].get(key)

    def modify_node_inside_dependency_tree(self, name, key, value):
        # @NOTE: this helper function helps to modify node inside our build tree
        if name in self._rules:
            if key in ['dir', 'define', 'depends_on', 'depends_by']:
                return False
            else:
                self._rules[name][key] = value
        else:
            return False
        return True

    @staticmethod
    def convert_absolute_name_to_name(absolute_name):
        # @NOTE: this helper function helps to convert abs_name to name
        return absolute_name.split(':')[1]

    @staticmethod
    def convert_absolute_name_to_dep(root, absolute_name):
        # @NOTE: this helper function helps to convert abs_name to dependency
        if absolute_name is None:
            return "Unknown"
        elif root[-1] == '/':
            return '//' + absolute_name[len(root):]
        else:
            return '/' + absolute_name[len(root):]

    def convert_name_to_absolute_name(self, name, path=None):
        # @NOTE: this helper function helps to convert name to abs_name
        if path is None:
            return '%s:%s' % (self._current_dir_path, name)
        else:
            return '%s:%s' % (path, name)

    def convert_dep_to_absolute_name(self, dep):
        # @NOTE: this helper function helps to convert dependency to abs_name
        if dep[0] == ':':
            return '%s:%s' % (self._current_dir_path, dep[1:])
        elif dep[0:2] == '//':
            if self._root[-1] == '/':
                return '%s%s' % (self._root, dep[2:])
            else:
                return '%s%s' % (self._root, dep[1:])

    def add_to_dependency_tree(self, name, node, deps):
        # @NOTE: this helper function helps to add a new node to building tree
        name = self.convert_name_to_absolute_name(name)

        if name in self._rules:
            return False
        elif deps is None:
            return self.create_new_node(name, node)
        else:
            wait_to_remove = []

            self._lock.acquire()
            self._rules[name] = {
                'dir': self._current_dir_path,
                'define': node,
                'depends_on': [],
                'depends_by': []
            }

            for dep in deps:
                dep = self.convert_dep_to_absolute_name(dep)

                self._rules[name]['depends_on'].append(dep)
                if not dep in self._rules:
                    Logger.debug('rule %s is waiting %s' % (name, dep))
                    self._await.append((name, dep))
                else:
                    Logger.debug('rule %s is depended by %s' % (name, dep))
                    self._rules[dep]['depends_by'].append(name)

            for index, (node_name, dep) in enumerate(self._await):
                if dep in self._rules:
                    Logger.debug('rule %s is depended by %s' % (dep, node_name))

                    # @NOTE: add dependency and mark to remove this waiting task
                    # at lastly
                    self._rules[dep]['depends_by'].append(node_name)
                    wait_to_remove.append(index)

            if len(wait_to_remove):
                wait_to_remove.sort()

                # @NOTE: be carefull removing an item of list, we must reduce
                # index after finish removing an item
                for counter, index in enumerate(wait_to_remove):
                    del self._await[index - counter]

            self._count_rules += 1
            self._lock.release()
            return True

    def eval_function(self, function, path=None, node=None, **kwargs):
        # @NOTE: this helper function helps to perform a function
        function = self.find_function(function)

        if function is None:
            return None
        elif node is None:
            return function(root=self._root, output=self._output, **kwargs)
        else:
            name_node = self.convert_name_to_absolute_name(node, path=path)

            if name_node in self._rules:
                return function(root=self._rules[name_node]['dir'],
                                output=self._output, **kwargs)
            else:
                return None

    def add_rule(self, owner, function, use_on_workspace=False):
        self._functions[function.__name__] = {
            'owner': owner,
            'function': function,
            'use_on_workspace': use_on_workspace
        }

    def find_function(self, name, position=None):
        if not name in self._functions:
            return None
        else:
            function = self._functions[name]

            if (not position is None) and (position == 'workspace') \
                    and function['use_on_workspace'] is False:
                return None
            else:
                if function['owner'].check() is False:
                    return None
                return function['function']

    def show_pending_tasks(self):
        # @NOTE: show pending tasks when threads are stucked, sometime it's because
        # there are nothing to do so nothing to be show and we will close application
        # after that

        have_pending = False
        for rule_name in self._rules:
            rule = self._rules[rule_name]

            if not 'done' in rule or rule['done'] is False:
                print('>> Rule %s is waiting tasks:' % \
                    Manager.convert_absolute_name_to_dep(self._root, rule_name))

                for dep in rule['depends_on']:
                    if not dep in self._rules:
                        print('\t\t%s -> didn\'t exist' % Manager.convert_absolute_name_to_dep(self._root, dep))
                        have_pending = True
                    elif not 'done' in self._rules[dep] or self._rules[dep]['done'] is False:
                        print('\t\t%s' % Manager.convert_absolute_name_to_dep(self._root, dep))
                        have_pending = True
        else:
            return have_pending

    def teardown(self, root, output=None):
        # @NOTE: now run teardown callbacks. Since i don't want to solve race
        # condition problems on 'root', everything will be run on master-thread
        # after all our consumer-thread have finished

        while self._teardown.empty() is False:
            callback, info = self._teardown.get()

            if callable(info) is True:
                info(is_on_running=False)

            if callable(callback) is False:
                continue
            elif callback(root=root, output=output) is False:
                raise AssertionError('there some problem when teardown this project')
            self._teardown.task_done()

    def found_bug(self, bug, turn_to_debug=False, no_lock=False):
        def update():
            self._error = True

        # @NOTE: this function will use to notify when a bug has been found
        if turn_to_debug is True:
            Logger.debug('Got an exception: %s -> going to teardown this project' % str(bug))
        else:
            Logger.error('Got an exception: %s -> going to teardown this project' % str(bug))

        if no_lock or self._use_parallel is False or self._current_dir_type == 'workspace':
            update()
        else:
            update_variable_safety(self._lock, update)

    def perform(self, root, output=None, timeout=1,
                retry_checking_multithread=30):
        # @NOTE: sometime, adding rules don't run as i expected, for example
        # adding rule with unknown dependecies, we must scan throw self._await
        # to make sure everything has been added completely
        if len(self._await) > 0:
            Logger.debug('It seems self._await still have %d rules '
                         'under implemented' % len(self._await))
            while True:
                wait_to_remove = []

                for index, (node_name, dep) in enumerate(self._await):
                    if dep in self._rules:
                        Logger.debug('rule %s is depended by %s' % (dep, node_name))

                        # @NOTE: add dependency and mark to remove this waiting task
                        # at lastly
                        self._rules[dep]['depends_by'].append(node_name)
                        wait_to_remove.append(index)
                else:
                    # @NOTE: if we don't have anything on waiting to remove, it
                    # means we finish checking now
                    if len(wait_to_remove) == 0:
                        break

                    # @NOTE: on the otherhand, we must remove waiting dependencies
                    # and do again
                    wait_to_remove.sort()
                    for counter, index in enumerate(wait_to_remove):
                        del self._await[index - counter]

            # @NOTE: sometime we use undefined dependencies and cause system
            # hange forever, this way will check everything before invoke
            # building system
            if len(self._await) > 0:
                list_untagged_rules = ()

                for rule, dep in self._await:
                    list_untagged_rules.add(rule)

                self.found_bug(
                    AssertionError('still have untagged rules, please check '
                                    'your `.build` and `.workspace` here it\'s '
                                    'the list of untagged rules:\n%s' % \
                                    '\n'.join(list_untagged_rules)))

        # @NOTE: unexpected error can happen before performing rules.
        # if it happens, abandon this project now
        if self._error:
            return not self._error

        # @NOTE: narrate how to implement the project according instructions
        if self._use_parallel is None:
            self._use_parallel = can_run_on_multi_thread(retry_checking_multithread)

        if self._use_parallel is True and self._current_dir_type == 'build':
            return self.perform_on_multi_thread(root, output, timeout)
        else:
            return self.perform_on_single_thread(root, output)

    def perform_on_single_thread(self, root, output=None, timeout=1):
        # @NOTE: there was a big issue when run this tool on a single core machine
        # since we only have a master, we never run payload at here and we can't
        # compile anything

        self._root = root
        self._output = output
        self._count_payload = 0
        self._performer.reset()

        Logger.debug('Run on single-thread')

        parsing = self._performer.perform_on_single_thread(timeout=timeout)
        running = self._performer.perform_on_single_thread(timeout=timeout)

        if parsing is None or running is None:
            self.found_bug(AssertionError('it seems performer got bugs'), no_lock=True)
            return False

        while self._keep is True and self.count_rules > 0 and not self._error:
            while self._current_dir_type == 'build' and \
                    not self._error and self._keep is True:
                if self._performer._pipe.qsize() > 0:
                    if self._keep is True:
                        parsing()
                    if self._keep is True:
                        running()
                elif self._performer._jobs.qsize() > 0:
                    if self._keep is True:
                        running()
                else:
                    break

            try:
                # @NOTE: fetch a new task
                task_name = self._build.get(timeout=timeout)
            except Empty:
                if self._performer._pipe.qsize() == 0 and \
                        self._performer._jobs.qsize() == 0:
                    self._keep = False
                continue

            # @NOTE: parse this task
            if not task_name in self._rules:
                self.found_bug(AssertionError('it seems there is a race condition with '
                                              'task %s' % task_name), no_lock=True)
                continue

            define = self._rules[task_name]['define']
            depends_by = self._rules[task_name]['depends_by']

            if 'info' in define:
                define['info'](is_on_running=True, **define)

            if 'callback' in define:
                kwargs = define.get('kwargs')

                try:
                    if self._rules[task_name]['dir'] is None:
                        workspace = root
                    else:
                        workspace = self._rules[task_name]['dir']

                    # @NOTE: check dependency here
                    if kwargs is None:
                        result = define['callback'](root=workspace,
                                                    workspace=output)
                    else:
                        result = define['callback'](root=workspace,
                                                    workspace=output, **kwargs)

                    if result is False:
                        self._keep = False
                    elif not define.get('teardown') is None:
                        self._teardown.put((define['teardown'], define.get('info')))

                    if 'info' in define:
                        define['info'](is_on_running=True,
                                       is_finish_successful=True, **define)

                    if self._keep is True:
                        parsing()

                    if self._keep is True:
                        running()

                    if self._keep is False:
                        continue

                    for name in depends_by:
                        # @NOTE: detect position of this dependency and remove
                        # it out of dependency list
                        try:
                            index = self._rules[name]['depends_on'].index(task_name)
                        except ValueError:
                            index = -1

                        if index >= 0:
                            del self._rules[name]['depends_on'][index]

                        # @NOTE: if the task's dependencies is None now, put it to queue 'build'
                        if len(self._rules[name]['depends_on']) == 0:
                            self._build.put(name)
                    else:
                        Logger.debug('Finish pushing task %s' % task_name)
                        self._rules[task_name]['done'] = True

                except Exception as error:
                    # @NOTE: update status to from running -> stoping because we
                    # found a bug inside our code
                    self.found_bug(error, no_lock=True)

                    # @NOTE: print exception
                    Logger.exception()
        else:
            return not self._error

    def perform_on_multi_thread(self, root, output=None, timeout=1, parallel_core=4):
        lock = Lock()

        # @NOTE: tracer consumes so much time so we must consider using it
        # start_tracer('/tmp/test.html')
        self._root = root
        self._output = output
        self._count_payload = 0
        self._performer.reset()

        Logger.debug('Run on multi-threads')

        if len(self._rules) == 0:
            return True

        if self._current_dir_type == 'build':
            for callback in self._departure_callbacks:
                callback(mode=self._current_dir_type)

        def stop_running():
            self._keep = False

        def wrapping():
            # @NOTE: make all consumer-threads wait until the main-thread finishs
            # creating and configures
            if (not self._current_dir_type is None) and self._current_dir_type.lower() == 'build':
                role, performing_callback = self._performer.perform_on_multi_thread(timeout=timeout)
            else:
                role, performing_callback = 'master', None
            wait(lock)

            if not performing_callback is None:
                if role == 'payload' and self._performer.pending():
                    self.__enter_payload()
            begin = True

            # @NOTE: fetch a task from queue and perform it
            while self._keep is True and (self.count_rules > 0 or self.count_payload > 0):
                if is_okey_to_continue(self._lock, lambda: self._keep) is False:
                    if role != 'master':
                        break
                    else:
                        self._performer.clear_pipe()
                        continue

                # @NOTE: payload must finish its tasks first before supporting
                # parsing rules
                if role == 'payload' and (self._performer.pending() or self._build.qsize() == 0):
                    performing_callback()

                if role == 'payload':
                    if self._build.qsize() == 0 and self.count_rules == 0:
                        # @NOTE: when payload determines that rules are converted
                        # it must reload to check and run finish its task
                        # instead of waiting task from main-thread
                        continue
                    else:
                        sleep(timeout)
                   #     Logger.debug('Payload turn off when '
                   #                  'count_rules=%d' % self.count_rules)

                # @NOTE: now get task from self._build and peform parsing
                try:
                    if role == 'master':
                        # @NOTE: when master determines that rules are converted
                        # it must jump to perform tasks instead of waiting new
                        # task from main-thread

                        Logger.debug('Master have %d to do now' \
                                     % self._performer._pipe.qsize())
                        while self._performer._pipe.qsize() > 0 and self._keep:
                            performing_callback()
                        else:
                            if self._build.qsize() == 0 and self.count_rules == 0:
                                Logger.debug('Master off but there were nothing '
                                             'to do now')
                                raise Empty

                    task_name = self._build.get(timeout=timeout)

                    self._lock.acquire()
                    Logger.debug('Convert rule %s now' % task_name)

                    if not task_name is None:
                        self._running += 1
                    self._lock.release()
                except Empty:
                    if self._running == 0 and self._performer.running == 0:
                        # @NOTE: since this might be caused by a race condition
                        # with the build script, we must show tasks that are on
                        # pending

                        if self._performer._jobs.qsize() == 0:
                            update_variable_safety(self._lock, stop_running)

                        if self.show_pending_tasks():
                            self.found_bug(AssertionError('it seems there is a race condition, '
                                'nothing run recently'), turn_to_debug=True)

                        if role != 'master' and self._performer._inside.locked():
                            performing_callback()
                        continue
                    else:
                        if role == 'payload':
                            performing_callback()
                        continue

                if not task_name in self._rules:
                    update_variable_safety(self._lock, stop_running)
                    raise AssertionError('it seems there is a race condition with task %s' % task_name)

                define = self._rules[task_name]['define']
                depends_by = self._rules[task_name]['depends_by']

                if 'info' in define:
                    define['info'](is_on_running=True, **define)

                if 'callback' in define:
                    kwargs = define.get('kwargs')
                    try:
                        if self._rules[task_name]['dir'] is None:
                            workspace = root
                        else:
                            workspace = self._rules[task_name]['dir']

                        # @NOTE: check dependency here
                        if kwargs is None:
                            result = define['callback'](root=workspace, workspace=output)
                        else:
                            result = define['callback'](root=workspace, workspace=output, **kwargs)

                        if result is False:
                            update_variable_safety(self._lock, stop_running)
                        elif not define.get('teardown') is None:
                            self._teardown.put((define['teardown'], define.get('info')))

                        if 'info' in define:
                            define['info'](is_on_running=True, is_finish_successful=True, **define)

                        self._lock.acquire()
                        for name in depends_by:
                            # @NOTE: detect position of this dependency and remove
                            # it out of dependency list
                            try:
                                index = self._rules[name]['depends_on'].index(task_name)
                            except ValueError:
                                index = -1

                            if index >= 0:
                                del self._rules[name]['depends_on'][index]

                            # @NOTE: if the task's dependencies is None now, put it to queue 'build'
                            if len(self._rules[name]['depends_on']) == 0:
                                self._build.put(name)
                        else:
                            self._rules[task_name]['done'] = True
                            self._count_rules -= 1
                        self._lock.release()

                        if role == 'master' and not performing_callback is None:
                            performing_callback()
                        elif role == 'payload' and self._performer.pending() is True:
                            performing_callback()
                    except Exception as error:
                        # @NOTE: update status to from running -> stoping because we found a bug inside our code
                        update_variable_safety(self._lock, stop_running)
                        self.found_bug(error)

                        # @NOTE: print exception
                        Logger.exception()
                    finally:
                        self._running -= 1
                self._build.task_done()

            if role == 'master':
                Logger.debug('Master finish transfer request to Performer')

                while self._performer._pipe.qsize() > 0 and self._keep:
                    performing_callback()
                else:
                    Logger.debug('Master deliver task to payloads')

                if self._keep:
                    if self._performer._jobs.qsize() == 0:
                        update_variable_safety(self._lock, stop_running)
                    else:
                        Logger.debug('Master become a payload and will support '
                                     'another payloads')
                        _, performing_callback = \
                            self._performer.perform_on_multi_thread(timeout=timeout)
                        performing_callback()

                Logger.debug('Teardown master now')
            else:
                Logger.debug('Teardown payload now')

            if not performing_callback is None:
                if role == 'payload':
                    self.__exit_payload()

        lock.acquire()
        consumers = []

        for i in range(parallel_core*multiprocessing.cpu_count()):
            thread = Thread(target=wrapping)

            # @NOTE: configure consumer-threads
            thread.setName('Builder-%d' % i)
            thread.start()

            # @NOTE: add this thread to consumer-list
            consumers.append(thread)
        else:
            # @NOTE: okey, now release lock to enable consumer-threads finish our
            # backlog and wait until they are finish our backlog
            if not self._current_dir_type is None:
                Logger.debug('begin parsing %s' % (self._current_dir_type.upper()))
            lock.release()

            for i in range(parallel_core*multiprocessing.cpu_count()):
                consumers[i].join()
            # stop_tracer()
            return not self._error

    @property
    def languages(self):
        return self._languages

    @property
    def plugins(self):
        return self._plugins

    @staticmethod
    def derived(instance):
        result = None

        try:
            for class_name in reversed(instance.derived()):
                if class_name == 'Plugin' or class_name == 'Language':
                    if result is None:
                        return instance.__class__.__name__.lower()
                    else:
                        return result.lower()
                else:
                    result = class_name
        except Exception as error:
            Logger.error(error)
            Logger.exception()
        return None

    def when_depart(self, type):
        def route(function):
            def wrapping(mode, *args, **kwargs):
                if mode.lower() == type.lower():
                    return function(*args, **kwargs)
                else:
                    return None

            self._departure_callbacks.append(wrapping)
            return wrapping
        return route

    def hook(self, when_calling_happen, status=True):
        def route(function):
            self._performer.install_event(when_calling_happen,
                                          status,
                                          function)
            return function
        return route
