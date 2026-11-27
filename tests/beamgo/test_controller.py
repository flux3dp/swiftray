"""Exercise the real Qt serial controller against a pseudo-terminal, never USB hardware."""
import binascii
import json
import os
from pathlib import Path
import select
import socket
import struct
import subprocess
import sys
import tempfile
import threading
import time
import tty

TX_KEY = '1b6e251826afd8462b17458866c0483e'
RX_KEY = '2b7e152628aed2a8abf7158808cf4f3c'
IV = '01' * 16


def crypt(data, key, decrypt=False):
    command = ['openssl', 'enc', '-aes-128-cbc', '-K', key, '-iv', IV]
    if decrypt:
        command.append('-d')
    return subprocess.run(command, input=data, capture_output=True, check=True).stdout


class Simulator(threading.Thread):
    def __init__(self):
        super().__init__(daemon=True)
        self.master, self.slave = os.openpty()
        tty.setraw(self.slave)
        self.port = os.ttyname(self.slave)
        self.buffer = bytearray()
        self.commands = []
        self.state = self.processed = self.uploaded = 0
        self.paused = self.end = False
        self.fail_upload = False
        self.stop_at = None
        self.running = True
        self.error = None
        self.lock = threading.Lock()
        self.registers = {
            0x0b: b'SIM00001;V2.0.78;HY-2328;;HY-LASER-SIM00001\x00',
            3: bytes.fromhex('005a000000'),
            2: bytes.fromhex('21342134044cfbb40000000000000000271027100000000011223344'),
            0x16: bytes(4), 0x17: bytes.fromhex('000000ff'), 0x1d: bytes.fromhex('e0000000'),
        }

    def send(self, plain):
        cipher = crypt(plain, RX_KEY)
        frame = b'\xb1' + struct.pack('>H', len(cipher) + 5) + cipher
        frame += struct.pack('>H', binascii.crc_hqx(frame, 0xffff))
        with self.lock:
            os.write(self.master, frame)

    def event(self, parameter, value):
        self.send(bytes([0x49, parameter]) + struct.pack('>I', value))

    def finish(self, done):
        self.state = 0
        self.event(4, 0)
        self.send(bytes.fromhex('49450a') + bytes([done]) + struct.pack('>II', 12345, 1) + bytes(3))

    def handle(self, command):
        self.commands.append(command)
        family, parameter = command[:2]
        if family == 0x11:
            if parameter == 4:
                body = struct.pack('>I', self.state)
            elif parameter == 6:
                body = struct.pack('>I', self.processed)
            else:
                body = self.registers.get(parameter)
            if body is not None:
                self.send(command[:2] + body)
        elif family == 0x10:
            if parameter in (1, 7):
                if self.fail_upload and parameter == 1:
                    return
                sequence = int.from_bytes(command[2:4], 'big')
                if sequence == 0xffff:
                    self.uploaded = self.processed = 0
                    self.end = False
                elif command[4:] == b'\xff' * 5:
                    self.end = True
                else:
                    self.uploaded += (len(command) - 4) // 5
            elif parameter != 0x21:
                self.registers[parameter] = command[2:]
            self.send(b'\x47\x01')
        elif family == 0x12:
            self.send(b'\x47\x01')
            if parameter in (0, 4):
                self.state = 1 if parameter == 0 else 2
                self.paused = False
                self.processed = 0
                self.event(4, self.state)
            elif parameter == 5:
                if self.state == 2:
                    self.state = 0
                    self.event(4, 0)
            elif parameter == 1:
                if self.state == 1:
                    self.paused = True
                    self.stop_at = time.monotonic() + 0.08
                else:
                    self.finish(0)
            elif parameter == 6:
                self.paused = True
            elif parameter == 7:
                self.paused = False

    def run(self):
        last_tick = time.monotonic()
        try:
            while self.running:
                ready, _, _ = select.select([self.master], [], [], 0.01)
                if ready:
                    self.buffer.extend(os.read(self.master, 65536))
                while len(self.buffer) >= 3:
                    assert self.buffer[0] == 0xa1, self.buffer[:20].hex()
                    length = int.from_bytes(self.buffer[1:3], 'big')
                    if len(self.buffer) < length:
                        break
                    frame = bytes(self.buffer[:length])
                    del self.buffer[:length]
                    assert frame[-4:-2] == b'\x66\x88'
                    assert int.from_bytes(frame[-2:], 'big') == binascii.crc_hqx(frame[:-2], 0xffff)
                    self.handle(crypt(frame[3:-4], TX_KEY, True))
                if self.stop_at is not None and time.monotonic() >= self.stop_at:
                    self.stop_at = None
                    self.finish(0)
                if self.state == 1 and not self.paused and time.monotonic() - last_tick >= 0.05:
                    last_tick = time.monotonic()
                    self.processed = min(self.uploaded, self.processed + 200)
                    if self.end and self.processed >= self.uploaded:
                        self.finish(1)
        except Exception as error:
            if self.running:
                self.error = error


def main():
    simulator = Simulator()
    simulator.start()
    with tempfile.TemporaryDirectory(prefix='beamgo-controller-') as temp:
        stderr = open(Path(temp) / 'stderr.log', 'w+')
        process = subprocess.Popen([sys.argv[1], simulator.port, temp], stdout=subprocess.PIPE, stderr=stderr, text=True)
        try:
            ready, _, _ = select.select([process.stdout], [], [], 15)
            if not ready:
                stderr.seek(0)
                raise AssertionError(f'Controller discovery timed out: commands={simulator.commands!r}, simulator={simulator.error!r}, stderr={stderr.read()}')
            port = int(process.stdout.readline())
            client = socket.create_connection(('127.0.0.1', port), timeout=10)
            reader = client.makefile('rb')
            request_id = 0

            def call(action, **params):
                nonlocal request_id
                request_id += 1
                client.sendall((json.dumps({'id': request_id, 'action': action, 'params': params}) + '\n').encode())
                while True:
                    message = json.loads(reader.readline())
                    assert message['id'] == request_id, message
                    if message['type'] == 'callback':
                        assert simulator.error is None, simulator.error
                        return message['result']

            def ok(action, **params):
                result = call(action, **params)
                assert result['success'], (action, result)
                return result

            def call_batch(requests):
                nonlocal request_id
                identifiers = []
                wire = []
                for action, params in requests:
                    request_id += 1
                    identifiers.append(request_id)
                    wire.append(json.dumps({'id': request_id, 'action': action, 'params': params}))
                client.sendall(('\n'.join(wire) + '\n').encode())
                results = {}
                while len(results) < len(identifiers):
                    message = json.loads(reader.readline())
                    if message['type'] == 'callback':
                        results[message['id']] = message['result']
                return [results[identifier] for identifier in identifiers]

            info = ok('getDeviceInfo')
            assert info['model'] == 'fbg1' and info['name'] == 'BeamGo-SIM00001' and info['fieldMm'] == 90
            assert not any(command[0] in (0x10, 0x12) for command in simulator.commands), 'Initialization wrote to hardware'
            for power in (0, 101, 1.5, '15', None):
                result = call('startFraming', points=[[35, 35], [55, 35]], width=90, framingPower=power)
                assert result == {'success': False, 'error': 'INVALID_PARAMS'}, result
            for power in (5, 15, 30):
                begin = len(simulator.commands)
                ok('startFraming', points=[[35, 35], [55, 35], [55, 55], [35, 35]], width=90, framingPower=power)
                blocks = [command[4:] for command in simulator.commands[begin:] if command[:2] == b'\x10\x07']
                records = b''.join(blocks)
                values = [int.from_bytes(records[i+1:i+5], 'big') for i in range(0, len(records), 5) if records[i] == 0x12]
                assert values == [power * 10], values
                assert not call('startFraming', points=[[0, 0], [1, 1]], width=90)['success']
                ok('stopFraming')
                ok('quit')

            before = ok('getCalibration')
            result = ok('setCalibration', scaleX=0.8501)
            after = ok('getCalibration')
            assert after['rawHex'][4:] == before['rawHex'][4:] and after['length'] == 28
            assert result['backupHex'] == before['rawHex']
            ok('restoreCalibration', rawHex=before['rawHex'], lens={key: before[key] for key in ('fieldMm', 'swapXY', 'reverseX', 'reverseY')})
            assert ok('getCalibration')['rawHex'] == before['rawHex']
            assert ok('setSafetySettings', doorCheck=False)['mask'] == 253
            assert ok('setSafetySettings', doorCheck=True)['mask'] == 255

            square = 'M100\nM3S150\nF6000\nG1X35Y35\nG1X55Y35\nG1X55Y55\nG1X35Y35\nM2'
            ok('upload', data=square)
            assert not call('upload', data='G1X100')['success']
            begin = len(simulator.commands)
            assert call('start')['error'] == 'EMPTY_JOB'
            assert not any(command[0] in (0x10, 0x12) for command in simulator.commands[begin:])
            ok('upload', data=square)
            ok('start')
            for _ in range(40):
                result = ok('getStatus')
                if result['st_id'] == 64:
                    break
                time.sleep(0.05)
            assert result['st_id'] == 64 and result['prog'] == 1, result
            ok('quit')
            code = 'M100\nM3S150\nF6000\nG1X35Y35\n' + 'G1X55Y35\nG1X35Y35\n' * 3500 + 'M2'
            ok('upload', data=code)
            ok('start')
            ok('pause')
            assert ok('getStatus')['st_id'] == 48
            assert simulator.state == 1 and simulator.paused
            ok('resume')
            assert ok('getStatus')['st_id'] == 16
            stopped, restarted = call_batch([('stop', {}), ('start', {})])
            assert stopped['success'] and restarted == {'success': False, 'error': 'JOB_RUNNING'}
            assert simulator.state == 0, 'Stop completed before hardware idle'
            assert ok('getStatus')['st_id'] == 128
            ok('quit')

            simulator.registers[0x16] = struct.pack('>I', 2)
            result = call('start')
            assert not result['success'] and result['error'] == 'HARDWARE_ERROR,DOOR_OPENED', result
            simulator.registers[0x16] = bytes(4)
            ok('start')
            simulator.registers[0x16] = struct.pack('>I', 2)
            simulator.event(0x16, 2)
            time.sleep(0.5)
            assert ok('getStatus')['st_id'] == 128
            assert simulator.state == 0
            simulator.registers[0x16] = bytes(4)
            ok('quit')

            simulator.state = 2
            simulator.event(0x16, 0)
            simulator.event(4, 2)
            time.sleep(0.1)
            assert ok('getStatus')['st_id'] == 16
            assert call('upload', data=square)['error'] == 'JOB_RUNNING'
            ok('stop')
            ok('quit')

            import base64
            ok('uploadProjectFile', dataBase64=base64.b64encode(b'FLUX-GO 0x21 TEST').decode())
            file_packets = [command for command in simulator.commands if command[:2] == b'\x10\x21']
            import hashlib
            for packet in file_packets:
                assert packet[-16:] == hashlib.md5(packet[2:-16]).digest()
            assert file_packets[-1][2:6] == bytes.fromhex('ffff0000')

            simulator.fail_upload = True
            begin = len(simulator.commands)
            result = call('start')
            assert not result['success'], result
            commands = [command for command in simulator.commands[begin:] if command[:2] == b'\x10\x01']
            assert len(commands) == 1, 'Ambiguous upload was retried'
            print('PASS: real Qt serial discovery, guards, framing, calibration, safety, lifecycle, streaming and no-retry timeout')
        finally:
            process.terminate()
            process.wait(timeout=5)
            simulator.running = False
            simulator.join(timeout=2)
            os.close(simulator.master)
            os.close(simulator.slave)
            stderr.seek(0)
            if simulator.error:
                print(stderr.read(), file=sys.stderr)
            stderr.close()


if __name__ == '__main__':
    main()
