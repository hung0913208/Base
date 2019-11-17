#!/bin/bash
# @NOTE: we should consider using proxy to overcome the fetching cookies problems

function main() {
	python -c """
import os, sys, json, time, urllib, subprocess, multiprocessing

TUN = 'https://colab.research.google.com/tun/m/$TUNNEL/socket.io/?'
WSS = 'wss://colab.research.google.com/api/kernels/{}/channels?session_id={}'

class Job(object):
    def __init__(self, name, tunnel, flow=None, dependencies=[], **kwargs):
        super(Job, self).__init__()
        self._kwargs = {}
        self._result = None
        self._tunnel = tunnel
        self._flow = flow
        self._name = name

        for name in kwargs:
            if name.startswith('global.'):
                self._tunnel.environments = (name.split('.')[1], kwargs[name])
            else:
                self._kwargs[name] = kwargs[name]

        self.dependencies = dependencies

    def __str__(self):
        if self._tunnel.verbose >= 1:
            def Print():
                print('-> {}/{}: {}'.format(self._flow, self._name, self._kwargs))

            self._tunnel.__safe__(Print)

        if self._flow in ['interact', 'analyse']:
            result = '{} '.format(self._name)
            variables = ''

            if self._tunnel.verbose == 1:
                result += '--verbose '

            for key in self._kwargs:
                if key in ['respawn', 'dependencies', 'background']:
                    continue
                elif isinstance(self._kwargs[key], list) is False:
                    variables += '%s=%s' % (key, self._kwargs[key])
            else:
                if len(self._tunnel.environments) > 0:
                    result += '--envs {} '.format(self._tunnel.environments)

                if len(variables) > 0:
                    result += '--vars {} '.format(variables)

        if self._flow == 'interact':
            if self.background:
                result += '--background {} '.format(1)
            else:
                result += '--background {} '.format(0)

            if 'commands' in self._kwargs:
                result += ' '.join(self._kwargs['commands'])
            return result
        elif self._flow == 'analyse':
            return result
        elif self._flow in ['get', 'post']:
            params = []

            if 'uri' not in self._kwargs:
                if 'sid' in self._tunnel._environments:
                    self._kwargs['uri'] = TUN + 'authuser=%s&EIO=3&transport=%s&sid=%s'
                else:
                    self._kwargs['uri'] = TUN + 'authuser=%s&EIO=3&transport=%s'

            if not 'params' in self._kwargs:
                if 'sid' in self._tunnel._environments:
                    self._kwargs['params'] = ['%auth', '%transport', '%sid']
                else:
                    self._kwargs['params'] = ['%auth', '%transport']

            for name in self._kwargs['params']:
                if name[0] == '%':
                    value = self._tunnel._environments.get(name[1:]) or ''
                else:
                    value = name

                params.append(value)
            return self._kwargs['uri'] % tuple(params)
        else:
            raise AttributeError('don\\'t support {}'.format(self._flow))

    def __getattr__(self, name):
        if name in ['_kwargs', '_result', '_tunnel', '_flow', '_name']:
            return self.__dict__[name]
        elif name in ['kwargs', 'result', 'tunnel', 'flow', 'name']:
            return self.__dict__['_{}'.format(name)]
        elif name in self._kwargs:
            return self._kwargs[name]
        elif name == 'ready':
            for dep in self._kwargs.get('dependencies') or []:
                if isinstance(dep, str):
                    flow, name = dep.split(':')
                    dep = self._tunnel.Find(name)
                elif isinstance(dep, bytes):
                    flow, name = dep.decode('utf-8') .split(':')
                    dep = self._tunnel.Find(name)
                else:
                    flow = dep.flow

                if dep is None or dep.done is False:
                    return False
                elif dep.flow != flow:
                    return False
            else:
                return True
        elif name == 'background':
            if 'background' in self._kwargs:
                return self._kwargs.get('background')
            else:
                return not self._name in ['set', 'print', 'callback']
        elif name == 'respawn':
            if 'respawn' in self._kwargs:
                return self._kwargs.get('respawn')
            else:
                return False
        elif name == 'done':
            return self.ready and not self._result is None
        else:
            raise AttributeError('not found {}'.format(name))

    def __setattr__(self, name, value):
        if name in ['_kwargs', '_result', '_tunnel', '_flow', '_name']:
            self.__dict__[name] = value
        elif name in ['kwargs', 'result', 'tunnel', 'flow', 'name']:
            self.__dict__['_{}'.format(name)] = value
        elif name == 'dependencies':
            dependencies = []

            for item in value:
                if isinstance(item, Job):
                    dependencies.append(item)
                else:
                    flow, name = item.split(':')
                    job = self._tunnel.Find(name)

                    if not job is None:
                        if job.flow != flow:
                           sys.exit(-1)
                        dependencies.append(job)
                    else:
                        dependencies.append(item.encode('utf-8'))
            else:
                self._kwargs['dependencies'] = dependencies
        else:
            self._kwargs[name] = value


class Queue(object):
    class Empty(Exception):
        def __init__(self, message):
            super(Empty, self).__init__(message)

    def __init__(self, verbose):
        from threading import Lock, Condition

        self._verbose = verbose
        self._array = []
        self._inner = Lock()
        self._outner = Condition()

    def Get(self, timeout=-1):
        from threading import Timer, current_thread

        keep = True
        result = None

        while result is None and keep is True:
            # @NOTE: lock the thread when the array reduce to 0, we should
            # wait until another thread provide new values

            if self._verbose >= 2:
                print('[ QUEUE ] {} is fetching'.format(current_thread().name))
            with self._outner:
                if len(self._array) == 0:
                    if timeout >= 0:
                        self._outner.wait(timeout)
                    else:
                        self._outner.wait()

                # @NOTE: fetch the first value from the array 
                with self._inner:
                    if len(self._array) > 0:
                        result = self._array[0]

                        # @NOTE: remove the catched value out of the array
                        if len(self._array) > 1:
                            self._array = self._array[1:]
                        else:
                            self._array = []

                        if self._verbose >= 2:
                            print('[ QUEUE ] <- {}!'.format(result.name))
        else:
            if result:
                return result
            else:
                raise Queue.Empty('timeout')

    def Put(self, job, compare=None):
        with self._inner:
            with self._outner:
                for item in self._array:
                    if compare is None:
                        if item == job:
                            break
                    elif compare(item, job):
                        break
                else:
                    self._array.append(job)

                # @NOTE: when we detect a new value has been appended, we
                # should notify another threads to know catch the new value

                if len(self._array) >= 2:
                    self._outner.notifyAll()

        if self._verbose >= 2:
            print('[ QUEUE ] -> {}!'.format(job.name))

    def Find(self, value, compare=None):
        with self._inner:
            for item in self._array:
                if compare is None:
                    if value == item:
                        return True
                elif compare(value, item) is True:
                    return item
            else:
                return None

    def QSize(self):
        with self._inner:
            return len(self._array)

    @property
    def verbose(self):
        return self._verbose

    @verbose.setter
    def verbose(self, value):
        self._verbose = value


class Tunnel(object):
    def __init__(self, cookies, headers={}, timeout=-1, verbose=0):
        from threading import Thread, Lock

        super(Tunnel, self).__init__()
        self._lock = Lock()
        self._done = False
        self._verbose = verbose
        self._status = 0
        self._counts = {'alive': 0, 'busy': 0}
        self._cookies = {}
        self._threads = []
        self._timeout = timeout
        self._headers = headers
        self._builtins = {}
        self._scalable = None if $SCALABLE == 1 else False
        self._jobqueue = Queue(self._verbose)
        self._callbacks = {
            'kernel': [self.__calculate_session_id__]
        }
        self._environments = {
            'sid': '',
            'auth': 0,
            'transport': 'polling',
        }

        if len('$PROXY') > 0:
            self._environments['proxy'] = '$PROXY'

        if self.scalable:
            for idx in range(2*multiprocessing.cpu_count() - 1):
                thread = Thread(target=self.__proc__)

                # @NOTE: configure thread
                thread.setName('tunnel-%d' % idx)
                thread.start()

                # @NOTE: add thread into our pool
                self._threads.append(thread)

        for cookie in cookies:
            try:
                key, value = cookie.strip().split('=')
                self._cookies[key] = value
            except Exception:
                pass

    def __proc__(self):
        import threading

        keep = True

        def inc(name):
            if self._verbose >= 1 and name == 'alive':
                print('{} is created'.format(threading.current_thread().name))
            self._counts[name] += 1

        def dec(name):
            if self._verbose >= 1 and name == 'alive':
                print('{} exits'.format(threading.current_thread().name))
            self._counts[name] -= 1

        # @NOTE: increase the number to calculate how many threads alive
        self.__safe__(inc, name='alive')

        while keep is True:
            count = 0
            job = None

            try:
                # @NOTE: it turns out that we may face unexpected Exception
                # and we should cover everything inside a bigger try-catch
                # to secure our flow

                while self.okey is True:
                    job = None

                    try:
                        job = self._jobqueue.Get(timeout=self._timeout)
                        count += 1

                        self.__safe__(inc, name='busy')
	
                        if job.ready is False:
                            # @NOTE: this job is still on planing so we must
                            # wait a little bit longer before it can be started

                            self._jobqueue.Put(job)
                        elif job.background is False:
                            # @NOTE: this job is required to be run on
                            # main-thread

                            self._jobqueue.Put(job)
                        elif job.flow == 'interact' or job.flow == 'analyze':
                            # @NOTE: this job is set to flow interact so we must
                            # handle it with method Interact

                            self.Interact(job, True)
                            count = 0

                            if job.respawn is True:
                                self._jobqueue.Put(job)
                        elif job.flow == 'get':
                            self.Get(job, True)
                        elif job.flow == 'post':
                            self.Post(job, True)

                        self.__safe__(dec, name='busy')
                    except Queue.Empty:
                        # @NOTE: if our jobqueue is empty, we have nothing to
                        # be done so we should set count to 0 to avoid sleeping
                        # more in the finally step

                        count = 0
                    finally:
                        # @NOTE: if we try to catch jobs but unsuccess starting
                        # a new job, it would mean that the jobs we have here
                        # can't be ready to be run and we should freeze a little
                        # bit to avoid overheat our CPU

                        if count > 0 and count == self._jobqueue.QSize():
                            time.sleep(1 if self._timeout < 0 else self._timeout)
                keep = False
            except Exception as error:
                keep = False

                if self._verbose >= 1:
                    print('[ TUNNEL ] thread {} catch error {}'
                        .format(threading.current_thread().name,error))
                if isinstance(error, AttributeError):
                    def Stop():
                        self.status = -1

                    self.__safe__(Stop)
                elif not job is None:
                    self._jobqueue.Put(job)
        else:
            # @NOTE: release thread, we should decrease the alive threads to
            # notify the main-thread to fix this issue

            self.__safe__(dec, name='alive')

    def __safe__(self, callback, *args, **kwargs):
        try:
            self._lock.acquire()
            return callback(*args, **kwargs)
        except Exception as error:
            raise error
        finally:
            self._lock.release()

    def __enter__(self):
        return self

    def __exit__(self, type, value, trackback):
        self._status = -1

        for thread in self._threads:
            thread.join()

    def __exec__(self, job, cmd, callbacks=[]):
        proc = subprocess.Popen(cmd, stdout=subprocess.PIPE,
                                    stderr=subprocess.PIPE)

        if self._verbose >= 3:
            while True:
                line = proc.stderr.readline()

                def Print():
                    print('[ VERBOSE/{} ] -> {}'.format(job.name, line))

                if line is None or len(line) == 0:
                    break
                else:
                    if line[-1] == '\n':
                        line = line[:-1]

                    self.__safe__(Print)

        for line in proc.stdout.readlines():
            done = False

            # @NOTE: if we can't parse the lines, it would mean that
            # the line is raw log and we should print them to console

            for callback in callbacks:
                if callback(line) is True:
                    done = True
            else:
                def Print(line):
                    if line[-1] == '\n':
                        line = line[:-1]

                    if self._verbose >= 3:
                        print('[ OUTPUT/{} ] -> {}'.format(job.name, line))

                if done is False:
                    self.__safe__(Print, line=line)
        else:
            # @NOTE: just to be safe, we should wait until the child proc
            # has done completely
                
            code = proc.wait()

            if self.verbose == 1:
                def Print():
                    print('-> done({})'.format(code))

                self.__safe__(Print)

            job.result = code
            if code != 0:
                self.status = -1

    def __calculate_session_id__(self, kernel):
        if self._verbose >= 1:
            print('[ TUNNEL ]: calculate session id from kernel {}'.format(kernel))

self._environments['session'] = session
        self._environments['uri'] = WSS.format(kernel, session)

    @property
    def verbose(self):
        return self._verbose

    @verbose.setter
    def verbose(self, value):
        self._verbose = value
        self._jobqueue.verbose = value

    @property
    def okey(self):
        return self._jobqueue.QSize() > 0 or (self._status >= 0 and self._done is False)

    @property
    def cookies(self):
        return ';'.join(['{}={}'.format(key, self._cookies[key]) for key in self._cookies])

    @property
    def scalable(self):
        first = self._scalable is None

        if self._scalable is None:
            self._scalable = can_run_on_multi_thread(check=False)

        if first:
            return self._scalable
        return self._scalable and self._counts['alive'] > self._counts['busy']

    @property
    def status(self):
        return self._status

    @status.setter
    def status(self, value):
        self._status = value

        if value < 0:
            sys.exit(-1)

    @property
    def environments(self):
        result = 'status={}'.format(self._status)

        for name in self._environments:
            result += ',{}={}'.format(name, self._environments[name])
        else:
            return result

    @environments.setter
    def environments(self, item):
        if isinstance(item, tuple):
            key, value = item

            for callback in self._callbacks.get(key) or []:
                if callback(value) is False:
                    break
            else:
                self._environments[key] = value

    def Enqueue(self, job):
        self._jobqueue.Put(job)

    def Communicate(self):
        initing = True
        count = 0

        while self.okey is True:
            try:
                if self.status == 0:
                    # @NOTE: call the initialize script to help to create 
                    # our reacting map
                    
                    if initing is False and self._jobqueue.QSize() >= 0:
                        self._status = 40
                    elif initing is True:
                        if os.path.exists('$(pwd)/colab.cookies'):
                            commands = ['--tunnel', '$TUNNEL',
                                        '--cookies', self.cookies,
                                        '--cookie', '$(pwd)/colab.cookies',
                                        '--cookie-jar', '$(pwd)/colab.cookies']
                        else:
                            commands = ['--tunnel', '$TUNNEL',
                                        '--cookies', self.cookies,
                                        '--cookie-jar', '$(pwd)/colab.cookies']
                        if 'proxy' in self._environments:
                            commands.extend(['--proxy', self._environments['proxy']])

                        yield Job(name='init', tunnel=self, flow='interact',
                                  commands=commands, background=False)
                    initing = False
                else:
                    job = self._jobqueue.Get(self._timeout)
                    count += 1

                    try:
                        if job.ready is False:
                            # @NOTE: this job is still on planing so we must
                            # wait a little bit longer before it can be started

                            self._jobqueue.Put(job)
                        elif job.background is False and self.scalable:
                            # @NOTE: this job is required to be run only on
                            # background

                            self._jobqueue.Put(job)
                        else:
                            count = 0
                    finally:
                        # @NOTE: if we found that every jobs still run in
                        # background or under planing, we should freeze our
                        # main-thread to avoid overheat our CPU

                        if count == self._jobqueue.QSize():
                            time.sleep(1 if self._timeout < 0 else self._timeout)
                        elif count == 0:
                            yield job
            except Queue.Empty:
                pass

    def Interact(self, job, background=False):
        from threading import Thread, Lock

        def produce_jobs_when_finish(line):
            try:
                kwargs = json.loads(line.decode('utf-8'))

                for name in kwargs:
                    # @NOTE: it turns out that we may receive strings. If
                    # that true, we should print them as a log instead of
                    # puting them into our jobqueue

                    def Print():
                        if self._verbose >= 3:
                            print('[ OUTPUT/{} ] -> {}'.format(job.name,
                                                               kwargs[name]))

                    if isinstance(kwargs[name], str): 
                        self.__safe__(Print)
                    elif isinstance(kwargs[name], dict):
                        self.Enqueue(Job(name=name, tunnel=self, **kwargs[name]))
                    elif isinstance(kwargs[name], list):
                        def wrapping():
                            for line in kwargs[name]:
                                print(line)

                        self.__safe__(wrapping)
                else:
                    return True
            except ValueError as error:
                return False


        if background != job.background and self.scalable:
            self.Enqueue(job)
        elif job.name in self._builtins:
            self._builtins[job.name](**job.kwargs)
        else:
            if self._verbose >= 3:
                cmd = ['/bin/bash', '-x', '$0', job.flow] + str(job).split(' ')
            else:
                cmd = ['/bin/bash', '$0', job.flow] + str(job).split(' ')

            self.__exec__(job, cmd, callbacks=[produce_jobs_when_finish])

    def Get(self, job, background=False):
        def expect_with_pattern(line):
            keep = True

            try:
                if job.expect is None:
                    return True
            except Exception:
                return True
 
            while keep:
                resp, next = Tunnel.Parse(line)

                if job.expect.get('code') and job.expect['code'] != resp.get('code'):
                    keep = False
                elif job.expect.get('type') and job.expect['type'] != resp.get('type'):
                    keep = False

                if len(next) > 0:
                    keep = True
                else:
                    return keep
            else:
                return False
            return True

        def store_with_pattern(line):
            keep = True
            done = False

            try:
                if job.store is None:
                    return True
            except Exception:
                return True

            while keep:
                resp, next = Tunnel.Parse(line)
                done = False

                if resp is None:
                    keep = False
                    done = True

                if done is False:
                    try:
                        parsed = json.loads(resp['body'])

                        for name in job.store:
                            value = parsed

                            for step in job.store[name].split(':'):
                                value = value[step]
                            else:
                                self._environments[name] = value
                                done = True
                    except Exception:
                        pass

                if len(next) > 0:
                    keep = True
                else:
                    return done
            else:
                return False


        if self.scalable and background != job.background:
            return self.Enqueue(job)

        if self._verbose >= 3:
            cmd = ['/bin/bash', '-x', '$0', job.flow, job.name, '--url', str(job)]
        else:
            cmd = ['/bin/bash', '$0', job.flow, job.name, '--url', str(job)]

        if self._verbose >= 3:
            cmd.extend(['--verbose'])

        try:
            cmd.extend(['--timeout', job.timeout])
        except Exception:
            pass

        if len(self._headers) > 0:
            cmd.extend(['--headers', ';'.join(self._headers)])

        if 'proxy' in self._environments:
            cmd.extend(['--proxy', self._environments['proxy']])

        cmd.extend(['--cookies', self.cookies,
                    '--cookie', '$(pwd)/colab.cookies',
                    '--cookie-jar', '$(pwd)/colab.cookies'])

        self.__exec__(job, cmd, callbacks=[expect_with_pattern])

    def Post(self, job, background=False):
        def expect_repsonse_ok(line):
            return line == 'ok'

        if self.scalable and background != job.background:
            return self.Enqueue(job)

        if job.data is None:
            return None

        if self._verbose >= 3:
            cmd = ['/bin/bash', '-x', '$0', job.flow, job.name, '--url', str(job)]
        else:
            cmd = ['/bin/bash', '$0', job.flow, job.name, '--url', str(job)]

        if self._verbose >= 3:
            cmd.extend(['--verbose'])

        if len(self._headers) > 0:
            cmd.extend(['--headers', ';'.join(self._headers)])

        try:
            cmd.extend(['--timeout', job.timeout])
        except Exception:
            pass

        if isinstance(job.data, dict):
            data = Tunnel.Serialize(job.data)

            if data is None:
                self.status = -1
        else:
            if sys.version_info[0] < 3:
                data = job.data.encode('utf-8')
            else:
                data = job.data.encode('utf-8')

        if 'proxy' in self._environments:
            cmd.extend(['--proxy', self._environments['proxy']])

        cmd.extend(['--data', data])
        cmd.extend(['--cookies', self.cookies,
                    '--cookie', '$(pwd)/colab.cookies',
                    '--cookie-jar', '$(pwd)/colab.cookies'])

        self.__exec__(job, cmd, callbacks=[expect_repsonse_ok])

    def Find(self, name): 
        return self._jobqueue.Find(name, compare=lambda x, y: x == y.name)

    def Set(self, job):
        for name in job.values:
            if name == 'status':
                self.status = int(job.values['status'])
            elif name == 'done':
                self._done = int(job.values['done']) != 1
            elif name == 'cookies':
                for cookie in job.values['cookies'].split(';'):
                    try:
                        key, value = cookie.split('=')

                        for callback in self._callbacks.get('cookies') or []:
                            if callback(value) is False:
                                break
                        else:
                            self._cookies[key] = value
                    except Exception:
                        pass
            else:
                self.environments = (name, job.values[name])
        else:
            job.result = True

    def Callback(self, job):
       pass

    def Print(self, job):
       pass

    @staticmethod
    def Serialize(packet):
       raw = '{}/{},{}'.format(packet['code'], packet['type'], packet['body'])
       return '{}:{}'.format(len(raw), raw)

    @staticmethod
    def Parse(line):
       result = {}
       save = line

       while len(line) > 0:
           size, line = slice(line, ':', int)

           if not size is None:
               block = line[0:size]
               line = line[size:] if size < len(line) else ''
               code, block = slice(block, '/', int)

               if code is None:
                   continue
               else:
                   result['code'] = code

               type, body = slice(block, ',', str)

               if type is None:
                   continue
               else:
                   result['type'] = type
                   result['body'] = body
               return result, line
       else:
            return None, save

    @staticmethod
    def Time():
        return '0'

def slice(string, sample, cast):
    if isinstance(string, bytes):
        string = string.decode('utf-8')
    end = string.find(sample)

    if end >= 0:
        return cast(string[0:end]), string[end + 1:]
    else:
        return None, string

def wait(lock):
    lock.acquire()
    lock.release()

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

def is_okey_to_continue(lock, check):
    if callable(check) is False:
        raise AssertionError('\'check\' must be callable')

    if not lock is None:
        lock.acquire()

    ret = check()

    if not lock is None:
        lock.release()
    return ret

def can_run_on_multi_thread(how_many_step_to_verify=300, parallel_core=4, check=True):
    from threading import Lock, Thread

    global lock
    global count
    global result

    import platform

    if multiprocessing.cpu_count() > 1:
        return True
    if check is False:
        return False

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

if __name__ == '__main__':
    with Tunnel('$COOKIES'.split(';'), verbose=$VERBOSE) as tunnel:
        try:
            for job in tunnel.Communicate():
                if job.flow == 'interact' or job.flow == 'analyse':
                    tunnel.Interact(job)
                elif job.flow == 'set':
                    tunnel.Set(job)
                elif job.flow == 'get':
                    tunnel.Get(job)
                elif job.flow == 'post':
                    tunnel.Post(job)
                elif job.flow == 'callback':
                    tunnel.Callback(job)
                elif job.flow == 'print':
                    tunnel.Print(job)
        except StopIteration as error:
            print(error)
"""
}

function create_tunnel() {
	NBH=$1

	if [ "$VERBOSE" != '0' ]; then
		DEBUG='-v'
	else
		DEBUG=''
	fi

	if [ ! -f "$(pwd)/colab.cookies" ]; then
		RAW=$($CURL $DEBUG -sS   --request GET                 	\
					--header "Cookie: $COOKIES"               	\
					--cookie-jar "$(pwd)/colab.cookies"	      	\
				"https://colab.research.google.com/tun/m/assign?authuser=0&isolate=1&nbh=$NBH")
	else
		RAW=$($CURL $DEBUG -sS   --request GET                 	\
					--header "Cookie: $COOKIES"               	\
					--cookie "$(pwd)/colab.cookies"             \
					--cookie-jar "$(pwd)/colab.cookies"	      	\
				"https://colab.research.google.com/tun/m/assign?authuser=0&isolate=1&nbh=$NBH")
	fi

	RAW=$(echo $RAW | cut -b 5-)

	if ! echo "$RAW" | grep "endpoint" >& /dev/null; then
		TOKEN=$(echo $RAW | python -c "import sys, json; print(json.load(sys.stdin).get('token') or '')")

		if [[ ${#TOKEN} -eq 0 ]]; then
			exit -1
		else
			read -r -d '' CMD <<- EOF
$CURL $DEBUG -sS --request POST                                                                         \
	 	 --header "Cookie: $COOKIES"                                                                    \
		 --cookie-jar "$(pwd)/colab.cookies"                                                            \
		 --header 'Content-Type: multipart/form-data; boundary=----WebKitFormBoundaryMJ16T76Zz4nXWJQz'  \
		 --data-binary $'------WebKitFormBoundaryMJ16T76Zz4nXWJQz\r\nContent-Disposition: form-data; name="token"\r\n\r\n$TOKEN\r\n------WebKitFormBoundaryMJ16T76Zz4nXWJQz--\r\n'
	'https://colab.research.google.com/tun/m/assign?authuser=0&isolate=1&nbh=LcrDbY25xQafZ6YAD481P6hQzSKyKGYHbTbwyHn-nP4.' 
EOF
			RAW=$(eval $CMD)

			if [[ $? -eq 0 ]]; then
				RAW=$(echo $RAW | cut -b 5-)
			else
				exit -1
			fi
		fi
	fi

	if [ -f $(pwd)/colab.cookies ]; then
		if cat $(pwd)/colab.cookies | grep 'SIDCC' >& /dev/null; then
			COOKIES="$COOKIES; SIDCC=$(cat $(pwd)/colab.cookies | tail -n +5 | awk '{print $7}')"
		else
			if [ $VERBOSE = '1' ]; then
				cat $(pwd)/colab.cookies
			fi
			exit -1
		fi
	fi

	echo $RAW | python -c "import json, sys; print(json.load(sys.stdin)['endpoint'])"
}

function load_session() {
	AUTH=$1
	TUNNEL=$2
	COOKIES=$3

	shift
	shift
	shift

	if [ "$VERBOSE" != '0' ]; then
		DEBUG='-v'
	else
		DEBUG=''
	fi

	RAW=$($CURL $DEBUG -sS 	--request POST          \
 			 	--header "Cookie: $COOKIES"         \
				--header 'X-Colab-Tunnel: Google'   \
				 $@                                 \
		--data '{"name":"Welcome%20To%20Colaboratory","path":"fileId=%2Fv2%2Fexternal%2Fnotebooks%2Fwelcome.ipynb","type":"notebook","kernel":{"name":"python3"}}' "https://colab.research.google.com/tun/m/$TUNNEL/api/sessions?authuser=$AUTH")
	
	if [ $? != 0 ]; then
		return $?
	else
		KERNEL=$(echo $RAW | python -c """
import json, sys

try:
    print(json.load(sys.stdin)['kernel']['id'])
except Exception:
    sys.exit(-1)
""")

			return $?
	fi
}

function register_websocket() {
	AUTH=$1
	TUNNEL=$2

	shift
	shift

	if [ "$VERBOSE" != '0' ]; then
		DEBUG='-v'
	else
		DEBUG=''
	fi

	RAW=$($CURL $DEBUG -sS 	--request GET           \
				--header 'X-Colab-Tunnel: Google'   \
 				--header "Cookie: $COOKIES"         \
				$@                                  \
			"https://colab.research.google.com/tun/m/$TUNNEL/socket.io/?authuser=0&EIO=3&transport=polling" |
		cut -b 5- | rev | cut -b 5- | rev)

	if [ $? != 0 ]; then
		exit -1
	fi

	SID=$(echo $RAW | python -c """
import json, sys

try:
    print(json.load(sys.stdin)['sid'])
except Exception:
    sys.exit(-1)
	""")

	if [ $? != 0 ]; then
		exit -1
	fi

	WSS="wss://colab.research.google.com/tun/m/$TUNNEL/socket.io/"

# @NOTE: this is the session_id code from javascript
#	var a = (new at).getTime();
#	return "xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx".replace(/[xy]/g, function(b) {
#		var c = (a + Math.floor(16 * Math.random())) % 16;
#		a = dh(a / 16);
#						                    
#		"y" == b && (c = c & 7 | 8);							                    
#		return c.toString(16)
#	})

	# @NOTE: update variables
	echo "{\"sid\":{\"flow\":\"set\",\"values\":{\"sid\":\"$SID\"}}}"
	echo "{\"kernel\":{\"flow\":\"set\",\"values\":{\"kernel\":\"$KERNEL\"}}}"

	# @NOTE: open a new websocket
	echo "{\"authenticate\":{\"flow\":\"post\",\"data\":\"22:40/session?authuser=0,\",\"dependencies\":[\"set:sid\"]}}"
	echo "{\"confirm\":{\"flow\":\"get\",\"expect\":{\"code\":40,\"type\":\"session\"},\"dependencies\":[\"post:authenticate\"]}}"
	echo "{\"start\":{\"flow\":\"post\",\"data\":{\"code\": 40,\"type\":\"session\",\"body\":\"$WSS\"},\"dependencies\":[\"get:confirm\"]}}"
	echo "{\"open\":{\"flow\":\"get\",\"store\":{\"wss\":\"1:url\"},\"dependencies\":[\"post:start\"]}}"

	# @NOTE: exit 
	echo "{\"exit\":{\"flow\":\"set\", \"values\":{\"done\":-1}},\"dependencies\":[\"get:open\"]}"
}

function start_websocket() {
	URL=$1
}

function update_proxier() {
	if [[ ${#PROXY} -gt 0 ]]; then
		CURL="$(which curl) -x $PROXY"
	else
		CURL="$(which curl)"
	fi
}

if [ "$1" = 'interact' ] || [ "$1" = 'analyze' ] || [ "$1" = 'config' ] || [ "$1" = 'post' ] || [ "$1" = 'get' ]; then
	# @NOTE: this place only run inside a child proc and it has been called
	# by the Tunnel to manipulate Google colab

	CMD=$1
	NAME=$2
	VERBOSE=0

	function init() { 
		AUTH=0

		while [[ $# -gt 0 ]]; do
			case $1 in
				--envs)         ENV=$2; shift;;
				--vars)         VAR=$2; shift;;
				--auth) 	AUTH=$2; shift;;
				--proxy)	PROXY=$2; shift;;
				--tunnel) 	TUNNEL=$2; shift;;
				--cookies) 	COOKIES=$2; shift;;
				--background)   BACKGROUND=$2; shift;;
				--verbose) 	VERBOSE=1;;
				--)             shift; break;;
				(*)             break;;
			esac
			shift
		done

		update_proxier	
		for I in {0..3}; do
			if load_session $AUTH $TUNNEL $COOKIES $@; then
				break
			else
				sleep 1
			fi
		done

		if [[ ${#KERNEL} -eq 0 ]]; then
			exit -1
		elif ! register_websocket $AUTH $TUNNEL; then
			exit -1
		fi
	}

	function analyze() {
		while [[ $# -gt 0 ]]; do
  			case $1 in
				--verbose) 	VERBOSE=1;;
				--)             shift; break;;
				(*)             exit -1;;
			esac
			shift
		done
	}

	function interact() {
		if [ $NAME = 'init' ]; then
			init $@
		else
			while [[ $# -gt 0 ]]; do
				case $1 in
					--verbose) 	VERBOSE=1;;
					--)             shift; break;;
					(*)             exit -1;;
				esac
				shift
			done
		fi
	}

	function get() {
		TIMEOUT=5

		while [[ $# -gt 0 ]]; do
			case $1 in
				--timeout) 	TIMEOUT=$2; shift;;
				--verbose) 	VERBOSE=1;;
				--headers) 	HEADERS=$2; shift;;
				--cookies) 	COOKIES=$2; shift;;
				--proxy) 	PROXY=$2; shift;;
				--url) 		URL=$2; shift;;
				--)             shift; break;;
				(*)             break;;
			esac
			shift
		done

		if [ "$VERBOSE" != '0' ]; then
			DEBUG='-v'
		else
			DEBUG=''
		fi

		update_proxier	
		timeout $TIMEOUT $CURL $DEBUG -sS --fail  --request GET             \
					 		 --header "Cookie: $COOKIES"                    \
	 						 --header 'X-Colab-Tunnel: Google'              \
							$@                                              \
							$URL
		exit $?
	}

	function post() {
		TIMEOUT=10

		while [[ $# -gt 0 ]]; do
			case $1 in
				--timeout) 	TIMEOUT=$2; shift;;
				--verbose) 	VERBOSE=1;;
				--headers) 	HEADERS=$2; shift;;
				--cookies) 	COOKIES=$2; shift;;
				--proxy) 	PROXY=$2; shift;;
				--data) 	DATA=$2; shift;;
				--url) 		URL=$2; shift;;
				--)             shift; break;;
				(*)             break;;
			esac
			shift
		done
	
		if [ "$VERBOSE" != '0' ]; then
			DEBUG='-v'
		else
			DEBUG=''
		fi

		update_proxier	
		timeout $TIMEOUT $CURL $DEBUG -sS --fail  --request POST                        \
							 --header "Cookie: $COOKIES"                                \
 	 						 --header 'X-Colab-Tunnel: Google'                          \
							 $@                                                         \
		 					 --data $DATA                                               \
							$URL
	}

	shift
	shift


	case $CMD in
		get)        get $@;;
		post)       post $@;;
		config)     config $@;;
		analyze)    analyze $@;;
		interact)   interact $@;;
		(*)         exit -1;;
	esac
else
	VERBOSE=0
	SCALABLE=0
	COOKIES='SID=rAddzFtVk9ZIXysJAyWe5ulEaFmjtTMtll7etBaF15BkELsSyw8goBFHXhScAhn86cZ_IA.; HSID=AXBDOGM1QJXir4oo3; SSID=AALDfmzW1Og8o37yi '
	TRANSACTION='LcrDbY25xQafZ6YAD481P6hQzSKyKGYHbTbwyHn-nP4.'

	while [[ $# -gt 0 ]]; do
		case $1 in
			--proxy) 		PROXY=$2; shift;;
			--verbose)		VERBOSE=$2; shift;;
			--cookies)      	COOKIES=$2; shift;;
			--scalable) 		SCALABLE=1;;
			--transaction)  	TRANSACTION=$2; shift;;
			(--)		    	shift; break;;
			(*)		        ;;
		esac
		shift
	done

	update_proxier

	if [ $VERBOSE = '1' ]; then
		set -x
	fi

	TUNNEL=$(create_tunnel $TRANSACTION)
	if [ $? != 0 ]; then
		exit -1
	else
		set +x
	fi

	main
fi
