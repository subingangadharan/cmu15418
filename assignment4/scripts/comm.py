#!/usr/bin/env python2.7

from abc import ABCMeta
import struct
import socket
import collections

class SocketClosed(Exception):
  pass

def listen_to(port):
  s = None
  for res in socket.getaddrinfo(None, port, socket.AF_UNSPEC, 
				socket.SOCK_STREAM, 0, socket.AI_PASSIVE):
      af, socktype, proto, canonname, sa = res
      try:
	  s = socket.socket(af, socktype, proto)
      except socket.error as msg:
	  s = None
	  continue
      try:
	  s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
	  s.bind(sa)
	  s.listen(1)
      except socket.error as msg:
	  s.close()
	  s = None
	  continue
      break
  if s is None:
      raise Exception("Invalid port", port)
  return s

def recv_all(sock, msglen):
    msg = ''
    while len(msg) < msglen:
       chunk = sock.recv(msglen-len(msg))
       if chunk == '':
           raise SocketClosed()
       msg = msg + chunk
    return msg

def send_string(sock, string):
  sock.sendall(struct.pack("i", len(string)))
  sock.sendall(string)

def recv_string(sock):
  length_string = recv_all(sock, struct.calcsize("i"))
  length = struct.unpack("i", length_string)[0]
  return recv_all(sock, int(length))

class CStruct(object):
  """An abstract class for network serialized C structs.

Expects Derived.struct to be a struct.Struct specifying the instance and
instance.tup to be a named tuple of the instance. See TaggedMessage below
for an example."""

  def to_bytes(self):
    return self.struct.pack(*self.tup)  

  def to_socket(self, socket):
    return socket.sendall(self.to_bytes())

  @classmethod
  def from_bytes(cls, bytes):
    tup = cls.struct.unpack(bytes)
    ret = cls(*tup)
    return ret

  @classmethod
  def from_socket(cls, socket):
    return cls.from_bytes(recv_all(socket, cls.struct.size))

  def __getattr__(self, name):
    return getattr(self.tup, name)

  def __repr__(self):
    return self.tup.__repr__()

# This needs to be kept in sync with the message_t enum in src/types.h
WORK=0
RESPONSE=1
NEW_WORKER=2
REQUEST_STATS=3
STATS=4
ISREADY=5
SHUTDOWN=6
WORKER_UP_TIME_STATS=7

messages = (WORK, RESPONSE, NEW_WORKER, REQUEST_STATS, STATS, ISREADY, SHUTDOWN, WORKER_UP_TIME_STATS)

class TaggedMessage(CStruct):
  struct = struct.Struct("ii")
  Tup = collections.namedtuple("TaggedMessage", "message tag")

  def __init__(self, message, tag):
    if message not in messages:
        raise Exception("Invalid message type", message)
    self.tup = TaggedMessage.Tup(message, tag)

class WorkerRequest(CStruct):
  struct = struct.Struct("iiii")
  Tup = collections.namedtuple("WorkerRequest", "cpu_threads memory_threads io_threads tag")

  def __init__(self, cpu_threads, memory_threads, io_threads, tag):
          self.tup = WorkerRequest.Tup(cpu_threads, memory_threads, io_threads, tag)

class Work(CStruct):
  HIGH_CPU = 0
  HIGH_MEMORY = 1
  HIGH_IO = 2

  struct = struct.Struct("ii")
  Tup = collections.namedtuple("Work", "type instance")

  def __init__(self, t, i):
          self.tup = Work.Tup(t, i)

class Resp(CStruct):
  struct = struct.Struct("i")
  Tup = collections.namedtuple("Resp", "x")

  def __init__(self, x):
          self.tup = Resp.Tup(x)
