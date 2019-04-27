import os
import shutil
import subprocess
import tempfile

from . import Plugin


class Git(Plugin):
    def __init__(self, **kwargs):
        super(Git, self).__init__()
        self._git = kwargs.get('git-binary')
        self._sh = kwargs.get('sh-binary')

    def check(self):
        if self._git is None:
            self._git = shutil.which('git')

        if self._git is None:
            raise AssertionError('plugin \'Git\' requires \'git\' to be installed')

        if self._sh is None:
            for sh in ['sh', 'bash', 'zsh']:
                self._sh = shutil.which(sh)

                if not self._sh is None:
                    break

        if self._sh is None:
            raise AssertionError('plugin \'Git\' requires \'sh\' to be installed')

    def define(self):
        @self.rule
        def git_repository(name, remote, commit=None, branch=None,
                           init_submodules=False, sha256=None, tag=None, 
                           output=None):
            def info(is_on_running, is_finish_successful=False, **kwargs):
                if is_on_running == True:
                    if is_finish_successful is True:
                        print('Finish cloning: %s' % remote)
                    else:
                        print('Clone: %s' % remote)
                else:
                    print('Remove: %s' % remote.split('.git')[0].split('/')[-1])

            def callback(root, **kwargs):
                fd, shscript = tempfile.mkstemp(suffix='.sh')
                repo_name = remote.split('.git')[0].split('/')[-1]
                cmd = [self._sh, shscript]
                git = [self._git, 'clone']

                # @NOTE: sometime we need to clone a repo to exact directory
                # so we must instruct 'builder' to do it
                if not output is None:
                    for dir_name in output.split('/'):
                        root = '%s/%s' % (root, dir_name)

                        if os.path.exists(root) is False:
                            os.mkdir(root, 0o777)

                # @NOTE: clone repo if it's needed
                if init_submodules is True:
                    git.append('-recurse-submodules')
                    git.appned('-j%d' % os.cpu_count())

                if not branch is None:
                    git.appned('--single-branch')
                    git.append('-b')
                    git.append(branch)
                git.append(remote)

                script = 'cd %s;%s' % (root, ' '.join(git))
                os.write(fd, script.encode())
                os.close(fd)

                clone = subprocess.Popen(cmd, stdout=subprocess.PIPE, \
                                         stderr=subprocess.PIPE)
                error = clone.stderr.read()
                os.remove(shscript)

                expected_msg = "Cloning into '%s'...\n" % repo_name
                if len(error) > 0 and error.decode('utf-8') != expected_msg:
                    raise AssertionError('there is an error while perform '
                                         'cloning repo %s' % (remote, error.decode('utf-8')))

                # @NOTE: jump to exact commit of this repo if it's requested
                if (not commit is None) or (not sha256 is None) or (not tag is None):
                    fd, shscript = tempfile.mkstemp(suffix='.sh')

                    cmd = [self._sh, shscript]

                    if not commit is None:
                        script = 'cd %s;cd %s;%s reset --hard %s' % (root, repo_name, commit)
                    elif not sha256 is None:
                        script = 'cd %s;cd %s;%s reset --hard %s' % (root, repo_name, sha256)
                    elif not tag is None:
                        script = 'cd %s;cd %s;%s checkout tags/%s' % (root, repo_name, tag)

                    os.write(fd, script.encode())
                    os.close(fd)

                    changing = subprocess.Popen(cmd, stdout=subprocess.PIPE, \
                                                stderr=subprocess.PIPE)
                    error = changing.stderr.read()
                    os.remove(shscript)

                    if len(error) > 0:
                        raise AssertionError('there is an error while perform '
                                             'changing repo %s to exact commit as you requested: %s'
                                             % (remote, error.decode('utf-8')))
                return True

            def teardown(root, **kwargs):
                fd, shscript = tempfile.mkstemp(suffix='.sh')
                repo_name = remote.split('.git')[0].split('/')[-1]
                cmd = [self._sh, shscript]

                if output is None:
                    script = 'rm -fr %s/%s' % (root, repo_name)
                else:
                    script = 'cd %s/%s;%s' % (root, output, ' '.join(git))
                
                os.write(fd, script.encode())
                os.close(fd)

                removing = subprocess.Popen(cmd, stdout=subprocess.PIPE, \
                                            stderr=subprocess.PIPE)
                error = removing.stderr.read()
                os.remove(shscript)

                if len(error) > 0:
                    raise AssertionError('there is an error while perform '
                                         'removing repo %s: %s' % (remote, error.decode('utf-8')))
                return True

            node = {
                'callback': callback,
                'info': info,
                'teardown': teardown,
                'remote': remote
            }
            self._manager.add_to_dependency_tree(name, node, None)

        @self.rule
        def new_git_repository(name, remote, commit=None,
                               init_submodules=False, sha256=None, tag=None):
            pass
