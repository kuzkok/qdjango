#!/usr/bin/python

import os

root = os.path.join(os.path.dirname(__file__), '..')

# set library path
path = []
for component in ['db', 'http', 'script']:
    path.append(os.path.join(root, 'src', component))
os.environ['LD_LIBRARY_PATH'] = ':'.join(path)

# run tests
TESTS = [
    'db/qdjangometamodel/tst_qdjangometamodel',
    'db/qdjangoqueryset/tst_qdjangoqueryset',
    'db/qdjangowhere/tst_qdjangowhere',
    'db/auth/tst_auth',
    'db/shares/tst_shares',
    'http/qdjangohttpserver/tst_qdjangohttpserver',
    'http/qdjangourlresolver/tst_qdjangourlresolver',
    'script/qdjangoscript/tst_qdjangoscript',
]
for test in TESTS:
    prog = os.path.join(os.path.dirname(__file__), test)
    os.system(prog)