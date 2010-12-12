# make sure we are run in a PL/Python block
try:
    import plpy
except ImportError:
    raise RuntimeError("%s can only be used in a PL/Python block" % __name__)
# execute a dummy select to make sure hstore is loaded in the database
plpy.execute("select '1=>1'::hstore")
# import the module functions
from _pyhstore import parse_hstore, serialize_hstore
