"""Read one local original DAT resource using the native archive wire rules."""
from pathlib import Path
import struct

class MT19937:
    def __init__(self, seed):
        self.state = [seed & 0xffffffff]
        for i in range(1, 624):
            prior = self.state[-1]
            self.state.append((1812433253*(prior^(prior>>30))+i)&0xffffffff)
        self.index = 624

    def next(self):
        state = self.state
        if self.index == 624:
            for i in range(624):
                value = (state[i]&0x80000000)|(state[(i+1)%624]&0x7fffffff)
                state[i] = state[(i+397)%624]^(value>>1)^(0x9908b0df if value&1 else 0)
            self.index = 0
        value = state[self.index]; self.index += 1
        value ^= value>>11
        value ^= (value<<7)&0x9d2c5680
        value ^= (value<<15)&0xefc60000
        return value^(value>>18)

def read_resource(directory, requested):
    requested = requested.replace('\\', '/').lower()
    found = None
    for letter in 'abc':
        path = Path(directory)/('6kinoko_'+letter+'.dat')
        with path.open('rb') as stream:
            header = stream.read(6)
            if len(header)!=6: raise ValueError('Truncated DAT header')
            count, size = struct.unpack('<HI', header)
            if size>16*1024*1024: raise ValueError('DAT index too large')
            data = bytearray(stream.read(size))
            if len(data)!=size: raise ValueError('Truncated DAT index')
            random = MT19937(size+6); key, step = 0xc5, 0x89
            for i in range(size):
                data[i] ^= (random.next()&255)^key
                key = (key+step)&255; step = (step+0x49)&255
            cursor = 0
            for _ in range(count):
                if cursor+9>size: raise ValueError('Truncated DAT entry')
                offset, length, name_length = struct.unpack_from('<IIB', data, cursor)
                cursor += 9
                if cursor+name_length>size: raise ValueError('Truncated DAT path')
                name = bytes(data[cursor:cursor+name_length]).replace(b'\\',b'/').lower()
                cursor += name_length
                if name != requested.encode('cp932'): continue
                if length>64*1024*1024 or offset+length>path.stat().st_size:
                    raise ValueError('Invalid resource bounds')
                stream.seek(offset); payload = stream.read(length)
                if len(payload)!=length: raise ValueError('Truncated resource')
                xor = ((offset>>1)|0x23)&255
                found = payload.translate(bytes(value^xor for value in range(256)))
    if found is None: raise ValueError('Original resource not found: '+requested)
    return found
