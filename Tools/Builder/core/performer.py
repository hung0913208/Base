from threading import Lock
from time import sleep
from .logger import Logger, DEBUG
from .utils import update_variable_safety

try:
    from queue import Queue, Empty
except ImportError:
    from Queue import Queue, Empty

import subprocess
import sys
import os

class Performer(object):
    def __init__(self, manager, silence=False, **kwargs):
        super(Performer, self).__init__()
        self._events = {}
        self._manager = manager
        self._consumer = 0
        self._running = 0
        self._online = 0
        self._count = 0
        self._outside = Lock()
        self._inside = Lock()
        self._lock = Lock()
        self._jobs = Queue()
        self._pipe = Queue()
        self._silence = silence

    @property
    def type(self):
        return 'Performer'

    @property
    def online(self):
        self._lock.acquire()
        result = self._online

        self._lock.release()
        return result

    def reset(self):
        if self._inside.locked():
            self._inside.release()

        self._manager._keep = True
        self._consumer = 0

    def apply(self, commands):
        self._manager._lock.acquire()
        if self._manager._keep is True:
            self._pipe.put({
                'type': 'implement',
                'commands': commands
            })
        self._manager._lock.release()

    def signal(self, callback):
        if isinstance(callback, list) or isinstance(callback, tuple):
            self._pipe.put({
                'type': 'signal',
                'commands': callback
            })
        elif callable(callback):
            self._pipe.put({
                'type': 'signal',
                'commands': [callback]
            })

    def pending(self):
        self._lock.acquire()

        if self._online == 0:
            result = True
        elif self._running <= self._online:
            result = self._online - self._running < self._count
        else:
            result = False
        self._lock.release()
        return result

    @property
    def running(self):
        self._lock.acquire()
        result = self._running

        self._lock.release()
        return result

    @property
    def consumer(self):
        self._outside.acquire()
        consumer = self._consumer
        self._outside.release()
        return consumer

    @consumer.setter
    def consumer(self, value):
        self._outside.acquire()
        self._consumer = value
        self._outside.release()

    @property
    def is_keeping(self):
        self._manager._lock.acquire()
        keep = self._manager._keep
        self._manager._lock.release()
        return keep

    @staticmethod
    def pretty(string, max_collumn):
        result = string
        for i in range(len(result), max_collumn):
            result += ' '
        return result

    def print_command(self, executor, expected):
        self._manager._lock.acquire()
        expected = expected.split('/')[-1]

        # @NOTE: make output look pretty and simple to understand
        if not executor.lower() in ['link', 'test']:
            expected = '.'.join(expected.split('.')[:-1])

        if self._silence is False:
            print('   %s %s' % (Performer.pretty(executor.upper(), 6), expected))
        self._manager._lock.release()

    def print_output(self, command):
        self._manager._lock.acquire()
        if self._silence is False:
            pass
        self._manager._lock.release()

    def perform_on_multi_thread(self, timeout=1):
        def stop_running():
            self._manager._keep = False
            if self._inside.locked():
                self._inside.release()

        def payload():
            is_okey = True
            do_nothing = False

            if self.pending() is False:
                return

            self._lock.acquire()
            self._online += 1
            self._lock.release()


            while self.is_keeping is True and (self._jobs.qsize() > 0 or self._pipe.qsize() > 0):
                finish_task = False
                do_nothing = False

                self._lock.acquire()
                if self._count == 0 and self._running > 0:
                    keep = False
                else:
                    keep = True
                self._lock.release()

                if keep is False:
                    Logger.debug('we have nothing to do, depath now')
                    break

                # @NOTE: catch an instruction and perform on payloads
                try:
                    executor, command, expected, event = \
                        self._jobs.get(timeout=timeout)

                    # @TODO: make command more color and look beautifull
                    self._lock.acquire()
                    self._running += 1
                    self._lock.release()

                    env = os.environ.copy()
                    env['LIBC_FATAL_STDERR_'] = '1'
                    build = subprocess.Popen(command.split(' '), env=env,
                                             stdout=subprocess.PIPE,
                                             stderr=subprocess.PIPE)
                    error_console = build.stderr.read()
                    output_console = build.stdout.read()

                    build.wait()

                    # @NOTE: we will show what we have done instead of what we
                    # have to do
                    self._lock.acquire()
                    self.print_command(executor, expected)
                    self._lock.release()

                    if not build.returncode is None and build.returncode != 0:
                        is_okey = False
                    # elif len(error_console) > 0:
                    #     is_okey = False
                    elif os.path.exists(expected) is False and \
                         not event in ['invoking', 'testing']:
                        is_okey = False

                    # @TODO: make output more color and more highlight

                    # @NOTE: check exit code
                    if is_okey is False:
                        update_variable_safety(self._manager._lock, stop_running)

                        # @NOTE: since Python2 usually return to str but Python3
                        # would prefer bytes
                        if len(error_console) == 0:
                            error_console = output_console

                        if isinstance(error_console, bytes):
                            error_console = error_console.decode('utf-8')

                        if sys.version_info < (3, 0) and isinstance(error_console, unicode):
                            error_console = error_console.encode('ascii', 'ignore')

                        # @NOTE: perform event when perform a command fail
                        if event in self._events:
                            for callback in self._events[event][False]:
                                callback(expected, build.returncode)

                        if build.returncode != -6:
                            self._manager.found_bug( \
                                AssertionError(u'error {} when runing command \'{}\': \n\n{}\n'
                                               .format(build.returncode, command, error_console)))
                        else:
                            self._manager.found_bug( \
                                AssertionError(u'crash when runing command \'{}\''.format(command)))
                    else:
                        finish_task = True

                        # @NOTE: perform event when perform a command pass
                        if event in self._events:
                            for callback in self._events[event][True]:
                                callback(expected)
                    Logger.debug('finish task %s %s' % (executor, expected))
                except Exception as error:
                    if isinstance(error, Empty):
                        do_nothing = True

                        if self.pending() is False:
                            break
                    else:
                        update_variable_safety(self._manager._lock, stop_running)

                        if sys.version_info < (3, 0) and isinstance(error_console, unicode):
                            error_console = error_console.encode('ascii', 'ignore')

                        self._manager.found_bug(
                            AssertionError(u'error when runing command \'{}\': \n\n{}\n'
                                .format(command, error_console)))
                        break
                finally:
                    if do_nothing is False:
                        self._lock.acquire()
                        self._count -= 1

                        if finish_task:
                            self._running -= 1

                        # @NOTE: everything done on good condition, release master
                        if self._jobs.qsize() == 0 and self._running <= 0 and finish_task:
                            if self._inside.locked():
                                Logger.debug('Release master when counter is %d '
                                             'and running is %d' % (self._count, self._running))
                                self._inside.release()

                        Logger.debug('Finish a task, counter=%d, running=%d' % (self._count, self._running))
                        self._lock.release()
                    else:
                        sleep(timeout)
                finish_task = False
            else:
                # @NOTE: when on fall cases, the lastest payload must release
                # master before closing

                if self._online > 1:
                    Logger.debug('counter show %d tasks on pending' % self._count)

                try:
                    if self.is_keeping is False and self._inside.locked() and self.online == 1:
                        self._inside.release()
                except RuntimeError:
                    pass

            # @NOTE: auto update status of payloads to optimize performance
            self._lock.acquire()
            self._online -= 1
            self._lock.release()

        def master():
            while self.is_keeping is True and self._pipe.qsize() > 0:
                try:
                    job = self._pipe.get(timeout=timeout)
                    need_to_wait = Logger.get_level() == DEBUG
                    error_message = None

                    if job['type'] == 'implement':
                        self._count = 0

                        # @NOTE: parse command structure and instruct payloads
                        for command in job['commands']:
                            pattern = command['pattern']
                            event = command['event']

                            if not command['output'] is None:
                                workspace, output_file = command['output']

                            # @NOTE: check and join inputs
                            if isinstance(command['input'], str):
                                if os.path.exists(command['input']) is False:
                                    error_message = 'missing %s while it has ' \
                                                    'been required by %s' % (command['input'], command['output'])
                                else:
                                    input_path = command['input']
                            else:
                                for item in command['input']:
                                    if os.path.exists(item) is False:
                                        error_message = 'missing %s while it has ' \
                                                        'been required by %s' % (command['input'], command['output'])
                                else:
                                    input_path = ' '.join(command['input'])

                            if not error_message is None:
                                update_variable_safety(self._manager._lock, stop_running, error_message)

                            if not command['output'] is None:
                                # @NOTE: check workspace and create if it's not existed
                                if command['executor'].lower() in ['ar']:
                                    instruct = pattern % (workspace, output_file, input_path,)
                                else:
                                    instruct = pattern % (input_path, workspace, output_file)
                                expected = '%s/%s' % (workspace, output_file)

                                if os.path.exists(workspace) is False:
                                    os.mkdir(workspace, 0o777)
                                elif os.path.isfile(workspace) is True:
                                    os.remove(workspace)
                                    os.mkdir(workspace, 0o777)

                                # @NOTE: prepare output dir if it needs
                                current_dir = workspace
                                for dir_name in output_file.split('/')[:-1]:

                                    # @NOTE: it seems on MacOS, python don't allow
                                    # to create dir with '//' in path
                                    if dir_name == '/' or len(dir_name) == 0:
                                        continue
                                    elif current_dir[-1] == '/':
                                        current_dir = '%s%s' % (current_dir, dir_name)
                                    else:
                                        current_dir = '%s/%s' % (current_dir, dir_name)

                                    if os.path.exists(current_dir) is False:
                                        os.mkdir(current_dir, 0o777)

                                if os.path.exists(expected) is True:
                                    continue
                                else:
                                    self._jobs.put((command['executor'], instruct,
                                                    expected, event))
                                    self._count += 1
                                    need_to_wait = True
                            elif not self._manager.backends['config'].os in ['Window', 'Drawin']:
                                if 'executor' in command:
                                    self._jobs.put((command['executor'],
                                                    pattern % input_path,
                                                    input_path.split('/')[-1],
                                                    event))
                                else:
                                    self._jobs.put(('TEST',
                                                    pattern % input_path,
                                                    input_path.split('/')[-1],
                                                    event))
                                self._count += 1
                        else:
                            Logger.debug('finish adding a bundle of tasks, '
                                         'count=%d, running=%d' % (self._count, self._running))

                        if need_to_wait is False:
                            continue
                        if self._manager.count_payload == 0:
                            # @TODO: in many case this would mean payloads have done completely
                            # and no pending tasks here now, so we must exit on safe way now
                            # However, we not sure about fail cases so we must be carefull
                            # checking before annouce any decision

                            Logger.debug("when count_payload == 0 we have %d jobs" % self._jobs.qsize())
                            if self._jobs.qsize() == 0:
                                Logger.debug('Finish jobs now, going to stop everything from now')
                                update_variable_safety(self._manager._lock, stop_running)
                            else:
                                Logger.debug("wait payload(s) join(s) to finish %d jobs" % self._jobs.qsize())
                                self._inside.acquire()
                        elif self._count > 0:
                            Logger.debug("wait %d finish %d jobs" % (self._manager.count_payload, self._jobs.qsize()))
                            self._inside.acquire()
                        else:
                            Logger.debug('going to add new task without wait payload, '
                                         'count=%d, running=%d' % (self._count, self._running))

                    elif job['type'] == 'signal':
                        for callback in job['commands']:
                            callback()
                except Empty:
                    continue


        # @NOTE: we will use bootstrap as a specific way to choose which role would be
        # perform to the current thread
        self._lock.acquire()
        current = self.consumer
        self.consumer += 1
        self._lock.release()

        if current == 0:
            self._inside.acquire()
            return 'master', master
        else:
            return 'payload', payload

    def perform_on_single_thread(self, timeout=1):
        self.consumer += 1

        def stop_running():
            self._manager._keep = False

        def master():
            while self.is_keeping is True and self._pipe.qsize() > 0:
                try:
                    job = self._pipe.get(timeout=timeout)
                    error_message = None

                    if job['type'] == 'implement':
                        # @NOTE: parse command structure and instruct payloads
                        for command in job['commands']:
                            pattern = command['pattern']
                            event = command['event']

                            if not command['output'] is None:
                                workspace, output_file = command['output']

                            # @NOTE: check and join inputs
                            if isinstance(command['input'], str):
                                if os.path.exists(command['input']) is False:
                                    error_message = 'missing %s' % command['input']
                                else:
                                    input_path = command['input']
                            else:
                                for item in command['input']:
                                    if os.path.exists(item) is False:
                                        error_message = AssertionError('missing %s' % item)
                                else:
                                    input_path = ' '.join(command['input'])

                            if not error_message is None:
                                update_variable_safety(None, stop_running, error_message)

                            if not command['output'] is None:
                                # @NOTE: check workspace and create if it's not existed
                                if command['executor'].lower() in ['ar']:
                                    instruct = pattern % (workspace, output_file, input_path,)
                                else:
                                    instruct = pattern % (input_path, workspace, output_file)
                                expected = '%s/%s' % (workspace, output_file)

                                if os.path.exists(workspace) is False:
                                    os.mkdir(workspace, 0o777)
                                elif os.path.isfile(workspace) is True:
                                    os.remove(workspace)
                                    os.mkdir(workspace, 0o777)

                                # @NOTE: prepare output dir if it needs
                                current_dir = workspace
                                for dir_name in output_file.split('/')[:-1]:

                                    # @NOTE: it seems on MacOS, python don't allow
                                    # to create dir with '//' in path
                                    if dir_name == '/' or len(dir_name) == 0:
                                        continue
                                    elif current_dir[-1] == '/':
                                        current_dir = '%s%s' % (current_dir, dir_name)
                                    else:
                                        current_dir = '%s/%s' % (current_dir, dir_name)

                                    if os.path.exists(current_dir) is False:
                                        os.mkdir(current_dir, 0o777)

                                if os.path.exists(expected) is True:
                                    continue
                                else:
                                    self._jobs.put((command['executor'], instruct,
                                                    expected, event))
                                    self._count += 1
                            elif not self._manager.backends['config'].os in ['Window', 'Drawin']:
                                if 'executor' in command:
                                    self._jobs.put((command['executor'],
                                                    pattern % input_path,
                                                    input_path.split('/')[-1],
                                                    event))
                                else:
                                    self._jobs.put(('TEST',
                                                    pattern % input_path,
                                                    input_path.split('/')[-1],
                                                    event))
                        else:
                            return True
                    elif job['type'] == 'signal':
                        for callback in job['commands']:
                            callback()
                except Empty:
                    return True

        def payload():
            error_console = None
            output_console = None

            while self.is_keeping is True and self._jobs.qsize() > 0:
                # @NOTE: catch an instruction and perform on payloads
                try:
                    executor, command, expected, event = \
                        self._jobs.get(timeout=timeout)

                    # @TODO: make command more color and look beautifull
                    self.print_command(executor, expected)

                    env = os.environ.copy()
                    env['LIBC_FATAL_STDERR_'] = '1'
                    build = subprocess.Popen(command.split(' '), env=env,
                                             stdout=subprocess.PIPE,
                                             stderr=subprocess.PIPE)
                    error_console = build.stderr.read()
                    output_console = build.stdout.read()

                    build.wait()

                    if not build.returncode is None and build.returncode != 0:
                        is_okey = False
                    elif os.path.exists(expected) is False and \
                         not event in ['invoking', 'testing']:
                        print(expected)
                        is_okey = False
                    else:
                        is_okey = True

                    # @TODO: make output more color and more highlight

                    # @NOTE: check exit code
                    if is_okey is False:
                        update_variable_safety(None, stop_running)

                        # @NOTE: since Python2 usually return to str but Python3
                        # would prefer bytes
                        if len(error_console) == 0:
                            error_console = output_console

                        if isinstance(error_console, bytes):
                            error_console = error_console.decode('utf-8')

                        if sys.version_info < (3, 0) and isinstance(error_console, unicode):
                            error_console = error_console.encode('ascii', 'ignore')

                        # @NOTE: perform event when perform a command fail
                        if event in self._events:
                            for callback in self._events[event][False]:
                                callback(expected, build.returncode)

                        if build.returncode != -6:
                            self._manager.found_bug( \
                                AssertionError('error when {} runing command \'{}\': \n\n{}\n'
                                               .format(build.returncode, command, error_console)),
                                no_lock=True)
                        else:
                            self._manager.found_bug( \
                                AssertionError('crash when runing command \'{}\': \n\n{}\n'
                                                .format(command)),
                                no_lock=True)

                    else:
                        # @NOTE: perform event when perform a command pass
                        if event in self._events:
                            for callback in self._events[event][True]:
                                callback(expected)
                except Exception as error:
                    if isinstance(error, Empty):
                        return True
                    else:
                        update_variable_safety(None, stop_running)

                        if sys.version_info < (3, 0) and isinstance(error_console, unicode):
                            error_console = error_console.encode('ascii', 'ignore')

                        self._manager.found_bug(
                            AssertionError('error when runing command \'{}\': \n\n{}\n'
                                           .format(command, error_console)),
                            no_lock=True)
                    return False
            else:
                return True

        if self.consumer == 1:
            return master
        else:
            return payload

    def install_event(self, command, passing, callback):
        if not command in self._events:
            self._events[command] = {True: [], False: []}

        self._events[command][passing].append(callback)
