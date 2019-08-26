#!/bin/bash

if [[ "$1" =~ 'ftp://' ]]; then
    CMD='download'
    LOCAL=$2
    REMOTE=$1
elif [[ "$2" =~ 'ftp://' ]]; then
    CMD='upload'
    LOCAL=$1
    REMOTE=$2
else
    exit -1
fi

python -c """
from time import time, sleep
from socket import gaierror, timeout
from glob import glob
from ftplib import FTP, error_perm, error_temp, all_errors
from threading import Thread, Lock, current_thread

try:
    from queue import Queue, Empty
except ImportError:
    from Queue import Queue, Empty

import sys
import os

class Launcher(object):
    def __init__(self, remote, blocksize=102400, timeout=60):
        super(Launcher, self).__init__()
        self._host = remote.split('@')[1].split('/')[0]
        self._user = remote.split('://')[1].split(':')[0]
        self._passwd = remote.split('://')[1].split(':')[1].split('@')[0]
        self._status = {
            'online': 0,
            'urate': 0.0,
            'drate': 0.0,
            'cpu': 0.0,
            'ram': 0.0,
            'timeout': 0,
            'spawning': 0,
            'uspeed': True,
            'dspeed': True,
            'resource': True,
            'server': True
        }
        self._sched = Queue()
        self._watch = None
        self._mapping = {}
        self._timeout = timeout
        self._isfinish = False
        self._blocksize = blocksize

    def __enter__(self):
        def watch():
            spawning = 0

            while self._isfinish is False or self._sched.qsize() > 0:
                okey = self._status['online'] == 0 or self._status['spawning'] < 5

                for name in self._status:
                    if isinstance(self._status[name], bool):
                        if self._status[name] is False:
                            break
                else:
                    okey = self._status['spawning'] < 5

                sys.stdout.write('\\r>> running {}, pending {}, up/down {}/{}'
                    .format(self._status['online'], self._sched.qsize(),
                            self._status['urate']/1024.0,
                            self._status['drate']/1024.0))
                sys.stdout.flush()

                try:
                    if okey is True:
                        session = FTP(self._host, timeout=self._timeout)

                        runner = Runner(self)
                        thread = Thread(target=self.__boot__, args=[runner])

                        self._mapping[thread.getName()] = [session, runner, thread]
                        self._status['spawning'] += 1

                        session.login(user=self._user, passwd=self._passwd)
                        thread.start()
                    else:
                        if self._status['uspeed'] or self._status['dspeed']:
                            self._status['spawning'] = 0
                except timeout as error:
                    self._status['timeout'] += 1
                except error_temp as error:
                    self._status['server'] = False
                    self._status['spawning'] = 0
                except (gaierror, error_perm, EOFError) as error:
                    self._isfinish = True

                    with self._sched.mutex:
                        self._sched.queue.clear()
                    sys.exit(-1)
                finally:
                    sleep(1)
            else:
                while self._status['online'] > 0:
                    sleep(1)

        if self._watch is None:
            self._watch = Thread(target=watch)
            self._watch.start()
        return self

    def __exit__(self, type, value, traceback):
        self._isfinish = True

        for tname in self._mapping:
            session, _, thread = self._mapping[tname]
            thread.join()
            session.quit()
        else:
            self._watch.join()

        self._watch = None
        self._isfinish = False

    def update(self, status, value):
        if status == 'uspeed':
            urate = 0.0

            for name in self._mapping:
                runner = self._mapping[name][1]

                if runner:
                    urate += runner.upload
            else:
                self._status['uspeed'] = self._status['urate'] > urate
                self._status['urate'] = urate
        elif status == 'dspeed':
            drate = 0.0

            for name in self._mapping:
                runner = self._mapping[name][1]

                if runner:
                    drate += runner.download
            else:
                self._status['dspeed'] = self._status['drate'] > drate
                self._status['drate'] = drate

    def enqueue(self, cmd, rpath=None, lpath=None):
        instruct = None
        if cmd == 'upload':
            instruct = Instruct(3, 'STOR %s' % os.path.basename(rpath),
                                rpath=rpath, fp=open(lpath))
        elif cmd == 'download':
            instruct = Instruct(1, 'RETR %s' % os.path.basename(rpath),
                                rpath=rpath, lpath=lpath)

        if not instruct is None:
            self._sched.put(instruct)

        return instruct

    def revive(self, function):
        def wrapper(*args, **kwargs):
            session = self._mapping[current_thread().name][0]
            keep = True

            while keep:
                try:
                    try:
                        return function(*args, **kwargs)
                    except (EOFError, error_perm, AttributeError) as error:
                        if self._sched.qsize() > 0:
                            session.connect(timeout=self._timeout)
                            session.login(user=self._user, passwd=self._passwd)
                        else:
                            keep = False
                    except error_temp as error:
                        keep = False
                except Exception as error:
                    keep = False
            else:
                self._status['server'] = True
            return None
        return wrapper

    def retrlines(self, cmd, **kwargs):
        return self._mapping[current_thread().name][0].retrlines(cmd, **kwargs)

    def retrbinary(self, cmd, rpath, lpath=None, callback=None, **kwargs):
        session, runner, _ = self._mapping[current_thread().name]
        root = session.pwd()
        blocksize = kwargs.get('blocksize')

        try:
            session.cwd(os.path.dirname(rpath))

            if not 'blocksize' in kwargs:
                kwargs['blocksize'] = self._blocksize

            if callback is None:
                fd = open(lpath, 'wb')

                def meter(data):
                    runner = self._mapping[current_thread().name][1]
                    runner.clock = self.__speedmeter__(True, runner.clock,
                                                       blocksize)
                    fd.write(data)
            else:
                def meter(data):
                    runner = self._mapping[current_thread().name][1]
                    runner.clock = self.__speedmeter__(True, runner.clock,
                                                       blocksize)
                    callback(data)

            runner.clock = time()
            return session.retrbinary(cmd, callback=meter, **kwargs)
        except IOError:
            return ''
        finally:
            session.cwd(root)

    def storlines(self, cmd, **kwargs):
        return self._mapping[current_thread().name][0].storlines(cmd, **kwargs)

    def storbinary(self, cmd, fp, rpath, callback=None, **kwargs):
        session, runner, _ = self._mapping[current_thread().name]
        root = session.pwd()
        blocksize = kwargs.get('blocksize')

        def meter(data):
            runner = self._mapping[current_thread().name][1]
            runner.clock = self.__speedmeter__(False, runner.clock,
                                               blocksize)
            if callback:
                callback(data)
        try:
            Launcher.cd(session, os.path.dirname(rpath))

            if not 'blocksize' in kwargs:
                kwargs['blocksize'] = self._blocksize

            runner.clock = time()
            result = session.storbinary(cmd, callback=meter, fp=fp, **kwargs)

        finally:
            Launcher.cd(session, root)
            fp.close()
        return result

    @staticmethod
    def rm(session, path, keep=False):
        wd = session.pwd()

        try:
            for name in session.nlst(path):
                if os.path.split(name)[1] in ('.', '..'):
                    continue

                try:
                    session.cwd(name)
                    session.cwd(wd)
                
                    Launcher.rm(session, name)
                except all_errors:
                    session.delete(name)
        except all_errors:
            keep = True

        if keep:
            try:
                session.rmd(path)
            except all_errors:
                pass

    @staticmethod
    def login(host, user, passwd):
        try:        
            session = FTP(self._host, timeout=self._timeout)
            session.login(self._user, self._passwd)

            return session
        except all_errors as error:
            pass

    @staticmethod
    def logout(session):
        session.quit()

    @staticmethod
    def cd(session, folder):
        if folder != '':
            try:
                session.cwd(folder)
            except IOError:
                Launcher.cd(session, '/'.join(folder.split('/')[:-1]))
                session.mkd(folder)
                session.cwd(folder)
        
    def __speedmeter__(self, io, begin, blocksize):
        current = time()
        runner = self._mapping[current_thread().name][1]

        if blocksize is None:
            blocksize = self._blocksize

        if io:
            runner.download = (1.0*blocksize)/(current - begin)
        else:
            runner.upload = (1.0*blocksize)/(current - begin)
        return current

    def __boot__(self, runner):
        keep = True

        while self._mapping.get(current_thread().name) is None:
            sleep(1)
        else:
            self._status['online'] += 1

        while keep and (self._isfinish is False or self._sched.qsize() > 0):
            try:
                instruct = self._sched.get(timeout=self._timeout)

                for dep in instruct.dependencies:
                    if dep.isfinish is False:
                        self._sched.put(instruct)
                else:
                    result = runner.process(instruct)

                    if result is None or len(result) == 0:
                        self._sched.put(instruct)
                    else:
                        instruct.done()

                    keep = not result is None
            except Empty:
               pass
        else:
            self._mapping[current_thread().name] = [None, None, None]
            self._status['online'] -= 1


class Instruct(object):
    def __init__(self, mode, cmd, **kwargs):
        super(Instruct, self).__init__()
        self._dependencies = []
        self._done = False
        self._mode = mode
        self._cmd = cmd
        self._kwargs = kwargs

    def done(self):
        self._done = True

    def dependby(self, instruct):
        self._dependencies.append(instruct)

    @property
    def dependencies(self):
        return self._dependencies

    @property
    def isfinish(self):
        return self._done

    @property
    def kwargs(self):
        return self._kwargs

    @kwargs.setter
    def kwargs(self, value):
        self._kwargs = value

    @property
    def mode(self):
        return self._mode

    @mode.setter
    def mode(self, value):
        self._mode = value

    @property
    def cmd(self):
        return self._cmd

    @cmd.setter
    def cmd(self, value):
        self._cmd = value


class Runner(object):
    def __init__(self, launcher):
        super(Runner, self).__init__()
        self.clock = None

        self._launcher = launcher
        self._upload = None
        self._download = None

    def process(self, instruct):
        cmd = instruct.cmd
        mode = instruct.mode
        kwargs = instruct.kwargs

        @self._launcher.revive
        def capsule():
            if mode == 0:
                return self._launcher.retrlines(cmd, **kwargs)
            elif mode == 1:
                return self._launcher.retrbinary(cmd, **kwargs)
            elif mode == 2:
                return self._launcher.storlines(cmd, **kwargs)
            elif mode == 3:
                return self._launcher.storbinary(cmd, **kwargs)
            else:
                return ''

        return capsule()

    @property
    def upload(self):
        return self._upload or 0.0

    @upload.setter
    def upload(self, value):
        old, self._upload = self._upload, value

        if not old is None:
            if old < value:
                self._launcher.update('uspeed', True)
            elif old == value:
                pass
            else:
                self._launcher.update('uspeed', False)

    @property
    def download(self):
        return self._download or 0.0

    @download.setter
    def download(self, value):
        old, self._download = self._download, value

        if not old is None:
            if old < value:
                self._launcher.update('dspeed', True)
            elif old == value:
                pass
            else:
                self._launcher.update('dspeed', False)


if __name__ == '__main__':
    root = '$REMOTE'.split('@')[1].split('/')[1]

    with Launcher('$REMOTE') as launcher:
        if os.path.isfile('$LOCAL'):
            launcher.enqueue('$CMD', '$REMOTE', '$LOCAL')
        else:
            schedule = ['$LOCAL']
            index = 0

            while index < len(schedule):
                for lpath in glob('%s/*' % schedule[index]):
                    if os.path.isfile(lpath):
                        rpath = lpath[len('$LOCAL'):-1]
                        launcher.enqueue('$CMD', '/'.join([root, rpath]), lpath)
                    else:
                        schedule.append(lpath)
                else:
                    index += 1
"""
exit $?
