from .Protocol import Protocol

class HTTP(Protocol):
    class Request(object):
        def __init__(self, method, path, headers={}, body=None):
            super(HTTP.Request, self).__init__()
            self._headers = headers
            self._method = method
            self._path = path
            self._body = body

        @property
        def method(self):
            return self._method

        @property
        def header(self):
            return self._headers

        @property
        def body(self):
            return self._body


    class Response(object):
        def __init__(self, socket):
            super(HTTP.Response, self).__init__()
            self._properties = {}
            self._release = False
            self._socket = socket
            self._status = None
            self._code = None

        @property
        def status(self):
            return self._status
        
        @property
        def code(self):
            return self._code

        @property
        def properties(self):
            return self._properties

        def Iter(self, chunk_size):
            if self._release is True:
                return None


    def __init__(self, socket):
        super(HTTP, self).__init__(socket)
        self._responses = []

    def __enter__(self):
        return self

    def __exit__(self, type, value, traceback):
        for resp in self._responses:
            resp._release = True

    def Version(self):
        return "1.1"
    
    def Parse(self):
        from ssl import SSLError
 
        try:
            split = '\r\n\r\n'
            keep = True
            raw = ''
            idx = 0
            
            # @NOTE: collect the header as the raw data
            while keep:
                data = self._socket.recv(1)

                if data is None:
                    break
                elif split is None:
                    raw += data
                elif split[idx] == data:
                    idx += 1
    
                    if idx < len(split):
                        continue
                    else:
                        keep = False
                else:
                    if idx > 0:
                        raw += split[0:idx]
                        idx = 0
                    raw += data
            else:
                # @NOTE: split the raw data into several path and we will put
                # them into the approviated places

                splited = raw.split('\r\n')
                header = splited[0].split(' ')
                result = HTTP.Response(self._socket)

                result._code = header[1]
                result._status = ' '.join(header[2:])

                for line in splited[1:]:
                    if len(line) == 0:
                        continue
                    else:
                        key, value = line.split(': ')
                        result._properties[key] = value
                else:
                    self._responses.append(result)
                    return result 
        except SSLError as error:
            self._status = False
        return None

    def Serialize(self, data):
        sent = 0

        if isinstance(data, list):
            raw = ('\r\n'.join(data) + '\r\n\r\n').encode('ascii')
        elif isinstance(data, HTTP.Request):
            raw = str(data)

        while len(raw) > sent:
            size = self._socket.send(raw[sent:])

            if size < 0:
                return False
            else:
                sent += size
        else:
            return sent == len(raw)
