/*
 * pyhstore, a Python extension module that allows bidirectional transformation
 * between PostgreSQL hstore and Python dictionary objects.
 */
#include <Python.h>

#include <postgres.h>
#include <hstore.h>

/*
 * These will be found by the linker only if hstore.so has already been loaded
 * in this session.
 */
Datum hstore_in(PG_FUNCTION_ARGS);
Datum hstore_out(PG_FUNCTION_ARGS);

static PyObject *
parse_hstore(PyObject *self, PyObject *args)
{
    char          *hstore_text;
    HStore        *hstore;
    char          *base;
    HEntry        *entries;
    int            count;
    int            i;
    PyObject      *ret;
    MemoryContext  oldcontext = CurrentMemoryContext;

    if (!PyArg_ParseTuple(args, "z", &hstore_text))
        return NULL;

    if (hstore_text == NULL) {
        Py_INCREF(Py_None);
        return Py_None;
    }

    PG_TRY();
    {
        /* transform the text representation to a hstore */
        hstore = DatumGetHStoreP(DirectFunctionCall1(hstore_in, CStringGetDatum(hstore_text)));
    }
    PG_CATCH();
    {
        ErrorData  *edata;

        /* get the error message, make Python raise the exception */
        MemoryContextSwitchTo(oldcontext);
        edata = CopyErrorData();
        FlushErrorState();

        PyErr_SetString(PyExc_ValueError, edata->message);
        return NULL;
    }
    PG_END_TRY();

    base = STRPTR(hstore);
    entries = ARRPTR(hstore);

    ret = PyDict_New();

    count = HS_COUNT(hstore);

    for (i = 0; i < count; i++)
        {
            PyObject *key, *val;

            key = PyString_FromStringAndSize(HS_KEY(entries, base, i),
                                             HS_KEYLEN(entries, i));
            if (HS_VALISNULL(entries, i)) {
                Py_INCREF(Py_None);
                val = Py_None;
            }
            else {
                val = PyString_FromStringAndSize(HS_VAL(entries, base, i),
                                                 HS_VALLEN(entries, i));
            }

            PyDict_SetItem(ret, key, val);
        }

    return ret;
}

static PyObject *
serialize_hstore(PyObject *self, PyObject *args)
{
    PyObject      *dict;
    HStore        *hstore;
    int            pcount;
    char          *hstore_text;
    MemoryContext  oldcontext = CurrentMemoryContext;

    if (!PyArg_ParseTuple(args, "O", &dict))
        return NULL;

    if (dict == Py_None) {
        Py_INCREF(Py_None);
        return Py_None;
    }

    if (!PyDict_Check(dict)) {
        PyErr_Format(PyExc_TypeError, "argument 1 must be dict, not %.200s",
                     dict->ob_type->tp_name);
        return NULL;
    }

    pcount = PyDict_Size(dict);

    PG_TRY();
    {
        Pairs      *pairs;
        PyObject   *key;
        PyObject   *value;
        Py_ssize_t  pos;
        char       *keys;
        char       *vals;
        Py_ssize_t  keylen;
        Py_ssize_t  vallen;
        int         buflen;
        int         i;

        pairs = palloc(pcount * sizeof(Pairs));
        pos = i = 0;
        /* loop over the dictionary, creating a Pair for each key/value pair */
        while (PyDict_Next(dict, &pos, &key, &value)) {
            if (!PyString_Check(key))
                elog(ERROR, "hstore keys have to be strings");

            PyString_AsStringAndSize(key, &keys, &keylen);

            if (strlen(keys) != keylen)
                elog(ERROR, "hstore keys cannot contain NUL bytes");

            pairs[i].key = pstrdup(keys);
            pairs[i].keylen = hstoreCheckKeyLen(keylen);
            pairs[i].needfree = true;

            if (value == Py_None) {
                pairs[i].val = NULL;
                pairs[i].vallen = 0;
                pairs[i].isnull = true;
            }
            else {
                if (!PyString_Check(value))
                    elog(ERROR, "hstore values have to be strings");

                PyString_AsStringAndSize(value, &vals, &vallen);

                if (strlen(vals) != vallen)
                    elog(ERROR, "hstore values cannot contain NUL bytes");

                pairs[i].val = pstrdup(vals);
                pairs[i].vallen = hstoreCheckValLen(vallen);
                pairs[i].isnull = false;
            }

            i++;
        }
        pcount = hstoreUniquePairs(pairs, pcount, &buflen);
        hstore = hstorePairs(pairs, pcount, buflen);

        hstore_text = DatumGetCString(DirectFunctionCall1(hstore_out, PointerGetDatum(hstore)));
    }
    PG_CATCH();
    {
        ErrorData  *edata;

        /* get the error message, make Python raise the exception */
        MemoryContextSwitchTo(oldcontext);
        edata = CopyErrorData();
        FlushErrorState();

        PyErr_SetString(PyExc_ValueError, edata->message);
        return NULL;
    }
    PG_END_TRY();

    return PyString_FromString(hstore_text);
}

static PyMethodDef PyHstore_methods[] = {
    {"parse_hstore", parse_hstore, METH_VARARGS,
     "Parse a text representation of a hstore into a dictionary."},
    {"serialize_hstore", serialize_hstore, METH_VARARGS,
     "Serialize a dictionary into text representation of a hstore."},
    {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC
init_pyhstore(void)
{
    (void) Py_InitModule("_pyhstore", PyHstore_methods);
}
