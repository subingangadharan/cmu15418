#!/usr/bin/python

import hashlib, os

MIN_PORT = 1024
MAX_PORT = 65535 # We leave out the last port. Guess why.

user_hash = hashlib.sha1(os.environ['USER']).hexdigest()
port = int(user_hash, 16) % (MAX_PORT - MIN_PORT) + MIN_PORT
print port
