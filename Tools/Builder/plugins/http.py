from . import Plugin


class Http(Plugin):
    def __init__(self, **kwargs):
        super(Http, self).__init__()

    def define(self):
        @self.rule
        def http_archive(name, sha256=None, strip_prefix=None, type=None,
                         url=None, urls=None):
            pass

        @self.rule
        def http_file(name, executable=False, sha256=None, url=None, urls=None):
            pass

        @self.rule
        def new_http_archive(name, build_file=None, build_file_content=None,
                             sha256=None, strip_prefix=None, type=None, url=None, urls=None,
                             workspace_file=None, workspace_file_content=None):
            pass
