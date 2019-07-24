TEMPLATE = subdirs

SUBDIRS = json-c \
          src

# build the project sequentially as listed in SUBDIRS !
CONFIG += ordered
