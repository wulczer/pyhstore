from distutils.core import setup, Extension

setup(name = "pyhstore",
      description=("Python extension module "
                   "for bidirectional hstore to dictionary "
                   "conversion in PL/Python"),
      version = "1.0",
      license="MIT",
      author="Jan Urbanski",
      maintainer="Jan Urbanski",
      author_email="wulczer@wulczer.org",
      maintainer_email="wulczer@wulczer.org",
      py_modules=["pyhstore"],
      ext_modules=[Extension("_pyhstore", ["pyhstore.c"])])
