from . import Backend
from ..logger import Logger
from .file import is_file_existed
from platform import linux_distribution, win32_ver, mac_ver, system

import subprocess
import sys

class Package(Backend):
    """ Package is a class which helps to manage pakage install on some popular
        distro like Ubuntu, Debian, Centos, Fedora, Opensuse, MacOS
    """

    def __init__(self, **kwargs):
        super(Package, self).__init__()

        self._maximum_retry_steps = kwargs.get('maximum_retry_steps') or 10
        if 'auto_update_packages' in kwargs:
            self._updated = not kwargs['auto_update_packages']
        else:
            self._updated = False

        self._list_installed_packages = set()
        self._use_package_management = kwargs.get('use_package_management') or False
        self._fixme = None
        self._install = None
        self._upgrade = None
        self._update = None

    def define(self):
        @self.rule
        def pkg_install(name, packages=None, force=False):
            def callback(**kwargs):
                if isinstance(packages, list) is True:
                    for package in packages:
                        if self.install(package) is False:
                            raise AssertionError('can\'t install package `%s`' \
                                                 % package)
                else:
                    if self.install(name) is False:
                        raise AssertionError('can\'t install package `%s`' \
                                             % name)

            if force is True:
                self.install(name, force=True)
            else:
                node = { 'callback': callback }
                self._manager.add_to_dependency_tree(name, node, None)

        @self.rule
        def pkg_update(name, force=False):
            def callback(**kwargs):
                self.update()

            if force is True:
                self.update()
            else:
                node = { 'callback': callback }
                self._manager.add_to_dependency_tree(name, node, None)

        @self.rule
        def pkg_upgrade(name, force=False):
            def callback(**kwargs):
                self.upgrade()

            if force is True:
                self.upgrade()
            else:
                node = { 'callback': callback }
                self._manager.add_to_dependency_tree(name, node, None)

    def install(self, package, force=False):
        import sys

        if self._use_package_management is False:
            return True
        if sys.version_info >= (3, 0):
            xrange = range

        if package in self._list_installed_packages:
            if force is False:
                return True

        for _ in range(self._maximum_retry_steps):
            cmd = '%s %s' % (self._install, package)
            ret = subprocess.Popen(cmd.split(' '),
                                   stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE)
            console = ret.stderr.read()

            ret.communicate()
            ret.wait()

            if ret.returncode != 0:
                # @NOTE: since Python2 usually return to str but Python3
                # would prefer bytes
                if isinstance(console, bytes):
                    console = console.decode('utf-8')
                elif sys.version_info < (3, 0) and isinstance(console, unicode):
                    console = console.encode('ascii', 'ignore')

                if self._fixme is None or self._fixme(console) is False:
                    raise AssertionError('can\'t fix this error\n: %s' % console)
                else:
                    Logger.debug('successful fixing a bug on Package -> going to run again')
            else:
                Logger.debug('successful install package `%s` '
                          '-> adding to list of installed packages' % package)
                self._list_installed_packages.add(package)
                return True
        else:
            return False

    def need_update(self):
        self._updated = False

    def update(self, passed=False):
        if self._use_package_management is False:
            return True
        if self._updated is True:
            return True
        elif self._update is None:
            return False

        ret = subprocess.Popen(self._update.split(' '),
                               stdout=subprocess.PIPE,
                               stderr=subprocess.PIPE)
        console = ret.stderr.read()

        ret.communicate()
        ret.wait()

        if ret.returncode != 0:
            # @NOTE: since Python2 usually return to str but Python3
            # would prefer bytes
            if isinstance(console, bytes):
                console = console.decode('utf-8')
            elif sys.version_info < (3, 0) and isinstance(console, unicode):
                console = console.encode('ascii', 'ignore')
            print(console)
        else:
            self._updated = True
            return True
        return False

    def upgrade(self):
        if self._use_package_management is False:
            return True
        if self._upgrade is None:
            return False

        ret = subprocess.Popen(self._upgrade.split(' '),
                               stdout=subprocess.PIPE,
                               stderr=subprocess.PIPE)
        console = ret.stderr.read()

        ret.communicate()
        ret.wait()

        if ret.returncode != 0:
            # @NOTE: since Python2 usually return to str but Python3
            # would prefer bytes
            if isinstance(console, bytes):
                console = console.decode('utf-8')
            elif sys.version_info < (3, 0) and isinstance(console, unicode):
                console = console.encode('ascii', 'ignore')
            print(console)
        else:
            self._updated = True
            return True
        return False

    def check(self):
        if len(linux_distribution()[0]) > 0 or system().lower() == 'linux':
            def ubuntu():
                def fixme_on_ubuntu(console):
                    import os

                    # @NOTE: there were so many problems with package management and
                    # we never have any chance to fix it manually, so it would be
                    # better if we can detect and fix this error automatically

                    if console.find('get lock') >= 0:
                        # @NOTE: issue related to missing remove lockfile time
                        begin = console.find('lock ')
                        end = console.find(' - open')

                        if begin == -1 or end == -1:
                            return False
                        else:
                            lockfile = console[begin + len('lock '):end]

                            Logger.debug('found lockfile on `%s` '
                                         '-> remove it now' % lockfile)
                            if os.path.exists(lockfile):
                               ret = subprocess.Popen(['sudo', 'rm', '-fr', lockfile])
                               ret.communicate()
                               ret.wait()
                               return ret.returncode is None or ret.returncode == 0
                            else:
                                return True
                    elif console.find('dpkg was interrupted') >= 0:
                        # @NOTE: issue related to dpkg was interrupted unexpectedly
                        Logger.debug('found dpkg issue` '
                                     '-> run `sudo dpkg --configure -a`')

                        ret = subprocess.Popen(['sudo', 'dpkg', '--configure', '-a', '-y'])
                        ret.communicate()
                        ret.wait()

                        return ret.returncode is None or ret.returncode == 0
                    else:
                        return False

                self._install = 'sudo apt install -y --allow-unauthenticated'
                self._upgrade = 'sudo apt upgrade -y --allow-unauthenticated'
                self._update  = 'sudo apt update --allow-insecure-repositories'
                self._fixme = fixme_on_ubuntu

            def debian():
                def fixme_on_debian(console):
                    import os

                    # @NOTE: there were so many problems with package management and
                    # we never have any chance to fix it manually, so it would be
                    # better if we can detect and fix this error automatically

                    if console.find('get lock') >= 0:
                        # @NOTE: issue related to missing remove lockfile time
                        begin = console.find('lock ')
                        end = console.find(' - open')

                        if begin == -1 or end == -1:
                            return False
                        else:
                            lockfile = console[begin + len('lock '):end]

                            Logger.debug('found lockfile on `%s` '
                                         '-> remove it now' % lockfile)
                            if os.path.exists(lockfile):
                               ret = subprocess.Popen(['sudo', 'rm', '-fr', lockfile])
                               ret.communicate()
                               ret.wait()
                               return ret.returncode is None or ret.returncode == 0
                            else:
                                return True
                    elif console.find('dpkg was interrupted') >= 0:
                        # @NOTE: issue related to dpkg was interrupted unexpectedly
                        Logger.debug('found dpkg issue` '
                                     '-> run `sudo dpkg --configure -a`')

                        ret = subprocess.Popen(['sudo', 'dpkg', '--configure', '-a', '-y'])
                        ret.communicate()
                        ret.wait()

                        return ret.returncode is None or ret.returncode == 0
                    else:
                        return False

                self._install = 'sudo apt-get install -y --allow-unauthenticated'
                self._upgrade = 'sudo apt-get upgrade -y --allow-unauthenticated'
                self._update = 'sudo apt-get update --allow-insecure-repositories'
                self._fixme = fixme_on_debian

            def suse():
                def fixme_on_suse(console):
                    # @NOTE: there were so many problems with package management and
                    # we never have any chance to fix it manually, so it would be
                    # better if we can detect and fix this error automatically

                    if console.find('System management') != -1 and \
                       console.find('locked') != -1:
                        import signal
                        import os

                        # @NOTE: issue related to run more than 1 zypper at the same
                        # time
                        begin = console.find('pid ')
                        end = console.find(' (zypper)')

                        if begin == -1 or end == -1:
                            return False
                        else:
                            pid = int(console[begin + len('pid '):end])

                            Logger.debug('found another `zypper` are running on pid %d' % pid)
                            os.kill(pid, signal.SIGKILL)
                            return True
                    else:
                        return False

                self._install = 'sudo zypper install -y'
                self._upgrade = 'sudo zypper upgrade -y'
                self._update  = 'sudo zypper update'
                self._fixme = fixme_on_suse

            def redhat():
                self._install = 'sudo yum install -y'
                self._upgrade = 'sudo yum upgrade -y'
                self._update  = 'sudo yum update -y'

            def arch():
                self._install = 'sudo pacman -Sy'
                self._upgrade = 'sudo pacman -Syu'
                self._update  = 'sudo pacman -Syy'

            mapping = {
                'apt': ubuntu,
                'apt-get': debian,
                'zypper': suse,
                'yum': redhat,
                'pacman': arch
            }

            for app in mapping:
                if is_file_existed(app):
                    mapping[app]()
                    break
            else:
                raise AssertionError('Don\'t support your distro')
        elif len(win32_ver()[0]) > 0:
            raise AssertionError('Unsupport Windown recently')
        elif len(mac_ver()[0]) > 0:
            if is_file_existed('brew'):
                self._install = 'brew install'
                self._upgrade = 'brew upgrade'
                self._update = 'brew update'
        elif system().lower() == 'freebsd':
            if is_file_existed('pkg'):
                self._install = 'sudo pkg install -y'
                self._upgrade = 'sudo pkg upgrade -y'
                self._update = 'sudo pkg update'
        else:
            raise AssertionError('Unsupport %s recently' % system())
