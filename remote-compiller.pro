TEMPLATE = subdirs
CONFIG += ordered

CONFIG += c++17

SUBDIRS = \
    protocol \
    client \
    server

client.depends = protocol
server.depends = protocol
