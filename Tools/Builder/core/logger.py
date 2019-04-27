from threading import Lock
from inspect import currentframe, getframeinfo

DEBUG = 0
INFO  = 1
WARN  = 2
FATAL = 3

NoError = 0
KeepContinue = 1
NoSupport = 2
BadLogic = 3
BadAccess = 4
OutOfRange = 5
NotFound = 6
DrainMem = 7
WatchErrno = 8
Interrupted = 9
DoNothing = 10
DoAgain = 11

class Logger(object):
    _instance = None

    def __new__(self):
        if Logger._instance is None:
            Logger._instance = super(Logger, self).__new__(self)
        return Logger._instance

    def __init__(self):
        super(Logger, self).__init__()

        # @NOTE: since we are going to use singleton pattern, we must check if
        # we already have defined Logger's parameters or not
        if not '_lock' in self.__dict__:
            self._lock = Lock()
        if not '_level' in self.__dict__:
            self._level = FATAL
        if not '_stacktrace' in self.__dict__:
            self._stacktrace = None
        if not '_fullpath' in self.__dict__:
            self._fullpath = False
        if not '_silence' in self.__dict__:
            self._silence = False

    def __turn_to_silence__(self, flag):
        self._silence = flag

    def __write_exception__(self, level):
        import traceback

        self._lock.acquire()
        if not self._stacktrace is None and self._stacktrace <= level and self._silence is False:
            print('\nFound a exception has been throw from:')
            traceback.print_exc()

        self._lock.release()

    def __write_log__(self, level, code, message, line=None, filename=None):
        import traceback
        import sys

        self._lock.acquire()

        if self._fullpath is False and (not filename is None):
            filename = filename.split('/')[-1]

        if self._silence is False and self._level <= level:
            if level == WARN and code != NoError:
                if (not line is None) and (not filename is None):
                    print('[ WARNING ][ %s ]: %s (%s:%d)' % \
                        (Logger.err2str(code), message, filename, line))
                else:
                    print('[ WARNING ][ %s ]: %s' % (Logger.err2str(code), message))

            elif level == FATAL and code != NoError:
                if (not line is None) and (not filename is None):
                    print('[  ERROR  ][ %s ]: %s (%s:%d)'
                        % (Logger.err2str(code), message, filename, line))
                else:
                    print('[  ERROR  ][ %s ]: %s' % (Logger.err2str(code), message))
            elif level == DEBUG or (code == NoError and self._level == DEBUG):
                print('[   INFO  ]: %s' % message)

        if not self._stacktrace is None and self._stacktrace <= level and self._silence is False:
            traceback.print_stack()
        sys.stdout.flush()
        self._lock.release()

    @staticmethod
    def err2str(code):
        mapping = {
            NoError: "NoError",
            KeepContinue: "KeepContinue",
            NoSupport: "NoSupport",
            BadLogic: "BadLogic",
            BadAccess: "BadAccess",
            OutOfRange: "OutOfRange",
            NotFound: "NotFound",
            DrainMem: "DrainMem",
            WatchErrno: "WatchErrno",
            Interrupted: "Interrupted",
            DoNothing: "DoNothing",
            DoAgain: "DoAgain"
        }
        return mapping[code]

    @staticmethod
    def set_stacktrace(level=None):
        logger = Logger()
        logger._stacktrace = level

    @staticmethod
    def set_level(level):
        logger = Logger()
        logger._level = level

    @staticmethod
    def get_level():
        return Logger()._level

    @staticmethod
    def error(message, code=WatchErrno):
        logger = Logger()
        logger.__write_log__(level=FATAL, code=code, message=message)

    @staticmethod
    def warning(message, code=WatchErrno):
        logger = Logger()
        logger.__write_log__(level=INFO, code=code, message=message)

    @staticmethod
    def debug(message):
        logger = Logger()
        logger.__write_log__(level=DEBUG, code=NoError, message=message)

    @staticmethod
    def exception(level=FATAL):
        Logger().__write_exception__(level)

    @staticmethod
    def silence(flag):
        Logger().__turn_to_silence__(flag)
