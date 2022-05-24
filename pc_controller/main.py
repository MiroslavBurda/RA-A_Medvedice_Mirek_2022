#!/usr/bin/env python2
import socket
import time
import json
import sys
import argparse
import webbrowser
import threading
import io

from websocket_server import WebsocketServer

BROADCAST_PORT = 42424
WEB_PORT = 9000
DISCOVER_SLEEP = 0.2

def discover(owner):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    sock.settimeout(DISCOVER_SLEEP)
    sock.bind(('', BROADCAST_PORT))

    devices = {}

    print("Looking for devices....\n")

    for i in range(int(5.0/DISCOVER_SLEEP)):
        while True:
            try:
                sock.sendto('{"c":"discover"}'.encode("utf-8"), ("255.255.255.255", BROADCAST_PORT))
                break
            except io.BlockingIOError:
                continue
            except socket.error as e:
                if str(e).find("11") == -1:
                    raise e

        time.sleep(DISCOVER_SLEEP)

        while True:
            try:
                msg, addr = sock.recvfrom(65535)
                if addr in devices:
                    continue
                msg = json.loads(msg)
                if msg["c"] != "found":
                    continue
                if msg["owner"] != owner and owner != "null":
                    continue
                #print("%d: %s (%s) -- %s" % (len(devices), msg["name"], msg["owner"], addr[0]))
                devices[addr] = msg
                if owner != "null":
                    return (addr[0], "http://%s:%d%s" % (addr[0], msg.get("port", 80), msg.get("path", "/index.html")))
            except io.BlockingIOError:
                break
            except socket.timeout:
                break
            except socket.error as e:
                if str(e).find("11") == -1:
                    raise e
                break
    sock.close()

    if len(devices) == 0:
        print("No devices found!")
        sys.exit(1)

    while True:
        devidx = input("Pick one device number [0-%d]: " % (len(devices)-1))
        try:
            devidx = int(devidx)
            if devidx >= 0 and devidx < len(devices):
                break
        except ValueError:
            pass

    addr = list(devices.keys())[devidx]
    dev = devices[addr]
    return (addr[0], "http://%s:%d%s" % (addr[0], dev.get("port", 80), dev.get("path", "/index.html")))

class RBSocket:
    def __init__(self, dest_ip, server):
        self.dest_ip = dest_ip
        self.server = server
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        if sys.platform.startswith("linux"):
            self.sock.setsockopt(socket.SOL_SOCKET, 12, 6)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 1)
        self.sock.bind(('', 0))
        self.write_counter = 0
        self.read_counter = 0
        self.last_possess = 0

    def send(self, msg):
        if msg["c"] == "possess" and time.time() - self.last_possess > 2:
            self.last_possess = time.time()
            self.write_counter = 0
            self.read_counter = 0
        msg["n"] = self.write_counter
        self.write_counter += 1
        msg = json.dumps(msg)
        print(msg)
        try:
            self.sock.sendto(msg.encode("utf-8"), (self.dest_ip, BROADCAST_PORT))
        except Exception as e:
            print(e)

    def process(self):
        while True:
            try:
                msg, addr = self.sock.recvfrom(65535)
            except Exception as e:
                print(e)
                time.sleep(0.1)
                continue

            try:
                msg = msg.decode("utf-8").replace("\n", "\\n")
                msg = json.loads(msg)
                #print(msg)
                if "n" in msg:
                    diff = self.read_counter - msg["n"]
                    if msg["n"] != -1 and diff > 0 and diff < 25:
                        continue
                    self.read_counter = msg["n"]
                self.server.send_message_to_all(json.dumps(msg))
            except Exception as e:
                print(e, msg)

    def wsOnMessage(self, client, server, msg):
        self.send(json.loads(msg))

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Mickoflus proxy')
    parser.add_argument("owner", type=str, help="The name of the owner")
    parser.add_argument("--ip", type=str, default="",
        help="An IP to connect to instead of searching for devices.")
    parser.add_argument("--port", type=int, default=80, help="The port to use.")
    parser.add_argument("--path", type=str, default="/index.html", help="Path to the control page.")
    parser.add_argument("-pp", type=int, default=9000, help="port of the websocket proxy")
    #parser.add_argument("-ph", type=str, default="0.0.0.0", help="hostname for the websocket proxy")
    args = parser.parse_args()

    if args.ip:
        addr = "http://%s:%d%s" % (args.ip, args.port, args.path)
        device_ip = args.ip
    else:
        device_ip, addr = discover(args.owner)

    print("Using %s as device address" % device_ip)
    webbrowser.open(addr)

    server = WebsocketServer(args.pp)
    rbsocket = RBSocket(device_ip, server)
    server.set_fn_message_received(rbsocket.wsOnMessage)

    th = threading.Thread(target=rbsocket.process)
    th.daemon = True
    th.start()

    server.run_forever()
