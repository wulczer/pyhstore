# make sure we are run in a PL/Python block
try:
    import plpy
except ImportError:
    raise RuntimeError("%s can only be used in a PL/Python block" % __name__)

try:
    # import the module functions
    from _pyhstore import parse_hstore, serialize_hstore
except ImportError, e:
    # this might mean hstore is not yet loaded in this session, so try to
    # force loading it
    try:
        plpy.execute("select '1=>1'::hstore")
    except:
        # did not work for whatever reason, complain with the original exception
        raise e
    # try loading the functions again
    from _pyhstore import parse_hstore, serialize_hstore
