from . import HTTP
from . import HYBI13

class WebSocket(object):
    """
        This class would be used to define a new websocket session.

    >>> from websocket import WebSocket
    >>> ws = WebSocket('wss://echo.websocket.org')
    >>> ws.Send('hello server')
    >>> ws.Recv()
    'hello server'
    >>> ws.close()

    uri: the uri which is used for this connection
    """

    def __init__(self, uri, protocol=HYBI13(None)):
        super(WebSocket, self).__init__()

        self._protocol = protocol
        self._headers = {}
        self._events = {}
        self._status = False
        self._socket = None
        self._host = None
        self._port = None
        self._ssl = False

        if not uri is None:
            self.Connect(uri)

    def __enter__(self):
        return self

    def __exit__(self, type, value, traceback):
        if self._status is True:
            self._socket.close()

    def __ping__(self):
        if 'ping' in self._events:
            self._events['ping']()

    def __closed__(self):
        self._status = False

        if 'closed' in self._events:
            self._events['closed']()

    def __connected__(self):
        if 'connected' in self._events:
            self._events['connected']()

    def __greeting__(self, headers={}):
        import sys

        if sys.version_info[0] < 3:
            from base64 import encodestring as encode
        else:
            from base64 import encodebytes as encode
        from os import urandom

        request = [
            'GET {} HTTP/1.1'.format(self._path),
            'Host: {}'.format(self._host),
            'Upgrade: websocket',
            'Connection: Upgrade'
        ]

        for name in headers:
            request.append('{}: {}'.format(name, headers[name]))
        else:
            if not 'Sec-WebSocket-Key' in headers:
                request.append('Sec-WebSocket-Key: {}'
                        .format(encode(urandom(16)).decode('utf-8').strip()))
            if not 'Sec-WebSocket-Version' in headers:
                request.append('Sec-WebSocket-Version: {}'
                        .format(self._protocol.Version()))
            if not 'Sec-WebSocket-Extensions' in headers:
                request.append('Sec-WebSocket-Extensions: permessage-deflate; client_max_window_bits')

            if not 'Origin' in headers:
                if self._ssl:
                    request.append('Origin: https://{}'.format(self._host))
                else:
                    request.append('Origin: http://{}'.format(self._host))

        with HTTP(self._socket) as fd:
            if fd.Serialize(request) is False:
                return False
            else:
                self._status = fd.Parse().status != 'Switching Protocol'
                self._protocol.socket = self._socket
        return self._status

    @property
    def status(self):
        return self._status

    @property
    def ssl(self):
        return self._ssl

    def Connect(self, uri=None, headers={}, timeout=10):
        from socket import socket, AF_INET, SOCK_STREAM
        from ssl import wrap_socket

        if uri is None:
            if self._host is None or self._port is None or self._path is None:
                raise AssertionError('can\'t reconnect a blank socket')
            elif len(headers) == 0:
                headers = self._headers

            self._socket = socket(AF_INET, SOCK_STREAM)
        elif uri.startswith('ws://') or uri.startswith('wss://'):
            remote = uri.split('://')[1].split('/')[0]

            self._socket = socket(AF_INET, SOCK_STREAM)
            self._host = remote.split(':')[0]
            self._port = int(remote.split(':')[1]) if ':' in remote else 0
            self._path = uri.split(remote)[1]

            if uri.startswith('wss://'):
                self._ssl = True

                if self._port == 0:
                    self._port = 443
            elif self._port == 0:
                self._port = 80

            if not self._path.startswith('/'):
                self._path = '/' + self._path
        else:
            raise AssertionError('can\'t recognize the protocol')

        self._socket.settimeout(timeout)

        if self._ssl is True:
            self._socket = wrap_socket(self._socket)
        self._socket.connect((self._host, self._port))

        if self.__greeting__(headers) is False:
            raise IOError('fail to greeting the server')
        else:
            self.__connected__()

            if not uri is None:
                self._headers = headers
                self._protocol.Event('closed', self.__closed__)
                self._protocol.Event('ping', self.__ping__)

    def Send(self, data):
        if self._status is False:
            self.Connect()
        self._protocol.Serialize(data)

    def Recv(self):
        if self._status is False:
            self.Connect()
        return self._protocol.Parse()

    def Expect(self, sending, receiving):
        if sending is None:
            return str(self.Recv) == receiving
        elif self.Send(sending) is False:
            return False
        else:
            return str(self.Recv) == receiving

    def Hook(self, event):
        def Redirect(callback):
            def Wrapping(*args, **kwargs):
                return callback(*args, **kwargs)

            self._events[event] = callback
            return Wrapping
        return Redirect
