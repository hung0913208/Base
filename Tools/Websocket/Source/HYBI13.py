from .Protocol import Protocol

class HYBI13(Protocol):
    class Request(object):
        def __init__(self, payload, opcode=0x1, flags=0x8, mask=[]):
            self._payload = payload
            self._opcode = opcode
            self._flags = flags
            self._mask = mask
       
        @property
        def raw(self):
            masking = 0 if len(self._mask) == 0 else 1
            array = []

            # @NOTE: make the first byte 
            array.append(self._flags << 4 | self._opcode) 
                
            # @NOTE: make the second byte 
            if len(self._payload) < 126:
                array.append(masking << 7 | len(self._payload))
                bytesize = 0
            elif len(self._payload) < pow(2, 16):
                array.append(masking << 7 | 126)
                bytesize = 2
            elif len(self._payload) < pow(2, 64):
                array.append(masking << 7 | 127)
                bytesize = 8

            # @NOTE: turn the extend payload size to bytes
            for i in range(bytesize, 0, -1):
                base = (len(self._payload) & 0xff << 8*(i - 1))

                if i > 1:
                    array.append(base >> 8*(i - 2))
                else:
                    array.append(base)

            # @NOTE: copy mask-key to array
            for byte in self._mask:
                array.append(byte)
            
            # @NOTE: copy raw data to array
            for byte in self._payload:
                array.append(ord(byte))
            return bytearray(array)

        @property
        def payload(self):
            return self._payload


    class Response(object):
        def __init__(self, socket):
            self._socket = socket
            self._consumed = 0

            # @NOTE: read the first byte
            byte = ord(self._socket.recv(1)[0])
            self._flags = byte >> 4
            self._opcode = byte & 0xf

            # @NOTE: read the second byte
            byte = ord(self._socket.recv(1)[0])
            if byte < 126:
                self._size = byte
                bytesize = 0
            elif byte == 126:
                self._size = 0
                bytesize = 2
            else:
                self._size = byte
                bytesize = 8

            # @NOTE: turn bytes to the extend payload size
            for i in range(bytesize, 0, -1):
                self._size += ord(self._socket.recv(1)[0]) << 8*(i - 1)
            else:
                if (byte >> 7) == 1:
                    self._mask = [ord(i) for i in self._socket.recv(4)]
                else:
                    self._mask = []

        def __repr__(self): 
            if self.size > 0:
                return next(self.Iter(self.size))
            else:
                return ''

        def __str__(self): 
            try:
                if self.size > 0:
                    return next(self.Iter(self.size))
                else:
                    return ''
            except StopIteration:
                return ''

        @property
        def flags(self):
            return self._flags

        @property
        def opcode(self):
            return self._opcode

        @property
        def mask(self):
            return self._mask

        @property
        def size(self):
            return self._size

        def Iter(self, chunk):
            from socket import error

            try:
                for i in range(0, self._size/chunk + (self._size%chunk != 0)):
                    result = ''

                    while len(result) < chunk:
                        result += self._socket.recv(chunk - len(result))
                    else:
                        yield result
            except error as e:
                raise StopIteration


    def __init__(self, socket):
        super(HYBI13, self).__init__(socket)
        self._events = {}

    def Version(self):
        return '13'

    def Event(self, name, callback):
        self._events[name] = callback

    def Parse(self):
        from socket import error

        try:
            resp = HYBI13.Response(self._socket)
        except error:
            return None

        # @NOTE: send Pong to prevent closing events
        if resp.opcode == 0x9:
            self.Serialize(HYBI13.Request('', 0xA))

            if 'ping' in self._events:
                self._events['ping']()
        elif resp.opcode == 0x8:
            self._socket.close()

            if 'closed' in self._events:
                self._events['closed']()
            else:
                raise IOError('connection has been closed')

        return resp

    def Serialize(self, data):
        if isinstance(data, str):
            # @NOTE: prepare header
            req = HYBI13.Request(data)
        elif isinstance(data, unicode):
            # @NOTE: prepare header
            req = HYBI13.Request(data)
        else:
            req = data

        # @NOTE: send header
        self._socket.sendall(req.raw)
