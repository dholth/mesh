# file "example_build.py"

# Note: we instantiate the same 'cffi.FFI' class as in the previous
# example, but call the result 'ffibuilder' now instead of 'ffi';
# this is to avoid confusion with the other 'ffi' object you get below

from cffi import FFI
ffibuilder = FFI()

ffibuilder.set_source(
    "_trickle",
    r"""
    #include "rand.c"
    #include "trickle.c"
    """,
    # libraries=['trickle'],
    # library_dirs=['.']
    )

ffibuilder.cdef(open('trickle_cffi.h', 'rb').read(), packed=True)

if __name__ == "__main__":
    ffibuilder.compile(verbose=True)
