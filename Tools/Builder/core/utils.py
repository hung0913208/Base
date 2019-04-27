from threading import Thread, Lock
from .logger import Logger, DEBUG
import multiprocessing

def wait(lock):
    lock.acquire()
    lock.release()

def is_okey_to_continue(lock, check):
    if callable(check) is False:
        raise AssertionError('\'check\' must be callable')

    if not lock is None:
        lock.acquire()

    ret = check()

    if not lock is None:
        lock.release()
    return ret

def update_variable_safety(lock, update, exception=None):
    if callable(update) is False:
        raise AssertionError('\'update\' must be callable')

    if not lock is None:
        lock.acquire()
    update()
    if not lock is None:
        lock.release()

    if not exception is None:
        raise AssertionError(exception)

def can_run_on_multi_thread(how_many_step_to_verify=300, parallel_core=4):
    global lock
    global count
    global result

    import platform

    if multiprocessing.cpu_count() > 1:
        return True

    # @NOTE: it looks stupid, but i do believe some OS handle multi-thread on
    # single core better than another, i have tested on FreeBSD and it proves
    # that i can run my tool smoothly even if its CPU is only have 1 core.

    lock = Lock()
    result = False
    count = 0

    def inc():
        global count
        count += 1

    def dec():
        global count
        count -= 1

    def wrapping():
        global lock
        global count
        global result

        wait(lock)
        update_variable_safety(lock, inc)

        while count > 0:
            testing = 0

            for _ in range(how_many_step_to_verify):
                if is_okey_to_continue(lock, lambda: count) > 1 and \
                        is_okey_to_continue(lock, lambda: result) is False:
                    result = True

                # @NOTE: do a simple stress test here
                for idx in range(how_many_step_to_verify):
                    testing += idx
            else:
                update_variable_safety(lock, dec)
    threads = []

    lock.acquire()
    for _ in range(parallel_core*multiprocessing.cpu_count()):
        thread = Thread(target=wrapping)

        thread.start()
        threads.append(thread)
    else:

        lock.release()
        for i in range(parallel_core*multiprocessing.cpu_count()):
            threads[i].join()
        else:
            return result

def bash_iterate(script, name=None, background=True, timeout=None, **kwargs):
    from .backend.file import is_file_existed
    from platform import system
    from os import write, close, remove, fsync
    from threading import Timer
    from time import sleep

    import subprocess
    import tempfile
    import sys

    # @NOTE: i deduct that everything will at least running on bash so we don't
    # need to reinstall `bash` everytime
    if system().lower() in ['linux', 'drawin', 'freebsd']:
        header = ""

        # @NOTE: prepare the header of our temp script
        for argname in kwargs:
            header = "{}{}={}\n".format(header, argname, kwargs[argname])
        else:
            fd, script_path = tempfile.mkstemp(suffix='.sh')
            write(fd, header.encode())
    else:
        raise AssertionError('don\'t support os `%s`' % system())

    write(fd, script.encode())
    fsync(fd)
    close(fd)

    # @NOTE: perform our script here now
    if system().lower() in ['linux', 'drawin', 'freebsd']:
        runner = subprocess.Popen([is_file_existed('bash'), script_path],
                                   stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE)
    else:
        raise AssertionError('don\'t support os `%s`' % system())

    # @NOTE: turn on timer to check timeout if it's requested
    if background is True:
        if name is None :
            Logger.debug('Output of unknown bash script:')
        else:
            Logger.debug('Output of the bash script `%s`:' % name)
    try:
        console = ''

        if not timeout is None or int(timeout) < 0:
            timer = Timer(timeout, runner.kill)

        # @NOTE: present the result of bash script if we decide it should be showed
        # on frontend

        while not console is None:
            output_console = runner.stdout.readline()
            error_console = runner.stderr.readline()

            if len(error_console) > 0:
                if isinstance(error_console, bytes):
                    console = error_console.decode('utf-8')
                else:
                    console = error_console
            elif len(output_console) > 0:
                if isinstance(output_console, bytes):
                    console = output_console.decode('utf-8')
                else:
                    console = output_console
            else:
                console = None

            if not console is None:
                if sys.version_info < (3, 0) and isinstance(console, unicode):
                    console = console.encode('ascii', 'ignore')

                if background is False or Logger.get_level() == DEBUG:
                    sys.stdout.write(console)
        else:
            runner.communicate()
    finally:
        if not timeout is None or int(timeout) < 0:
            timer.cancel()

    remove(script_path)


    # @NOTE: estimate whether or not, runner is finished or failed
    if not runner.returncode is None and runner.returncode != 0:
        return False
    else:
        return True

def run_on_parallel(callback, *args, **kwargs):
    from threading import Thread

    thread = Thread(target=callback, args=args, kwargs=kwargs)
    thread.start()
