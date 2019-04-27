import os

from . import Language
from .configs import *
from .templates import *

class C(Language):
    def __init__(self, **kwargs):
        super(C, self).__init__()
        self._system = System(**kwargs)
        self._debug = kwargs.get('debug') or 0
        self._imports = {}
        self._compiles = {}

        # @NOTE: configure System with our expective compiling tools
        self._system.config_with(os='Linux', apply={
            'C_COMPILERS': ['c', 'gcc', 'clang'],
            'CC_COMPILERS': ['c++', 'g++', 'clang++'],
            'C_PACKAGES': ['gcc', 'clang'],
            'CC_PACKAGES': ['gcc-c++', 'clang', 'g++'],
            'SHARED_EXT': '.so',
            'AR_BINARY': 'ar',
            'AR_FLAGS': ['rcT']
        })

        self._system.config_with(os='Darwin', apply={
            'C_COMPILERS': ['c', 'gcc', 'clang'],
            'CC_COMPILERS': ['c++', 'g++', 'clang++'],
            'SHARED_EXT': '.dylib',
            'RANLIB_BINARY': 'ranlib',
            'AR_BINARY': 'libtool',
            'AR_FLAGS': ['-static', '-o']
        })

        self._system.config_with(os='FreeBSD', apply={
            'C_COMPILERS': ['c', 'gcc', 'clang'],
            'CC_COMPILERS': ['c++', 'g++', 'clang++'],
            'C_PACKAGES': ['gcc', 'clang'],
            'CC_PACKAGES': ['gcc-c++', 'clang', 'g++'],
            'SHARED_EXT': '.so',
            'AR_BINARY': 'ar',
            'AR_FLAGS': ['rcT']
        })

        # @NOTE: use default compilers if it's defined
        self.c_compile = kwargs.get('c_compile')
        self.cc_compile = kwargs.get('cc_compile')

        # @NOTE: set default flags if it's defined or use ENV's flags
        self.c_flags = kwargs.get('c_flags')
        self.cc_flags = kwargs.get('cc_flags')

        # @NOTE: use default definition of c_flags if they are defined
        if self.c_flags is None:
            if os.environ.get('CPPFLAGS'):
                self.c_flags = os.environ.get('CPPFLAGS')

            if os.environ.get('CFLAGS'):
                if self.cc_flags is None:
                    self.cc_flags = os.environ.get('CFLAGS')
                else:
                    self.cc_flags += os.environ.get('CFLAGS')

        # @NOTE: use default definition of cc_flags if they are defined
        if self.cc_flags is None:
            if os.environ.get('CPPFLAGS'):
                self.cc_flags = os.environ.get('CPPFLAGS')

            if os.environ.get('CXXFLAGS'):
                if self.cc_flags is None:
                    self.cc_flags = os.environ.get('CXXFLAGS')
                else:
                    self.cc_flags += os.environ.get('CXXFLAGS')

        self.ranlib = None
        self.archive = None
        self.ar_flags = None

    def check(self):
        if (not self.c_compile is None) and (not self.cc_compile is None):
            return True

        def verify_c_compile(compile):
            return Language.test_compile("%s %s %s -o %s", compile, ".c",
                                         self.c_flags,
                                         """
                                         #include <stdio.h>

                                         int main(int argc, char const *argv[])
                                         {
                                             printf("hello world\\n");
                                             return 0;
                                         }
                                         """, "test")

        def verify_cc_compile(compile):
            return Language.test_compile("%s %s %s -o %s", compile, ".cc",
                                         self.cc_flags,
                                         """
                                         #include <iostream>
                                         using namespace std;

                                         int main(int argc, char const *argv[])
                                         {
                                             cout << "hello world" << endl;
                                             return 0;
                                         }
                                         """, "test")

        AR_BINARY = self._system.select('AR_BINARY')
        RANLIB_BINARY = self._system.select('RANLIB_BINARY')
        C_COMPILERS = self._system.select('C_COMPILERS')
        CC_COMPILERS = self._system.select('CC_COMPILERS')

        if self.archive is None:
            self.archive = Language.which(AR_BINARY)
            self.ar_flags = self._system.select('AR_FLAGS')

        if self.ranlib is None and not RANLIB_BINARY is None:
            self.ranlib = Language.which(RANLIB_BINARY)

        if self.c_compile is None:
            # @NOTE: install compiler automatically if none of them are existed
            # inside the environment
            if Language.which(C_COMPILERS, count=True) == 0:
                if self._manager.package_update() is False:
                    raise AssertionError('can\'t update package')
                elif self._manager.package_install(self._system.select('C_PACKAGES')) is False:
                    raise AssertionError('can\'t install one of these compiles %s' % str(C_COMPILERS))

            # @NOTE: select which compiler would be best to compile this project
            self.c_compile = Language.which(C_COMPILERS, verify_c_compile)
        if verify_c_compile(self.c_compile) is False:
            raise AssertionError('please provide a real c_compile')

        if self.cc_compile is None:
            # @NOTE: install compiler automatically if none of them are existed
            # inside the environment
            if self._manager.package_update() is False:
                raise AssertionError('can\'t update package')
            elif Language.which(CC_COMPILERS, count=True) == 0:
                if self._manager.package_install(self._system.select('CC_PACKAGES')) is False:
                    raise AssertionError('can\'t install one of these compiles %s' % str(CC_COMPILERS))

            # @NOTE: select which compiler would be best to compile this project
            self.cc_compile = Language.which(CC_COMPILERS, verify_cc_compile)
        if verify_cc_compile(self.cc_compile) is False:
            raise AssertionError('please provide a real cc_compile')

        if self.c_compile is None or self.cc_compile is None:
            return False

        # @TODO: detech which System can be used with these configuration, since
        # we only forcus on self.archive as a main object to generate binary code
        return True

    def make(self, name, root, workspace, type, deps=None, c=None, cc=None,
             copts=None, defines=None, linkopts=[]):
        from itertools import chain

        # @NOTE: use debug flag if we use builder with flags.debug is True
        if not self._debug is None and self._debug != 0:
            if copts is None:
                copts = ['-g']
            else:
                copts += ['-g']

        # @NOTE: use compiler as a linker instead of checking a specific since
        # they support this function
        linker = self.c_compile if cc is None else self.cc_compile
        externals = ''
        libraries = {}
        headers = {
            'c': c['headers'] or [],
            'c++': cc['headers'] or []
        }

        if len(linkopts) > 0:
            for link in linkopts:
                if len(externals) == 0:
                    externals += link
                else:
                    externals += ' ' + link

        if not deps is None:
            for dep in deps:
                dep = self._manager.convert_dep_to_absolute_name(dep)

                if dep in self._imports:
                    has_found = 0

                    # @NOTE: append outside libraries
                    headers['c'].extend(self._imports[dep]['c'])
                    headers['c++'].extend(self._imports[dep]['c++'])

                    if 'static' in self._imports[dep] and 'shared' in self._imports[dep]:
                        for lib in chain(self._imports[dep]['static'], self._imports[dep]['shared']):
                            path = self._manager.backends['file'].scan(lib)

                            if not path is None:
                                libraries.append(path)
                                has_found += 1
                        else:
                            if has_found == 0:
                                raise AssertionError('not found \'%s\'' % dep)
                else:
                    outputs = self._manager.read_value_from_node(dep, 'output')
                    included = self._manager.read_value_from_node(dep, 'headers')

                    # @NOTE: link with another libraries
                    if (not included is None) and (not outputs is None):
                        if 'c' in included or 'c++' in included:
                            for item in outputs:
                                _, file = item
                                library = file.split('.')[0]

                                if not library in libraries:
                                    libraries[library] = '/'.join(item)
                            if 'c' in included:
                                headers['c'].extend(included['c'])
                            if 'c++' in included:
                                headers['c++'].extend(included['c++'])

        if not defines is None:
            defines = '-D'.join(defines)

        cmds = []

        # @NOTE: by default we have 2 different kind of output: library and binary
        # they require different ways to compile and build so we must separate
        # them to 2 different branchs
        if type == 'library':
            if (not self.ar_flags is None) and len(self.ar_flags) > 0:
                archive = {
                    'executor': 'ar',
                    'event': 'archiving',
                    'output': (workspace, name + '.a'),
                    'input': []
                }

                # @NOTE: since macos use different archive tool we must manually
                # handle it here
                if self._manager.backends['config'].os in ['Window']:
                    raise AssertionError('unsupport os %s' % \
                        self._manager.backends['config'].os)
                else:
                    archive['pattern'] = '%s %s %s' % (self.archive, \
                        ' '.join(self.ar_flags), '%s/%s %s')
            else:
                archive = {
                    'executor': 'ar',
                    'event': 'archiving',
                    'output': (workspace, name + '.a'),
                    'input': []
                }

                # @NOTE: since macos use different archive tool we must manually
                # handle it here
                if self._manager.backends['config'].os in ['Linux', 'Darwin']:
                    archive['pattern'] = self.archive + ' %s/%s %s'
                else:
                    raise AssertionError('unsupport os %s' % \
                        self._manager.backends['config'].os)

            libname = 'lib' + name + (self._system.select('SHARED_EXT') or '.so')
            shared = {
                'executor': 'link',
                'event': 'linking',
                'pattern': linker + ' %s -shared -Wl,-soname,'+ libname + ' -o %s/%s',
                'output': (workspace, libname),
                'input': []
            }

            if not self.ranlib is None:
                ranlib = {
                    'executor': 'ranlib',
                    'event': 'archiving',
                    'pattern': self.ranlib + ' %s',
                    'input': '%s/%s.a' % (workspace, name),
                    'output': None
                }
        else:
            link = {
                'executor': 'link',
                'event': 'linking',
                'pattern': linker + ' %s -o %s/%s',
                'output': (workspace, name),
                'input': []
            }

        # @NOTE: generate command which will be used to create object files
        if not cc is None:
            # @NOTE: use custom flags from outside
            if cc['flags'] is None:
                cc['flags'] = copts
            elif not copts is None:
                cc['flags'] += copts

            converted_headers = ''
            if len(headers['c++']) > 0:
                set_of_added = set()

                for header in headers['c++']:
                    if header[0] != '/':
                        if header.split('.')[-1] in ['h', 'hh', 'hpp']:
                            dir_of_headers = '%s/%s' % (cc['root'], '/'.join(header.split('/')[:-1]))
                        else:
                            dir_of_headers = header

                        if not dir_of_headers in set_of_added:
                            converted_headers += (' -I%s' % dir_of_headers)
                            set_of_added.add(dir_of_headers)
                    else:
                        if header.split('.')[-1] in ['h', 'hh', 'hpp']:
                            converted_headers += ' -I%s' % '/'.join(header.split('/')[:-1])
                        else:
                            converted_headers += ' -I%s' % header
            else:
                converted_headers = ' -I%s -I%s' % (workspace, cc['root'])

            for source in cc['sources']:
                compile = {}

                # @NOTE: create command to build object files
                compile['input'] = '%s/%s' % (cc['root'], source)
                compile['output'] = (cc['output'], source + '.o')
                compile['executor'] = 'cc'
                compile['event'] = 'compiling'

                if defines is None:
                    if type == 'library':
                        pattern = '-fPIC -c %s -o %s/%s'
                    else:
                        pattern = '-c %s -o %s/%s'

                    if cc['flags'] is None:
                        compile['pattern'] = '%s %s' % (cc['compiler'], pattern)
                    else:
                        compile['pattern'] = '%s %s %s' % (cc['compiler'], ' '.join(cc['flags']), pattern)
                else:
                    if type == 'library':
                        pattern = '-fPIC -c %s -o %s/%s'
                    else:
                        pattern = '-c %s -o %s/%s'

                    if cc['flags'] is None:
                        compile['pattern'] = '%s %s %s' % (cc['compiler'], defines, '-o %s/%s -c %s')
                    else:
                        compile['pattern'] = '%s %s %s %s' % (cc['compiler'], cc['flags'], defines, '-o %s/%s -c %s')

                if len(converted_headers) > 0:
                    compile['pattern'] += converted_headers

                if type == 'library':
                    archive['input'].append('%s/%s.o' % (cc['output'], source))
                    shared['input'].append('%s/%s.o' % (cc['output'], source))
                else:
                    link['input'].append('%s/%s.o' % (cc['output'], source))
                cmds.append(compile)
        if not c is None:
            # @NOTE: use custom flags from outside
            if c['flags'] is None:
                c['flags'] = copts
            elif not copts is None:
                c['flags'] += copts

            converted_headers = ''
            if len(headers['c++']) > 0:
                set_of_added = set()

                for header in headers['c']:
                    if header[0] != '/':
                        if header.split('.')[-1] == 'h':
                            dir_of_headers = '%s/%s' % (c['root'], '/'.join(header.split('/')[:-1]))
                        else:
                            dir_of_headers = header

                        if not dir_of_headers in set_of_added:
                            converted_headers += (' -I%s' % dir_of_headers)
                            set_of_added.add(dir_of_headers)
                    else:
                        if header.split('.')[-1] in ['h', 'hh', 'hpp']:
                            converted_headers += ' -I%s' % '/'.join(header.split('/')[:-1])
                        else:
                            converted_headers += ' -I%s' % header
            else:
                converted_headers = ' -I%s -I%s' % (workspace, c['root'])

            for source in c['sources']:
                compile = {}

                # @NOTE: create command to build object files
                compile['input'] = '%s/%s' % (c['root'], source)
                compile['output'] = (c['output'], source + '.o')
                compile['executor'] = 'c'
                compile['event'] = 'compiling'

                if defines is None:
                    if c['flags'] is None:
                        if type == 'library':
                            compile['pattern'] = '%s -fPIC %s' % (c['compiler'], '-c %s -o %s/%s')
                        else:
                            compile['pattern'] = '%s %s' % (c['compiler'], '-c %s -o %s/%s')
                    else:
                        if type == 'library':
                            compile['pattern'] = '%s -fPIC %s %s' % (c['compiler'], c['flags'], '-c %s -o %s/%s')
                        else:
                            compile['pattern'] = '%s %s %s' % (c['compiler'], c['flags'], '-c %s -o %s/%s')
                else:
                    if c['flags'] is None:
                        if type == 'library':
                            compile['pattern'] = '%s  -fPIC %s %s' % (c['compiler'], defines, '-c %s -o %s/%s')
                        else:
                            compile['pattern'] = '%s %s %s' % (c['compiler'], defines, '-c %s -o %s/%s')
                    else:
                        if type == 'library':
                            compile['pattern'] = '%s -fPIC %s %s %s' % (c['compiler'], c['flags'], defines, '-c %s -o %s/%s')
                        else:
                            compile['pattern'] = '%s %s %s %s' % (c['compiler'], c['flags'], defines, '-c %s -o %s/%s')

                if len(converted_headers) > 0:
                    compile['pattern'] += converted_headers

                if type == 'library':
                    archive['input'].append('%s/%s.o' % (c['output'], source))
                    shared['input'].append('%s/%s.o' % (c['output'], source))
                else:
                    link['input'].append('%s/%s.o' % (c['output'], source))
                cmds.append(compile)

        # @NOTE: everything has been prepared now apply compiling command to _performer
        self._performer.apply(cmds)

        # @NOTE: everything is done now, add link task and run test task at here
        if type == 'library':
            cc_headers = []
            c_headers = []

            for header in c['headers']:
                c_headers.append('%s/%s' % (c['root'], header))
            for header in cc['headers']:
                cc_headers.append('%s/%s' % (cc['root'], header))

            abs_name = self._manager.convert_name_to_absolute_name(name, root)

            # @NOTE: mark this library to be used on publish
            self._manager.modify_node_inside_dependency_tree(abs_name, 'output',
                [archive['output'], shared['output']])
            self._manager.modify_node_inside_dependency_tree(abs_name, 'headers',
                {
                    'c': c_headers,
                    'c++': cc_headers
                })

            # @NOTE: add link command after finishing apply every object files.,
            if not libraries is None and len(libraries) > 0:
                so_packed = ''
                a_packed = ''

                for library in list(libraries.values()):
                    if library.endswith('.a'):
                        a_packed += ' ' + library
                    else:
                        so_packed += ' -Wl,-rpath=%s,%s' % (os.path.dirname(library), library)

                if len(a_packed) > 0:
                    shared['pattern'] += a_packed
                if len(so_packed) > 0:
                    shared['pattern'] += so_packed

            # @NOTE: by default we must create both shared library and dynamic
            # linkable library
            self._performer.apply([archive, shared])
            if not self.ranlib is None:
                self._performer.apply([ranlib])
        else:
            # @NOTE: add link command after finishing apply every object files.,
            if not libraries is None and len(libraries) > 0:
                so_packed = ''
                a_packed = ''

                for library in list(libraries.values()):
                    if library.endswith('.a'):
                        a_packed += ' ' + library
                    else:
                        so_packed += ' -Wl,-rpath=%s,%s' % (os.path.dirname(library), library)

                link['pattern'] = linker + ' %s -o %s/%s'
                if len(a_packed) > 0:
                    link['pattern'] += a_packed
                if len(so_packed) > 0:
                    link['pattern'] += so_packed

            if len(externals) > 0:
                link['pattern'] += ' ' + externals

            self._performer.apply([link])

            # @NOTE: if this is a testing task, just perform it after linking
            if type == 'test':
                test = {
                    'pattern': '%s',
                    'event': 'invoking',
                    'input': '%s/%s' % (workspace, name),
                    'output': None
                }
                self._performer.apply([test])

    def define(self):
        @self.rule
        def cc_binary(name, deps=None, srcs=None, hdrs=None, copts=None,
                      defines=None, linkshared=False, linkstatic=False,
                      linkopts=None):
            def callback(root, workspace, **kwargs):
                build_list = self.arrange_label_build_list(name, root=root,
                                                           srcs=srcs, hdrs=hdrs,
                                                           label_build_list={})
                if os.path.exists(workspace) is False:
                    os.mkdir(workspace, 0o777)

                if os.path.exists('%s/objects' % workspace) is False:
                    os.mkdir('%s/objects' % workspace, 0o777)

                output = '%s/objects/%s' % (workspace, name)
                if os.path.exists(output) is False:
                    os.mkdir(output, 0o777)

                if len(build_list) > 0:
                    # @NOTE: we split our build_list to source and header
                    c_headers = build_list['hdrs'].get('c')
                    c_sources = build_list['srcs'].get('c')
                    cc_headers = build_list['hdrs'].get('c++')
                    cc_sources = build_list['srcs'].get('c++')

                    # @NOTE: prepare a build instruction to help to build our c/c++ source
                    self.make(name, root=root, workspace=workspace, deps=deps, type='binary',
                              cc={
                                  'root': root,
                                  'output': output,
                                  'compiler': self.cc_compile,
                                  'flags': self.cc_flags,
                                  'headers': (cc_headers or []) + (c_headers or []),
                                  'sources': cc_sources or []
                              }, c={
                                  'root': root,
                                  'output': output,
                                  'compiler': self.c_compile,
                                  'flags': self.c_flags,
                                  'headers': c_headers or [],
                                  'sources': c_sources or []
                              }, copts=copts, defines=defines, linkopts=linkopts)
                else:
                    raise AssertionError('Can\'t find any sources to perform '
                        'building this project')

            def teardown(root, output, **kwargs):
                pass

            node = { 'callback': callback, 'teardown': teardown }
            self.add_to_dependencies(name, node, deps)

        @self.rule
        def cc_import(name, hdrs=None, alwayslink=False, interface_library=None,
                      shared_library=None, static_library=None):
            def callback(root, **kwargs):
                headers = self.parse_list_hdrs(name, hdrs=hdrs)

                if len(build_list) > 0:
                    # @NOTE: we split our build_list to source and header
                    c_headers = headers.get('c')
                    cc_headers = headers.get('c++')

                if self._manager.backends['config'].os == 'Window':
                    self._imports[self._manager.convert_name_to_absolute_name(name, root)] = {
                        'c': c_headers,
                        'c++': cc_headers,
                        'shared': shared_library,
                        'static': interface_library
                    }
                else:
                    self._imports[name] = {
                        'c': c_headers,
                        'c++': cc_headers,
                        'shared': shared_library,
                        'static': static_library
                    }

            node = { 'callback': callback }
            self.add_to_dependencies(name, node, deps)

        @self.rule
        def cc_library(name, deps=None, srcs=None, hdrs=None, copts=None, defines=None):
            def callback(root, workspace, **kwargs):
                build_list = self.arrange_label_build_list(name, root=root, srcs=srcs,
                                                           hdrs=hdrs, label_build_list={})
                if os.path.exists(workspace) is False:
                    os.mkdir(workspace, 0o777)

                if os.path.exists('%s/objects' % workspace) is False:
                    os.mkdir('%s/objects' % workspace, 0o777)

                output = '%s/objects/%s' % (workspace, name)
                if os.path.exists(output) is False:
                    os.mkdir(output, 0o777)

                if len(build_list) > 0:
                    # @NOTE: we split our build_list to source and header
                    c_headers = build_list['hdrs'].get('c') or []
                    c_sources = build_list['srcs'].get('c')
                    cc_headers = build_list['hdrs'].get('c++') or []
                    cc_sources = build_list['srcs'].get('c++')

                    # @NOTE: prepare a build instruction to help to build our c/c++ source
                    self.make(name, root=root, workspace=workspace, deps=deps, type='library',
                              cc={
                                    'root': root,
                                    'output': output,
                                    'compiler': self.cc_compile,
                                    'flags': self.cc_flags,
                                    'headers': (cc_headers or []) + (c_headers or []),
                                    'sources': cc_sources or []
                              }, c={
                                    'root': root,
                                    'output': output,
                                    'compiler': self.c_compile,
                                    'flags': self.c_flags,
                                    'headers': c_headers or [],
                                    'sources': c_sources or []
                              }, copts=copts, defines=defines)
                else:
                    raise AssertionError('Can\'t find any sources to perform '
                        'building this project')

            def teardown(root, output, **kwargs):
                pass

            node = { 'callback': callback, 'teardown': teardown }
            self.add_to_dependencies(name, node, deps)

        @self.rule
        def cc_test(name, deps=None, srcs=None, copts=None, defines=None,
                    linkshared=False, linkopts=None):
            def callback(root, workspace, **kwargs):
                build_list = self.arrange_label_build_list(name, root=root,
                                                           srcs=srcs, label_build_list={})
                if os.path.exists(workspace) is False:
                    os.mkdir(workspace, 0o777)

                if os.path.exists('%s/objects' % workspace) is False:
                    os.mkdir('%s/objects' % workspace, 0o777)

                output = '%s/objects/%s' % (workspace, name)
                if os.path.exists(output) is False:
                    os.mkdir(output, 0o777)

                if len(build_list) > 0:
                    # @NOTE: we split our build_list to source and header
                    c_headers = build_list['hdrs'].get('c')
                    c_sources = build_list['srcs'].get('c')
                    cc_headers = build_list['hdrs'].get('c++')
                    cc_sources = build_list['srcs'].get('c++')

                    # @NOTE: prepare a build instruction to help to build our c/c++ source
                    self.make(name, root=root, workspace=workspace, deps=deps, type='test',
                              cc={
                                  'root': root,
                                  'output': output,
                                  'compiler': self.cc_compile,
                                  'flags': ' '.join(self.cc_flags) if not self.cc_flags is None else None,
                                  'headers': (cc_headers or []) + (c_headers or []),
                                  'sources': cc_sources or []
                              }, c={
                                  'root': root,
                                  'output': output,
                                  'compiler': self.c_compile,
                                  'flags': ' '.join(self.c_flags) if not self.c_flags is None else None,
                                  'headers': c_headers or [],
                                  'sources': c_sources or []
                              }, copts=copts, defines=defines, linkopts=linkopts)
                else:
                    raise AssertionError('Can\'t find any sources to perform '
                        'building this project')

            def teardown(root, output, **kwargs):
                pass

            node = { 'callback': callback, 'teardown': teardown }
            self.add_to_dependencies(name, node, deps)

    def parse_list_hdrs(self, name, hdrs):
        build_list = self.arrange_label_build_list(name, hdrs=hdrs)

        if 'hdrs' in build_list and len(build_list['hdrs']) > 0:
            return build_list['hdrs']
        else:
            raise AssertionError('Not found any headers in list %s' % str(hdrs))

    def arrange_label_build_list(self, name, root, srcs=None, hdrs=None, label_build_list={}):
        if not 'srcs' in label_build_list:
            label_build_list['srcs'] = {}
        if not 'hdrs' in label_build_list:
            label_build_list['hdrs'] = {}

            # @NOTE: define list source code of this node
        if not srcs is None:
            if isinstance(srcs, list):
                for src in srcs:
                    if isinstance(src, str):
                        ext = src.split('.')[-1]

                        if ext in ['c', 'C']:
                            if not 'c' in label_build_list['srcs']:
                                label_build_list['srcs']['c'] = []

                            label_build_list['srcs']['c'].append(src)
                        elif ext in ['cc', 'cpp', 'cxx', 'c++']:
                            if not 'c++' in label_build_list['srcs']:
                                label_build_list['srcs']['c++'] = []

                            label_build_list['srcs']['c++'].append(src)
                        elif ext == 'h':
                            if not 'c' in label_build_list['hdrs']:
                                label_build_list['hdrs']['c'] = []

                            label_build_list['hdrs']['c'].append(src)
                        elif ext in ['hh', 'hxx', 'hxx']:
                            if not 'c++' in label_build_list['hdrs']:
                                label_build_list['hdrs']['c++'] = []

                            label_build_list['hdrs']['c++'].append(src)
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

                        label_build_list = self.arrange_label_build_list(name, **kwargs)
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

        # @NOTE: define list header of this label_build_list
        if not hdrs is None:
            if isinstance(hdrs, list):
                for hdr in hdrs:
                    if isinstance(hdr, str):
                        ext = hdr.split('.')[-1]

                        if ext == 'h':
                            if not 'c' in label_build_list['hdrs']:
                                label_build_list['hdrs']['c'] = []

                            label_build_list['hdrs']['c'].append(hdr)
                        elif ext in ['hh', 'hxx', 'hxx']:
                            if not 'c++' in label_build_list['hdrs']:
                                label_build_list['hdrs']['c++'] = []

                            label_build_list['hdrs']['c++'].append(hdr)
                        else:
                            raise AssertionError('can\'t recognize extension .%s' \
                                         % hdr.split('.')[-1])
                    elif isinstance(hdr, dict):
                        # @NOTE: somtime, src is designed to use with function
                        # there are several functions and we can invoke them
                        # whenever we want from 'BUILD' or from 'WORKSPACE'
                        # everything will be stored inside manager
                        ret = self._manager.eval_function(simple_path=True, path=root,
                                                          node=name, **hdr)
                        kwargs = {
                            'label_build_list': label_build_list
                        }

                        if not ret is None and isinstance(ret, list):
                            if len(ret) > 0:
                                kwargs['hdrs'] = ret
                            else:
                                raise AssertionError('found empty with pattern \'%s\'' % hdr['variables'])
                        else:
                            raise AssertionError('can\'t eval function %s' % hdr['function'])

                        label_build_list = self.arrange_label_build_list(name, **kwargs)
            elif isinstance(hdrs, dict):
                # @NOTE: somtime, src is designed to use with function there
                # are several functions and we can invoke them whenever we
                # want from 'BUILD' or from 'WORKSPACE' everything will be
                # stored inside manager
                ret = self._manager.eval_function(simple_path=True, path=root,
                                                  node=name, **hdrs)
                kwargs = {
                    'label_build_list': label_build_list
                }

                if not ret is None and isinstance(ret, list):
                    if len(ret) > 0:
                        kwargs['hdrs'] = ret
                    else:
                        raise AssertionError('found empty with pattern \'%s\'' % hdrs['variables'])
                else:
                    raise AssertionError('can\'t eval function %s' % hdrs['function'])

                label_build_list = self.arrange_label_build_list(name, root=root, **kwargs)
        return label_build_list
