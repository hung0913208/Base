
class Protocol(object):
    def __init__(self, socket):
        self._socket = socket

    @property
    def socket(self):
        return self._socket

    @socket.setter
    def socket(self, value):
        self._socket = value

    def Event(self, name, callback):
        raise AssertionError('this is virtual class Protocol')

    def Version(self):
        raise AssertionError('this is virtual class Protocol')

    def Parse(self):
        raise AssertionError('this is virtual class Protocol')

    def Serialize(self, data):
        raise AssertionError('this is virtual class Protocol')
