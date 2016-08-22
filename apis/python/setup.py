""" Setup for Koheron TCP server API

    From https://github.com/pypa/sampleproject/blob/master/setup.py

    (c) Koheron
"""

# Always prefer setuptools over distutils
from setuptools import setup, find_packages
# To use a consistent encoding
from codecs import open
from os import path

# http://stackoverflow.com/questions/19919905/how-to-bootstrap-numpy-installation-in-setup-py
from setuptools import setup
from setuptools.command.build_ext import build_ext as _build_ext

#class build_ext(_build_ext):
#    def finalize_options(self):
#        _build_ext.finalize_options(self)
#        # Prevent numpy from thinking it is still in its setup process:
#        __builtins__.__NUMPY_SETUP__ = False
#        import numpy
#        self.include_dirs.append(numpy.get_include())

here = path.abspath(path.dirname(__file__))

# Get the long description from the README file
with open(path.join(here, 'README.rst'), encoding='utf-8') as f:
    long_description = f.read()

setup(
    name='koheron_tcp_client',

    # Versions should comply with PEP440.  For a discussion on single-sourcing
    # the version across setup.py and the project code, see
    # https://packaging.python.org/en/latest/single_source_version.html
    version='0.9.0',

    description='Client for Koheron TCP server',
    long_description=long_description,

    # The project's main homepage.
    url='https://github.com/Koheron/tcp-server/tree/master/apis/python',

    # Author details
    author='Koheron',
    author_email='hello@koheron.com',

    # Choose your license
    license='MIT',

    # See https://pypi.python.org/pypi?%3Aaction=list_classifiers
    classifiers=[
        # How mature is this project? Common values are
        #   3 - Alpha
        #   4 - Beta
        #   5 - Production/Stable
        'Development Status :: 4 - Beta',

        # Indicate who your project is intended for
        'Intended Audience :: Education',
        'Intended Audience :: Science/Research',
        'Intended Audience :: Developers',
        'Topic :: System :: Hardware',
        'Topic :: Scientific/Engineering :: Physics',

        # Pick your license as you wish (should match "license" above)
        'License :: OSI Approved :: MIT License',

        # Specify the Python versions you support here. In particular, ensure
        # that you indicate whether you support Python 2, Python 3 or both.
        'Programming Language :: Python :: 2.7',
        'Programming Language :: Python :: 3'
    ],

    # What does your project relate to?
    keywords='TCP server FPGA Instrumentation',

    # You can just specify the packages manually here if your project is
    # simple. Or you can use find_packages().
    packages=find_packages(exclude=['docs', 'tests']),

    # List run-time dependencies here.  These will be installed by pip when
    # your project is installed. For an analysis of "install_requires" vs pip's
    # requirements files see:
    # https://packaging.python.org/en/latest/requirements.html
    install_requires=[],
    
#    cmdclass={'build_ext':build_ext},
#    setup_requires=['numpy'],

    # To provide executable scripts, use entry points in preference to the
    # "scripts" keyword. Entry points provide cross-platform support and allow
    # pip to create the appropriate form of executable for the target platform.
    entry_points={
        'console_scripts': [],
    },
)
