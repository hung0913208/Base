import os
import platform
import subprocess

from . import Language

FETCHING_APT_LIST = 'sudo wget http://master.dl.sourceforge.net/project/d-apt/files/d-apt.list -O /etc/apt/sources.list.d/d-apt.list'
D_COMPILERS = ['dmd']
D_PACKAGES = ['dmd', 'dmd-compiler']

AFTER_COMPILING_FLAGS = {
    'dmd':{
        'library': '-shared',
        'test': '-main -unittest -run'
    }
}


class D(Language):
    def __init__(self, **kwargs):
        super(D, self).__init__()

        # @NOTE: some of distribute doesn't support dlang and will be handle here
        if platform.linux_distribution()[0].lower() in ['ubuntu', 'debian']:
            ret = subprocess.Popen(FETCHING_APT_LIST.split(' '), 
                                   stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE)
            error_console = ret.stderr.read()

            if not ret.returncode is None and ret.returncode != 0:
                raise AssertionError('error when fetching d-apt.list')

        # @NOTE: use default compilers if it's defined
        self.d_compile = kwargs.get('d_compile')
        self.archive = Language.which('ar')

        # @NOTE: set default flags if it's defined or use ENV's flags
        self.d_flags = kwargs.get('d_flags')
        self.ar_flags = ['rcv']

        # @NOTE: use default definition of c_flags if they are defined
        if self.d_flags is None:
            if os.environ.get('DFLAGS'):
                self.d_flags = os.environ.get('DFLAGS')
        if  self.archive is None:
            raise AssertionError('please install GNU utils first')

    def check(self):
        if self._manager.backends['config'].os == 'FreeBSD':
            return True
        elif not self.d_compile is None:
            return True

        def verify_d_compile(compile):
            return Language.test_compile("%s %s %s -of=%s", compile, ".d",
                                         self.d_flags,
                                         """
                                         import std.stdio;

                                         int main()
                                         {
                                             writeln("hello world\\n");
                                             return 0;
                                         }
                                         """, "test")

        if self.d_compile is None:
            # @NOTE: install compiler automatically if none of them are existed
            # inside the environment

            if Language.which(D_COMPILERS, count=True) == 0:
                if platform.linux_distribution()[0].lower() in ['ubuntu', 'debian']:
                    self._manager.backends['package'].need_update()

                if self._manager.package_update() is False:
                    raise AssertionError('can\'t update package')
                elif self._manager.package_install(D_PACKAGES) is False:
                    raise AssertionError('can\'t install one of these compiles %s' % str(D_COMPILERS))

            # @NOTE: select which compiler would be best to compile this project
            self.d_compile = Language.which(D_COMPILERS, verify_d_compile)
        elif verify_d_compile(self.d_compile) is False:
            raise AssertionError('please provide a real d_compile')

        if self.d_compile is None:
            raise AssertionError('cannot find any approviated d_compile ammong %s' % D_COMPILERS)
        return True

    def make(self, name, type, deps=None, root=None, workspace=None, output=None,
             compiler=None, flags=None, sources=None, imports=None):
        # @NOTE: don't support D lang with these OS
        if not self._manager.backends['config'].os in ['Linux', 'MacOS']:
            return

        # @NOTE: notify change this line when i support window
        compiler_name = compiler.split('/')[-1]
        linker = compiler
        cmds = []

        if compiler_name in AFTER_COMPILING_FLAGS:
            if type in AFTER_COMPILING_FLAGS[compiler_name]:
                after_compiling_flags = AFTER_COMPILING_FLAGS[compiler_name][type]

        if type == 'library':
            if (not self.ar_flags is None) and len(self.ar_flags) > 0:
                archive = {
                    'executor': 'ar',
                    'event': 'archiving',
                    'pattern': '%s %s %s' % (self.archive, ' '.join(self.ar_flags), '%s/%s %s'),
                    'output': (workspace, name + '.a'),
                    'input': []
                }
            else:
                archive = {
                    'executor': 'ar',
                    'event': 'invoking',
                    'pattern': self.archive + ' %s/%s %s',
                    'output': (workspace, name + '.a'),
                    'input': []
                }

            shared = {
                'executor': 'link',
                'event': 'linking',
                'pattern': linker + ' ' + after_compiling_flags + ' %s -of=%s/%s',
                'output': (workspace, name + '.so'),
                'input': []
            }
        elif type == 'binary':
            link = {
                'executor': 'link',
                'event': 'linking',
                'pattern': linker + ' %s -of=%s/%s',
                'output': (workspace, name),
                'input': []
            }
        elif type == 'test':
            link = {
                'executor': 'link',
                'event': 'linking',
                'pattern': linker + ' ' + after_compiling_flags + ' %s -of=%s/%s',
                'output': (workspace, name),
                'input': []
            }

        # @NOTE: create command to compile object files
        for source in sources:
            compile = {
                'input': '%s/%s' % (root, source),
                'output': (output, source + '.o'),
                'executor': 'd',
                'event': 'compiling',
            }

            if type == 'library':
                shared['input'].append('%s/%s.o' % (output, source))
                archive['input'].append('%s/%s.o' % (output, source))
            else:
                link['input'].append('%s/%s.o' % (output, source))

            # @TODO: we are not sure about how another d compiler handle the way
            # of building so we must note this to taking care later
            if flags is None:
                if type == 'library':
                    compile['pattern'] = '%s -fPIC %s' % (compiler, '-c %s -of=%s/%s')
                else:
                    compile['pattern'] = '%s %s' % (compiler, '-c %s -of=%s/%s')
            else:
                if type == 'library':
                    compile['pattern'] = '%s -fPIC %s %s' % (compiler, flags, '-c %s -of=%s/%s')
                else:
                    compile['pattern'] = '%s %s %s' % (compiler, flags, '-c %s -of=%s/%s')
            cmds.append(compile)
        else:
            self._performer.apply(cmds)

        # @NOTE: finishing generate command for compiling object files now we must
        # generate command to link these object files together
        if type == 'library':
            self._performer.apply([shared, archive])
        else:
            self._performer.apply([link])

            # @NOTE: if this is a testing task, just perform it after linking
            if type == 'test':
                test = {
                    'executor': 'test',
                    'event': 'invoking',
                    'pattern': '%s',
                    'input': '%s/%s' % (workspace, name),
                    'output': None
                }
                self._performer.apply([test])

    def define(self):
        @self.rule
        def d_library(name, srcs, deps=None, imports=None, linkopts=None,
                      versions=None):
            def callback(root, workspace, **kwargs):
                d_sources = self.arrange_label_build_list(name, root=root,
                                                          srcs=srcs)

                if os.path.exists(workspace) is False:
                    os.mkdir(workspace, 0o777)

                if os.path.exists('%s/objects' % workspace) is False:
                    os.mkdir('%s/objects' % workspace, 0o777)

                output = '%s/objects/%s' % (workspace, name)
                if os.path.exists(output) is False:
                    os.mkdir(output, 0o777)

                if len(d_sources) > 0:
                    # @NOTE: prepare a build instruction to help to build our
                    # d source
                    self.make(name, deps=deps, type='library', root=root, 
                              output=output, workspace=workspace,
                              compiler=self.d_compile,
                              flags=self.d_flags, sources=d_sources or [])
                else:
                    raise AssertionError('Can\'t find any sources to perform '
                        'building this project')

            def teardown(root, output, **kwargs):
                pass

            node = { 'callback': callback, 'teardown': teardown }
            self.add_to_dependencies(name, node, deps)

        @self.rule
        def d_source_library(name, srcs, deps=None, includes=None, linkopts=None,
                             versions=None):
            pass

        @self.rule
        def d_binary(name, srcs, deps=None, includes=None, linkopts=None,
                     versions=None):
            def callback(root, workspace, **kwargs):
                d_sources = self.arrange_label_build_list(name, srcs=srcs)

                if os.path.exists(workspace) is False:
                    os.mkdir(workspace, 0o777)

                if os.path.exists('%s/objects' % workspace) is False:
                    os.mkdir('%s/objects' % workspace, 0o777)

                output = '%s/objects/%s' % (workspace, name)
                if os.path.exists(output) is False:
                    os.mkdir(output, 0o777)

                if len(d_sources) > 0:
                    # @NOTE: prepare a build instruction to help to build our 
                    # d source
                    self.make(name, deps=deps, type='binary', root=root,
                              output=output, workspace=workspace,
                              compiler=self.d_compile, flags=self.d_flags,
                              sources=d_sources or [])

                else:
                    raise AssertionError('Can\'t find any sources to perform '
                        'building this project')

            def teardown(root, output, **kwargs):
                pass

            node = { 'callback': callback, 'teardown': teardown }
            self.add_to_dependencies(name, node, deps)

        @self.rule
        def d_test(name, srcs, deps=None, includes=None, linkopts=None,
                   versions=None):
            def callback(root, workspace, **kwargs):
                d_sources = self.arrange_label_build_list(name, srcs=srcs)

                if os.path.exists(workspace) is False:
                    os.mkdir(workspace, 0o777)

                if os.path.exists('%s/objects' % workspace) is False:
                    os.mkdir('%s/objects' % workspace, 0o777)

                output = '%s/objects/%s' % (workspace, name)
                if os.path.exists(output) is False:
                    os.mkdir(output, 0o777)

                if len(d_sources) > 0:
                    # @NOTE: prepare a build instruction to help to build our 
                    # d source
                    self.make(name, deps=deps, type='test', root=root, 
                              output=output, workspace=workspace,
                              compiler=self.d_compile,
                              flags=self.d_flags, sources=d_sources or [])

                else:
                    raise AssertionError('Can\'t find any sources to perform '
                        'building this project')

            def teardown(root, output, **kwargs):
                pass

            node = { 'callback': callback, 'teardown': teardown }
            self.add_to_dependencies(name, node, deps)

        @self.rule
        def d_docs(name, dep):
            pass

    def arrange_label_build_list(self, name, root, srcs=None, label_build_list={}):
        if (not srcs is None) or (not hdrs is None):
            label_build_list = []

            # @NOTE: define list source code of this node
        if not srcs is None:
            if isinstance(srcs, list):
                for src in srcs:
                    if isinstance(src, str):
                        ext = src.split('.')[-1]

                        if ext in ['d']:
                            label_build_list.append(src)
                        else:
                            raise AssertionError('can\'t recognize extension .%s' \
                                                 % src.split('.')[-1])
                    elif isinstance(src, dict):
                        # @NOTE: somtime, src is designed to use with function
                        # there are several functions and we can invoke them
                        # whenever we want from 'BUILD' or from 'WORKSPACE'
                        # everything will be stored inside manager
                        ret = self._manager.eval_function(simple_path=True, path=root,
                                                          node=name, **src)
                        kwargs = {
                            'label_build_list': label_build_list
                        }

                        if not ret is None and isinstance(ret, list):
                            if len(ret) > 0:
                                kwargs['srcs'] = ret
                            else:
                                raise AssertionError('found empty with pattern \'%s\'' % src['variables'])
                        else:
                            raise AssertionError('can\'t eval function %s' % src['function'])

                        label_build_list = self.arrange_label_build_list(name, root=root, **kwargs)
            elif isinstance(srcs, dict):
                # @NOTE: somtime, src is designed to use with function there
                # are several functions and we can invoke them whenever we
                # want from 'BUILD' or from 'WORKSPACE' everything will be
                # stored inside manager
                ret = self._manager.eval_function(simple_path=True, path=root,
                                                  node=name, **srcs)
                kwargs = {
                    'label_build_list': label_build_list
                }

                if not ret is None and isinstance(ret, list):
                    if len(ret) > 0:
                        kwargs['srcs'] = ret
                    else:
                        raise AssertionError('found empty with pattern \'%s\'' % srcs['variables'])
                else:
                    raise AssertionError('can\'t eval function %s' % srcs['function'])

                label_build_list = self.arrange_label_build_list(name, root=root, **kwargs)
        return label_build_list
