env = Environment(
    CPPDEFINES = Split('THREADSAFE'),
    CCFLAGS = Split('-g -ansi -pedantic -Wall'),
    LINKFLAGS = Split('-pthread') )

libsources = [
    'tripledb.c', 'urlencoding.c', 'hash.c', 'hashtable.c' ]

lib = env.Library('libtripledb', libsources)

env.Program( 'test', [ 'tests.c', lib ] )

