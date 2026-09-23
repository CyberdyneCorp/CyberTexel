# C header SHA-256: 639d4fbe186a3230b417e454ae940c3720048a3677c05140288a2549e4bb2a05
"""Generated raw ctypes declarations for the CyberTexel C ABI.

Regenerate with ``just generate-python-capi``.
"""

__docformat__ = "restructuredtext"

# Begin preamble for Python

import ctypes
import sys
from ctypes import *  # noqa: F401, F403

_int_types = (ctypes.c_int16, ctypes.c_int32)
if hasattr(ctypes, "c_int64"):
    # Some builds of ctypes apparently do not have ctypes.c_int64
    # defined; it's a pretty good bet that these builds do not
    # have 64-bit pointers.
    _int_types += (ctypes.c_int64,)
for t in _int_types:
    if ctypes.sizeof(t) == ctypes.sizeof(ctypes.c_size_t):
        c_ptrdiff_t = t
del t
del _int_types



class UserString:
    def __init__(self, seq):
        if isinstance(seq, bytes):
            self.data = seq
        elif isinstance(seq, UserString):
            self.data = seq.data[:]
        else:
            self.data = str(seq).encode()

    def __bytes__(self):
        return self.data

    def __str__(self):
        return self.data.decode()

    def __repr__(self):
        return repr(self.data)

    def __int__(self):
        return int(self.data.decode())

    def __long__(self):
        return int(self.data.decode())

    def __float__(self):
        return float(self.data.decode())

    def __complex__(self):
        return complex(self.data.decode())

    def __hash__(self):
        return hash(self.data)

    def __le__(self, string):
        if isinstance(string, UserString):
            return self.data <= string.data
        else:
            return self.data <= string

    def __lt__(self, string):
        if isinstance(string, UserString):
            return self.data < string.data
        else:
            return self.data < string

    def __ge__(self, string):
        if isinstance(string, UserString):
            return self.data >= string.data
        else:
            return self.data >= string

    def __gt__(self, string):
        if isinstance(string, UserString):
            return self.data > string.data
        else:
            return self.data > string

    def __eq__(self, string):
        if isinstance(string, UserString):
            return self.data == string.data
        else:
            return self.data == string

    def __ne__(self, string):
        if isinstance(string, UserString):
            return self.data != string.data
        else:
            return self.data != string

    def __contains__(self, char):
        return char in self.data

    def __len__(self):
        return len(self.data)

    def __getitem__(self, index):
        return self.__class__(self.data[index])

    def __getslice__(self, start, end):
        start = max(start, 0)
        end = max(end, 0)
        return self.__class__(self.data[start:end])

    def __add__(self, other):
        if isinstance(other, UserString):
            return self.__class__(self.data + other.data)
        elif isinstance(other, bytes):
            return self.__class__(self.data + other)
        else:
            return self.__class__(self.data + str(other).encode())

    def __radd__(self, other):
        if isinstance(other, bytes):
            return self.__class__(other + self.data)
        else:
            return self.__class__(str(other).encode() + self.data)

    def __mul__(self, n):
        return self.__class__(self.data * n)

    __rmul__ = __mul__

    def __mod__(self, args):
        return self.__class__(self.data % args)

    # the following methods are defined in alphabetical order:
    def capitalize(self):
        return self.__class__(self.data.capitalize())

    def center(self, width, *args):
        return self.__class__(self.data.center(width, *args))

    def count(self, sub, start=0, end=sys.maxsize):
        return self.data.count(sub, start, end)

    def decode(self, encoding=None, errors=None):  # XXX improve this?
        if encoding:
            if errors:
                return self.__class__(self.data.decode(encoding, errors))
            else:
                return self.__class__(self.data.decode(encoding))
        else:
            return self.__class__(self.data.decode())

    def encode(self, encoding=None, errors=None):  # XXX improve this?
        if encoding:
            if errors:
                return self.__class__(self.data.encode(encoding, errors))
            else:
                return self.__class__(self.data.encode(encoding))
        else:
            return self.__class__(self.data.encode())

    def endswith(self, suffix, start=0, end=sys.maxsize):
        return self.data.endswith(suffix, start, end)

    def expandtabs(self, tabsize=8):
        return self.__class__(self.data.expandtabs(tabsize))

    def find(self, sub, start=0, end=sys.maxsize):
        return self.data.find(sub, start, end)

    def index(self, sub, start=0, end=sys.maxsize):
        return self.data.index(sub, start, end)

    def isalpha(self):
        return self.data.isalpha()

    def isalnum(self):
        return self.data.isalnum()

    def isdecimal(self):
        return self.data.isdecimal()

    def isdigit(self):
        return self.data.isdigit()

    def islower(self):
        return self.data.islower()

    def isnumeric(self):
        return self.data.isnumeric()

    def isspace(self):
        return self.data.isspace()

    def istitle(self):
        return self.data.istitle()

    def isupper(self):
        return self.data.isupper()

    def join(self, seq):
        return self.data.join(seq)

    def ljust(self, width, *args):
        return self.__class__(self.data.ljust(width, *args))

    def lower(self):
        return self.__class__(self.data.lower())

    def lstrip(self, chars=None):
        return self.__class__(self.data.lstrip(chars))

    def partition(self, sep):
        return self.data.partition(sep)

    def replace(self, old, new, maxsplit=-1):
        return self.__class__(self.data.replace(old, new, maxsplit))

    def rfind(self, sub, start=0, end=sys.maxsize):
        return self.data.rfind(sub, start, end)

    def rindex(self, sub, start=0, end=sys.maxsize):
        return self.data.rindex(sub, start, end)

    def rjust(self, width, *args):
        return self.__class__(self.data.rjust(width, *args))

    def rpartition(self, sep):
        return self.data.rpartition(sep)

    def rstrip(self, chars=None):
        return self.__class__(self.data.rstrip(chars))

    def split(self, sep=None, maxsplit=-1):
        return self.data.split(sep, maxsplit)

    def rsplit(self, sep=None, maxsplit=-1):
        return self.data.rsplit(sep, maxsplit)

    def splitlines(self, keepends=0):
        return self.data.splitlines(keepends)

    def startswith(self, prefix, start=0, end=sys.maxsize):
        return self.data.startswith(prefix, start, end)

    def strip(self, chars=None):
        return self.__class__(self.data.strip(chars))

    def swapcase(self):
        return self.__class__(self.data.swapcase())

    def title(self):
        return self.__class__(self.data.title())

    def translate(self, *args):
        return self.__class__(self.data.translate(*args))

    def upper(self):
        return self.__class__(self.data.upper())

    def zfill(self, width):
        return self.__class__(self.data.zfill(width))


class MutableString(UserString):
    """mutable string objects

    Python strings are immutable objects.  This has the advantage, that
    strings may be used as dictionary keys.  If this property isn't needed
    and you insist on changing string values in place instead, you may cheat
    and use MutableString.

    But the purpose of this class is an educational one: to prevent
    people from inventing their own mutable string class derived
    from UserString and than forget thereby to remove (override) the
    __hash__ method inherited from UserString.  This would lead to
    errors that would be very hard to track down.

    A faster and better solution is to rewrite your program using lists."""

    def __init__(self, string=""):
        self.data = string

    def __hash__(self):
        raise TypeError("unhashable type (it is mutable)")

    def __setitem__(self, index, sub):
        if index < 0:
            index += len(self.data)
        if index < 0 or index >= len(self.data):
            raise IndexError
        self.data = self.data[:index] + sub + self.data[index + 1 :]

    def __delitem__(self, index):
        if index < 0:
            index += len(self.data)
        if index < 0 or index >= len(self.data):
            raise IndexError
        self.data = self.data[:index] + self.data[index + 1 :]

    def __setslice__(self, start, end, sub):
        start = max(start, 0)
        end = max(end, 0)
        if isinstance(sub, UserString):
            self.data = self.data[:start] + sub.data + self.data[end:]
        elif isinstance(sub, bytes):
            self.data = self.data[:start] + sub + self.data[end:]
        else:
            self.data = self.data[:start] + str(sub).encode() + self.data[end:]

    def __delslice__(self, start, end):
        start = max(start, 0)
        end = max(end, 0)
        self.data = self.data[:start] + self.data[end:]

    def immutable(self):
        return UserString(self.data)

    def __iadd__(self, other):
        if isinstance(other, UserString):
            self.data += other.data
        elif isinstance(other, bytes):
            self.data += other
        else:
            self.data += str(other).encode()
        return self

    def __imul__(self, n):
        self.data *= n
        return self


class String(MutableString, ctypes.Union):

    _fields_ = [("raw", ctypes.POINTER(ctypes.c_char)), ("data", ctypes.c_char_p)]

    def __init__(self, obj=b""):
        if isinstance(obj, (bytes, UserString)):
            self.data = bytes(obj)
        else:
            self.raw = obj

    def __len__(self):
        return self.data and len(self.data) or 0

    def from_param(cls, obj):
        # Convert None or 0
        if obj is None or obj == 0:
            return cls(ctypes.POINTER(ctypes.c_char)())

        # Convert from String
        elif isinstance(obj, String):
            return obj

        # Convert from bytes
        elif isinstance(obj, bytes):
            return cls(obj)

        # Convert from str
        elif isinstance(obj, str):
            return cls(obj.encode())

        # Convert from c_char_p
        elif isinstance(obj, ctypes.c_char_p):
            return obj

        # Convert from POINTER(ctypes.c_char)
        elif isinstance(obj, ctypes.POINTER(ctypes.c_char)):
            return obj

        # Convert from raw pointer
        elif isinstance(obj, int):
            return cls(ctypes.cast(obj, ctypes.POINTER(ctypes.c_char)))

        # Convert from ctypes.c_char array
        elif isinstance(obj, ctypes.c_char * len(obj)):
            return obj

        # Convert from object
        else:
            return String.from_param(obj._as_parameter_)

    from_param = classmethod(from_param)


def ReturnString(obj, func=None, arguments=None):
    return String.from_param(obj)


# As of ctypes 1.0, ctypes does not support custom error-checking
# functions on callbacks, nor does it support custom datatypes on
# callbacks, so we must ensure that all callbacks return
# primitive datatypes.
#
# Non-primitive return values wrapped with UNCHECKED won't be
# typechecked, and will be converted to ctypes.c_void_p.
def UNCHECKED(type):
    if hasattr(type, "_type_") and isinstance(type._type_, str) and type._type_ != "P":
        return type
    else:
        return ctypes.c_void_p


# ctypes doesn't have direct support for variadic functions, so we have to write
# our own wrapper class
class _variadic_function(object):
    def __init__(self, func, restype, argtypes, errcheck):
        self.func = func
        self.func.restype = restype
        self.argtypes = argtypes
        if errcheck:
            self.func.errcheck = errcheck

    def _as_parameter_(self):
        # So we can pass this variadic function as a function pointer
        return self.func

    def __call__(self, *args):
        fixed_args = []
        i = 0
        for argtype in self.argtypes:
            # Typecheck what we can
            fixed_args.append(argtype.from_param(args[i]))
            i += 1
        return self.func(*fixed_args + list(args[i:]))


def ord_if_char(value):
    """
    Simple helper used for casts to simple builtin types:  if the argument is a
    string type, it will be converted to it's ordinal value.

    This function will raise an exception if the argument is string with more
    than one characters.
    """
    return ord(value) if (isinstance(value, bytes) or isinstance(value, str)) else value

# End preamble

_libs = {}
_libdirs = []

# Begin loader

"""
Load libraries - appropriately for all our supported platforms
"""
# ----------------------------------------------------------------------------
# Copyright (c) 2008 David James
# Copyright (c) 2006-2008 Alex Holkner
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
#  * Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in
#    the documentation and/or other materials provided with the
#    distribution.
#  * Neither the name of pyglet nor the names of its
#    contributors may be used to endorse or promote products
#    derived from this software without specific prior written
#    permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
# ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
# ----------------------------------------------------------------------------

import ctypes
import ctypes.util
import glob
import os.path
import platform
import re
import sys


def _environ_path(name):
    """Split an environment variable into a path-like list elements"""
    if name in os.environ:
        return os.environ[name].split(":")
    return []


class LibraryLoader:
    """
    A base class For loading of libraries ;-)
    Subclasses load libraries for specific platforms.
    """

    # library names formatted specifically for platforms
    name_formats = ["%s"]

    class Lookup:
        """Looking up calling conventions for a platform"""

        mode = ctypes.DEFAULT_MODE

        def __init__(self, path):
            super(LibraryLoader.Lookup, self).__init__()
            self.access = dict(cdecl=ctypes.CDLL(path, self.mode))

        def get(self, name, calling_convention="cdecl"):
            """Return the given name according to the selected calling convention"""
            if calling_convention not in self.access:
                raise LookupError(
                    "Unknown calling convention '{}' for function '{}'".format(
                        calling_convention, name
                    )
                )
            return getattr(self.access[calling_convention], name)

        def has(self, name, calling_convention="cdecl"):
            """Return True if this given calling convention finds the given 'name'"""
            if calling_convention not in self.access:
                return False
            return hasattr(self.access[calling_convention], name)

        def __getattr__(self, name):
            return getattr(self.access["cdecl"], name)

    def __init__(self):
        self.other_dirs = []

    def __call__(self, libname):
        """Given the name of a library, load it."""
        paths = self.getpaths(libname)

        for path in paths:
            # noinspection PyBroadException
            try:
                return self.Lookup(path)
            except Exception:  # pylint: disable=broad-except
                pass

        raise ImportError("Could not load %s." % libname)

    def getpaths(self, libname):
        """Return a list of paths where the library might be found."""
        if os.path.isabs(libname):
            yield libname
        else:
            # search through a prioritized series of locations for the library

            # we first search any specific directories identified by user
            for dir_i in self.other_dirs:
                for fmt in self.name_formats:
                    # dir_i should be absolute already
                    yield os.path.join(dir_i, fmt % libname)

            # check if this code is even stored in a physical file
            try:
                this_file = __file__
            except NameError:
                this_file = None

            # then we search the directory where the generated python interface is stored
            if this_file is not None:
                for fmt in self.name_formats:
                    yield os.path.abspath(os.path.join(os.path.dirname(__file__), fmt % libname))

            # now, use the ctypes tools to try to find the library
            for fmt in self.name_formats:
                path = ctypes.util.find_library(fmt % libname)
                if path:
                    yield path

            # then we search all paths identified as platform-specific lib paths
            for path in self.getplatformpaths(libname):
                yield path

            # Finally, we'll try the users current working directory
            for fmt in self.name_formats:
                yield os.path.abspath(os.path.join(os.path.curdir, fmt % libname))

    def getplatformpaths(self, _libname):  # pylint: disable=no-self-use
        """Return all the library paths available in this platform"""
        return []


# Darwin (Mac OS X)


class DarwinLibraryLoader(LibraryLoader):
    """Library loader for MacOS"""

    name_formats = [
        "lib%s.dylib",
        "lib%s.so",
        "lib%s.bundle",
        "%s.dylib",
        "%s.so",
        "%s.bundle",
        "%s",
    ]

    class Lookup(LibraryLoader.Lookup):
        """
        Looking up library files for this platform (Darwin aka MacOS)
        """

        # Darwin requires dlopen to be called with mode RTLD_GLOBAL instead
        # of the default RTLD_LOCAL.  Without this, you end up with
        # libraries not being loadable, resulting in "Symbol not found"
        # errors
        mode = ctypes.RTLD_GLOBAL

    def getplatformpaths(self, libname):
        if os.path.pathsep in libname:
            names = [libname]
        else:
            names = [fmt % libname for fmt in self.name_formats]

        for directory in self.getdirs(libname):
            for name in names:
                yield os.path.join(directory, name)

    @staticmethod
    def getdirs(libname):
        """Implements the dylib search as specified in Apple documentation:

        http://developer.apple.com/documentation/DeveloperTools/Conceptual/
            DynamicLibraries/Articles/DynamicLibraryUsageGuidelines.html

        Before commencing the standard search, the method first checks
        the bundle's ``Frameworks`` directory if the application is running
        within a bundle (OS X .app).
        """

        dyld_fallback_library_path = _environ_path("DYLD_FALLBACK_LIBRARY_PATH")
        if not dyld_fallback_library_path:
            dyld_fallback_library_path = [
                os.path.expanduser("~/lib"),
                "/usr/local/lib",
                "/usr/lib",
            ]

        dirs = []

        if "/" in libname:
            dirs.extend(_environ_path("DYLD_LIBRARY_PATH"))
        else:
            dirs.extend(_environ_path("LD_LIBRARY_PATH"))
            dirs.extend(_environ_path("DYLD_LIBRARY_PATH"))
            dirs.extend(_environ_path("LD_RUN_PATH"))

        if hasattr(sys, "frozen") and getattr(sys, "frozen") == "macosx_app":
            dirs.append(os.path.join(os.environ["RESOURCEPATH"], "..", "Frameworks"))

        dirs.extend(dyld_fallback_library_path)

        return dirs


# Posix


class PosixLibraryLoader(LibraryLoader):
    """Library loader for POSIX-like systems (including Linux)"""

    _ld_so_cache = None

    _include = re.compile(r"^\s*include\s+(?P<pattern>.*)")

    name_formats = ["lib%s.so", "%s.so", "%s"]

    class _Directories(dict):
        """Deal with directories"""

        def __init__(self):
            dict.__init__(self)
            self.order = 0

        def add(self, directory):
            """Add a directory to our current set of directories"""
            if len(directory) > 1:
                directory = directory.rstrip(os.path.sep)
            # only adds and updates order if exists and not already in set
            if not os.path.exists(directory):
                return
            order = self.setdefault(directory, self.order)
            if order == self.order:
                self.order += 1

        def extend(self, directories):
            """Add a list of directories to our set"""
            for a_dir in directories:
                self.add(a_dir)

        def ordered(self):
            """Sort the list of directories"""
            return (i[0] for i in sorted(self.items(), key=lambda d: d[1]))

    def _get_ld_so_conf_dirs(self, conf, dirs):
        """
        Recursive function to help parse all ld.so.conf files, including proper
        handling of the `include` directive.
        """

        try:
            with open(conf) as fileobj:
                for dirname in fileobj:
                    dirname = dirname.strip()
                    if not dirname:
                        continue

                    match = self._include.match(dirname)
                    if not match:
                        dirs.add(dirname)
                    else:
                        for dir2 in glob.glob(match.group("pattern")):
                            self._get_ld_so_conf_dirs(dir2, dirs)
        except IOError:
            pass

    def _create_ld_so_cache(self):
        # Recreate search path followed by ld.so.  This is going to be
        # slow to build, and incorrect (ld.so uses ld.so.cache, which may
        # not be up-to-date).  Used only as fallback for distros without
        # /sbin/ldconfig.
        #
        # We assume the DT_RPATH and DT_RUNPATH binary sections are omitted.

        directories = self._Directories()
        for name in (
            "LD_LIBRARY_PATH",
            "SHLIB_PATH",  # HP-UX
            "LIBPATH",  # OS/2, AIX
            "LIBRARY_PATH",  # BE/OS
        ):
            if name in os.environ:
                directories.extend(os.environ[name].split(os.pathsep))

        self._get_ld_so_conf_dirs("/etc/ld.so.conf", directories)

        bitage = platform.architecture()[0]

        unix_lib_dirs_list = []
        if bitage.startswith("64"):
            # prefer 64 bit if that is our arch
            unix_lib_dirs_list += ["/lib64", "/usr/lib64"]

        # must include standard libs, since those paths are also used by 64 bit
        # installs
        unix_lib_dirs_list += ["/lib", "/usr/lib"]
        if sys.platform.startswith("linux"):
            # Try and support multiarch work in Ubuntu
            # https://wiki.ubuntu.com/MultiarchSpec
            if bitage.startswith("32"):
                # Assume Intel/AMD x86 compat
                unix_lib_dirs_list += ["/lib/i386-linux-gnu", "/usr/lib/i386-linux-gnu"]
            elif bitage.startswith("64"):
                # Assume Intel/AMD x86 compatible
                unix_lib_dirs_list += [
                    "/lib/x86_64-linux-gnu",
                    "/usr/lib/x86_64-linux-gnu",
                ]
            else:
                # guess...
                unix_lib_dirs_list += glob.glob("/lib/*linux-gnu")
        directories.extend(unix_lib_dirs_list)

        cache = {}
        lib_re = re.compile(r"lib(.*)\.s[ol]")
        # ext_re = re.compile(r"\.s[ol]$")
        for our_dir in directories.ordered():
            try:
                for path in glob.glob("%s/*.s[ol]*" % our_dir):
                    file = os.path.basename(path)

                    # Index by filename
                    cache_i = cache.setdefault(file, set())
                    cache_i.add(path)

                    # Index by library name
                    match = lib_re.match(file)
                    if match:
                        library = match.group(1)
                        cache_i = cache.setdefault(library, set())
                        cache_i.add(path)
            except OSError:
                pass

        self._ld_so_cache = cache

    def getplatformpaths(self, libname):
        if self._ld_so_cache is None:
            self._create_ld_so_cache()

        result = self._ld_so_cache.get(libname, set())
        for i in result:
            # we iterate through all found paths for library, since we may have
            # actually found multiple architectures or other library types that
            # may not load
            yield i


# Windows


class WindowsLibraryLoader(LibraryLoader):
    """Library loader for Microsoft Windows"""

    name_formats = ["%s.dll", "lib%s.dll", "%slib.dll", "%s"]

    class Lookup(LibraryLoader.Lookup):
        """Lookup class for Windows libraries..."""

        def __init__(self, path):
            super(WindowsLibraryLoader.Lookup, self).__init__(path)
            self.access["stdcall"] = ctypes.windll.LoadLibrary(path)


# Platform switching

# If your value of sys.platform does not appear in this dict, please contact
# the Ctypesgen maintainers.

loaderclass = {
    "darwin": DarwinLibraryLoader,
    "cygwin": WindowsLibraryLoader,
    "win32": WindowsLibraryLoader,
    "msys": WindowsLibraryLoader,
}

load_library = loaderclass.get(sys.platform, PosixLibraryLoader)()


def add_library_search_dirs(other_dirs):
    """
    Add libraries to search paths.
    If library paths are relative, convert them to absolute with respect to this
    file's directory
    """
    for path in other_dirs:
        if not os.path.isabs(path):
            path = os.path.abspath(path)
        load_library.other_dirs.append(path)


del loaderclass

# End loader

add_library_search_dirs([os.path.join(os.path.dirname(__file__), "_native_lib")])

# Begin libraries
_libs["cybertexel_c"] = load_library(os.environ.get("CYBERTEXEL_LIBRARY", "cybertexel_c"))

# 1 libraries
# End libraries

# No modules

uint8_t = c_ubyte
uint32_t = c_uint
uint64_t = c_ulonglong
enum_ctex_result = c_int
CTEX_RESULT_SUCCESS = 0
CTEX_RESULT_INVALID_ARGUMENT = 1
CTEX_RESULT_MISSING_RESOURCE = 2
CTEX_RESULT_UNSUPPORTED_OPERATION = 3
CTEX_RESULT_OUT_OF_MEMORY = 4
CTEX_RESULT_OVER_BUDGET = 5
CTEX_RESULT_CANCELLED = 6
CTEX_RESULT_INTERNAL_ERROR = 7
CTEX_RESULT_BUFFER_TOO_SMALL = 8
CTEX_RESULT_NO_UNDO = 9
CTEX_RESULT_NO_REDO = 10
CTEX_RESULT_STALE_STATE = 11
ctex_result = enum_ctex_result
enum_ctex_diagnostic_code = c_int
CTEX_DIAGNOSTIC_NONE = 0
CTEX_DIAGNOSTIC_NULL_ARGUMENT = 1
CTEX_DIAGNOSTIC_INVALID_DESCRIPTOR_SIZE = 2
CTEX_DIAGNOSTIC_INVALID_ENUM_VALUE = 3
CTEX_DIAGNOSTIC_INVALID_DESCRIPTOR_VALUE = 4
CTEX_DIAGNOSTIC_BUFFER_TOO_SMALL = 5
CTEX_DIAGNOSTIC_ALLOCATION_FAILED = 6
CTEX_DIAGNOSTIC_UNEXPECTED_EXCEPTION = 7
CTEX_DIAGNOSTIC_EMPTY_TEXTURE_SET_DISPLAY_NAME = 8
CTEX_DIAGNOSTIC_EMPTY_TEXTURE_SET_PARTITION_KEY = 9
CTEX_DIAGNOSTIC_EMPTY_TEXTURE_SET_UV_SET = 10
CTEX_DIAGNOSTIC_INVALID_TEXTURE_SET_RESOLUTION = 11
CTEX_DIAGNOSTIC_INVALID_TEXTURE_SET_BIT_DEPTH = 12
CTEX_DIAGNOSTIC_DUPLICATE_TEXTURE_SET = 13
CTEX_DIAGNOSTIC_ALLOCATOR_CONTRACT_VIOLATION = 14
CTEX_DIAGNOSTIC_MISSING_TEXTURE_SET = 15
CTEX_DIAGNOSTIC_EMPTY_CHANNEL_SEMANTIC_ID = 16
CTEX_DIAGNOSTIC_EMPTY_CHANNEL_EXPORT_MAPPING = 17
CTEX_DIAGNOSTIC_INVALID_CHANNEL_COMPONENT_COUNT = 18
CTEX_DIAGNOSTIC_INVALID_CHANNEL_DEFAULT_VALUE_COUNT = 19
CTEX_DIAGNOSTIC_INVALID_CHANNEL_BIT_DEPTH = 20
CTEX_DIAGNOSTIC_DUPLICATE_CHANNEL = 21
CTEX_DIAGNOSTIC_MISSING_CHANNEL = 22
CTEX_DIAGNOSTIC_UNSUPPORTED_COLOR_SPACE = 23
CTEX_DIAGNOSTIC_UNSUPPORTED_CHANNEL_SEMANTIC = 24
CTEX_DIAGNOSTIC_INVALID_COLOR_COMPONENT = 25
CTEX_DIAGNOSTIC_INVALID_COLOR_BIT_DEPTH = 26
CTEX_DIAGNOSTIC_INVALID_CUBE_LUT = 27
CTEX_DIAGNOSTIC_MESH_LIMIT_EXCEEDED = 28
CTEX_DIAGNOSTIC_INVALID_MESH = 29
CTEX_DIAGNOSTIC_MISSING_UV_SET = 30
CTEX_DIAGNOSTIC_UNSUPPORTED_IMAGE_FORMAT = 31
CTEX_DIAGNOSTIC_INVALID_IMAGE_DATA = 32
CTEX_DIAGNOSTIC_IMAGE_LIMIT_EXCEEDED = 33
CTEX_DIAGNOSTIC_UNSUPPORTED_IMAGE_COMBINATION = 34
CTEX_DIAGNOSTIC_IMAGE_ENCODING_FAILED = 35
CTEX_DIAGNOSTIC_INVALID_STROKE = 36
CTEX_DIAGNOSTIC_INVALID_STROKE_PRESET = 37
CTEX_DIAGNOSTIC_INVALID_PAINT_COVERAGE = 38
CTEX_DIAGNOSTIC_PAINT_LIMIT_EXCEEDED = 39
CTEX_DIAGNOSTIC_INVALID_PAINT_BLEND = 40
CTEX_DIAGNOSTIC_INVALID_PAINT_MASK = 41
CTEX_DIAGNOSTIC_INVALID_PAINT_COORDINATES = 42
CTEX_DIAGNOSTIC_INVALID_PAINT_REJECTION = 43
CTEX_DIAGNOSTIC_INVALID_PAINT_WORK = 44
CTEX_DIAGNOSTIC_INVALID_PAINT_DILATION = 45
CTEX_DIAGNOSTIC_INVALID_PAINT_FILTER = 46
CTEX_DIAGNOSTIC_INVALID_PAINT_SURFACE_CACHE = 47
CTEX_DIAGNOSTIC_INVALID_PAINT_PREVIEW = 48
CTEX_DIAGNOSTIC_INVALID_PICK_QUERY = 49
CTEX_DIAGNOSTIC_INVALID_TEXTURE_EXPORT = 50
CTEX_DIAGNOSTIC_INVALID_PROJECT_CONTAINER = 51
CTEX_DIAGNOSTIC_INVALID_SMART_MATERIAL = 52
CTEX_DIAGNOSTIC_INVALID_PRESET_LIBRARY = 53
CTEX_DIAGNOSTIC_INVALID_HOST_TRANSPORT = 54
CTEX_DIAGNOSTIC_INVALID_EXECUTOR = 55
CTEX_DIAGNOSTIC_INVALID_PAINT_TOOL = 56
CTEX_DIAGNOSTIC_INVALID_MATERIAL_GRAPH = 57
CTEX_DIAGNOSTIC_INVALID_SHADER_EMISSION = 58
CTEX_DIAGNOSTIC_INVALID_MESH_MAP = 59
CTEX_DIAGNOSTIC_INVALID_TILE_HISTORY = 60
CTEX_DIAGNOSTIC_INVALID_OPERATION_RECORD = 61
CTEX_DIAGNOSTIC_INVALID_EDITABLE_AUTHORING = 62
CTEX_DIAGNOSTIC_INVALID_MESH_REPROJECTION = 63
CTEX_DIAGNOSTIC_INVALID_RESOLUTION_CHANGE = 64
CTEX_DIAGNOSTIC_IMAGE_DECODE_CANCELLED = 65
ctex_diagnostic_code = enum_ctex_diagnostic_code
enum_ctex_log_severity = c_int
CTEX_LOG_SEVERITY_TRACE = 0
CTEX_LOG_SEVERITY_DEBUG = 1
CTEX_LOG_SEVERITY_INFO = 2
CTEX_LOG_SEVERITY_WARNING = 3
CTEX_LOG_SEVERITY_ERROR = 4
CTEX_LOG_SEVERITY_FATAL = 5
ctex_log_severity = enum_ctex_log_severity
ctex_log_callback = CFUNCTYPE(UNCHECKED(None), ctex_log_severity, String, String, POINTER(None))

class struct_ctex_log_sink_descriptor(Structure):
    pass

struct_ctex_log_sink_descriptor.__slots__ = [
    'size',
    'callback',
    'user_data',
    'minimum_severity',
]
struct_ctex_log_sink_descriptor._fields_ = [
    ('size', uint32_t),
    ('callback', ctex_log_callback),
    ('user_data', POINTER(None)),
    ('minimum_severity', uint32_t),
]

ctex_log_sink_descriptor = struct_ctex_log_sink_descriptor
ctex_allocate_callback = CFUNCTYPE(UNCHECKED(POINTER(c_ubyte)), c_size_t, c_size_t, POINTER(None))
ctex_deallocate_callback = CFUNCTYPE(UNCHECKED(None), POINTER(None), c_size_t, c_size_t, POINTER(None))

class struct_ctex_allocator_descriptor(Structure):
    pass

struct_ctex_allocator_descriptor.__slots__ = [
    'size',
    'allocate',
    'deallocate',
    'user_data',
]
struct_ctex_allocator_descriptor._fields_ = [
    ('size', uint32_t),
    ('allocate', ctex_allocate_callback),
    ('deallocate', ctex_deallocate_callback),
    ('user_data', POINTER(None)),
]

ctex_allocator_descriptor = struct_ctex_allocator_descriptor

class struct_ctex_document(Structure):
    pass

ctex_document = struct_ctex_document

class struct_ctex_cube_lut(Structure):
    pass

ctex_cube_lut = struct_ctex_cube_lut

class struct_ctex_mesh(Structure):
    pass

ctex_mesh = struct_ctex_mesh

class struct_ctex_mesh_replacement_plan(Structure):
    pass

ctex_mesh_replacement_plan = struct_ctex_mesh_replacement_plan

class struct_ctex_paint_dilation_session(Structure):
    pass

ctex_paint_dilation_session = struct_ctex_paint_dilation_session

class struct_ctex_paint_surface_map_cache(Structure):
    pass

ctex_paint_surface_map_cache = struct_ctex_paint_surface_map_cache

class struct_ctex_paint_preview_session(Structure):
    pass

ctex_paint_preview_session = struct_ctex_paint_preview_session

class struct_ctex_pick_index(Structure):
    pass

ctex_pick_index = struct_ctex_pick_index

class struct_ctex_uv_pick_index(Structure):
    pass

ctex_uv_pick_index = struct_ctex_uv_pick_index

class struct_ctex_transport_snapshot_pool(Structure):
    pass

ctex_transport_snapshot_pool = struct_ctex_transport_snapshot_pool

class struct_ctex_transport_snapshot(Structure):
    pass

ctex_transport_snapshot = struct_ctex_transport_snapshot

class struct_ctex_transport_readback(Structure):
    pass

ctex_transport_readback = struct_ctex_transport_readback

class struct_ctex_resource_ledger(Structure):
    pass

ctex_resource_ledger = struct_ctex_resource_ledger

class struct_ctex_resource_reservation(Structure):
    pass

ctex_resource_reservation = struct_ctex_resource_reservation

class struct_ctex_mesh_map_set(Structure):
    pass

ctex_mesh_map_set = struct_ctex_mesh_map_set

class struct_ctex_mesh_map_bake_session(Structure):
    pass

ctex_mesh_map_bake_session = struct_ctex_mesh_map_bake_session

class struct_ctex_mesh_map_bake_request_token(Structure):
    pass

ctex_mesh_map_bake_request_token = struct_ctex_mesh_map_bake_request_token

class struct_ctex_project_autosave_session(Structure):
    pass

ctex_project_autosave_session = struct_ctex_project_autosave_session

class struct_ctex_executor_registry(Structure):
    pass

ctex_executor_registry = struct_ctex_executor_registry

class struct_ctex_cpu_execution_result(Structure):
    pass

ctex_cpu_execution_result = struct_ctex_cpu_execution_result

class struct_ctex_parity_gate_result(Structure):
    pass

ctex_parity_gate_result = struct_ctex_parity_gate_result

class struct_ctex_host_execution_session(Structure):
    pass

ctex_host_execution_session = struct_ctex_host_execution_session

class struct_ctex_host_completion_result(Structure):
    pass

ctex_host_completion_result = struct_ctex_host_completion_result

class struct_ctex_host_recovery_report(Structure):
    pass

ctex_host_recovery_report = struct_ctex_host_recovery_report

class struct_ctex_material_graph_workspace(Structure):
    pass

ctex_material_graph_workspace = struct_ctex_material_graph_workspace

class struct_ctex_material_graph_node_registry(Structure):
    pass

ctex_material_graph_node_registry = struct_ctex_material_graph_node_registry

class struct_ctex_shader_emission_cache(Structure):
    pass

ctex_shader_emission_cache = struct_ctex_shader_emission_cache

class struct_ctex_tile_history_capture(Structure):
    pass

ctex_tile_history_capture = struct_ctex_tile_history_capture

class struct_ctex_layer_snapshot(Structure):
    pass

ctex_layer_snapshot = struct_ctex_layer_snapshot

class struct_ctex_texture_set_transaction(Structure):
    pass

ctex_texture_set_transaction = struct_ctex_texture_set_transaction
enum_ctex_partition_source_kind = c_int
CTEX_PARTITION_SOURCE_MATERIAL = 0
CTEX_PARTITION_SOURCE_OBJECT = 1
CTEX_PARTITION_SOURCE_SUBMESH = 2
CTEX_PARTITION_SOURCE_EXPLICIT_FACES = 3
ctex_partition_source_kind = enum_ctex_partition_source_kind

class struct_ctex_vec2f(Structure):
    pass

struct_ctex_vec2f.__slots__ = [
    'x',
    'y',
]
struct_ctex_vec2f._fields_ = [
    ('x', c_float),
    ('y', c_float),
]

ctex_vec2f = struct_ctex_vec2f

class struct_ctex_vec3f(Structure):
    pass

struct_ctex_vec3f.__slots__ = [
    'x',
    'y',
    'z',
]
struct_ctex_vec3f._fields_ = [
    ('x', c_float),
    ('y', c_float),
    ('z', c_float),
]

ctex_vec3f = struct_ctex_vec3f

class struct_ctex_vec4f(Structure):
    pass

struct_ctex_vec4f.__slots__ = [
    'x',
    'y',
    'z',
    'w',
]
struct_ctex_vec4f._fields_ = [
    ('x', c_float),
    ('y', c_float),
    ('z', c_float),
    ('w', c_float),
]

ctex_vec4f = struct_ctex_vec4f

class struct_ctex_vec2d(Structure):
    pass

struct_ctex_vec2d.__slots__ = [
    'x',
    'y',
]
struct_ctex_vec2d._fields_ = [
    ('x', c_double),
    ('y', c_double),
]

ctex_vec2d = struct_ctex_vec2d

class struct_ctex_vec3d(Structure):
    pass

struct_ctex_vec3d.__slots__ = [
    'x',
    'y',
    'z',
]
struct_ctex_vec3d._fields_ = [
    ('x', c_double),
    ('y', c_double),
    ('z', c_double),
]

ctex_vec3d = struct_ctex_vec3d

class struct_ctex_stroke_frame(Structure):
    pass

struct_ctex_stroke_frame.__slots__ = [
    'tangent',
    'bitangent',
    'normal',
]
struct_ctex_stroke_frame._fields_ = [
    ('tangent', ctex_vec3d),
    ('bitangent', ctex_vec3d),
    ('normal', ctex_vec3d),
]

ctex_stroke_frame = struct_ctex_stroke_frame

class struct_ctex_stroke_input_sample(Structure):
    pass

struct_ctex_stroke_input_sample.__slots__ = [
    'size',
    'position',
    'frame',
    'timestamp_nanoseconds',
    'has_pressure',
    'pressure',
    'tilt',
]
struct_ctex_stroke_input_sample._fields_ = [
    ('size', uint32_t),
    ('position', ctex_vec3d),
    ('frame', ctex_stroke_frame),
    ('timestamp_nanoseconds', uint64_t),
    ('has_pressure', uint32_t),
    ('pressure', c_double),
    ('tilt', ctex_vec2d),
]

ctex_stroke_input_sample = struct_ctex_stroke_input_sample
enum_ctex_stroke_tip_mode = c_int
CTEX_STROKE_TIP_CONTINUOUS_SWEEP = 0
CTEX_STROKE_TIP_DISCRETE_ALPHA = 1
ctex_stroke_tip_mode = enum_ctex_stroke_tip_mode

class struct_ctex_response_curve_point(Structure):
    pass

struct_ctex_response_curve_point.__slots__ = [
    'input',
    'output',
]
struct_ctex_response_curve_point._fields_ = [
    ('input', c_double),
    ('output', c_double),
]

ctex_response_curve_point = struct_ctex_response_curve_point

class struct_ctex_response_mapping_descriptor(Structure):
    pass

struct_ctex_response_mapping_descriptor.__slots__ = [
    'size',
    'enabled',
    'points',
    'point_count',
    'minimum_output',
    'maximum_output',
]
struct_ctex_response_mapping_descriptor._fields_ = [
    ('size', uint32_t),
    ('enabled', uint32_t),
    ('points', POINTER(ctex_response_curve_point)),
    ('point_count', c_size_t),
    ('minimum_output', c_double),
    ('maximum_output', c_double),
]

ctex_response_mapping_descriptor = struct_ctex_response_mapping_descriptor

class struct_ctex_stroke_stabilizer_descriptor(Structure):
    pass

struct_ctex_stroke_stabilizer_descriptor.__slots__ = [
    'size',
    'radius',
    'time_constant_seconds',
]
struct_ctex_stroke_stabilizer_descriptor._fields_ = [
    ('size', uint32_t),
    ('radius', c_double),
    ('time_constant_seconds', c_double),
]

ctex_stroke_stabilizer_descriptor = struct_ctex_stroke_stabilizer_descriptor

class struct_ctex_stroke_jitter_descriptor(Structure):
    pass

struct_ctex_stroke_jitter_descriptor.__slots__ = [
    'size',
    'seed',
    'position_fraction',
    'radius_fraction',
    'rotation_radians',
    'opacity',
    'flow',
]
struct_ctex_stroke_jitter_descriptor._fields_ = [
    ('size', uint32_t),
    ('seed', uint64_t),
    ('position_fraction', c_double),
    ('radius_fraction', c_double),
    ('rotation_radians', c_double),
    ('opacity', c_double),
    ('flow', c_double),
]

ctex_stroke_jitter_descriptor = struct_ctex_stroke_jitter_descriptor
enum_ctex_stroke_taper_unit = c_int
CTEX_STROKE_TAPER_NONE = 0
CTEX_STROKE_TAPER_STAMP_COUNT = 1
CTEX_STROKE_TAPER_DISTANCE = 2
ctex_stroke_taper_unit = enum_ctex_stroke_taper_unit

class struct_ctex_stroke_taper_span_descriptor(Structure):
    pass

struct_ctex_stroke_taper_span_descriptor.__slots__ = [
    'size',
    'unit',
    'extent',
]
struct_ctex_stroke_taper_span_descriptor._fields_ = [
    ('size', uint32_t),
    ('unit', uint32_t),
    ('extent', c_double),
]

ctex_stroke_taper_span_descriptor = struct_ctex_stroke_taper_span_descriptor

class struct_ctex_stroke_taper_descriptor(Structure):
    pass

struct_ctex_stroke_taper_descriptor.__slots__ = [
    'size',
    'entry',
    'exit',
    'floor',
    'affect_radius',
    'affect_opacity',
]
struct_ctex_stroke_taper_descriptor._fields_ = [
    ('size', uint32_t),
    ('entry', ctex_stroke_taper_span_descriptor),
    ('exit', ctex_stroke_taper_span_descriptor),
    ('floor', c_double),
    ('affect_radius', uint32_t),
    ('affect_opacity', uint32_t),
]

ctex_stroke_taper_descriptor = struct_ctex_stroke_taper_descriptor
enum_ctex_stroke_constraint_mode = c_int
CTEX_STROKE_CONSTRAINT_NONE = 0
CTEX_STROKE_CONSTRAINT_STRAIGHT_LINE = 1
CTEX_STROKE_CONSTRAINT_DOMINANT_AXIS = 2
CTEX_STROKE_CONSTRAINT_GRID = 3
ctex_stroke_constraint_mode = enum_ctex_stroke_constraint_mode

class struct_ctex_stroke_constraint_descriptor(Structure):
    pass

struct_ctex_stroke_constraint_descriptor.__slots__ = [
    'size',
    'mode',
    'grid_step',
]
struct_ctex_stroke_constraint_descriptor._fields_ = [
    ('size', uint32_t),
    ('mode', uint32_t),
    ('grid_step', c_double),
]

ctex_stroke_constraint_descriptor = struct_ctex_stroke_constraint_descriptor
enum_ctex_stroke_symmetry_axis = c_int
CTEX_STROKE_SYMMETRY_AXIS_X = 0
CTEX_STROKE_SYMMETRY_AXIS_Y = 1
CTEX_STROKE_SYMMETRY_AXIS_Z = 2
ctex_stroke_symmetry_axis = enum_ctex_stroke_symmetry_axis

class struct_ctex_stroke_symmetry_descriptor(Structure):
    pass

struct_ctex_stroke_symmetry_descriptor.__slots__ = [
    'size',
    'mirror_x',
    'mirror_y',
    'mirror_z',
    'radial_count',
    'radial_axis',
]
struct_ctex_stroke_symmetry_descriptor._fields_ = [
    ('size', uint32_t),
    ('mirror_x', uint32_t),
    ('mirror_y', uint32_t),
    ('mirror_z', uint32_t),
    ('radial_count', uint32_t),
    ('radial_axis', uint32_t),
]

ctex_stroke_symmetry_descriptor = struct_ctex_stroke_symmetry_descriptor

class struct_ctex_stroke_settings_descriptor(Structure):
    pass

struct_ctex_stroke_settings_descriptor.__slots__ = [
    'size',
    'reconstruction_version',
    'tip_mode',
    'spacing_fraction',
    'radius',
    'opacity',
    'hardness',
    'rotation_radians',
    'elongation',
    'flow',
    'tip_resource_identity',
    'stabilizer',
    'pressure_radius',
    'pressure_opacity',
    'pressure_hardness',
    'pressure_flow',
    'pressure_rotation',
    'tilt_rotation',
    'tilt_elongation',
    'jitter',
    'taper',
    'constraint',
    'symmetry',
]
struct_ctex_stroke_settings_descriptor._fields_ = [
    ('size', uint32_t),
    ('reconstruction_version', uint32_t),
    ('tip_mode', uint32_t),
    ('spacing_fraction', c_double),
    ('radius', c_double),
    ('opacity', c_double),
    ('hardness', c_double),
    ('rotation_radians', c_double),
    ('elongation', c_double),
    ('flow', c_double),
    ('tip_resource_identity', String),
    ('stabilizer', ctex_stroke_stabilizer_descriptor),
    ('pressure_radius', ctex_response_mapping_descriptor),
    ('pressure_opacity', ctex_response_mapping_descriptor),
    ('pressure_hardness', ctex_response_mapping_descriptor),
    ('pressure_flow', ctex_response_mapping_descriptor),
    ('pressure_rotation', ctex_response_mapping_descriptor),
    ('tilt_rotation', ctex_response_mapping_descriptor),
    ('tilt_elongation', ctex_response_mapping_descriptor),
    ('jitter', ctex_stroke_jitter_descriptor),
    ('taper', ctex_stroke_taper_descriptor),
    ('constraint', ctex_stroke_constraint_descriptor),
    ('symmetry', ctex_stroke_symmetry_descriptor),
]

ctex_stroke_settings_descriptor = struct_ctex_stroke_settings_descriptor

class struct_ctex_resolved_stamp(Structure):
    pass

struct_ctex_resolved_stamp.__slots__ = [
    'position',
    'frame',
    'radius',
    'opacity',
    'hardness',
    'rotation_radians',
    'elongation',
    'flow',
    'tip_resource_identity',
    'source_ordinal',
    'symmetry_instance',
    'ordinal',
]
struct_ctex_resolved_stamp._fields_ = [
    ('position', ctex_vec3d),
    ('frame', ctex_stroke_frame),
    ('radius', c_double),
    ('opacity', c_double),
    ('hardness', c_double),
    ('rotation_radians', c_double),
    ('elongation', c_double),
    ('flow', c_double),
    ('tip_resource_identity', String),
    ('source_ordinal', uint64_t),
    ('symmetry_instance', uint64_t),
    ('ordinal', uint64_t),
]

ctex_resolved_stamp = struct_ctex_resolved_stamp

class struct_ctex_swept_segment(Structure):
    pass

struct_ctex_swept_segment.__slots__ = [
    'start_stamp_ordinal',
    'end_stamp_ordinal',
]
struct_ctex_swept_segment._fields_ = [
    ('start_stamp_ordinal', uint64_t),
    ('end_stamp_ordinal', uint64_t),
]

ctex_swept_segment = struct_ctex_swept_segment

class struct_ctex_resolved_stroke_info(Structure):
    pass

struct_ctex_resolved_stroke_info.__slots__ = [
    'size',
    'reconstruction_version',
    'tip_mode',
    'symmetry_instance_count',
    'stamp_count',
    'swept_segment_count',
]
struct_ctex_resolved_stroke_info._fields_ = [
    ('size', uint32_t),
    ('reconstruction_version', uint32_t),
    ('tip_mode', uint32_t),
    ('symmetry_instance_count', uint64_t),
    ('stamp_count', c_size_t),
    ('swept_segment_count', c_size_t),
]

ctex_resolved_stroke_info = struct_ctex_resolved_stroke_info

class struct_ctex_resolved_stroke_descriptor(Structure):
    pass

struct_ctex_resolved_stroke_descriptor.__slots__ = [
    'size',
    'reconstruction_version',
    'tip_mode',
    'symmetry_instance_count',
    'stamps',
    'stamp_count',
    'swept_segments',
    'swept_segment_count',
]
struct_ctex_resolved_stroke_descriptor._fields_ = [
    ('size', uint32_t),
    ('reconstruction_version', uint32_t),
    ('tip_mode', uint32_t),
    ('symmetry_instance_count', uint64_t),
    ('stamps', POINTER(ctex_resolved_stamp)),
    ('stamp_count', c_size_t),
    ('swept_segments', POINTER(ctex_swept_segment)),
    ('swept_segment_count', c_size_t),
]

ctex_resolved_stroke_descriptor = struct_ctex_resolved_stroke_descriptor

class struct_ctex_paint_tile_coverage_descriptor(Structure):
    pass

struct_ctex_paint_tile_coverage_descriptor.__slots__ = [
    'size',
    'uv_set',
    'width',
    'height',
    'tile_origin',
]
struct_ctex_paint_tile_coverage_descriptor._fields_ = [
    ('size', uint32_t),
    ('uv_set', String),
    ('width', uint32_t),
    ('height', uint32_t),
    ('tile_origin', ctex_vec2d),
]

ctex_paint_tile_coverage_descriptor = struct_ctex_paint_tile_coverage_descriptor
enum_ctex_paint_deposition_mode = c_int
CTEX_PAINT_DEPOSITION_NON_BUILDING = 0
CTEX_PAINT_DEPOSITION_BUILD_UP = 1
ctex_paint_deposition_mode = enum_ctex_paint_deposition_mode
enum_ctex_alpha_discard_format = c_int
CTEX_ALPHA_DISCARD_UNORM8 = 0
CTEX_ALPHA_DISCARD_UNORM16 = 1
CTEX_ALPHA_DISCARD_FLOATING_POINT = 2
ctex_alpha_discard_format = enum_ctex_alpha_discard_format

class struct_ctex_paint_mask_view(Structure):
    pass

struct_ctex_paint_mask_view.__slots__ = [
    'values',
    'value_count',
]
struct_ctex_paint_mask_view._fields_ = [
    ('values', POINTER(c_double)),
    ('value_count', c_size_t),
]

ctex_paint_mask_view = struct_ctex_paint_mask_view

class struct_ctex_paint_mask_inputs_descriptor(Structure):
    pass

struct_ctex_paint_mask_inputs_descriptor.__slots__ = [
    'size',
    'active_layer_masks',
    'active_layer_mask_count',
    'colour_id_selection',
    'geometry_selection',
    'screen_selection',
    'uv_island_selection',
]
struct_ctex_paint_mask_inputs_descriptor._fields_ = [
    ('size', uint32_t),
    ('active_layer_masks', POINTER(ctex_paint_mask_view)),
    ('active_layer_mask_count', c_size_t),
    ('colour_id_selection', POINTER(ctex_paint_mask_view)),
    ('geometry_selection', POINTER(ctex_paint_mask_view)),
    ('screen_selection', POINTER(ctex_paint_mask_view)),
    ('uv_island_selection', POINTER(ctex_paint_mask_view)),
]

ctex_paint_mask_inputs_descriptor = struct_ctex_paint_mask_inputs_descriptor

class struct_ctex_paint_mask_info(Structure):
    pass

struct_ctex_paint_mask_info.__slots__ = [
    'size',
    'active_input_count',
]
struct_ctex_paint_mask_info._fields_ = [
    ('size', uint32_t),
    ('active_input_count', c_size_t),
]

ctex_paint_mask_info = struct_ctex_paint_mask_info
enum_ctex_paint_material_coordinate_mode = c_int
CTEX_PAINT_MATERIAL_COORDINATE_UV = 0
CTEX_PAINT_MATERIAL_COORDINATE_TRIPLANAR = 1
CTEX_PAINT_MATERIAL_COORDINATE_PLANAR = 2
ctex_paint_material_coordinate_mode = enum_ctex_paint_material_coordinate_mode

class struct_ctex_paint_material_coordinate_descriptor(Structure):
    pass

struct_ctex_paint_material_coordinate_descriptor.__slots__ = [
    'size',
    'mode',
    'planar_origin',
    'planar_u_axis',
    'planar_v_axis',
]
struct_ctex_paint_material_coordinate_descriptor._fields_ = [
    ('size', uint32_t),
    ('mode', uint32_t),
    ('planar_origin', ctex_vec3d),
    ('planar_u_axis', ctex_vec3d),
    ('planar_v_axis', ctex_vec3d),
]

ctex_paint_material_coordinate_descriptor = struct_ctex_paint_material_coordinate_descriptor

class struct_ctex_paint_material_coordinate_sample(Structure):
    pass

struct_ctex_paint_material_coordinate_sample.__slots__ = [
    'covered',
    'projection_count',
    'coordinates',
    'weights',
]
struct_ctex_paint_material_coordinate_sample._fields_ = [
    ('covered', uint32_t),
    ('projection_count', uint32_t),
    ('coordinates', ctex_vec2d * int(3)),
    ('weights', c_double * int(3)),
]

ctex_paint_material_coordinate_sample = struct_ctex_paint_material_coordinate_sample
enum_ctex_paint_symmetry_depth_policy = c_int
CTEX_PAINT_SYMMETRY_DEPTH_REQUIRE_CONSISTENT = 0
CTEX_PAINT_SYMMETRY_DEPTH_DISABLE_DERIVED = 1
ctex_paint_symmetry_depth_policy = enum_ctex_paint_symmetry_depth_policy
enum_ctex_paint_depth_disposition = c_int
CTEX_PAINT_DEPTH_DISABLED_BY_OPERATION = 0
CTEX_PAINT_DEPTH_CONSISTENT_PER_INSTANCE = 1
CTEX_PAINT_DEPTH_DISABLED_FOR_DERIVED_SYMMETRY = 2
ctex_paint_depth_disposition = enum_ctex_paint_depth_disposition

class struct_ctex_paint_depth_context_descriptor(Structure):
    pass

struct_ctex_paint_depth_context_descriptor.__slots__ = [
    'size',
    'symmetry_instance',
    'viewport_width',
    'viewport_height',
    'screen_positions',
    'surface_depth',
    'surface_sample_count',
    'visible_depth',
    'visible_depth_count',
    'transform_consistent',
]
struct_ctex_paint_depth_context_descriptor._fields_ = [
    ('size', uint32_t),
    ('symmetry_instance', uint64_t),
    ('viewport_width', uint32_t),
    ('viewport_height', uint32_t),
    ('screen_positions', POINTER(ctex_vec2d)),
    ('surface_depth', POINTER(c_double)),
    ('surface_sample_count', c_size_t),
    ('visible_depth', POINTER(c_double)),
    ('visible_depth_count', c_size_t),
    ('transform_consistent', uint32_t),
]

ctex_paint_depth_context_descriptor = struct_ctex_paint_depth_context_descriptor

class struct_ctex_paint_rejection_descriptor(Structure):
    pass

struct_ctex_paint_rejection_descriptor.__slots__ = [
    'size',
    'depth_enabled',
    'depth_bias',
    'symmetry_depth_policy',
    'angle_enabled',
    'minimum_normal_dot',
    'backface_enabled',
    'depth_contexts',
    'depth_context_count',
    'view_directions',
    'view_direction_count',
]
struct_ctex_paint_rejection_descriptor._fields_ = [
    ('size', uint32_t),
    ('depth_enabled', uint32_t),
    ('depth_bias', c_double),
    ('symmetry_depth_policy', uint32_t),
    ('angle_enabled', uint32_t),
    ('minimum_normal_dot', c_double),
    ('backface_enabled', uint32_t),
    ('depth_contexts', POINTER(ctex_paint_depth_context_descriptor)),
    ('depth_context_count', c_size_t),
    ('view_directions', POINTER(ctex_vec3d)),
    ('view_direction_count', c_size_t),
]

ctex_paint_rejection_descriptor = struct_ctex_paint_rejection_descriptor

class struct_ctex_paint_rejection_info(Structure):
    pass

struct_ctex_paint_rejection_info.__slots__ = [
    'size',
    'depth_disposition',
    'depth_rejected_contributions',
    'angle_rejected_contributions',
    'backface_rejected_texels',
    'resolved_depth_bias',
    'resolved_minimum_normal_dot',
    'depth_bias_clamped',
    'minimum_normal_dot_clamped',
]
struct_ctex_paint_rejection_info._fields_ = [
    ('size', uint32_t),
    ('depth_disposition', uint32_t),
    ('depth_rejected_contributions', c_size_t),
    ('angle_rejected_contributions', c_size_t),
    ('backface_rejected_texels', c_size_t),
    ('resolved_depth_bias', c_double),
    ('resolved_minimum_normal_dot', c_double),
    ('depth_bias_clamped', uint32_t),
    ('minimum_normal_dot_clamped', uint32_t),
]

ctex_paint_rejection_info = struct_ctex_paint_rejection_info

class struct_ctex_paint_stamp_footprint(Structure):
    pass

struct_ctex_paint_stamp_footprint.__slots__ = [
    'stamp_ordinal',
    'minimum_x',
    'minimum_y',
    'maximum_x',
    'maximum_y',
]
struct_ctex_paint_stamp_footprint._fields_ = [
    ('stamp_ordinal', uint64_t),
    ('minimum_x', uint32_t),
    ('minimum_y', uint32_t),
    ('maximum_x', uint32_t),
    ('maximum_y', uint32_t),
]

ctex_paint_stamp_footprint = struct_ctex_paint_stamp_footprint

class struct_ctex_paint_tile_coordinate(Structure):
    pass

struct_ctex_paint_tile_coordinate.__slots__ = [
    'x',
    'y',
]
struct_ctex_paint_tile_coordinate._fields_ = [
    ('x', uint32_t),
    ('y', uint32_t),
]

ctex_paint_tile_coordinate = struct_ctex_paint_tile_coordinate
enum_ctex_paint_preview_state = c_int
CTEX_PAINT_PREVIEW_PROVISIONAL = 0
CTEX_PAINT_PREVIEW_FINAL = 1
CTEX_PAINT_PREVIEW_COMMITTED = 2
CTEX_PAINT_PREVIEW_CANCELLED = 3
ctex_paint_preview_state = enum_ctex_paint_preview_state

class struct_ctex_paint_preview_info(Structure):
    pass

struct_ctex_paint_preview_info.__slots__ = [
    'size',
    'state',
    'width',
    'height',
    'component_count',
    'scalar_representation',
    'bit_depth',
    'pixel_byte_count',
    'resolved_dilation_radius',
    'dilation_radius_clamped',
    'dilated_texel_count',
    'zero_gradient_texel_count',
    'baseline_epoch',
    'baseline_revision',
    'preview_epoch',
    'preview_revision',
    'committed_epoch',
    'committed_revision',
    'changed_tile_count',
    'maximum_component_error',
]
struct_ctex_paint_preview_info._fields_ = [
    ('size', uint32_t),
    ('state', uint32_t),
    ('width', uint32_t),
    ('height', uint32_t),
    ('component_count', uint32_t),
    ('scalar_representation', uint32_t),
    ('bit_depth', uint32_t),
    ('pixel_byte_count', c_size_t),
    ('resolved_dilation_radius', uint32_t),
    ('dilation_radius_clamped', uint32_t),
    ('dilated_texel_count', c_size_t),
    ('zero_gradient_texel_count', c_size_t),
    ('baseline_epoch', uint64_t),
    ('baseline_revision', uint64_t),
    ('preview_epoch', uint64_t),
    ('preview_revision', uint64_t),
    ('committed_epoch', uint64_t),
    ('committed_revision', uint64_t),
    ('changed_tile_count', c_size_t),
    ('maximum_component_error', c_double),
]

ctex_paint_preview_info = struct_ctex_paint_preview_info

class struct_ctex_paint_work_descriptor(Structure):
    pass

struct_ctex_paint_work_descriptor.__slots__ = [
    'size',
    'canvas_width',
    'canvas_height',
    'tile_size',
    'dilation_radius',
    'stamp_footprints',
    'stamp_footprint_count',
]
struct_ctex_paint_work_descriptor._fields_ = [
    ('size', uint32_t),
    ('canvas_width', uint32_t),
    ('canvas_height', uint32_t),
    ('tile_size', uint32_t),
    ('dilation_radius', uint32_t),
    ('stamp_footprints', POINTER(ctex_paint_stamp_footprint)),
    ('stamp_footprint_count', c_size_t),
]

ctex_paint_work_descriptor = struct_ctex_paint_work_descriptor

class struct_ctex_paint_work_info(Structure):
    pass

struct_ctex_paint_work_info.__slots__ = [
    'size',
    'canvas_tile_count',
    'footprint_count',
    'candidate_tile_visits',
    'processed_tile_count',
    'resolved_dilation_radius',
    'dilation_radius_clamped',
]
struct_ctex_paint_work_info._fields_ = [
    ('size', uint32_t),
    ('canvas_tile_count', uint64_t),
    ('footprint_count', c_size_t),
    ('candidate_tile_visits', c_size_t),
    ('processed_tile_count', c_size_t),
    ('resolved_dilation_radius', uint32_t),
    ('dilation_radius_clamped', uint32_t),
]

ctex_paint_work_info = struct_ctex_paint_work_info

class struct_ctex_paint_seam_dilation_descriptor(Structure):
    pass

struct_ctex_paint_seam_dilation_descriptor.__slots__ = [
    'size',
    'width',
    'height',
    'component_count',
    'radius',
    'pixels',
    'pixel_count',
    'coverage',
    'coverage_count',
]
struct_ctex_paint_seam_dilation_descriptor._fields_ = [
    ('size', uint32_t),
    ('width', uint32_t),
    ('height', uint32_t),
    ('component_count', uint32_t),
    ('radius', uint32_t),
    ('pixels', POINTER(c_double)),
    ('pixel_count', c_size_t),
    ('coverage', POINTER(uint8_t)),
    ('coverage_count', c_size_t),
]

ctex_paint_seam_dilation_descriptor = struct_ctex_paint_seam_dilation_descriptor

class struct_ctex_paint_seam_dilation_info(Structure):
    pass

struct_ctex_paint_seam_dilation_info.__slots__ = [
    'size',
    'required_pixel_count',
    'dilated_texel_count',
    'zero_gradient_texel_count',
    'resolved_radius',
    'radius_clamped',
]
struct_ctex_paint_seam_dilation_info._fields_ = [
    ('size', uint32_t),
    ('required_pixel_count', c_size_t),
    ('dilated_texel_count', c_size_t),
    ('zero_gradient_texel_count', c_size_t),
    ('resolved_radius', uint32_t),
    ('radius_clamped', uint32_t),
]

ctex_paint_seam_dilation_info = struct_ctex_paint_seam_dilation_info
enum_ctex_paint_dilation_state = c_int
CTEX_PAINT_DILATION_PROVISIONAL = 0
CTEX_PAINT_DILATION_FINAL = 1
ctex_paint_dilation_state = enum_ctex_paint_dilation_state

class struct_ctex_paint_dilation_tile_descriptor(Structure):
    pass

struct_ctex_paint_dilation_tile_descriptor.__slots__ = [
    'size',
    'u',
    'v',
    'width',
    'height',
    'component_count',
    'pixels',
    'pixel_count',
    'coverage',
    'coverage_count',
]
struct_ctex_paint_dilation_tile_descriptor._fields_ = [
    ('size', uint32_t),
    ('u', c_int32),
    ('v', c_int32),
    ('width', uint32_t),
    ('height', uint32_t),
    ('component_count', uint32_t),
    ('pixels', POINTER(c_double)),
    ('pixel_count', c_size_t),
    ('coverage', POINTER(uint8_t)),
    ('coverage_count', c_size_t),
]

ctex_paint_dilation_tile_descriptor = struct_ctex_paint_dilation_tile_descriptor

class struct_ctex_paint_dilation_tile_info(Structure):
    pass

struct_ctex_paint_dilation_tile_info.__slots__ = [
    'u',
    'v',
    'width',
    'height',
    'component_count',
    'pixel_offset',
    'pixel_count',
    'dilated_texel_count',
    'zero_gradient_texel_count',
]
struct_ctex_paint_dilation_tile_info._fields_ = [
    ('u', c_int32),
    ('v', c_int32),
    ('width', uint32_t),
    ('height', uint32_t),
    ('component_count', uint32_t),
    ('pixel_offset', c_size_t),
    ('pixel_count', c_size_t),
    ('dilated_texel_count', c_size_t),
    ('zero_gradient_texel_count', c_size_t),
]

ctex_paint_dilation_tile_info = struct_ctex_paint_dilation_tile_info

class struct_ctex_paint_dilation_session_info(Structure):
    pass

struct_ctex_paint_dilation_session_info.__slots__ = [
    'size',
    'state',
    'tile_count',
    'required_pixel_count',
    'dilation_pass_count',
    'resolved_radius',
    'radius_clamped',
]
struct_ctex_paint_dilation_session_info._fields_ = [
    ('size', uint32_t),
    ('state', uint32_t),
    ('tile_count', c_size_t),
    ('required_pixel_count', c_size_t),
    ('dilation_pass_count', c_size_t),
    ('resolved_radius', uint32_t),
    ('radius_clamped', uint32_t),
]

ctex_paint_dilation_session_info = struct_ctex_paint_dilation_session_info
enum_ctex_paint_surface_filter_operation = c_int
CTEX_PAINT_SURFACE_FILTER_BLUR = 0
CTEX_PAINT_SURFACE_FILTER_SMEAR = 1
CTEX_PAINT_SURFACE_FILTER_DERIVATIVE = 2
CTEX_PAINT_SURFACE_FILTER_MIP_GENERATION = 3
ctex_paint_surface_filter_operation = enum_ctex_paint_surface_filter_operation

class struct_ctex_paint_surface_filter_sample(Structure):
    pass

struct_ctex_paint_surface_filter_sample.__slots__ = [
    'texel_index',
    'tangent_frame',
    'offset_x',
    'offset_y',
    'weight',
]
struct_ctex_paint_surface_filter_sample._fields_ = [
    ('texel_index', c_size_t),
    ('tangent_frame', ctex_stroke_frame),
    ('offset_x', c_int32),
    ('offset_y', c_int32),
    ('weight', c_double),
]

ctex_paint_surface_filter_sample = struct_ctex_paint_surface_filter_sample

class struct_ctex_paint_surface_filter_descriptor(Structure):
    pass

struct_ctex_paint_surface_filter_descriptor.__slots__ = [
    'size',
    'operation',
    'radius_x',
    'radius_y',
    'output_frame',
    'samples',
    'sample_count',
]
struct_ctex_paint_surface_filter_descriptor._fields_ = [
    ('size', uint32_t),
    ('operation', uint32_t),
    ('radius_x', uint32_t),
    ('radius_y', uint32_t),
    ('output_frame', ctex_stroke_frame),
    ('samples', POINTER(ctex_paint_surface_filter_sample)),
    ('sample_count', c_size_t),
]

ctex_paint_surface_filter_descriptor = struct_ctex_paint_surface_filter_descriptor

class struct_ctex_paint_island_padding_descriptor(Structure):
    pass

struct_ctex_paint_island_padding_descriptor.__slots__ = [
    'size',
    'width',
    'height',
    'component_count',
    'radius_x',
    'radius_y',
    'requested_mip_levels',
    'island_identity',
    'island_identity_count',
    'pixels',
    'pixel_count',
]
struct_ctex_paint_island_padding_descriptor._fields_ = [
    ('size', uint32_t),
    ('width', uint32_t),
    ('height', uint32_t),
    ('component_count', uint32_t),
    ('radius_x', uint32_t),
    ('radius_y', uint32_t),
    ('requested_mip_levels', uint32_t),
    ('island_identity', POINTER(uint32_t)),
    ('island_identity_count', c_size_t),
    ('pixels', POINTER(c_double)),
    ('pixel_count', c_size_t),
]

ctex_paint_island_padding_descriptor = struct_ctex_paint_island_padding_descriptor

class struct_ctex_paint_unsupported_mip_level(Structure):
    pass

struct_ctex_paint_unsupported_mip_level.__slots__ = [
    'mip_level',
    'required_gutter_radius',
    'affected_island_offset',
    'affected_island_count',
]
struct_ctex_paint_unsupported_mip_level._fields_ = [
    ('mip_level', uint32_t),
    ('required_gutter_radius', uint32_t),
    ('affected_island_offset', c_size_t),
    ('affected_island_count', c_size_t),
]

ctex_paint_unsupported_mip_level = struct_ctex_paint_unsupported_mip_level

class struct_ctex_paint_island_padding_info(Structure):
    pass

struct_ctex_paint_island_padding_info.__slots__ = [
    'size',
    'required_ownership_count',
    'unsupported_mip_level_count',
    'required_affected_island_count',
    'required_pixel_count',
    'padded_texel_count',
    'padding_radius',
]
struct_ctex_paint_island_padding_info._fields_ = [
    ('size', uint32_t),
    ('required_ownership_count', c_size_t),
    ('unsupported_mip_level_count', c_size_t),
    ('required_affected_island_count', c_size_t),
    ('required_pixel_count', c_size_t),
    ('padded_texel_count', c_size_t),
    ('padding_radius', uint32_t),
]

ctex_paint_island_padding_info = struct_ctex_paint_island_padding_info

class struct_ctex_paint_surface_map_request(Structure):
    pass

struct_ctex_paint_surface_map_request.__slots__ = [
    'size',
    'partition_index',
    'uv_set',
    'width',
    'height',
    'tile_origin',
]
struct_ctex_paint_surface_map_request._fields_ = [
    ('size', uint32_t),
    ('partition_index', c_size_t),
    ('uv_set', String),
    ('width', uint32_t),
    ('height', uint32_t),
    ('tile_origin', ctex_vec2d),
]

ctex_paint_surface_map_request = struct_ctex_paint_surface_map_request

class struct_ctex_paint_surface_texel(Structure):
    pass

struct_ctex_paint_surface_texel.__slots__ = [
    'position',
    'normal',
    'geometric_normal',
    'uv',
    'triangle',
]
struct_ctex_paint_surface_texel._fields_ = [
    ('position', ctex_vec3d),
    ('normal', ctex_vec3d),
    ('geometric_normal', ctex_vec3d),
    ('uv', ctex_vec2d),
    ('triangle', uint32_t),
]

ctex_paint_surface_texel = struct_ctex_paint_surface_texel

class struct_ctex_paint_surface_map_info(Structure):
    pass

struct_ctex_paint_surface_map_info.__slots__ = [
    'size',
    'cache_hit',
    'mesh_revision',
    'required_texture_set_id_size',
    'required_uv_set_size',
    'required_texel_count',
]
struct_ctex_paint_surface_map_info._fields_ = [
    ('size', uint32_t),
    ('cache_hit', uint32_t),
    ('mesh_revision', uint64_t),
    ('required_texture_set_id_size', c_size_t),
    ('required_uv_set_size', c_size_t),
    ('required_texel_count', c_size_t),
]

ctex_paint_surface_map_info = struct_ctex_paint_surface_map_info

class struct_ctex_paint_surface_map_buffers(Structure):
    pass

struct_ctex_paint_surface_map_buffers.__slots__ = [
    'size',
    'texture_set_id',
    'texture_set_id_size',
    'uv_set',
    'uv_set_size',
    'surface_texels',
    'surface_texel_capacity',
    'coverage',
    'coverage_capacity',
    'triangle_identity',
    'triangle_identity_capacity',
    'uv_island_identity',
    'uv_island_identity_capacity',
]
struct_ctex_paint_surface_map_buffers._fields_ = [
    ('size', uint32_t),
    ('texture_set_id', String),
    ('texture_set_id_size', c_size_t),
    ('uv_set', String),
    ('uv_set_size', c_size_t),
    ('surface_texels', POINTER(ctex_paint_surface_texel)),
    ('surface_texel_capacity', c_size_t),
    ('coverage', POINTER(uint8_t)),
    ('coverage_capacity', c_size_t),
    ('triangle_identity', POINTER(uint32_t)),
    ('triangle_identity_capacity', c_size_t),
    ('uv_island_identity', POINTER(uint32_t)),
    ('uv_island_identity_capacity', c_size_t),
]

ctex_paint_surface_map_buffers = struct_ctex_paint_surface_map_buffers

class struct_ctex_paint_surface_map_statistics(Structure):
    pass

struct_ctex_paint_surface_map_statistics.__slots__ = [
    'size',
    'entries',
    'hits',
    'misses',
    'invalidated_entries',
]
struct_ctex_paint_surface_map_statistics._fields_ = [
    ('size', uint32_t),
    ('entries', c_size_t),
    ('hits', c_size_t),
    ('misses', c_size_t),
    ('invalidated_entries', c_size_t),
]

ctex_paint_surface_map_statistics = struct_ctex_paint_surface_map_statistics

class struct_ctex_paint_deposition_descriptor(Structure):
    pass

struct_ctex_paint_deposition_descriptor.__slots__ = [
    'size',
    'mode',
    'alpha_discard_format',
    'has_custom_alpha_discard_threshold',
    'custom_alpha_discard_threshold',
    'masks',
]
struct_ctex_paint_deposition_descriptor._fields_ = [
    ('size', uint32_t),
    ('mode', uint32_t),
    ('alpha_discard_format', uint32_t),
    ('has_custom_alpha_discard_threshold', uint32_t),
    ('custom_alpha_discard_threshold', c_double),
    ('masks', POINTER(ctex_paint_mask_inputs_descriptor)),
]

ctex_paint_deposition_descriptor = struct_ctex_paint_deposition_descriptor

class struct_ctex_paint_deposition_info(Structure):
    pass

struct_ctex_paint_deposition_info.__slots__ = [
    'size',
    'mode',
    'applied_stamp_count',
    'alpha_discard_threshold',
    'alpha_discard_threshold_clamped',
]
struct_ctex_paint_deposition_info._fields_ = [
    ('size', uint32_t),
    ('mode', uint32_t),
    ('applied_stamp_count', c_size_t),
    ('alpha_discard_threshold', c_double),
    ('alpha_discard_threshold_clamped', uint32_t),
]

ctex_paint_deposition_info = struct_ctex_paint_deposition_info

class struct_ctex_paint_deposition_sample(Structure):
    pass

struct_ctex_paint_deposition_sample.__slots__ = [
    'non_building_coverage',
    'build_up_deposition',
    'strength',
    'retained_strength',
    'write',
]
struct_ctex_paint_deposition_sample._fields_ = [
    ('non_building_coverage', c_double),
    ('build_up_deposition', c_double),
    ('strength', c_double),
    ('retained_strength', c_double),
    ('write', uint32_t),
]

ctex_paint_deposition_sample = struct_ctex_paint_deposition_sample

class struct_ctex_paint_blend_descriptor(Structure):
    pass

struct_ctex_paint_blend_descriptor.__slots__ = [
    'size',
    'width',
    'height',
    'blend_mode',
    'stroke_start_snapshot',
    'paint',
    'deposition',
    'pixel_count',
]
struct_ctex_paint_blend_descriptor._fields_ = [
    ('size', uint32_t),
    ('width', uint32_t),
    ('height', uint32_t),
    ('blend_mode', String),
    ('stroke_start_snapshot', POINTER(ctex_vec4f)),
    ('paint', POINTER(ctex_vec4f)),
    ('deposition', POINTER(ctex_paint_deposition_sample)),
    ('pixel_count', c_size_t),
]

ctex_paint_blend_descriptor = struct_ctex_paint_blend_descriptor

class struct_ctex_paint_tool_channel_descriptor(Structure):
    pass

struct_ctex_paint_tool_channel_descriptor.__slots__ = [
    'size',
    'semantic_id',
    'component_count',
    'pixels',
    'pixel_count',
]
struct_ctex_paint_tool_channel_descriptor._fields_ = [
    ('size', uint32_t),
    ('semantic_id', String),
    ('component_count', uint32_t),
    ('pixels', POINTER(ctex_vec4f)),
    ('pixel_count', c_size_t),
]

ctex_paint_tool_channel_descriptor = struct_ctex_paint_tool_channel_descriptor

class struct_ctex_paint_tool_channel_output(Structure):
    pass

struct_ctex_paint_tool_channel_output.__slots__ = [
    'size',
    'pixels',
    'pixel_capacity',
]
struct_ctex_paint_tool_channel_output._fields_ = [
    ('size', uint32_t),
    ('pixels', POINTER(ctex_vec4f)),
    ('pixel_capacity', c_size_t),
]

ctex_paint_tool_channel_output = struct_ctex_paint_tool_channel_output

class struct_ctex_paint_brush_descriptor(Structure):
    pass

struct_ctex_paint_brush_descriptor.__slots__ = [
    'size',
    'width',
    'height',
    'enabled_layer_snapshot',
    'enabled_layer_channel_count',
    'material',
    'material_channel_count',
    'deposition',
    'deposition_count',
    'blend_mode',
]
struct_ctex_paint_brush_descriptor._fields_ = [
    ('size', uint32_t),
    ('width', uint32_t),
    ('height', uint32_t),
    ('enabled_layer_snapshot', POINTER(ctex_paint_tool_channel_descriptor)),
    ('enabled_layer_channel_count', c_size_t),
    ('material', POINTER(ctex_paint_tool_channel_descriptor)),
    ('material_channel_count', c_size_t),
    ('deposition', POINTER(ctex_paint_deposition_sample)),
    ('deposition_count', c_size_t),
    ('blend_mode', String),
]

ctex_paint_brush_descriptor = struct_ctex_paint_brush_descriptor

class struct_ctex_paint_brush_info(Structure):
    pass

struct_ctex_paint_brush_info.__slots__ = [
    'size',
    'applied_channel_count',
    'required_pixels_per_channel',
]
struct_ctex_paint_brush_info._fields_ = [
    ('size', uint32_t),
    ('applied_channel_count', c_size_t),
    ('required_pixels_per_channel', c_size_t),
]

ctex_paint_brush_info = struct_ctex_paint_brush_info
enum_ctex_paint_eraser_target = c_int
CTEX_PAINT_ERASER_TARGET_LAYER_OPACITY = 0
CTEX_PAINT_ERASER_TARGET_MASK = 1
ctex_paint_eraser_target = enum_ctex_paint_eraser_target

class struct_ctex_paint_eraser_descriptor(Structure):
    pass

struct_ctex_paint_eraser_descriptor.__slots__ = [
    'size',
    'width',
    'height',
    'target',
    'stroke_start_values',
    'value_count',
    'deposition',
    'deposition_count',
]
struct_ctex_paint_eraser_descriptor._fields_ = [
    ('size', uint32_t),
    ('width', uint32_t),
    ('height', uint32_t),
    ('target', uint32_t),
    ('stroke_start_values', POINTER(c_double)),
    ('value_count', c_size_t),
    ('deposition', POINTER(ctex_paint_deposition_sample)),
    ('deposition_count', c_size_t),
]

ctex_paint_eraser_descriptor = struct_ctex_paint_eraser_descriptor

class struct_ctex_paint_eraser_info(Structure):
    pass

struct_ctex_paint_eraser_info.__slots__ = [
    'size',
    'target',
    'required_value_count',
]
struct_ctex_paint_eraser_info._fields_ = [
    ('size', uint32_t),
    ('target', uint32_t),
    ('required_value_count', c_size_t),
]

ctex_paint_eraser_info = struct_ctex_paint_eraser_info
enum_ctex_paint_fill_scope = c_int
CTEX_PAINT_FILL_WHOLE_SET = 0
CTEX_PAINT_FILL_TRIANGLE = 1
CTEX_PAINT_FILL_CONNECTED_BY_ANGLE = 2
CTEX_PAINT_FILL_UV_ISLAND = 3
CTEX_PAINT_FILL_UV_TILE = 4
CTEX_PAINT_FILL_SELECTION = 5
ctex_paint_fill_scope = enum_ctex_paint_fill_scope

class struct_ctex_paint_fill_triangle_topology(Structure):
    pass

struct_ctex_paint_fill_triangle_topology.__slots__ = [
    'triangle_identity',
    'geometric_normal',
    'adjacent_triangles',
    'adjacent_triangle_count',
]
struct_ctex_paint_fill_triangle_topology._fields_ = [
    ('triangle_identity', uint32_t),
    ('geometric_normal', ctex_vec3d),
    ('adjacent_triangles', POINTER(uint32_t)),
    ('adjacent_triangle_count', c_size_t),
]

ctex_paint_fill_triangle_topology = struct_ctex_paint_fill_triangle_topology

class struct_ctex_paint_fill_descriptor(Structure):
    pass

struct_ctex_paint_fill_descriptor.__slots__ = [
    'size',
    'width',
    'height',
    'scope',
    'has_picked_texel',
    'picked_texel',
    'maximum_angle_degrees',
    'surface_texels',
    'surface_texel_count',
    'coverage',
    'coverage_count',
    'triangle_identity',
    'triangle_identity_count',
    'uv_island_identity',
    'uv_island_identity_count',
    'triangle_topology',
    'triangle_topology_count',
    'selection',
    'selection_count',
    'enabled_layer_snapshot',
    'enabled_layer_channel_count',
    'material',
    'material_channel_count',
    'masks',
    'rejection_acceptance',
    'rejection_acceptance_count',
    'blend_mode',
]
struct_ctex_paint_fill_descriptor._fields_ = [
    ('size', uint32_t),
    ('width', uint32_t),
    ('height', uint32_t),
    ('scope', uint32_t),
    ('has_picked_texel', uint32_t),
    ('picked_texel', c_size_t),
    ('maximum_angle_degrees', c_double),
    ('surface_texels', POINTER(ctex_paint_surface_texel)),
    ('surface_texel_count', c_size_t),
    ('coverage', POINTER(uint8_t)),
    ('coverage_count', c_size_t),
    ('triangle_identity', POINTER(uint32_t)),
    ('triangle_identity_count', c_size_t),
    ('uv_island_identity', POINTER(uint32_t)),
    ('uv_island_identity_count', c_size_t),
    ('triangle_topology', POINTER(ctex_paint_fill_triangle_topology)),
    ('triangle_topology_count', c_size_t),
    ('selection', POINTER(c_double)),
    ('selection_count', c_size_t),
    ('enabled_layer_snapshot', POINTER(ctex_paint_tool_channel_descriptor)),
    ('enabled_layer_channel_count', c_size_t),
    ('material', POINTER(ctex_paint_tool_channel_descriptor)),
    ('material_channel_count', c_size_t),
    ('masks', POINTER(ctex_paint_mask_inputs_descriptor)),
    ('rejection_acceptance', POINTER(c_double)),
    ('rejection_acceptance_count', c_size_t),
    ('blend_mode', String),
]

ctex_paint_fill_descriptor = struct_ctex_paint_fill_descriptor

class struct_ctex_paint_fill_info(Structure):
    pass

struct_ctex_paint_fill_info.__slots__ = [
    'size',
    'scope',
    'resolved_maximum_angle_degrees',
    'maximum_angle_clamped',
    'selected_texel_count',
    'selected_triangle_count',
    'applied_channel_count',
    'required_pixels_per_channel',
]
struct_ctex_paint_fill_info._fields_ = [
    ('size', uint32_t),
    ('scope', uint32_t),
    ('resolved_maximum_angle_degrees', c_double),
    ('maximum_angle_clamped', uint32_t),
    ('selected_texel_count', c_size_t),
    ('selected_triangle_count', c_size_t),
    ('applied_channel_count', c_size_t),
    ('required_pixels_per_channel', c_size_t),
]

ctex_paint_fill_info = struct_ctex_paint_fill_info

class struct_ctex_paint_fill_outputs(Structure):
    pass

struct_ctex_paint_fill_outputs.__slots__ = [
    'size',
    'scope_values',
    'scope_value_capacity',
    'selected_triangle_ids',
    'selected_triangle_capacity',
    'channels',
    'channel_count',
]
struct_ctex_paint_fill_outputs._fields_ = [
    ('size', uint32_t),
    ('scope_values', POINTER(c_double)),
    ('scope_value_capacity', c_size_t),
    ('selected_triangle_ids', POINTER(uint32_t)),
    ('selected_triangle_capacity', c_size_t),
    ('channels', POINTER(ctex_paint_tool_channel_output)),
    ('channel_count', c_size_t),
]

ctex_paint_fill_outputs = struct_ctex_paint_fill_outputs
enum_ctex_paint_clone_mode = c_int
CTEX_PAINT_CLONE_ALIGNED = 0
CTEX_PAINT_CLONE_FIXED = 1
ctex_paint_clone_mode = enum_ctex_paint_clone_mode

class struct_ctex_paint_clone_source_descriptor(Structure):
    pass

struct_ctex_paint_clone_source_descriptor.__slots__ = [
    'size',
    'texture_set_id',
    'uv',
]
struct_ctex_paint_clone_source_descriptor._fields_ = [
    ('size', uint32_t),
    ('texture_set_id', String),
    ('uv', ctex_vec2d),
]

ctex_paint_clone_source_descriptor = struct_ctex_paint_clone_source_descriptor

class struct_ctex_paint_clone_descriptor(Structure):
    pass

struct_ctex_paint_clone_descriptor.__slots__ = [
    'size',
    'width',
    'height',
    'mode',
    'destination_texture_set_id',
    'tile_origin',
    'destination_anchor_uv',
    'source',
    'destination_surface_texels',
    'destination_surface_texel_count',
    'destination_coverage',
    'destination_coverage_count',
    'enabled_layer_snapshot',
    'enabled_layer_channel_count',
    'source_snapshot',
    'source_channel_count',
    'deposition',
    'deposition_count',
    'blend_mode',
]
struct_ctex_paint_clone_descriptor._fields_ = [
    ('size', uint32_t),
    ('width', uint32_t),
    ('height', uint32_t),
    ('mode', uint32_t),
    ('destination_texture_set_id', String),
    ('tile_origin', ctex_vec2d),
    ('destination_anchor_uv', ctex_vec2d),
    ('source', POINTER(ctex_paint_clone_source_descriptor)),
    ('destination_surface_texels', POINTER(ctex_paint_surface_texel)),
    ('destination_surface_texel_count', c_size_t),
    ('destination_coverage', POINTER(uint8_t)),
    ('destination_coverage_count', c_size_t),
    ('enabled_layer_snapshot', POINTER(ctex_paint_tool_channel_descriptor)),
    ('enabled_layer_channel_count', c_size_t),
    ('source_snapshot', POINTER(ctex_paint_tool_channel_descriptor)),
    ('source_channel_count', c_size_t),
    ('deposition', POINTER(ctex_paint_deposition_sample)),
    ('deposition_count', c_size_t),
    ('blend_mode', String),
]

ctex_paint_clone_descriptor = struct_ctex_paint_clone_descriptor

class struct_ctex_paint_clone_info(Structure):
    pass

struct_ctex_paint_clone_info.__slots__ = [
    'size',
    'mode',
    'source_anchor_uv',
    'destination_anchor_uv',
    'required_source_sample_count',
    'applied_channel_count',
    'required_pixels_per_channel',
]
struct_ctex_paint_clone_info._fields_ = [
    ('size', uint32_t),
    ('mode', uint32_t),
    ('source_anchor_uv', ctex_vec2d),
    ('destination_anchor_uv', ctex_vec2d),
    ('required_source_sample_count', c_size_t),
    ('applied_channel_count', c_size_t),
    ('required_pixels_per_channel', c_size_t),
]

ctex_paint_clone_info = struct_ctex_paint_clone_info

class struct_ctex_paint_clone_outputs(Structure):
    pass

struct_ctex_paint_clone_outputs.__slots__ = [
    'size',
    'source_sample_indices',
    'source_sample_capacity',
    'channels',
    'channel_count',
]
struct_ctex_paint_clone_outputs._fields_ = [
    ('size', uint32_t),
    ('source_sample_indices', POINTER(c_size_t)),
    ('source_sample_capacity', c_size_t),
    ('channels', POINTER(ctex_paint_tool_channel_output)),
    ('channel_count', c_size_t),
]

ctex_paint_clone_outputs = struct_ctex_paint_clone_outputs

class struct_ctex_paint_blur_neighborhood_descriptor(Structure):
    pass

struct_ctex_paint_blur_neighborhood_descriptor.__slots__ = [
    'size',
    'output_frame',
    'horizontal_samples',
    'horizontal_sample_count',
    'vertical_samples',
    'vertical_sample_count',
]
struct_ctex_paint_blur_neighborhood_descriptor._fields_ = [
    ('size', uint32_t),
    ('output_frame', ctex_stroke_frame),
    ('horizontal_samples', POINTER(ctex_paint_surface_filter_sample)),
    ('horizontal_sample_count', c_size_t),
    ('vertical_samples', POINTER(ctex_paint_surface_filter_sample)),
    ('vertical_sample_count', c_size_t),
]

ctex_paint_blur_neighborhood_descriptor = struct_ctex_paint_blur_neighborhood_descriptor

class struct_ctex_paint_blur_descriptor(Structure):
    pass

struct_ctex_paint_blur_descriptor.__slots__ = [
    'size',
    'width',
    'height',
    'radius',
    'stroke_start_snapshot',
    'channel_count',
    'deposition',
    'deposition_count',
    'blend_mode',
    'neighborhoods',
    'neighborhood_count',
]
struct_ctex_paint_blur_descriptor._fields_ = [
    ('size', uint32_t),
    ('width', uint32_t),
    ('height', uint32_t),
    ('radius', uint32_t),
    ('stroke_start_snapshot', POINTER(ctex_paint_tool_channel_descriptor)),
    ('channel_count', c_size_t),
    ('deposition', POINTER(ctex_paint_deposition_sample)),
    ('deposition_count', c_size_t),
    ('blend_mode', String),
    ('neighborhoods', POINTER(ctex_paint_blur_neighborhood_descriptor)),
    ('neighborhood_count', c_size_t),
]

ctex_paint_blur_descriptor = struct_ctex_paint_blur_descriptor

class struct_ctex_paint_blur_info(Structure):
    pass

struct_ctex_paint_blur_info.__slots__ = [
    'size',
    'resolved_radius',
    'radius_clamped',
    'applied_channel_count',
    'required_pixels_per_channel',
]
struct_ctex_paint_blur_info._fields_ = [
    ('size', uint32_t),
    ('resolved_radius', uint32_t),
    ('radius_clamped', uint32_t),
    ('applied_channel_count', c_size_t),
    ('required_pixels_per_channel', c_size_t),
]

ctex_paint_blur_info = struct_ctex_paint_blur_info

class struct_ctex_paint_smear_mapping_descriptor(Structure):
    pass

struct_ctex_paint_smear_mapping_descriptor.__slots__ = [
    'size',
    'output_frame',
    'upstream_sample',
]
struct_ctex_paint_smear_mapping_descriptor._fields_ = [
    ('size', uint32_t),
    ('output_frame', ctex_stroke_frame),
    ('upstream_sample', ctex_paint_surface_filter_sample),
]

ctex_paint_smear_mapping_descriptor = struct_ctex_paint_smear_mapping_descriptor

class struct_ctex_paint_smear_descriptor(Structure):
    pass

struct_ctex_paint_smear_descriptor.__slots__ = [
    'size',
    'width',
    'height',
    'strength',
    'footprint_radius_x',
    'footprint_radius_y',
    'stroke_start_snapshot',
    'channel_count',
    'deposition',
    'deposition_count',
    'blend_mode',
    'mappings',
    'mapping_count',
]
struct_ctex_paint_smear_descriptor._fields_ = [
    ('size', uint32_t),
    ('width', uint32_t),
    ('height', uint32_t),
    ('strength', c_double),
    ('footprint_radius_x', uint32_t),
    ('footprint_radius_y', uint32_t),
    ('stroke_start_snapshot', POINTER(ctex_paint_tool_channel_descriptor)),
    ('channel_count', c_size_t),
    ('deposition', POINTER(ctex_paint_deposition_sample)),
    ('deposition_count', c_size_t),
    ('blend_mode', String),
    ('mappings', POINTER(ctex_paint_smear_mapping_descriptor)),
    ('mapping_count', c_size_t),
]

ctex_paint_smear_descriptor = struct_ctex_paint_smear_descriptor

class struct_ctex_paint_smear_info(Structure):
    pass

struct_ctex_paint_smear_info.__slots__ = [
    'size',
    'resolved_strength',
    'strength_clamped',
    'resolved_footprint_radius_x',
    'resolved_footprint_radius_y',
    'footprint_radius_x_clamped',
    'footprint_radius_y_clamped',
    'applied_channel_count',
    'required_pixels_per_channel',
]
struct_ctex_paint_smear_info._fields_ = [
    ('size', uint32_t),
    ('resolved_strength', c_double),
    ('strength_clamped', uint32_t),
    ('resolved_footprint_radius_x', uint32_t),
    ('resolved_footprint_radius_y', uint32_t),
    ('footprint_radius_x_clamped', uint32_t),
    ('footprint_radius_y_clamped', uint32_t),
    ('applied_channel_count', c_size_t),
    ('required_pixels_per_channel', c_size_t),
]

ctex_paint_smear_info = struct_ctex_paint_smear_info

class struct_ctex_paint_stencil_descriptor(Structure):
    pass

struct_ctex_paint_stencil_descriptor.__slots__ = [
    'size',
    'width',
    'height',
    'screen_positions',
    'screen_position_count',
    'image_width',
    'image_height',
    'image_opacity',
    'image_opacity_count',
    'position',
    'rotation_radians',
    'scale',
    'inverted',
]
struct_ctex_paint_stencil_descriptor._fields_ = [
    ('size', uint32_t),
    ('width', uint32_t),
    ('height', uint32_t),
    ('screen_positions', POINTER(ctex_vec2d)),
    ('screen_position_count', c_size_t),
    ('image_width', uint32_t),
    ('image_height', uint32_t),
    ('image_opacity', POINTER(c_double)),
    ('image_opacity_count', c_size_t),
    ('position', ctex_vec2d),
    ('rotation_radians', c_double),
    ('scale', ctex_vec2d),
    ('inverted', uint32_t),
]

ctex_paint_stencil_descriptor = struct_ctex_paint_stencil_descriptor

class struct_ctex_paint_stencil_info(Structure):
    pass

struct_ctex_paint_stencil_info.__slots__ = [
    'size',
    'resolved_position',
    'resolved_rotation_radians',
    'resolved_scale',
    'inverted',
    'position_x_clamped',
    'position_y_clamped',
    'rotation_clamped',
    'scale_x_clamped',
    'scale_y_clamped',
    'required_mask_value_count',
]
struct_ctex_paint_stencil_info._fields_ = [
    ('size', uint32_t),
    ('resolved_position', ctex_vec2d),
    ('resolved_rotation_radians', c_double),
    ('resolved_scale', ctex_vec2d),
    ('inverted', uint32_t),
    ('position_x_clamped', uint32_t),
    ('position_y_clamped', uint32_t),
    ('rotation_clamped', uint32_t),
    ('scale_x_clamped', uint32_t),
    ('scale_y_clamped', uint32_t),
    ('required_mask_value_count', c_size_t),
]

ctex_paint_stencil_info = struct_ctex_paint_stencil_info

class struct_ctex_paint_decal_transform(Structure):
    pass

struct_ctex_paint_decal_transform.__slots__ = [
    'rotation_radians',
    'uniform_scale',
    'axis_scale',
]
struct_ctex_paint_decal_transform._fields_ = [
    ('rotation_radians', c_double),
    ('uniform_scale', c_double),
    ('axis_scale', ctex_vec2d),
]

ctex_paint_decal_transform = struct_ctex_paint_decal_transform

class struct_ctex_paint_decal_placement(Structure):
    pass

struct_ctex_paint_decal_placement.__slots__ = [
    'position',
    'surface_normal',
    'transform',
]
struct_ctex_paint_decal_placement._fields_ = [
    ('position', ctex_vec3d),
    ('surface_normal', ctex_vec3d),
    ('transform', ctex_paint_decal_transform),
]

ctex_paint_decal_placement = struct_ctex_paint_decal_placement

class struct_ctex_paint_decal_descriptor(Structure):
    pass

struct_ctex_paint_decal_descriptor.__slots__ = [
    'size',
    'width',
    'height',
    'surface_texels',
    'surface_texel_count',
    'coverage',
    'coverage_count',
    'placement',
    'material_width',
    'material_height',
    'material',
    'material_channel_count',
    'material_opacity',
    'material_opacity_count',
    'enabled_layer_snapshot',
    'enabled_layer_channel_count',
    'masks',
    'rejection_acceptance',
    'rejection_acceptance_count',
    'blend_mode',
]
struct_ctex_paint_decal_descriptor._fields_ = [
    ('size', uint32_t),
    ('width', uint32_t),
    ('height', uint32_t),
    ('surface_texels', POINTER(ctex_paint_surface_texel)),
    ('surface_texel_count', c_size_t),
    ('coverage', POINTER(uint8_t)),
    ('coverage_count', c_size_t),
    ('placement', ctex_paint_decal_placement),
    ('material_width', uint32_t),
    ('material_height', uint32_t),
    ('material', POINTER(ctex_paint_tool_channel_descriptor)),
    ('material_channel_count', c_size_t),
    ('material_opacity', POINTER(c_double)),
    ('material_opacity_count', c_size_t),
    ('enabled_layer_snapshot', POINTER(ctex_paint_tool_channel_descriptor)),
    ('enabled_layer_channel_count', c_size_t),
    ('masks', POINTER(ctex_paint_mask_inputs_descriptor)),
    ('rejection_acceptance', POINTER(c_double)),
    ('rejection_acceptance_count', c_size_t),
    ('blend_mode', String),
]

ctex_paint_decal_descriptor = struct_ctex_paint_decal_descriptor

class struct_ctex_paint_decal_info(Structure):
    pass

struct_ctex_paint_decal_info.__slots__ = [
    'size',
    'resolved_placement',
    'frame_tangent',
    'frame_bitangent',
    'frame_scale',
    'rotation_clamped',
    'uniform_scale_clamped',
    'axis_scale_x_clamped',
    'axis_scale_y_clamped',
    'applied_channel_count',
    'required_pixels_per_channel',
]
struct_ctex_paint_decal_info._fields_ = [
    ('size', uint32_t),
    ('resolved_placement', ctex_paint_decal_placement),
    ('frame_tangent', ctex_vec3d),
    ('frame_bitangent', ctex_vec3d),
    ('frame_scale', ctex_vec2d),
    ('rotation_clamped', uint32_t),
    ('uniform_scale_clamped', uint32_t),
    ('axis_scale_x_clamped', uint32_t),
    ('axis_scale_y_clamped', uint32_t),
    ('applied_channel_count', c_size_t),
    ('required_pixels_per_channel', c_size_t),
]

ctex_paint_decal_info = struct_ctex_paint_decal_info

class struct_ctex_paint_decal_outputs(Structure):
    pass

struct_ctex_paint_decal_outputs.__slots__ = [
    'size',
    'source_sample_indices',
    'source_sample_capacity',
    'strength',
    'strength_capacity',
    'channels',
    'channel_count',
]
struct_ctex_paint_decal_outputs._fields_ = [
    ('size', uint32_t),
    ('source_sample_indices', POINTER(c_size_t)),
    ('source_sample_capacity', c_size_t),
    ('strength', POINTER(c_double)),
    ('strength_capacity', c_size_t),
    ('channels', POINTER(ctex_paint_tool_channel_output)),
    ('channel_count', c_size_t),
]

ctex_paint_decal_outputs = struct_ctex_paint_decal_outputs
enum_ctex_paint_projection_mode = c_int
CTEX_PAINT_PROJECTION_CAMERA = 0
CTEX_PAINT_PROJECTION_PLANAR = 1
CTEX_PAINT_PROJECTION_TRIPLANAR = 2
ctex_paint_projection_mode = enum_ctex_paint_projection_mode

class struct_ctex_paint_projection_descriptor(Structure):
    pass

struct_ctex_paint_projection_descriptor.__slots__ = [
    'size',
    'width',
    'height',
    'surface_texels',
    'surface_texel_count',
    'coverage',
    'coverage_count',
    'mode',
    'camera_view_projection',
    'camera_visible_surface',
    'camera_visible_surface_count',
    'planar_origin',
    'planar_u_axis',
    'planar_v_axis',
    'planar_extent',
    'triplanar_scale',
    'triplanar_offset',
    'material_width',
    'material_height',
    'material',
    'material_channel_count',
    'material_opacity',
    'material_opacity_count',
    'enabled_layer_snapshot',
    'enabled_layer_channel_count',
    'masks',
    'rejection_acceptance',
    'rejection_acceptance_count',
    'blend_mode',
]
struct_ctex_paint_projection_descriptor._fields_ = [
    ('size', uint32_t),
    ('width', uint32_t),
    ('height', uint32_t),
    ('surface_texels', POINTER(ctex_paint_surface_texel)),
    ('surface_texel_count', c_size_t),
    ('coverage', POINTER(uint8_t)),
    ('coverage_count', c_size_t),
    ('mode', uint32_t),
    ('camera_view_projection', c_float * int(16)),
    ('camera_visible_surface', POINTER(c_double)),
    ('camera_visible_surface_count', c_size_t),
    ('planar_origin', ctex_vec3d),
    ('planar_u_axis', ctex_vec3d),
    ('planar_v_axis', ctex_vec3d),
    ('planar_extent', ctex_vec2d),
    ('triplanar_scale', c_double),
    ('triplanar_offset', ctex_vec2d),
    ('material_width', uint32_t),
    ('material_height', uint32_t),
    ('material', POINTER(ctex_paint_tool_channel_descriptor)),
    ('material_channel_count', c_size_t),
    ('material_opacity', POINTER(c_double)),
    ('material_opacity_count', c_size_t),
    ('enabled_layer_snapshot', POINTER(ctex_paint_tool_channel_descriptor)),
    ('enabled_layer_channel_count', c_size_t),
    ('masks', POINTER(ctex_paint_mask_inputs_descriptor)),
    ('rejection_acceptance', POINTER(c_double)),
    ('rejection_acceptance_count', c_size_t),
    ('blend_mode', String),
]

ctex_paint_projection_descriptor = struct_ctex_paint_projection_descriptor

class struct_ctex_paint_projection_sample(Structure):
    pass

struct_ctex_paint_projection_sample.__slots__ = [
    'source_indices',
    'weights',
    'count',
]
struct_ctex_paint_projection_sample._fields_ = [
    ('source_indices', c_size_t * int(3)),
    ('weights', c_double * int(3)),
    ('count', c_size_t),
]

ctex_paint_projection_sample = struct_ctex_paint_projection_sample

class struct_ctex_paint_projection_info(Structure):
    pass

struct_ctex_paint_projection_info.__slots__ = [
    'size',
    'resolved_mode',
    'resolved_planar_extent',
    'resolved_triplanar_scale',
    'resolved_triplanar_offset',
    'planar_extent_x_clamped',
    'planar_extent_y_clamped',
    'triplanar_scale_clamped',
    'triplanar_offset_x_clamped',
    'triplanar_offset_y_clamped',
    'applied_channel_count',
    'required_sample_count',
    'required_pixels_per_channel',
]
struct_ctex_paint_projection_info._fields_ = [
    ('size', uint32_t),
    ('resolved_mode', uint32_t),
    ('resolved_planar_extent', ctex_vec2d),
    ('resolved_triplanar_scale', c_double),
    ('resolved_triplanar_offset', ctex_vec2d),
    ('planar_extent_x_clamped', uint32_t),
    ('planar_extent_y_clamped', uint32_t),
    ('triplanar_scale_clamped', uint32_t),
    ('triplanar_offset_x_clamped', uint32_t),
    ('triplanar_offset_y_clamped', uint32_t),
    ('applied_channel_count', c_size_t),
    ('required_sample_count', c_size_t),
    ('required_pixels_per_channel', c_size_t),
]

ctex_paint_projection_info = struct_ctex_paint_projection_info

class struct_ctex_paint_projection_outputs(Structure):
    pass

struct_ctex_paint_projection_outputs.__slots__ = [
    'size',
    'samples',
    'sample_capacity',
    'strength',
    'strength_capacity',
    'channels',
    'channel_count',
]
struct_ctex_paint_projection_outputs._fields_ = [
    ('size', uint32_t),
    ('samples', POINTER(ctex_paint_projection_sample)),
    ('sample_capacity', c_size_t),
    ('strength', POINTER(c_double)),
    ('strength_capacity', c_size_t),
    ('channels', POINTER(ctex_paint_tool_channel_output)),
    ('channel_count', c_size_t),
]

ctex_paint_projection_outputs = struct_ctex_paint_projection_outputs
enum_ctex_paint_text_alignment = c_int
CTEX_PAINT_TEXT_ALIGN_LEFT = 0
CTEX_PAINT_TEXT_ALIGN_CENTRE = 1
CTEX_PAINT_TEXT_ALIGN_RIGHT = 2
ctex_paint_text_alignment = enum_ctex_paint_text_alignment

class struct_ctex_paint_font_glyph_descriptor(Structure):
    pass

struct_ctex_paint_font_glyph_descriptor.__slots__ = [
    'size',
    'codepoint',
    'width',
    'height',
    'bearing_x',
    'bearing_y',
    'advance',
    'coverage',
    'coverage_count',
]
struct_ctex_paint_font_glyph_descriptor._fields_ = [
    ('size', uint32_t),
    ('codepoint', uint32_t),
    ('width', uint32_t),
    ('height', uint32_t),
    ('bearing_x', c_double),
    ('bearing_y', c_double),
    ('advance', c_double),
    ('coverage', POINTER(c_double)),
    ('coverage_count', c_size_t),
]

ctex_paint_font_glyph_descriptor = struct_ctex_paint_font_glyph_descriptor

class struct_ctex_paint_font_descriptor(Structure):
    pass

struct_ctex_paint_font_descriptor.__slots__ = [
    'size',
    'identity',
    'pixels_per_em',
    'ascent',
    'descent',
    'line_gap',
    'glyphs',
    'glyph_count',
]
struct_ctex_paint_font_descriptor._fields_ = [
    ('size', uint32_t),
    ('identity', String),
    ('pixels_per_em', c_double),
    ('ascent', c_double),
    ('descent', c_double),
    ('line_gap', c_double),
    ('glyphs', POINTER(ctex_paint_font_glyph_descriptor)),
    ('glyph_count', c_size_t),
]

ctex_paint_font_descriptor = struct_ctex_paint_font_descriptor

class struct_ctex_paint_text_material_value(Structure):
    pass

struct_ctex_paint_text_material_value.__slots__ = [
    'size',
    'semantic_id',
    'component_count',
    'value',
]
struct_ctex_paint_text_material_value._fields_ = [
    ('size', uint32_t),
    ('semantic_id', String),
    ('component_count', uint32_t),
    ('value', ctex_vec4f),
]

ctex_paint_text_material_value = struct_ctex_paint_text_material_value

class struct_ctex_paint_text_descriptor(Structure):
    pass

struct_ctex_paint_text_descriptor.__slots__ = [
    'size',
    'width',
    'height',
    'surface_texels',
    'surface_texel_count',
    'coverage',
    'coverage_count',
    'font',
    'utf8',
    'utf8_size',
    'tracking_em',
    'alignment',
    'text_size',
    'placement',
    'material',
    'material_channel_count',
    'enabled_layer_snapshot',
    'enabled_layer_channel_count',
    'masks',
    'rejection_acceptance',
    'rejection_acceptance_count',
    'blend_mode',
]
struct_ctex_paint_text_descriptor._fields_ = [
    ('size', uint32_t),
    ('width', uint32_t),
    ('height', uint32_t),
    ('surface_texels', POINTER(ctex_paint_surface_texel)),
    ('surface_texel_count', c_size_t),
    ('coverage', POINTER(uint8_t)),
    ('coverage_count', c_size_t),
    ('font', POINTER(ctex_paint_font_descriptor)),
    ('utf8', String),
    ('utf8_size', c_size_t),
    ('tracking_em', c_double),
    ('alignment', uint32_t),
    ('text_size', c_double),
    ('placement', ctex_paint_decal_placement),
    ('material', POINTER(ctex_paint_text_material_value)),
    ('material_channel_count', c_size_t),
    ('enabled_layer_snapshot', POINTER(ctex_paint_tool_channel_descriptor)),
    ('enabled_layer_channel_count', c_size_t),
    ('masks', POINTER(ctex_paint_mask_inputs_descriptor)),
    ('rejection_acceptance', POINTER(c_double)),
    ('rejection_acceptance_count', c_size_t),
    ('blend_mode', String),
]

ctex_paint_text_descriptor = struct_ctex_paint_text_descriptor

class struct_ctex_paint_text_info(Structure):
    pass

struct_ctex_paint_text_info.__slots__ = [
    'size',
    'resolved_tracking_em',
    'resolved_alignment',
    'resolved_text_size',
    'tracking_clamped',
    'text_size_clamped',
    'raster_width',
    'raster_height',
    'line_count',
    'width_em',
    'height_em',
    'resolved_placement',
    'frame_tangent',
    'frame_bitangent',
    'frame_scale',
    'rotation_clamped',
    'uniform_scale_clamped',
    'axis_scale_x_clamped',
    'axis_scale_y_clamped',
    'editable_revision',
    'applied_channel_count',
    'required_codepoint_count',
    'required_raster_opacity_count',
    'required_pixels_per_channel',
]
struct_ctex_paint_text_info._fields_ = [
    ('size', uint32_t),
    ('resolved_tracking_em', c_double),
    ('resolved_alignment', uint32_t),
    ('resolved_text_size', c_double),
    ('tracking_clamped', uint32_t),
    ('text_size_clamped', uint32_t),
    ('raster_width', uint32_t),
    ('raster_height', uint32_t),
    ('line_count', c_size_t),
    ('width_em', c_double),
    ('height_em', c_double),
    ('resolved_placement', ctex_paint_decal_placement),
    ('frame_tangent', ctex_vec3d),
    ('frame_bitangent', ctex_vec3d),
    ('frame_scale', ctex_vec2d),
    ('rotation_clamped', uint32_t),
    ('uniform_scale_clamped', uint32_t),
    ('axis_scale_x_clamped', uint32_t),
    ('axis_scale_y_clamped', uint32_t),
    ('editable_revision', uint64_t),
    ('applied_channel_count', c_size_t),
    ('required_codepoint_count', c_size_t),
    ('required_raster_opacity_count', c_size_t),
    ('required_pixels_per_channel', c_size_t),
]

ctex_paint_text_info = struct_ctex_paint_text_info

class struct_ctex_paint_text_outputs(Structure):
    pass

struct_ctex_paint_text_outputs.__slots__ = [
    'size',
    'codepoints',
    'codepoint_capacity',
    'raster_opacity',
    'raster_opacity_capacity',
    'source_sample_indices',
    'source_sample_capacity',
    'strength',
    'strength_capacity',
    'channels',
    'channel_count',
]
struct_ctex_paint_text_outputs._fields_ = [
    ('size', uint32_t),
    ('codepoints', POINTER(uint32_t)),
    ('codepoint_capacity', c_size_t),
    ('raster_opacity', POINTER(c_double)),
    ('raster_opacity_capacity', c_size_t),
    ('source_sample_indices', POINTER(c_size_t)),
    ('source_sample_capacity', c_size_t),
    ('strength', POINTER(c_double)),
    ('strength_capacity', c_size_t),
    ('channels', POINTER(ctex_paint_tool_channel_output)),
    ('channel_count', c_size_t),
]

ctex_paint_text_outputs = struct_ctex_paint_text_outputs

class struct_ctex_paint_particle_settings(Structure):
    pass

struct_ctex_paint_particle_settings.__slots__ = [
    'count',
    'lifetime_seconds',
    'initial_speed',
    'mass',
    'gravity',
    'friction',
    'restitution',
    'randomness',
    'seed',
]
struct_ctex_paint_particle_settings._fields_ = [
    ('count', uint32_t),
    ('lifetime_seconds', c_double),
    ('initial_speed', c_double),
    ('mass', c_double),
    ('gravity', ctex_vec3d),
    ('friction', c_double),
    ('restitution', c_double),
    ('randomness', c_double),
    ('seed', uint64_t),
]

ctex_paint_particle_settings = struct_ctex_paint_particle_settings

class struct_ctex_paint_particle_contact(Structure):
    pass

struct_ctex_paint_particle_contact.__slots__ = [
    'particle_ordinal',
    'collision_ordinal',
    'time_seconds',
    'position',
    'normal',
    'uv',
    'triangle',
    'impact_speed',
    'impulse',
    'strength',
    'texture_set_id_offset',
    'texture_set_id_size',
    'mapped_texel',
]
struct_ctex_paint_particle_contact._fields_ = [
    ('particle_ordinal', uint32_t),
    ('collision_ordinal', uint32_t),
    ('time_seconds', c_double),
    ('position', ctex_vec3d),
    ('normal', ctex_vec3d),
    ('uv', ctex_vec2d),
    ('triangle', uint32_t),
    ('impact_speed', c_double),
    ('impulse', c_double),
    ('strength', c_double),
    ('texture_set_id_offset', c_size_t),
    ('texture_set_id_size', c_size_t),
    ('mapped_texel', c_size_t),
]

ctex_paint_particle_contact = struct_ctex_paint_particle_contact

class struct_ctex_paint_particle_state(Structure):
    pass

struct_ctex_paint_particle_state.__slots__ = [
    'position',
    'velocity',
    'simulated_seconds',
    'collision_count',
    'resting',
]
struct_ctex_paint_particle_state._fields_ = [
    ('position', ctex_vec3d),
    ('velocity', ctex_vec3d),
    ('simulated_seconds', c_double),
    ('collision_count', uint32_t),
    ('resting', uint32_t),
]

ctex_paint_particle_state = struct_ctex_paint_particle_state

class struct_ctex_pick_texture_set_binding_descriptor(Structure):
    pass


class struct_ctex_paint_particle_descriptor(Structure):
    pass

struct_ctex_paint_particle_descriptor.__slots__ = [
    'size',
    'width',
    'height',
    'tile_origin',
    'texture_set_id',
    'mesh_revision',
    'surface_texels',
    'surface_texel_count',
    'coverage',
    'coverage_count',
    'triangle_identity',
    'triangle_identity_count',
    'texture_sets',
    'texture_set_count',
    'emitter_position',
    'emitter_direction',
    'simulation',
    'material',
    'material_channel_count',
    'enabled_layer_snapshot',
    'enabled_layer_channel_count',
    'masks',
    'rejection_acceptance',
    'rejection_acceptance_count',
    'blend_mode',
]
struct_ctex_paint_particle_descriptor._fields_ = [
    ('size', uint32_t),
    ('width', uint32_t),
    ('height', uint32_t),
    ('tile_origin', ctex_vec2d),
    ('texture_set_id', String),
    ('mesh_revision', uint64_t),
    ('surface_texels', POINTER(ctex_paint_surface_texel)),
    ('surface_texel_count', c_size_t),
    ('coverage', POINTER(uint8_t)),
    ('coverage_count', c_size_t),
    ('triangle_identity', POINTER(uint32_t)),
    ('triangle_identity_count', c_size_t),
    ('texture_sets', POINTER(struct_ctex_pick_texture_set_binding_descriptor)),
    ('texture_set_count', c_size_t),
    ('emitter_position', ctex_vec3d),
    ('emitter_direction', ctex_vec3d),
    ('simulation', ctex_paint_particle_settings),
    ('material', POINTER(ctex_paint_tool_channel_descriptor)),
    ('material_channel_count', c_size_t),
    ('enabled_layer_snapshot', POINTER(ctex_paint_tool_channel_descriptor)),
    ('enabled_layer_channel_count', c_size_t),
    ('masks', POINTER(ctex_paint_mask_inputs_descriptor)),
    ('rejection_acceptance', POINTER(c_double)),
    ('rejection_acceptance_count', c_size_t),
    ('blend_mode', String),
]

ctex_paint_particle_descriptor = struct_ctex_paint_particle_descriptor

class struct_ctex_paint_particle_info(Structure):
    pass

struct_ctex_paint_particle_info.__slots__ = [
    'size',
    'resolved_settings',
    'count_clamped',
    'lifetime_clamped',
    'initial_speed_clamped',
    'mass_clamped',
    'gravity_x_clamped',
    'gravity_y_clamped',
    'gravity_z_clamped',
    'friction_clamped',
    'restitution_clamped',
    'randomness_clamped',
    'emitted_count',
    'mapped_contact_count',
    'applied_channel_count',
    'required_contact_count',
    'required_final_state_count',
    'required_texture_set_id_size',
    'required_pixels_per_channel',
]
struct_ctex_paint_particle_info._fields_ = [
    ('size', uint32_t),
    ('resolved_settings', ctex_paint_particle_settings),
    ('count_clamped', uint32_t),
    ('lifetime_clamped', uint32_t),
    ('initial_speed_clamped', uint32_t),
    ('mass_clamped', uint32_t),
    ('gravity_x_clamped', uint32_t),
    ('gravity_y_clamped', uint32_t),
    ('gravity_z_clamped', uint32_t),
    ('friction_clamped', uint32_t),
    ('restitution_clamped', uint32_t),
    ('randomness_clamped', uint32_t),
    ('emitted_count', uint32_t),
    ('mapped_contact_count', c_size_t),
    ('applied_channel_count', c_size_t),
    ('required_contact_count', c_size_t),
    ('required_final_state_count', c_size_t),
    ('required_texture_set_id_size', c_size_t),
    ('required_pixels_per_channel', c_size_t),
]

ctex_paint_particle_info = struct_ctex_paint_particle_info

class struct_ctex_paint_particle_outputs(Structure):
    pass

struct_ctex_paint_particle_outputs.__slots__ = [
    'size',
    'contacts',
    'contact_capacity',
    'final_states',
    'final_state_capacity',
    'texture_set_ids',
    'texture_set_id_size',
    'strength',
    'strength_capacity',
    'channels',
    'channel_count',
]
struct_ctex_paint_particle_outputs._fields_ = [
    ('size', uint32_t),
    ('contacts', POINTER(ctex_paint_particle_contact)),
    ('contact_capacity', c_size_t),
    ('final_states', POINTER(ctex_paint_particle_state)),
    ('final_state_capacity', c_size_t),
    ('texture_set_ids', String),
    ('texture_set_id_size', c_size_t),
    ('strength', POINTER(c_double)),
    ('strength_capacity', c_size_t),
    ('channels', POINTER(ctex_paint_tool_channel_output)),
    ('channel_count', c_size_t),
]

ctex_paint_particle_outputs = struct_ctex_paint_particle_outputs

class struct_ctex_stroke_preset_info(Structure):
    pass

struct_ctex_stroke_preset_info.__slots__ = [
    'size',
    'schema_version',
    'required_name_size',
    'required_tip_resource_identity_size',
    'required_curve_point_count',
]
struct_ctex_stroke_preset_info._fields_ = [
    ('size', uint32_t),
    ('schema_version', uint32_t),
    ('required_name_size', c_size_t),
    ('required_tip_resource_identity_size', c_size_t),
    ('required_curve_point_count', c_size_t),
]

ctex_stroke_preset_info = struct_ctex_stroke_preset_info

class struct_ctex_stroke_preset_buffers_descriptor(Structure):
    pass

struct_ctex_stroke_preset_buffers_descriptor.__slots__ = [
    'size',
    'name_buffer',
    'name_buffer_size',
    'tip_resource_identity_buffer',
    'tip_resource_identity_buffer_size',
    'curve_points',
    'curve_point_capacity',
]
struct_ctex_stroke_preset_buffers_descriptor._fields_ = [
    ('size', uint32_t),
    ('name_buffer', String),
    ('name_buffer_size', c_size_t),
    ('tip_resource_identity_buffer', String),
    ('tip_resource_identity_buffer_size', c_size_t),
    ('curve_points', POINTER(ctex_response_curve_point)),
    ('curve_point_capacity', c_size_t),
]

ctex_stroke_preset_buffers_descriptor = struct_ctex_stroke_preset_buffers_descriptor

class struct_ctex_uv_set_descriptor(Structure):
    pass

struct_ctex_uv_set_descriptor.__slots__ = [
    'size',
    'name',
    'values',
    'value_count',
]
struct_ctex_uv_set_descriptor._fields_ = [
    ('size', uint32_t),
    ('name', String),
    ('values', POINTER(ctex_vec2f)),
    ('value_count', c_size_t),
]

ctex_uv_set_descriptor = struct_ctex_uv_set_descriptor

class struct_ctex_mesh_partition_descriptor(Structure):
    pass

struct_ctex_mesh_partition_descriptor.__slots__ = [
    'size',
    'kind',
    'stable_key',
    'display_name',
]
struct_ctex_mesh_partition_descriptor._fields_ = [
    ('size', uint32_t),
    ('kind', uint32_t),
    ('stable_key', String),
    ('display_name', String),
]

ctex_mesh_partition_descriptor = struct_ctex_mesh_partition_descriptor

class struct_ctex_mesh_descriptor(Structure):
    pass

struct_ctex_mesh_descriptor.__slots__ = [
    'size',
    'positions',
    'position_count',
    'normals',
    'normal_count',
    'vertex_colors',
    'vertex_color_count',
    'triangle_indices',
    'triangle_index_count',
    'uv_sets',
    'uv_set_count',
    'default_uv_set',
    'partitions',
    'partition_count',
    'face_partition_indices',
    'face_partition_index_count',
    'face_material_ids',
    'face_material_id_count',
]
struct_ctex_mesh_descriptor._fields_ = [
    ('size', uint32_t),
    ('positions', POINTER(ctex_vec3f)),
    ('position_count', c_size_t),
    ('normals', POINTER(ctex_vec3f)),
    ('normal_count', c_size_t),
    ('vertex_colors', POINTER(ctex_vec4f)),
    ('vertex_color_count', c_size_t),
    ('triangle_indices', POINTER(uint32_t)),
    ('triangle_index_count', c_size_t),
    ('uv_sets', POINTER(ctex_uv_set_descriptor)),
    ('uv_set_count', c_size_t),
    ('default_uv_set', String),
    ('partitions', POINTER(ctex_mesh_partition_descriptor)),
    ('partition_count', c_size_t),
    ('face_partition_indices', POINTER(uint32_t)),
    ('face_partition_index_count', c_size_t),
    ('face_material_ids', POINTER(uint32_t)),
    ('face_material_id_count', c_size_t),
]

ctex_mesh_descriptor = struct_ctex_mesh_descriptor

class struct_ctex_mesh_info(Structure):
    pass

struct_ctex_mesh_info.__slots__ = [
    'size',
    'vertex_count',
    'triangle_count',
    'uv_set_count',
    'partition_count',
    'has_vertex_colors',
    'revision',
]
struct_ctex_mesh_info._fields_ = [
    ('size', uint32_t),
    ('vertex_count', c_size_t),
    ('triangle_count', c_size_t),
    ('uv_set_count', c_size_t),
    ('partition_count', c_size_t),
    ('has_vertex_colors', uint32_t),
    ('revision', uint64_t),
]

ctex_mesh_info = struct_ctex_mesh_info

class struct_ctex_mesh_uv_overlap_info(Structure):
    pass

struct_ctex_mesh_uv_overlap_info.__slots__ = [
    'size',
    'required_face_count',
    'overlap_pair_count',
    'candidate_pair_count',
]
struct_ctex_mesh_uv_overlap_info._fields_ = [
    ('size', uint32_t),
    ('required_face_count', c_size_t),
    ('overlap_pair_count', c_size_t),
    ('candidate_pair_count', c_size_t),
]

ctex_mesh_uv_overlap_info = struct_ctex_mesh_uv_overlap_info

class struct_ctex_mesh_uv_coverage_info(Structure):
    pass

struct_ctex_mesh_uv_coverage_info.__slots__ = [
    'size',
    'width',
    'height',
    'selected_face_count',
    'covered_texel_count',
    'uncovered_texel_count',
    'tested_texel_count',
    'required_outside_face_count',
    'uncovered_fraction',
]
struct_ctex_mesh_uv_coverage_info._fields_ = [
    ('size', uint32_t),
    ('width', uint32_t),
    ('height', uint32_t),
    ('selected_face_count', c_size_t),
    ('covered_texel_count', c_size_t),
    ('uncovered_texel_count', c_size_t),
    ('tested_texel_count', c_size_t),
    ('required_outside_face_count', c_size_t),
    ('uncovered_fraction', c_double),
]

ctex_mesh_uv_coverage_info = struct_ctex_mesh_uv_coverage_info
enum_ctex_mesh_uv_change = c_int
CTEX_MESH_UV_UNCHANGED = 0
CTEX_MESH_UV_CHANGED = 1
CTEX_MESH_SOURCE_PARTITION_MISSING = 2
CTEX_MESH_REPLACEMENT_PARTITION_MISSING = 3
CTEX_MESH_UV_SET_MISSING = 4
ctex_mesh_uv_change = enum_ctex_mesh_uv_change
enum_ctex_mesh_replacement_policy = c_int
CTEX_MESH_REPLACEMENT_KEEP_TEXELS = 0
CTEX_MESH_REPLACEMENT_REQUEST_REPROJECTION = 1
CTEX_MESH_REPLACEMENT_CLEAR = 2
ctex_mesh_replacement_policy = enum_ctex_mesh_replacement_policy

class struct_ctex_mesh_replacement_entry(Structure):
    pass

struct_ctex_mesh_replacement_entry.__slots__ = [
    'uv_change',
    'source_partition_index',
    'replacement_partition_index',
    'source_face_count',
    'replacement_face_count',
    'texture_set_id_offset',
    'texture_set_id_size',
]
struct_ctex_mesh_replacement_entry._fields_ = [
    ('uv_change', uint32_t),
    ('source_partition_index', uint32_t),
    ('replacement_partition_index', uint32_t),
    ('source_face_count', c_size_t),
    ('replacement_face_count', c_size_t),
    ('texture_set_id_offset', c_size_t),
    ('texture_set_id_size', c_size_t),
]

ctex_mesh_replacement_entry = struct_ctex_mesh_replacement_entry

class struct_ctex_mesh_replacement_plan_info(Structure):
    pass

struct_ctex_mesh_replacement_plan_info.__slots__ = [
    'size',
    'source_mesh_revision',
    'texture_set_count',
    'changed_texture_set_count',
    'required_texture_set_id_size',
]
struct_ctex_mesh_replacement_plan_info._fields_ = [
    ('size', uint32_t),
    ('source_mesh_revision', uint64_t),
    ('texture_set_count', c_size_t),
    ('changed_texture_set_count', c_size_t),
    ('required_texture_set_id_size', c_size_t),
]

ctex_mesh_replacement_plan_info = struct_ctex_mesh_replacement_plan_info

class struct_ctex_mesh_replacement_decision(Structure):
    pass

struct_ctex_mesh_replacement_decision.__slots__ = [
    'size',
    'texture_set_id',
    'policy',
]
struct_ctex_mesh_replacement_decision._fields_ = [
    ('size', uint32_t),
    ('texture_set_id', String),
    ('policy', uint32_t),
]

ctex_mesh_replacement_decision = struct_ctex_mesh_replacement_decision

class struct_ctex_mesh_replacement_apply_info(Structure):
    pass

struct_ctex_mesh_replacement_apply_info.__slots__ = [
    'size',
    'replacement_applied',
    'kept_texture_set_count',
    'cleared_texture_set_count',
    'reprojection_pending_texture_set_count',
    'replacement_mesh_revision',
]
struct_ctex_mesh_replacement_apply_info._fields_ = [
    ('size', uint32_t),
    ('replacement_applied', uint32_t),
    ('kept_texture_set_count', c_size_t),
    ('cleared_texture_set_count', c_size_t),
    ('reprojection_pending_texture_set_count', c_size_t),
    ('replacement_mesh_revision', uint64_t),
]

ctex_mesh_replacement_apply_info = struct_ctex_mesh_replacement_apply_info
enum_ctex_mesh_reprojection_hole_policy = c_int
CTEX_MESH_REPROJECTION_RETAIN_TARGET = 0
CTEX_MESH_REPROJECTION_CHANNEL_DEFAULT = 1
ctex_mesh_reprojection_hole_policy = enum_ctex_mesh_reprojection_hole_policy
enum_ctex_mesh_reprojection_ambiguity_policy = c_int
CTEX_MESH_REPROJECTION_REFUSE_AMBIGUITY = 0
CTEX_MESH_REPROJECTION_NEAREST_LOWEST_TRIANGLE = 1
ctex_mesh_reprojection_ambiguity_policy = enum_ctex_mesh_reprojection_ambiguity_policy
ctex_mesh_reprojection_cancel_callback = CFUNCTYPE(UNCHECKED(uint32_t), POINTER(None))
ctex_mesh_reprojection_progress_callback = CFUNCTYPE(UNCHECKED(None), c_size_t, POINTER(None))

class struct_ctex_mesh_reprojection_descriptor(Structure):
    pass

struct_ctex_mesh_reprojection_descriptor.__slots__ = [
    'size',
    'maximum_distance',
    'maximum_normal_angle_radians',
    'require_visibility',
    'visibility_epsilon',
    'ambiguity_distance_epsilon',
    'maximum_work_items',
    'progress_interval',
    'user_data',
    'is_cancelled',
    'report_progress',
]
struct_ctex_mesh_reprojection_descriptor._fields_ = [
    ('size', uint32_t),
    ('maximum_distance', c_double),
    ('maximum_normal_angle_radians', c_double),
    ('require_visibility', uint32_t),
    ('visibility_epsilon', c_double),
    ('ambiguity_distance_epsilon', c_double),
    ('maximum_work_items', c_size_t),
    ('progress_interval', c_size_t),
    ('user_data', POINTER(None)),
    ('is_cancelled', ctex_mesh_reprojection_cancel_callback),
    ('report_progress', ctex_mesh_reprojection_progress_callback),
]

ctex_mesh_reprojection_descriptor = struct_ctex_mesh_reprojection_descriptor

class struct_ctex_mesh_reprojection_preflight_info(Structure):
    pass

struct_ctex_mesh_reprojection_preflight_info.__slots__ = [
    'size',
    'source_mesh_revision',
    'replacement_mesh_revision',
    'mapped_texel_count',
    'unmapped_texel_count',
    'ambiguous_texel_count',
    'affected_entry_count',
    'tested_candidate_count',
    'required_mapping_json_size',
]
struct_ctex_mesh_reprojection_preflight_info._fields_ = [
    ('size', uint32_t),
    ('source_mesh_revision', uint64_t),
    ('replacement_mesh_revision', uint64_t),
    ('mapped_texel_count', c_size_t),
    ('unmapped_texel_count', c_size_t),
    ('ambiguous_texel_count', c_size_t),
    ('affected_entry_count', c_size_t),
    ('tested_candidate_count', c_size_t),
    ('required_mapping_json_size', c_size_t),
]

ctex_mesh_reprojection_preflight_info = struct_ctex_mesh_reprojection_preflight_info

class struct_ctex_mesh_reprojection_commit_info(Structure):
    pass

struct_ctex_mesh_reprojection_commit_info.__slots__ = [
    'size',
    'replacement_mesh_revision',
    'reprojected_texel_count',
    'retained_hole_count',
    'defaulted_hole_count',
    'resolved_ambiguity_count',
    'transformed_tangent_normal_count',
    'reprojected_entry_count',
]
struct_ctex_mesh_reprojection_commit_info._fields_ = [
    ('size', uint32_t),
    ('replacement_mesh_revision', uint64_t),
    ('reprojected_texel_count', c_size_t),
    ('retained_hole_count', c_size_t),
    ('defaulted_hole_count', c_size_t),
    ('resolved_ambiguity_count', c_size_t),
    ('transformed_tangent_normal_count', c_size_t),
    ('reprojected_entry_count', c_size_t),
]

ctex_mesh_reprojection_commit_info = struct_ctex_mesh_reprojection_commit_info
enum_ctex_pick_occlusion_policy = c_int
CTEX_PICK_OCCLUSION_NEAREST = 0
CTEX_PICK_OCCLUSION_ALL_HITS = 1
ctex_pick_occlusion_policy = enum_ctex_pick_occlusion_policy
enum_ctex_pick_backface_policy = c_int
CTEX_PICK_BACKFACE_ACCEPT = 0
CTEX_PICK_BACKFACE_REJECT = 1
ctex_pick_backface_policy = enum_ctex_pick_backface_policy
enum_ctex_pick_projection_kind = c_int
CTEX_PICK_PROJECTION_PERSPECTIVE = 0
CTEX_PICK_PROJECTION_ORTHOGRAPHIC = 1
ctex_pick_projection_kind = enum_ctex_pick_projection_kind
enum_ctex_pick_batch_status = c_int
CTEX_PICK_BATCH_COMPLETE = 0
CTEX_PICK_BATCH_CANCELLED = 1
CTEX_PICK_BATCH_MEMORY_CEILING_EXCEEDED = 2
ctex_pick_batch_status = enum_ctex_pick_batch_status

class struct_ctex_pick_ray(Structure):
    pass

struct_ctex_pick_ray.__slots__ = [
    'origin',
    'direction',
]
struct_ctex_pick_ray._fields_ = [
    ('origin', ctex_vec3f),
    ('direction', ctex_vec3f),
]

ctex_pick_ray = struct_ctex_pick_ray
struct_ctex_pick_texture_set_binding_descriptor.__slots__ = [
    'size',
    'partition_index',
    'uv_set',
]
struct_ctex_pick_texture_set_binding_descriptor._fields_ = [
    ('size', uint32_t),
    ('partition_index', uint32_t),
    ('uv_set', String),
]

ctex_pick_texture_set_binding_descriptor = struct_ctex_pick_texture_set_binding_descriptor

class struct_ctex_pick_options_descriptor(Structure):
    pass

struct_ctex_pick_options_descriptor.__slots__ = [
    'size',
    'maximum_distance',
    'occlusion_policy',
    'backface_policy',
]
struct_ctex_pick_options_descriptor._fields_ = [
    ('size', uint32_t),
    ('maximum_distance', c_float),
    ('occlusion_policy', uint32_t),
    ('backface_policy', uint32_t),
]

ctex_pick_options_descriptor = struct_ctex_pick_options_descriptor

class struct_ctex_pick_screen_view_descriptor(Structure):
    pass

struct_ctex_pick_screen_view_descriptor.__slots__ = [
    'size',
    'viewport_width',
    'viewport_height',
    'view',
    'projection',
]
struct_ctex_pick_screen_view_descriptor._fields_ = [
    ('size', uint32_t),
    ('viewport_width', uint32_t),
    ('viewport_height', uint32_t),
    ('view', c_float * int(16)),
    ('projection', c_float * int(16)),
]

ctex_pick_screen_view_descriptor = struct_ctex_pick_screen_view_descriptor

class struct_ctex_pick_hit(Structure):
    pass

struct_ctex_pick_hit.__slots__ = [
    'has_hit',
    'position',
    'interpolated_normal',
    'geometric_normal',
    'uv',
    'udim_u',
    'udim_v',
    'udim_number',
    'triangle_index',
    'barycentric',
    'material_id',
    'distance',
    'texture_set_id_offset',
    'texture_set_id_size',
]
struct_ctex_pick_hit._fields_ = [
    ('has_hit', uint32_t),
    ('position', ctex_vec3f),
    ('interpolated_normal', ctex_vec3f),
    ('geometric_normal', ctex_vec3f),
    ('uv', ctex_vec2f),
    ('udim_u', c_int32),
    ('udim_v', c_int32),
    ('udim_number', c_int64),
    ('triangle_index', uint32_t),
    ('barycentric', ctex_vec3f),
    ('material_id', uint32_t),
    ('distance', c_float),
    ('texture_set_id_offset', c_size_t),
    ('texture_set_id_size', c_size_t),
]

ctex_pick_hit = struct_ctex_pick_hit

class struct_ctex_pick_index_info(Structure):
    pass

struct_ctex_pick_index_info.__slots__ = [
    'size',
    'mesh_revision',
    'build_count',
    'node_count',
    'triangle_count',
]
struct_ctex_pick_index_info._fields_ = [
    ('size', uint32_t),
    ('mesh_revision', uint64_t),
    ('build_count', c_size_t),
    ('node_count', c_size_t),
    ('triangle_count', c_size_t),
]

ctex_pick_index_info = struct_ctex_pick_index_info

class struct_ctex_pick_query_info(Structure):
    pass

struct_ctex_pick_query_info.__slots__ = [
    'size',
    'result_count',
    'required_texture_set_id_size',
    'visited_nodes',
    'tested_leaf_triangles',
    'index_build_count',
    'mesh_revision',
]
struct_ctex_pick_query_info._fields_ = [
    ('size', uint32_t),
    ('result_count', c_size_t),
    ('required_texture_set_id_size', c_size_t),
    ('visited_nodes', c_size_t),
    ('tested_leaf_triangles', c_size_t),
    ('index_build_count', c_size_t),
    ('mesh_revision', uint64_t),
]

ctex_pick_query_info = struct_ctex_pick_query_info

class struct_ctex_paint_picker_texture_view_descriptor(Structure):
    pass

struct_ctex_paint_picker_texture_view_descriptor.__slots__ = [
    'size',
    'texture_set_id',
    'tile_origin',
    'width',
    'height',
    'enabled_channels',
    'enabled_channel_count',
    'material_identities',
    'material_identity_count',
]
struct_ctex_paint_picker_texture_view_descriptor._fields_ = [
    ('size', uint32_t),
    ('texture_set_id', String),
    ('tile_origin', ctex_vec2d),
    ('width', uint32_t),
    ('height', uint32_t),
    ('enabled_channels', POINTER(ctex_paint_tool_channel_descriptor)),
    ('enabled_channel_count', c_size_t),
    ('material_identities', POINTER(POINTER(c_char))),
    ('material_identity_count', c_size_t),
]

ctex_paint_picker_texture_view_descriptor = struct_ctex_paint_picker_texture_view_descriptor

class struct_ctex_paint_picker_descriptor(Structure):
    pass

struct_ctex_paint_picker_descriptor.__slots__ = [
    'size',
    'hit',
    'hit_texture_set_id',
    'texture_views',
    'texture_view_count',
]
struct_ctex_paint_picker_descriptor._fields_ = [
    ('size', uint32_t),
    ('hit', ctex_pick_hit),
    ('hit_texture_set_id', String),
    ('texture_views', POINTER(ctex_paint_picker_texture_view_descriptor)),
    ('texture_view_count', c_size_t),
]

ctex_paint_picker_descriptor = struct_ctex_paint_picker_descriptor

class struct_ctex_paint_picker_channel_value(Structure):
    pass

struct_ctex_paint_picker_channel_value.__slots__ = [
    'component_count',
    'value',
    'semantic_id_offset',
    'semantic_id_size',
]
struct_ctex_paint_picker_channel_value._fields_ = [
    ('component_count', uint32_t),
    ('value', ctex_vec4f),
    ('semantic_id_offset', c_size_t),
    ('semantic_id_size', c_size_t),
]

ctex_paint_picker_channel_value = struct_ctex_paint_picker_channel_value

class struct_ctex_paint_picker_info(Structure):
    pass

struct_ctex_paint_picker_info.__slots__ = [
    'size',
    'tile_origin',
    'uv',
    'texel',
    'has_material_identity',
    'texture_set_id_offset',
    'texture_set_id_size',
    'material_identity_offset',
    'material_identity_size',
    'required_channel_count',
    'required_string_size',
]
struct_ctex_paint_picker_info._fields_ = [
    ('size', uint32_t),
    ('tile_origin', ctex_vec2d),
    ('uv', ctex_vec2d),
    ('texel', c_size_t),
    ('has_material_identity', uint32_t),
    ('texture_set_id_offset', c_size_t),
    ('texture_set_id_size', c_size_t),
    ('material_identity_offset', c_size_t),
    ('material_identity_size', c_size_t),
    ('required_channel_count', c_size_t),
    ('required_string_size', c_size_t),
]

ctex_paint_picker_info = struct_ctex_paint_picker_info
enum_anon_2 = c_int
CTEX_PAINT_COLOUR_ID_SELECTION_EMPTY = 0
CTEX_PAINT_COLOUR_ID_SELECTION_MATCHED = 1

class struct_ctex_paint_colour_id_descriptor(Structure):
    pass

struct_ctex_paint_colour_id_descriptor.__slots__ = [
    'size',
    'width',
    'height',
    'pixels',
    'pixel_count',
    'picked_colour',
    'tolerance',
]
struct_ctex_paint_colour_id_descriptor._fields_ = [
    ('size', uint32_t),
    ('width', uint32_t),
    ('height', uint32_t),
    ('pixels', POINTER(ctex_vec4f)),
    ('pixel_count', c_size_t),
    ('picked_colour', ctex_vec4f),
    ('tolerance', c_double),
]

ctex_paint_colour_id_descriptor = struct_ctex_paint_colour_id_descriptor

class struct_ctex_paint_colour_id_info(Structure):
    pass

struct_ctex_paint_colour_id_info.__slots__ = [
    'size',
    'resolved_tolerance',
    'tolerance_clamped',
    'status',
    'selected_texel_count',
    'required_value_count',
]
struct_ctex_paint_colour_id_info._fields_ = [
    ('size', uint32_t),
    ('resolved_tolerance', c_double),
    ('tolerance_clamped', uint32_t),
    ('status', uint32_t),
    ('selected_texel_count', c_size_t),
    ('required_value_count', c_size_t),
]

ctex_paint_colour_id_info = struct_ctex_paint_colour_id_info
enum_ctex_paint_parameter_context = c_int
CTEX_PAINT_PARAMETER_CONTEXT_GENERAL = 0
CTEX_PAINT_PARAMETER_CONTEXT_TAPER_DISABLED = 1
CTEX_PAINT_PARAMETER_CONTEXT_TAPER_STAMP_COUNT = 2
CTEX_PAINT_PARAMETER_CONTEXT_TAPER_DISTANCE = 3
CTEX_PAINT_PARAMETER_CONTEXT_ALPHA_UNORM8 = 4
CTEX_PAINT_PARAMETER_CONTEXT_ALPHA_HIGH_PRECISION = 5
ctex_paint_parameter_context = enum_ctex_paint_parameter_context
enum_ctex_paint_parameter_value_kind = c_int
CTEX_PAINT_PARAMETER_CONTINUOUS = 0
CTEX_PAINT_PARAMETER_INTEGER = 1
ctex_paint_parameter_value_kind = enum_ctex_paint_parameter_value_kind

class struct_ctex_paint_parameter_descriptor(Structure):
    pass

struct_ctex_paint_parameter_descriptor.__slots__ = [
    'size',
    'context',
    'value_kind',
    'default_value',
    'minimum',
    'maximum',
    'name_offset',
    'name_size',
]
struct_ctex_paint_parameter_descriptor._fields_ = [
    ('size', uint32_t),
    ('context', uint32_t),
    ('value_kind', uint32_t),
    ('default_value', c_double),
    ('minimum', c_double),
    ('maximum', c_double),
    ('name_offset', c_size_t),
    ('name_size', c_size_t),
]

ctex_paint_parameter_descriptor = struct_ctex_paint_parameter_descriptor

class struct_ctex_paint_parameter_catalogue_info(Structure):
    pass

struct_ctex_paint_parameter_catalogue_info.__slots__ = [
    'size',
    'required_parameter_count',
    'required_name_size',
]
struct_ctex_paint_parameter_catalogue_info._fields_ = [
    ('size', uint32_t),
    ('required_parameter_count', c_size_t),
    ('required_name_size', c_size_t),
]

ctex_paint_parameter_catalogue_info = struct_ctex_paint_parameter_catalogue_info

class struct_ctex_paint_parameter_validation_info(Structure):
    pass

struct_ctex_paint_parameter_validation_info.__slots__ = [
    'size',
    'supplied',
    'resolved',
    'clamped',
]
struct_ctex_paint_parameter_validation_info._fields_ = [
    ('size', uint32_t),
    ('supplied', c_double),
    ('resolved', c_double),
    ('clamped', uint32_t),
]

ctex_paint_parameter_validation_info = struct_ctex_paint_parameter_validation_info
enum_ctex_paint_selection_kind = c_int
CTEX_PAINT_SELECTION_SCREEN_RECTANGLE = 0
CTEX_PAINT_SELECTION_SCREEN_LASSO = 1
CTEX_PAINT_SELECTION_POLYGON_TRIANGLE = 2
CTEX_PAINT_SELECTION_POLYGON_UV_ISLAND = 3
CTEX_PAINT_SELECTION_POLYGON_CONNECTED_BY_ANGLE = 4
ctex_paint_selection_kind = enum_ctex_paint_selection_kind

class struct_ctex_paint_selection_surface_descriptor(Structure):
    pass

struct_ctex_paint_selection_surface_descriptor.__slots__ = [
    'size',
    'width',
    'height',
    'tile_origin',
    'texture_set_id',
    'uv_set',
    'mesh_revision',
    'surface_texels',
    'surface_texel_count',
    'coverage',
    'coverage_count',
    'triangle_identity',
    'triangle_identity_count',
    'uv_island_identity',
    'uv_island_identity_count',
]
struct_ctex_paint_selection_surface_descriptor._fields_ = [
    ('size', uint32_t),
    ('width', uint32_t),
    ('height', uint32_t),
    ('tile_origin', ctex_vec2d),
    ('texture_set_id', String),
    ('uv_set', String),
    ('mesh_revision', uint64_t),
    ('surface_texels', POINTER(ctex_paint_surface_texel)),
    ('surface_texel_count', c_size_t),
    ('coverage', POINTER(uint8_t)),
    ('coverage_count', c_size_t),
    ('triangle_identity', POINTER(uint32_t)),
    ('triangle_identity_count', c_size_t),
    ('uv_island_identity', POINTER(uint32_t)),
    ('uv_island_identity_count', c_size_t),
]

ctex_paint_selection_surface_descriptor = struct_ctex_paint_selection_surface_descriptor

class struct_ctex_paint_screen_selection_descriptor(Structure):
    pass

struct_ctex_paint_screen_selection_descriptor.__slots__ = [
    'size',
    'kind',
    'surface',
    'minimum',
    'maximum',
    'lasso_points',
    'lasso_point_count',
    'view',
]
struct_ctex_paint_screen_selection_descriptor._fields_ = [
    ('size', uint32_t),
    ('kind', uint32_t),
    ('surface', POINTER(ctex_paint_selection_surface_descriptor)),
    ('minimum', ctex_vec2f),
    ('maximum', ctex_vec2f),
    ('lasso_points', POINTER(ctex_vec2f)),
    ('lasso_point_count', c_size_t),
    ('view', ctex_pick_screen_view_descriptor),
]

ctex_paint_screen_selection_descriptor = struct_ctex_paint_screen_selection_descriptor

class struct_ctex_paint_polygon_selection_descriptor(Structure):
    pass

struct_ctex_paint_polygon_selection_descriptor.__slots__ = [
    'size',
    'kind',
    'surface',
    'picked_texel',
    'maximum_angle_degrees',
    'triangle_topology',
    'triangle_topology_count',
]
struct_ctex_paint_polygon_selection_descriptor._fields_ = [
    ('size', uint32_t),
    ('kind', uint32_t),
    ('surface', POINTER(ctex_paint_selection_surface_descriptor)),
    ('picked_texel', c_size_t),
    ('maximum_angle_degrees', c_double),
    ('triangle_topology', POINTER(ctex_paint_fill_triangle_topology)),
    ('triangle_topology_count', c_size_t),
]

ctex_paint_polygon_selection_descriptor = struct_ctex_paint_polygon_selection_descriptor

class struct_ctex_paint_selection_info(Structure):
    pass

struct_ctex_paint_selection_info.__slots__ = [
    'size',
    'kind',
    'resolved_maximum_angle_degrees',
    'maximum_angle_clamped',
    'selected_texel_count',
    'selected_triangle_count',
    'visited_nodes',
    'tested_leaf_triangles',
    'required_value_count',
]
struct_ctex_paint_selection_info._fields_ = [
    ('size', uint32_t),
    ('kind', uint32_t),
    ('resolved_maximum_angle_degrees', c_double),
    ('maximum_angle_clamped', uint32_t),
    ('selected_texel_count', c_size_t),
    ('selected_triangle_count', c_size_t),
    ('visited_nodes', c_size_t),
    ('tested_leaf_triangles', c_size_t),
    ('required_value_count', c_size_t),
]

ctex_paint_selection_info = struct_ctex_paint_selection_info

class struct_ctex_paint_selection_outputs(Structure):
    pass

struct_ctex_paint_selection_outputs.__slots__ = [
    'size',
    'values',
    'value_capacity',
    'selected_triangle_ids',
    'selected_triangle_capacity',
]
struct_ctex_paint_selection_outputs._fields_ = [
    ('size', uint32_t),
    ('values', POINTER(c_double)),
    ('value_capacity', c_size_t),
    ('selected_triangle_ids', POINTER(uint32_t)),
    ('selected_triangle_capacity', c_size_t),
]

ctex_paint_selection_outputs = struct_ctex_paint_selection_outputs
ctex_pick_cancel_callback = CFUNCTYPE(UNCHECKED(uint32_t), POINTER(None))
ctex_pick_progress_callback = CFUNCTYPE(UNCHECKED(None), c_size_t, c_size_t, POINTER(None))

class struct_ctex_pick_batch_control_descriptor(Structure):
    pass

struct_ctex_pick_batch_control_descriptor.__slots__ = [
    'size',
    'memory_ceiling_bytes',
    'progress_interval',
    'user_data',
    'is_cancelled',
    'report_progress',
]
struct_ctex_pick_batch_control_descriptor._fields_ = [
    ('size', uint32_t),
    ('memory_ceiling_bytes', c_size_t),
    ('progress_interval', c_size_t),
    ('user_data', POINTER(None)),
    ('is_cancelled', ctex_pick_cancel_callback),
    ('report_progress', ctex_pick_progress_callback),
]

ctex_pick_batch_control_descriptor = struct_ctex_pick_batch_control_descriptor

class struct_ctex_pick_batch_info(Structure):
    pass

struct_ctex_pick_batch_info.__slots__ = [
    'size',
    'status',
    'processed_rays',
    'required_memory_bytes',
    'required_hit_count',
    'required_texture_set_id_size',
    'visited_nodes',
    'tested_leaf_triangles',
]
struct_ctex_pick_batch_info._fields_ = [
    ('size', uint32_t),
    ('status', uint32_t),
    ('processed_rays', c_size_t),
    ('required_memory_bytes', c_size_t),
    ('required_hit_count', c_size_t),
    ('required_texture_set_id_size', c_size_t),
    ('visited_nodes', c_size_t),
    ('tested_leaf_triangles', c_size_t),
]

ctex_pick_batch_info = struct_ctex_pick_batch_info

class struct_ctex_texture_set_descriptor(Structure):
    pass

struct_ctex_texture_set_descriptor.__slots__ = [
    'size',
    'display_name',
    'partition_kind',
    'partition_key',
    'uv_set',
    'width',
    'height',
    'default_bit_depth',
    'udim_tiling',
]
struct_ctex_texture_set_descriptor._fields_ = [
    ('size', uint32_t),
    ('display_name', String),
    ('partition_kind', uint32_t),
    ('partition_key', String),
    ('uv_set', String),
    ('width', uint32_t),
    ('height', uint32_t),
    ('default_bit_depth', uint8_t),
    ('udim_tiling', uint32_t),
]

ctex_texture_set_descriptor = struct_ctex_texture_set_descriptor

class struct_ctex_udim_pixel_write_descriptor(Structure):
    pass

struct_ctex_udim_pixel_write_descriptor.__slots__ = [
    'size',
    'u',
    'v',
    'pixel',
    'pixel_size',
]
struct_ctex_udim_pixel_write_descriptor._fields_ = [
    ('size', uint32_t),
    ('u', c_double),
    ('v', c_double),
    ('pixel', POINTER(None)),
    ('pixel_size', c_size_t),
]

ctex_udim_pixel_write_descriptor = struct_ctex_udim_pixel_write_descriptor

class struct_ctex_udim_write_info(Structure):
    pass

struct_ctex_udim_write_info.__slots__ = [
    'size',
    'changed_tile_count',
    'allocated_tile_count',
    'changed_pixel_count',
]
struct_ctex_udim_write_info._fields_ = [
    ('size', uint32_t),
    ('changed_tile_count', c_size_t),
    ('allocated_tile_count', c_size_t),
    ('changed_pixel_count', c_size_t),
]

ctex_udim_write_info = struct_ctex_udim_write_info

class struct_ctex_atlas_region_descriptor(Structure):
    pass

struct_ctex_atlas_region_descriptor.__slots__ = [
    'size',
    'texture_set_id',
    'x',
    'y',
    'width',
    'height',
]
struct_ctex_atlas_region_descriptor._fields_ = [
    ('size', uint32_t),
    ('texture_set_id', String),
    ('x', uint32_t),
    ('y', uint32_t),
    ('width', uint32_t),
    ('height', uint32_t),
]

ctex_atlas_region_descriptor = struct_ctex_atlas_region_descriptor

class struct_ctex_atlas_descriptor(Structure):
    pass

struct_ctex_atlas_descriptor.__slots__ = [
    'size',
    'identifier',
    'display_name',
    'width',
    'height',
    'regions',
    'region_count',
]
struct_ctex_atlas_descriptor._fields_ = [
    ('size', uint32_t),
    ('identifier', String),
    ('display_name', String),
    ('width', uint32_t),
    ('height', uint32_t),
    ('regions', POINTER(ctex_atlas_region_descriptor)),
    ('region_count', c_size_t),
]

ctex_atlas_descriptor = struct_ctex_atlas_descriptor

class struct_ctex_atlas_region(Structure):
    pass

struct_ctex_atlas_region.__slots__ = [
    'texture_set_id_offset',
    'texture_set_id_size',
    'x',
    'y',
    'width',
    'height',
]
struct_ctex_atlas_region._fields_ = [
    ('texture_set_id_offset', c_size_t),
    ('texture_set_id_size', c_size_t),
    ('x', uint32_t),
    ('y', uint32_t),
    ('width', uint32_t),
    ('height', uint32_t),
]

ctex_atlas_region = struct_ctex_atlas_region

class struct_ctex_atlas_info(Structure):
    pass

struct_ctex_atlas_info.__slots__ = [
    'size',
    'width',
    'height',
    'region_count',
    'required_display_name_size',
    'required_texture_set_id_size',
]
struct_ctex_atlas_info._fields_ = [
    ('size', uint32_t),
    ('width', uint32_t),
    ('height', uint32_t),
    ('region_count', c_size_t),
    ('required_display_name_size', c_size_t),
    ('required_texture_set_id_size', c_size_t),
]

ctex_atlas_info = struct_ctex_atlas_info
enum_ctex_layer_entry_kind = c_int
CTEX_LAYER_ENTRY_PAINT = 0
CTEX_LAYER_ENTRY_FILL = 1
CTEX_LAYER_ENTRY_GROUP = 2
CTEX_LAYER_ENTRY_MASK = 3
CTEX_LAYER_ENTRY_FILTER = 4
CTEX_LAYER_ENTRY_INSTANCE = 5
CTEX_LAYER_ENTRY_EDITABLE_DECAL = 6
CTEX_LAYER_ENTRY_EDITABLE_TEXT = 7
CTEX_LAYER_ENTRY_SURFACE_PATH = 8
ctex_layer_entry_kind = enum_ctex_layer_entry_kind
enum_ctex_layer_source_deletion_policy = c_int
CTEX_LAYER_SOURCE_DELETION_REFUSE = 0
CTEX_LAYER_SOURCE_DELETION_MAKE_INSTANCES_INDEPENDENT = 1
ctex_layer_source_deletion_policy = enum_ctex_layer_source_deletion_policy

class struct_ctex_layer_channel_descriptor(Structure):
    pass

struct_ctex_layer_channel_descriptor.__slots__ = [
    'size',
    'semantic_id',
    'enabled',
    'opacity',
]
struct_ctex_layer_channel_descriptor._fields_ = [
    ('size', uint32_t),
    ('semantic_id', String),
    ('enabled', uint32_t),
    ('opacity', c_double),
]

ctex_layer_channel_descriptor = struct_ctex_layer_channel_descriptor

class struct_ctex_layer_entry_descriptor(Structure):
    pass

struct_ctex_layer_entry_descriptor.__slots__ = [
    'size',
    'identifier',
    'display_name',
    'kind',
    'parent_identifier',
    'target_identifier',
    'source_identifier',
    'enabled',
    'opacity',
    'blend_mode',
    'channels',
    'channel_count',
]
struct_ctex_layer_entry_descriptor._fields_ = [
    ('size', uint32_t),
    ('identifier', String),
    ('display_name', String),
    ('kind', uint32_t),
    ('parent_identifier', String),
    ('target_identifier', String),
    ('source_identifier', String),
    ('enabled', uint32_t),
    ('opacity', c_double),
    ('blend_mode', String),
    ('channels', POINTER(ctex_layer_channel_descriptor)),
    ('channel_count', c_size_t),
]

ctex_layer_entry_descriptor = struct_ctex_layer_entry_descriptor

class struct_ctex_layer_mask_sample(Structure):
    pass

struct_ctex_layer_mask_sample.__slots__ = [
    'size',
    'mask_identifier',
    'value',
]
struct_ctex_layer_mask_sample._fields_ = [
    ('size', uint32_t),
    ('mask_identifier', String),
    ('value', c_double),
]

ctex_layer_mask_sample = struct_ctex_layer_mask_sample

class struct_ctex_layer_participation_info(Structure):
    pass

struct_ctex_layer_participation_info.__slots__ = [
    'size',
    'participates',
    'effective_opacity',
    'mask_count',
    'required_mask_id_size',
]
struct_ctex_layer_participation_info._fields_ = [
    ('size', uint32_t),
    ('participates', uint32_t),
    ('effective_opacity', c_double),
    ('mask_count', c_size_t),
    ('required_mask_id_size', c_size_t),
]

ctex_layer_participation_info = struct_ctex_layer_participation_info

class struct_ctex_tile_history_target_descriptor(Structure):
    pass

struct_ctex_tile_history_target_descriptor.__slots__ = [
    'size',
    'semantic_id',
    'tile_x',
    'tile_y',
]
struct_ctex_tile_history_target_descriptor._fields_ = [
    ('size', uint32_t),
    ('semantic_id', String),
    ('tile_x', uint32_t),
    ('tile_y', uint32_t),
]

ctex_tile_history_target_descriptor = struct_ctex_tile_history_target_descriptor

class struct_ctex_tile_history_budget_report(Structure):
    pass

struct_ctex_tile_history_budget_report.__slots__ = [
    'size',
    'budget_bytes',
    'retained_bytes',
    'available_bytes',
    'proposed_step_bytes',
    'additional_steps_at_proposed_size',
    'undo_steps',
    'redo_steps',
]
struct_ctex_tile_history_budget_report._fields_ = [
    ('size', uint32_t),
    ('budget_bytes', c_size_t),
    ('retained_bytes', c_size_t),
    ('available_bytes', c_size_t),
    ('proposed_step_bytes', c_size_t),
    ('additional_steps_at_proposed_size', c_size_t),
    ('undo_steps', c_size_t),
    ('redo_steps', c_size_t),
]

ctex_tile_history_budget_report = struct_ctex_tile_history_budget_report

class struct_ctex_tile_history_commit_info(Structure):
    pass

struct_ctex_tile_history_commit_info.__slots__ = [
    'size',
    'committed',
    'tile_count',
    'retained_bytes',
    'layer_stack_changed',
]
struct_ctex_tile_history_commit_info._fields_ = [
    ('size', uint32_t),
    ('committed', uint32_t),
    ('tile_count', c_size_t),
    ('retained_bytes', c_size_t),
    ('layer_stack_changed', uint32_t),
]

ctex_tile_history_commit_info = struct_ctex_tile_history_commit_info

class struct_ctex_tile_history_restore_info(Structure):
    pass

struct_ctex_tile_history_restore_info.__slots__ = [
    'size',
    'tile_count',
    'exchanged_storage_count',
    'copied_pixel_bytes',
    'layer_stack_exchanged',
]
struct_ctex_tile_history_restore_info._fields_ = [
    ('size', uint32_t),
    ('tile_count', c_size_t),
    ('exchanged_storage_count', c_size_t),
    ('copied_pixel_bytes', c_size_t),
    ('layer_stack_exchanged', uint32_t),
]

ctex_tile_history_restore_info = struct_ctex_tile_history_restore_info

class struct_ctex_layer_composite_raster_descriptor(Structure):
    pass

struct_ctex_layer_composite_raster_descriptor.__slots__ = [
    'size',
    'entry_identifier',
    'semantic_id',
    'width',
    'height',
    'pixels',
    'pixel_count',
    'coverage',
    'coverage_count',
]
struct_ctex_layer_composite_raster_descriptor._fields_ = [
    ('size', uint32_t),
    ('entry_identifier', String),
    ('semantic_id', String),
    ('width', uint32_t),
    ('height', uint32_t),
    ('pixels', POINTER(ctex_vec4f)),
    ('pixel_count', c_size_t),
    ('coverage', POINTER(c_float)),
    ('coverage_count', c_size_t),
]

ctex_layer_composite_raster_descriptor = struct_ctex_layer_composite_raster_descriptor

class struct_ctex_layer_composite_mask_descriptor(Structure):
    pass

struct_ctex_layer_composite_mask_descriptor.__slots__ = [
    'size',
    'mask_identifier',
    'width',
    'height',
    'values',
    'value_count',
]
struct_ctex_layer_composite_mask_descriptor._fields_ = [
    ('size', uint32_t),
    ('mask_identifier', String),
    ('width', uint32_t),
    ('height', uint32_t),
    ('values', POINTER(c_double)),
    ('value_count', c_size_t),
]

ctex_layer_composite_mask_descriptor = struct_ctex_layer_composite_mask_descriptor

class struct_ctex_layer_composite_info(Structure):
    pass

struct_ctex_layer_composite_info.__slots__ = [
    'size',
    'width',
    'height',
    'channel_count',
    'required_semantic_id_size',
    'required_pixel_count',
]
struct_ctex_layer_composite_info._fields_ = [
    ('size', uint32_t),
    ('width', uint32_t),
    ('height', uint32_t),
    ('channel_count', c_size_t),
    ('required_semantic_id_size', c_size_t),
    ('required_pixel_count', c_size_t),
]

ctex_layer_composite_info = struct_ctex_layer_composite_info

class struct_ctex_layer_composite_channel_info(Structure):
    pass

struct_ctex_layer_composite_channel_info.__slots__ = [
    'component_count',
    'semantic_id_offset',
    'semantic_id_size',
    'pixel_offset',
    'pixel_count',
]
struct_ctex_layer_composite_channel_info._fields_ = [
    ('component_count', uint32_t),
    ('semantic_id_offset', c_size_t),
    ('semantic_id_size', c_size_t),
    ('pixel_offset', c_size_t),
    ('pixel_count', c_size_t),
]

ctex_layer_composite_channel_info = struct_ctex_layer_composite_channel_info

class struct_ctex_layer_snapshot_info(Structure):
    pass

struct_ctex_layer_snapshot_info.__slots__ = [
    'size',
    'width',
    'height',
    'content_count',
    'mask_count',
    'required_string_size',
    'required_pixel_count',
    'required_coverage_count',
    'required_mask_value_count',
]
struct_ctex_layer_snapshot_info._fields_ = [
    ('size', uint32_t),
    ('width', uint32_t),
    ('height', uint32_t),
    ('content_count', c_size_t),
    ('mask_count', c_size_t),
    ('required_string_size', c_size_t),
    ('required_pixel_count', c_size_t),
    ('required_coverage_count', c_size_t),
    ('required_mask_value_count', c_size_t),
]

ctex_layer_snapshot_info = struct_ctex_layer_snapshot_info

class struct_ctex_layer_snapshot_content_info(Structure):
    pass

struct_ctex_layer_snapshot_content_info.__slots__ = [
    'width',
    'height',
    'entry_identifier_offset',
    'entry_identifier_size',
    'semantic_id_offset',
    'semantic_id_size',
    'pixel_offset',
    'pixel_count',
    'coverage_offset',
    'coverage_count',
]
struct_ctex_layer_snapshot_content_info._fields_ = [
    ('width', uint32_t),
    ('height', uint32_t),
    ('entry_identifier_offset', c_size_t),
    ('entry_identifier_size', c_size_t),
    ('semantic_id_offset', c_size_t),
    ('semantic_id_size', c_size_t),
    ('pixel_offset', c_size_t),
    ('pixel_count', c_size_t),
    ('coverage_offset', c_size_t),
    ('coverage_count', c_size_t),
]

ctex_layer_snapshot_content_info = struct_ctex_layer_snapshot_content_info

class struct_ctex_layer_snapshot_mask_info(Structure):
    pass

struct_ctex_layer_snapshot_mask_info.__slots__ = [
    'width',
    'height',
    'mask_identifier_offset',
    'mask_identifier_size',
    'value_offset',
    'value_count',
]
struct_ctex_layer_snapshot_mask_info._fields_ = [
    ('width', uint32_t),
    ('height', uint32_t),
    ('mask_identifier_offset', c_size_t),
    ('mask_identifier_size', c_size_t),
    ('value_offset', c_size_t),
    ('value_count', c_size_t),
]

ctex_layer_snapshot_mask_info = struct_ctex_layer_snapshot_mask_info
enum_ctex_layer_operation_kind = c_int
CTEX_LAYER_OPERATION_CREATE = 0
CTEX_LAYER_OPERATION_DUPLICATE = 1
CTEX_LAYER_OPERATION_DELETE = 2
CTEX_LAYER_OPERATION_REORDER = 3
CTEX_LAYER_OPERATION_REPARENT = 4
CTEX_LAYER_OPERATION_CLEAR = 5
CTEX_LAYER_OPERATION_INVERT = 6
CTEX_LAYER_OPERATION_MERGE_DOWN = 7
CTEX_LAYER_OPERATION_MERGE_GROUP = 8
CTEX_LAYER_OPERATION_FLATTEN = 9
CTEX_LAYER_OPERATION_CONVERT = 10
CTEX_LAYER_OPERATION_APPLY_MASK = 11
ctex_layer_operation_kind = enum_ctex_layer_operation_kind

class struct_ctex_layer_operation_descriptor(Structure):
    pass

struct_ctex_layer_operation_descriptor.__slots__ = [
    'size',
    'kind',
    'identifier',
    'duplicate_identifier',
    'parent_identifier',
    'before_identifier',
    'semantic_id',
    'entry',
    'replacement_content',
    'replacement_content_count',
    'replacement_masks',
    'replacement_mask_count',
    'source_deletion_policy',
    'target_kind',
    'graph_serialized',
    'graph_serialized_size',
    'maximum_output_bytes',
    'appearance_tolerance',
]
struct_ctex_layer_operation_descriptor._fields_ = [
    ('size', uint32_t),
    ('kind', uint32_t),
    ('identifier', String),
    ('duplicate_identifier', String),
    ('parent_identifier', String),
    ('before_identifier', String),
    ('semantic_id', String),
    ('entry', POINTER(ctex_layer_entry_descriptor)),
    ('replacement_content', POINTER(ctex_layer_composite_raster_descriptor)),
    ('replacement_content_count', c_size_t),
    ('replacement_masks', POINTER(ctex_layer_composite_mask_descriptor)),
    ('replacement_mask_count', c_size_t),
    ('source_deletion_policy', uint32_t),
    ('target_kind', uint32_t),
    ('graph_serialized', POINTER(None)),
    ('graph_serialized_size', c_size_t),
    ('maximum_output_bytes', c_size_t),
    ('appearance_tolerance', c_float),
]

ctex_layer_operation_descriptor = struct_ctex_layer_operation_descriptor

class struct_ctex_layer_operation_info(Structure):
    pass

struct_ctex_layer_operation_info.__slots__ = [
    'size',
    'affected_count',
    'required_affected_id_size',
]
struct_ctex_layer_operation_info._fields_ = [
    ('size', uint32_t),
    ('affected_count', c_size_t),
    ('required_affected_id_size', c_size_t),
]

ctex_layer_operation_info = struct_ctex_layer_operation_info
enum_ctex_scalar_representation = c_int
CTEX_SCALAR_REPRESENTATION_UNSIGNED_NORMALIZED = 0
CTEX_SCALAR_REPRESENTATION_FLOATING_POINT = 1
ctex_scalar_representation = enum_ctex_scalar_representation
enum_ctex_image_file_format = c_int
CTEX_IMAGE_FILE_FORMAT_UNKNOWN = 0
CTEX_IMAGE_FILE_FORMAT_PNG = 1
CTEX_IMAGE_FILE_FORMAT_JPEG = 2
CTEX_IMAGE_FILE_FORMAT_BMP = 3
CTEX_IMAGE_FILE_FORMAT_TIFF = 4
CTEX_IMAGE_FILE_FORMAT_OPENEXR = 5
CTEX_IMAGE_FILE_FORMAT_RADIANCE_HDR = 6
CTEX_IMAGE_FILE_FORMAT_PSD = 7
CTEX_IMAGE_FILE_FORMAT_TGA = 8
ctex_image_file_format = enum_ctex_image_file_format
enum_ctex_color_space_source = c_int
CTEX_COLOR_SPACE_SOURCE_CALLER = 0
CTEX_COLOR_SPACE_SOURCE_EMBEDDED_SRGB = 1
CTEX_COLOR_SPACE_SOURCE_AUTOMATIC_RULE = 2
ctex_color_space_source = enum_ctex_color_space_source

class struct_ctex_image_decode_limits_descriptor(Structure):
    pass

struct_ctex_image_decode_limits_descriptor.__slots__ = [
    'size',
    'maximum_width',
    'maximum_height',
    'maximum_decoded_bytes',
]
struct_ctex_image_decode_limits_descriptor._fields_ = [
    ('size', uint32_t),
    ('maximum_width', uint32_t),
    ('maximum_height', uint32_t),
    ('maximum_decoded_bytes', c_size_t),
]

ctex_image_decode_limits_descriptor = struct_ctex_image_decode_limits_descriptor
enum_ctex_image_decode_phase = c_int
CTEX_IMAGE_DECODE_PHASE_INSPECTION = 0
CTEX_IMAGE_DECODE_PHASE_CODEC = 1
CTEX_IMAGE_DECODE_PHASE_UNPACK = 2
CTEX_IMAGE_DECODE_PHASE_COMPLETE = 3
ctex_image_decode_phase = enum_ctex_image_decode_phase

class struct_ctex_image_decode_progress_info(Structure):
    pass

struct_ctex_image_decode_progress_info.__slots__ = [
    'size',
    'phase',
    'completed_rows',
    'total_rows',
    'estimated_peak_working_bytes',
]
struct_ctex_image_decode_progress_info._fields_ = [
    ('size', uint32_t),
    ('phase', uint32_t),
    ('completed_rows', uint32_t),
    ('total_rows', uint32_t),
    ('estimated_peak_working_bytes', c_size_t),
]

ctex_image_decode_progress_info = struct_ctex_image_decode_progress_info
ctex_image_decode_cancel_callback = CFUNCTYPE(UNCHECKED(uint32_t), POINTER(None))
ctex_image_decode_progress_callback = CFUNCTYPE(UNCHECKED(None), POINTER(None), POINTER(ctex_image_decode_progress_info))

class struct_ctex_image_decode_control_descriptor(Structure):
    pass

struct_ctex_image_decode_control_descriptor.__slots__ = [
    'size',
    'maximum_working_bytes',
    'progress_interval_rows',
    'user_data',
    'is_cancelled',
    'report_progress',
]
struct_ctex_image_decode_control_descriptor._fields_ = [
    ('size', uint32_t),
    ('maximum_working_bytes', c_size_t),
    ('progress_interval_rows', uint32_t),
    ('user_data', POINTER(None)),
    ('is_cancelled', ctex_image_decode_cancel_callback),
    ('report_progress', ctex_image_decode_progress_callback),
]

ctex_image_decode_control_descriptor = struct_ctex_image_decode_control_descriptor

class struct_ctex_image_decode_execution_info(Structure):
    pass

struct_ctex_image_decode_execution_info.__slots__ = [
    'size',
    'estimated_peak_working_bytes',
    'progress_event_count',
    'cancelled',
]
struct_ctex_image_decode_execution_info._fields_ = [
    ('size', uint32_t),
    ('estimated_peak_working_bytes', c_size_t),
    ('progress_event_count', c_size_t),
    ('cancelled', uint32_t),
]

ctex_image_decode_execution_info = struct_ctex_image_decode_execution_info

class struct_ctex_decoded_image_info(Structure):
    pass

struct_ctex_decoded_image_info.__slots__ = [
    'size',
    'width',
    'height',
    'channel_count',
    'scalar_representation',
    'bit_depth',
    'color_space',
    'detected_format',
    'extension_mismatch',
    'color_space_source',
    'uninterpretable_profile',
]
struct_ctex_decoded_image_info._fields_ = [
    ('size', uint32_t),
    ('width', uint32_t),
    ('height', uint32_t),
    ('channel_count', uint32_t),
    ('scalar_representation', uint32_t),
    ('bit_depth', uint32_t),
    ('color_space', uint32_t),
    ('detected_format', uint32_t),
    ('extension_mismatch', uint32_t),
    ('color_space_source', uint32_t),
    ('uninterpretable_profile', uint32_t),
]

ctex_decoded_image_info = struct_ctex_decoded_image_info
enum_ctex_layered_image_decode_mode = c_int
CTEX_LAYERED_IMAGE_DECODE_COMPOSITE = 0
CTEX_LAYERED_IMAGE_DECODE_INDIVIDUAL = 1
ctex_layered_image_decode_mode = enum_ctex_layered_image_decode_mode

class struct_ctex_layered_image_decode_descriptor(Structure):
    pass

struct_ctex_layered_image_decode_descriptor.__slots__ = [
    'size',
    'mode',
    'intended_channel',
    'input_color_space',
    'maximum_image_count',
]
struct_ctex_layered_image_decode_descriptor._fields_ = [
    ('size', uint32_t),
    ('mode', uint32_t),
    ('intended_channel', uint32_t),
    ('input_color_space', uint32_t),
    ('maximum_image_count', c_size_t),
]

ctex_layered_image_decode_descriptor = struct_ctex_layered_image_decode_descriptor

class struct_ctex_layered_image_decode_info(Structure):
    pass

struct_ctex_layered_image_decode_info.__slots__ = [
    'size',
    'detected_format',
    'source_was_layered',
    'image_count',
    'required_image_info_count',
    'required_name_buffer_size',
    'required_pixel_buffer_size',
]
struct_ctex_layered_image_decode_info._fields_ = [
    ('size', uint32_t),
    ('detected_format', uint32_t),
    ('source_was_layered', uint32_t),
    ('image_count', c_size_t),
    ('required_image_info_count', c_size_t),
    ('required_name_buffer_size', c_size_t),
    ('required_pixel_buffer_size', c_size_t),
]

ctex_layered_image_decode_info = struct_ctex_layered_image_decode_info

class struct_ctex_layered_decoded_image_info(Structure):
    pass

struct_ctex_layered_decoded_image_info.__slots__ = [
    'size',
    'origin_x',
    'origin_y',
    'width',
    'height',
    'channel_count',
    'scalar_representation',
    'bit_depth',
    'color_space',
    'name_offset',
    'name_size',
    'pixel_offset',
    'pixel_size',
]
struct_ctex_layered_decoded_image_info._fields_ = [
    ('size', uint32_t),
    ('origin_x', c_int32),
    ('origin_y', c_int32),
    ('width', uint32_t),
    ('height', uint32_t),
    ('channel_count', uint32_t),
    ('scalar_representation', uint32_t),
    ('bit_depth', uint32_t),
    ('color_space', uint32_t),
    ('name_offset', c_size_t),
    ('name_size', c_size_t),
    ('pixel_offset', c_size_t),
    ('pixel_size', c_size_t),
]

ctex_layered_decoded_image_info = struct_ctex_layered_decoded_image_info
enum_ctex_image_channel_expansion_rule = c_int
CTEX_IMAGE_CHANNEL_EXPANSION_IDENTITY = 0
CTEX_IMAGE_CHANNEL_EXPANSION_GRAYSCALE_TO_RGB = 1
CTEX_IMAGE_CHANNEL_EXPANSION_GRAYSCALE_TO_RGBA = 2
CTEX_IMAGE_CHANNEL_EXPANSION_GRAYSCALE_ALPHA_TO_RGBA = 3
CTEX_IMAGE_CHANNEL_EXPANSION_RGB_TO_RGBA = 4
ctex_image_channel_expansion_rule = enum_ctex_image_channel_expansion_rule

class struct_ctex_image_channel_expansion_descriptor(Structure):
    pass

struct_ctex_image_channel_expansion_descriptor.__slots__ = [
    'size',
    'width',
    'height',
    'source_channel_count',
    'scalar_representation',
    'bit_depth',
    'source_row_stride_bytes',
    'target_channel_count',
]
struct_ctex_image_channel_expansion_descriptor._fields_ = [
    ('size', uint32_t),
    ('width', uint32_t),
    ('height', uint32_t),
    ('source_channel_count', uint32_t),
    ('scalar_representation', uint32_t),
    ('bit_depth', uint32_t),
    ('source_row_stride_bytes', c_size_t),
    ('target_channel_count', uint32_t),
]

ctex_image_channel_expansion_descriptor = struct_ctex_image_channel_expansion_descriptor

class struct_ctex_image_channel_expansion_info(Structure):
    pass

struct_ctex_image_channel_expansion_info.__slots__ = [
    'size',
    'channel_count',
    'scalar_representation',
    'bit_depth',
    'rule',
    'required_pixel_buffer_size',
]
struct_ctex_image_channel_expansion_info._fields_ = [
    ('size', uint32_t),
    ('channel_count', uint32_t),
    ('scalar_representation', uint32_t),
    ('bit_depth', uint32_t),
    ('rule', uint32_t),
    ('required_pixel_buffer_size', c_size_t),
]

ctex_image_channel_expansion_info = struct_ctex_image_channel_expansion_info
enum_ctex_image_resample_filter = c_int
CTEX_IMAGE_RESAMPLE_FILTER_DEFAULT = 0
CTEX_IMAGE_RESAMPLE_FILTER_NEAREST = 1
CTEX_IMAGE_RESAMPLE_FILTER_BILINEAR = 2
ctex_image_resample_filter = enum_ctex_image_resample_filter

class struct_ctex_image_resample_descriptor(Structure):
    pass

struct_ctex_image_resample_descriptor.__slots__ = [
    'size',
    'source_width',
    'source_height',
    'channel_count',
    'scalar_representation',
    'bit_depth',
    'source_row_stride_bytes',
    'output_width',
    'output_height',
    'filter',
    'maximum_output_bytes',
]
struct_ctex_image_resample_descriptor._fields_ = [
    ('size', uint32_t),
    ('source_width', uint32_t),
    ('source_height', uint32_t),
    ('channel_count', uint32_t),
    ('scalar_representation', uint32_t),
    ('bit_depth', uint32_t),
    ('source_row_stride_bytes', c_size_t),
    ('output_width', uint32_t),
    ('output_height', uint32_t),
    ('filter', uint32_t),
    ('maximum_output_bytes', c_size_t),
]

ctex_image_resample_descriptor = struct_ctex_image_resample_descriptor

class struct_ctex_image_resample_info(Structure):
    pass

struct_ctex_image_resample_info.__slots__ = [
    'size',
    'width',
    'height',
    'channel_count',
    'scalar_representation',
    'bit_depth',
    'filter',
    'required_pixel_buffer_size',
]
struct_ctex_image_resample_info._fields_ = [
    ('size', uint32_t),
    ('width', uint32_t),
    ('height', uint32_t),
    ('channel_count', uint32_t),
    ('scalar_representation', uint32_t),
    ('bit_depth', uint32_t),
    ('filter', uint32_t),
    ('required_pixel_buffer_size', c_size_t),
]

ctex_image_resample_info = struct_ctex_image_resample_info

class struct_ctex_image_encode_descriptor(Structure):
    pass

struct_ctex_image_encode_descriptor.__slots__ = [
    'size',
    'width',
    'height',
    'channel_count',
    'scalar_representation',
    'input_bit_depth',
    'row_stride_bytes',
    'color_space',
    'output_format',
    'output_bit_depth',
    'jpeg_quality',
]
struct_ctex_image_encode_descriptor._fields_ = [
    ('size', uint32_t),
    ('width', uint32_t),
    ('height', uint32_t),
    ('channel_count', uint32_t),
    ('scalar_representation', uint32_t),
    ('input_bit_depth', uint32_t),
    ('row_stride_bytes', c_size_t),
    ('color_space', uint32_t),
    ('output_format', uint32_t),
    ('output_bit_depth', uint32_t),
    ('jpeg_quality', uint32_t),
]

ctex_image_encode_descriptor = struct_ctex_image_encode_descriptor
enum_ctex_channel_classification = c_int
CTEX_CHANNEL_CLASSIFICATION_COLOR = 0
CTEX_CHANNEL_CLASSIFICATION_DATA = 1
ctex_channel_classification = enum_ctex_channel_classification
enum_ctex_blending_policy = c_int
CTEX_BLENDING_POLICY_COLOR = 0
CTEX_BLENDING_POLICY_SCALAR = 1
CTEX_BLENDING_POLICY_NORMAL_VECTOR = 2
CTEX_BLENDING_POLICY_ADDITIVE = 3
ctex_blending_policy = enum_ctex_blending_policy

class struct_ctex_channel_descriptor(Structure):
    pass

struct_ctex_channel_descriptor.__slots__ = [
    'size',
    'semantic_id',
    'component_count',
    'scalar_representation',
    'preferred_bit_depth',
    'default_value',
    'default_value_count',
    'classification',
    'blending_policy',
    'export_mapping',
    'evaluable',
]
struct_ctex_channel_descriptor._fields_ = [
    ('size', uint32_t),
    ('semantic_id', String),
    ('component_count', uint32_t),
    ('scalar_representation', uint32_t),
    ('preferred_bit_depth', uint32_t),
    ('default_value', c_double * int(4)),
    ('default_value_count', uint32_t),
    ('classification', uint32_t),
    ('blending_policy', uint32_t),
    ('export_mapping', String),
    ('evaluable', uint32_t),
]

ctex_channel_descriptor = struct_ctex_channel_descriptor

class struct_ctex_channel_info(Structure):
    pass

struct_ctex_channel_info.__slots__ = [
    'size',
    'component_count',
    'scalar_representation',
    'preferred_bit_depth',
    'default_value',
    'default_value_count',
    'classification',
    'blending_policy',
    'evaluable',
    'enabled',
    'storage_bit_depth',
]
struct_ctex_channel_info._fields_ = [
    ('size', uint32_t),
    ('component_count', uint32_t),
    ('scalar_representation', uint32_t),
    ('preferred_bit_depth', uint32_t),
    ('default_value', c_double * int(4)),
    ('default_value_count', uint32_t),
    ('classification', uint32_t),
    ('blending_policy', uint32_t),
    ('evaluable', uint32_t),
    ('enabled', uint32_t),
    ('storage_bit_depth', uint32_t),
]

ctex_channel_info = struct_ctex_channel_info

class struct_ctex_texture_set_memory_report(Structure):
    pass

struct_ctex_texture_set_memory_report.__slots__ = [
    'size',
    'enabled_channel_count',
    'channel_pixel_bytes',
    'mesh_map_pixel_bytes',
    'total_resident_bytes',
]
struct_ctex_texture_set_memory_report._fields_ = [
    ('size', uint32_t),
    ('enabled_channel_count', c_size_t),
    ('channel_pixel_bytes', c_size_t),
    ('mesh_map_pixel_bytes', c_size_t),
    ('total_resident_bytes', c_size_t),
]

ctex_texture_set_memory_report = struct_ctex_texture_set_memory_report

class struct_ctex_document_texture_set_memory_info(Structure):
    pass

struct_ctex_document_texture_set_memory_info.__slots__ = [
    'texture_set_id_offset',
    'texture_set_id_size',
    'channel_pixel_bytes',
    'history_retained_bytes',
    'mesh_map_pixel_bytes',
    'total_resident_bytes',
    'estimated_save_bytes',
]
struct_ctex_document_texture_set_memory_info._fields_ = [
    ('texture_set_id_offset', c_size_t),
    ('texture_set_id_size', c_size_t),
    ('channel_pixel_bytes', c_size_t),
    ('history_retained_bytes', c_size_t),
    ('mesh_map_pixel_bytes', c_size_t),
    ('total_resident_bytes', c_size_t),
    ('estimated_save_bytes', c_size_t),
]

ctex_document_texture_set_memory_info = struct_ctex_document_texture_set_memory_info

class struct_ctex_document_memory_info(Structure):
    pass

struct_ctex_document_memory_info.__slots__ = [
    'size',
    'texture_set_count',
    'required_texture_set_id_size',
    'channel_pixel_bytes',
    'history_retained_bytes',
    'mesh_map_pixel_bytes',
    'total_resident_bytes',
    'estimated_save_bytes',
]
struct_ctex_document_memory_info._fields_ = [
    ('size', uint32_t),
    ('texture_set_count', c_size_t),
    ('required_texture_set_id_size', c_size_t),
    ('channel_pixel_bytes', c_size_t),
    ('history_retained_bytes', c_size_t),
    ('mesh_map_pixel_bytes', c_size_t),
    ('total_resident_bytes', c_size_t),
    ('estimated_save_bytes', c_size_t),
]

ctex_document_memory_info = struct_ctex_document_memory_info

class struct_ctex_transport_revision_cursor(Structure):
    pass

struct_ctex_transport_revision_cursor.__slots__ = [
    'epoch',
    'revision',
]
struct_ctex_transport_revision_cursor._fields_ = [
    ('epoch', uint64_t),
    ('revision', uint64_t),
]

ctex_transport_revision_cursor = struct_ctex_transport_revision_cursor
enum_ctex_transport_delta_disposition = c_int
CTEX_TRANSPORT_DELTA_COMPLETE = 0
CTEX_TRANSPORT_FULL_RESYNCHRONIZATION_REQUIRED = 1
ctex_transport_delta_disposition = enum_ctex_transport_delta_disposition
enum_ctex_transport_tile_residency = c_int
CTEX_TRANSPORT_TILE_CPU = 0
CTEX_TRANSPORT_TILE_HOST_DEVICE = 1
ctex_transport_tile_residency = enum_ctex_transport_tile_residency

class struct_ctex_transport_tile_version(Structure):
    pass

struct_ctex_transport_tile_version.__slots__ = [
    'x',
    'y',
    'revision',
    'generation',
    'residency',
]
struct_ctex_transport_tile_version._fields_ = [
    ('x', uint32_t),
    ('y', uint32_t),
    ('revision', uint64_t),
    ('generation', uint64_t),
    ('residency', uint32_t),
]

ctex_transport_tile_version = struct_ctex_transport_tile_version

class struct_ctex_transport_delta_info(Structure):
    pass

struct_ctex_transport_delta_info.__slots__ = [
    'size',
    'disposition',
    'synchronized_cursor',
    'current_cursor',
    'changed_tile_count',
    'indexed_tiles_visited',
]
struct_ctex_transport_delta_info._fields_ = [
    ('size', uint32_t),
    ('disposition', uint32_t),
    ('synchronized_cursor', ctex_transport_revision_cursor),
    ('current_cursor', ctex_transport_revision_cursor),
    ('changed_tile_count', c_size_t),
    ('indexed_tiles_visited', c_size_t),
]

ctex_transport_delta_info = struct_ctex_transport_delta_info
enum_ctex_transport_component_type = c_int
CTEX_TRANSPORT_COMPONENT_UINT8_UNORM = 0
CTEX_TRANSPORT_COMPONENT_UINT16_UNORM = 1
CTEX_TRANSPORT_COMPONENT_FLOAT32 = 2
ctex_transport_component_type = enum_ctex_transport_component_type

class struct_ctex_transport_pixel_format(Structure):
    pass

struct_ctex_transport_pixel_format.__slots__ = [
    'component_type',
    'channel_count',
]
struct_ctex_transport_pixel_format._fields_ = [
    ('component_type', uint32_t),
    ('channel_count', uint32_t),
]

ctex_transport_pixel_format = struct_ctex_transport_pixel_format
enum_ctex_transport_conversion_policy = c_int
CTEX_TRANSPORT_EXACT_FORMAT_ONLY = 0
CTEX_TRANSPORT_ALLOW_FORMAT_CONVERSION = 1
ctex_transport_conversion_policy = enum_ctex_transport_conversion_policy
enum_ctex_transport_format_conversion = c_int
CTEX_TRANSPORT_CONVERSION_NONE = 0
CTEX_TRANSPORT_CONVERSION_UINT8_TO_UINT16 = 1
CTEX_TRANSPORT_CONVERSION_UINT8_TO_FLOAT32 = 2
CTEX_TRANSPORT_CONVERSION_UINT16_TO_UINT8 = 3
CTEX_TRANSPORT_CONVERSION_UINT16_TO_FLOAT32 = 4
CTEX_TRANSPORT_CONVERSION_FLOAT32_TO_UINT8 = 5
CTEX_TRANSPORT_CONVERSION_FLOAT32_TO_UINT16 = 6
ctex_transport_format_conversion = enum_ctex_transport_format_conversion

class struct_ctex_transport_format_selection(Structure):
    pass

struct_ctex_transport_format_selection.__slots__ = [
    'size',
    'source_format',
    'output_format',
    'conversion',
]
struct_ctex_transport_format_selection._fields_ = [
    ('size', uint32_t),
    ('source_format', ctex_transport_pixel_format),
    ('output_format', ctex_transport_pixel_format),
    ('conversion', uint32_t),
]

ctex_transport_format_selection = struct_ctex_transport_format_selection

class struct_ctex_transport_snapshot_query_info(Structure):
    pass

struct_ctex_transport_snapshot_query_info.__slots__ = [
    'size',
    'disposition',
    'synchronized_cursor',
    'current_cursor',
    'changed_tile_count',
    'indexed_tiles_visited',
    'retained_bytes',
    'additional_pinned_bytes',
]
struct_ctex_transport_snapshot_query_info._fields_ = [
    ('size', uint32_t),
    ('disposition', uint32_t),
    ('synchronized_cursor', ctex_transport_revision_cursor),
    ('current_cursor', ctex_transport_revision_cursor),
    ('changed_tile_count', c_size_t),
    ('indexed_tiles_visited', c_size_t),
    ('retained_bytes', c_size_t),
    ('additional_pinned_bytes', c_size_t),
]

ctex_transport_snapshot_query_info = struct_ctex_transport_snapshot_query_info

class struct_ctex_transport_snapshot_memory_report(Structure):
    pass

struct_ctex_transport_snapshot_memory_report.__slots__ = [
    'size',
    'budget_bytes',
    'pinned_bytes',
    'active_snapshots',
    'pinned_allocations',
]
struct_ctex_transport_snapshot_memory_report._fields_ = [
    ('size', uint32_t),
    ('budget_bytes', c_size_t),
    ('pinned_bytes', c_size_t),
    ('active_snapshots', c_size_t),
    ('pinned_allocations', c_size_t),
]

ctex_transport_snapshot_memory_report = struct_ctex_transport_snapshot_memory_report
enum_ctex_resource_category = c_int
CTEX_RESOURCE_DOCUMENT_STORAGE = 0
CTEX_RESOURCE_HISTORY = 1
CTEX_RESOURCE_RECOVERY_RECORD = 2
CTEX_RESOURCE_MESH_MAP = 3
CTEX_RESOURCE_COMPOSITE = 4
CTEX_RESOURCE_CACHE = 5
CTEX_RESOURCE_TEMPORARY = 6
CTEX_RESOURCE_CATEGORY_COUNT = 7
ctex_resource_category = enum_ctex_resource_category
enum_ctex_resource_role = c_int
CTEX_RESOURCE_CPU_RESIDENT = 1
CTEX_RESOURCE_GPU_RESIDENT = 2
CTEX_RESOURCE_BACKING_STORE = 4
CTEX_RESOURCE_PINNED = 8
CTEX_RESOURCE_IN_FLIGHT = 16
ctex_resource_role = enum_ctex_resource_role
ctex_resource_cache_eviction_callback = CFUNCTYPE(UNCHECKED(None), uint64_t, POINTER(None))

class struct_ctex_resource_allocation_descriptor(Structure):
    pass

struct_ctex_resource_allocation_descriptor.__slots__ = [
    'size',
    'allocation_identity',
    'category',
    'physical_bytes',
    'roles',
    'device_backend',
    'device_identifier',
    'heap_identifier',
]
struct_ctex_resource_allocation_descriptor._fields_ = [
    ('size', uint32_t),
    ('allocation_identity', uint64_t),
    ('category', uint32_t),
    ('physical_bytes', c_size_t),
    ('roles', uint32_t),
    ('device_backend', String),
    ('device_identifier', String),
    ('heap_identifier', String),
]

ctex_resource_allocation_descriptor = struct_ctex_resource_allocation_descriptor

class struct_ctex_resource_category_report(Structure):
    pass

struct_ctex_resource_category_report.__slots__ = [
    'size',
    'category',
    'allocation_count',
    'physical_bytes',
    'cpu_resident_bytes',
    'gpu_resident_bytes',
    'backing_store_bytes',
    'pinned_bytes',
    'in_flight_bytes',
]
struct_ctex_resource_category_report._fields_ = [
    ('size', uint32_t),
    ('category', uint32_t),
    ('allocation_count', c_size_t),
    ('physical_bytes', c_size_t),
    ('cpu_resident_bytes', c_size_t),
    ('gpu_resident_bytes', c_size_t),
    ('backing_store_bytes', c_size_t),
    ('pinned_bytes', c_size_t),
    ('in_flight_bytes', c_size_t),
]

ctex_resource_category_report = struct_ctex_resource_category_report

class struct_ctex_resource_accounting_report(Structure):
    pass

struct_ctex_resource_accounting_report.__slots__ = [
    'size',
    'allocation_count',
    'physical_bytes',
    'cpu_resident_bytes',
    'gpu_resident_bytes',
    'backing_store_bytes',
    'pinned_bytes',
    'in_flight_bytes',
    'category_count',
]
struct_ctex_resource_accounting_report._fields_ = [
    ('size', uint32_t),
    ('allocation_count', c_size_t),
    ('physical_bytes', c_size_t),
    ('cpu_resident_bytes', c_size_t),
    ('gpu_resident_bytes', c_size_t),
    ('backing_store_bytes', c_size_t),
    ('pinned_bytes', c_size_t),
    ('in_flight_bytes', c_size_t),
    ('category_count', c_size_t),
]

ctex_resource_accounting_report = struct_ctex_resource_accounting_report

class struct_ctex_resource_budget_limits(Structure):
    pass

struct_ctex_resource_budget_limits.__slots__ = [
    'size',
    'cpu_bytes',
    'gpu_bytes',
    'backing_store_bytes',
    'temporary_bytes',
]
struct_ctex_resource_budget_limits._fields_ = [
    ('size', uint32_t),
    ('cpu_bytes', c_size_t),
    ('gpu_bytes', c_size_t),
    ('backing_store_bytes', c_size_t),
    ('temporary_bytes', c_size_t),
]

ctex_resource_budget_limits = struct_ctex_resource_budget_limits

class struct_ctex_resource_requirement(Structure):
    pass

struct_ctex_resource_requirement.__slots__ = [
    'size',
    'category',
    'physical_bytes',
    'roles',
]
struct_ctex_resource_requirement._fields_ = [
    ('size', uint32_t),
    ('category', uint32_t),
    ('physical_bytes', c_size_t),
    ('roles', uint32_t),
]

ctex_resource_requirement = struct_ctex_resource_requirement

class struct_ctex_resource_admission_descriptor(Structure):
    pass

struct_ctex_resource_admission_descriptor.__slots__ = [
    'size',
    'operation',
    'limits',
    'fixed_requirements',
    'fixed_requirement_count',
    'per_work_item',
    'work_item_count',
]
struct_ctex_resource_admission_descriptor._fields_ = [
    ('size', uint32_t),
    ('operation', String),
    ('limits', ctex_resource_budget_limits),
    ('fixed_requirements', POINTER(ctex_resource_requirement)),
    ('fixed_requirement_count', c_size_t),
    ('per_work_item', ctex_resource_requirement),
    ('work_item_count', c_size_t),
]

ctex_resource_admission_descriptor = struct_ctex_resource_admission_descriptor
enum_ctex_resource_admission_status = c_int
CTEX_RESOURCE_ADMITTED_WHOLE = 0
CTEX_RESOURCE_ADMITTED_TILED = 1
CTEX_RESOURCE_OVER_BUDGET = 2
CTEX_RESOURCE_QUIESCING = 3
ctex_resource_admission_status = enum_ctex_resource_admission_status

class struct_ctex_resource_admission_report(Structure):
    pass

struct_ctex_resource_admission_report.__slots__ = [
    'size',
    'status',
    'work_item_count',
    'admitted_work_items',
    'projected_cpu_bytes',
    'projected_gpu_bytes',
    'projected_backing_store_bytes',
    'projected_temporary_bytes',
    'evicted_allocation_count',
]
struct_ctex_resource_admission_report._fields_ = [
    ('size', uint32_t),
    ('status', uint32_t),
    ('work_item_count', c_size_t),
    ('admitted_work_items', c_size_t),
    ('projected_cpu_bytes', c_size_t),
    ('projected_gpu_bytes', c_size_t),
    ('projected_backing_store_bytes', c_size_t),
    ('projected_temporary_bytes', c_size_t),
    ('evicted_allocation_count', c_size_t),
]

ctex_resource_admission_report = struct_ctex_resource_admission_report

class struct_ctex_preview_quality_option(Structure):
    pass

struct_ctex_preview_quality_option.__slots__ = [
    'size',
    'width',
    'height',
    'requirements',
    'requirement_count',
    'derived_work_deferred',
]
struct_ctex_preview_quality_option._fields_ = [
    ('size', uint32_t),
    ('width', uint32_t),
    ('height', uint32_t),
    ('requirements', POINTER(ctex_resource_requirement)),
    ('requirement_count', c_size_t),
    ('derived_work_deferred', uint32_t),
]

ctex_preview_quality_option = struct_ctex_preview_quality_option

class struct_ctex_preview_quality_admission_descriptor(Structure):
    pass

struct_ctex_preview_quality_admission_descriptor.__slots__ = [
    'size',
    'operation',
    'limits',
    'full_quality_width',
    'full_quality_height',
    'options',
    'option_count',
]
struct_ctex_preview_quality_admission_descriptor._fields_ = [
    ('size', uint32_t),
    ('operation', String),
    ('limits', ctex_resource_budget_limits),
    ('full_quality_width', uint32_t),
    ('full_quality_height', uint32_t),
    ('options', POINTER(ctex_preview_quality_option)),
    ('option_count', c_size_t),
]

ctex_preview_quality_admission_descriptor = struct_ctex_preview_quality_admission_descriptor
enum_ctex_preview_quality_status = c_int
CTEX_PREVIEW_FULL_QUALITY = 0
CTEX_PREVIEW_REDUCED_RESOLUTION = 1
CTEX_PREVIEW_DEFERRED_DERIVED = 2
CTEX_PREVIEW_REDUCED_AND_DEFERRED = 3
CTEX_PREVIEW_OVER_BUDGET = 4
CTEX_PREVIEW_QUIESCING = 5
ctex_preview_quality_status = enum_ctex_preview_quality_status

class struct_ctex_preview_quality_admission_report(Structure):
    pass

struct_ctex_preview_quality_admission_report.__slots__ = [
    'size',
    'status',
    'selected_option',
    'full_quality_width',
    'full_quality_height',
    'selected_width',
    'selected_height',
    'derived_work_deferred',
    'projected_cpu_bytes',
    'projected_gpu_bytes',
    'projected_backing_store_bytes',
    'projected_temporary_bytes',
    'evicted_allocation_count',
]
struct_ctex_preview_quality_admission_report._fields_ = [
    ('size', uint32_t),
    ('status', uint32_t),
    ('selected_option', c_size_t),
    ('full_quality_width', uint32_t),
    ('full_quality_height', uint32_t),
    ('selected_width', uint32_t),
    ('selected_height', uint32_t),
    ('derived_work_deferred', uint32_t),
    ('projected_cpu_bytes', c_size_t),
    ('projected_gpu_bytes', c_size_t),
    ('projected_backing_store_bytes', c_size_t),
    ('projected_temporary_bytes', c_size_t),
    ('evicted_allocation_count', c_size_t),
]

ctex_preview_quality_admission_report = struct_ctex_preview_quality_admission_report

class struct_ctex_tile_backing_key(Structure):
    pass

struct_ctex_tile_backing_key.__slots__ = [
    'namespace_identity',
    'tile_x',
    'tile_y',
    'generation',
]
struct_ctex_tile_backing_key._fields_ = [
    ('namespace_identity', uint64_t),
    ('tile_x', uint32_t),
    ('tile_y', uint32_t),
    ('generation', uint64_t),
]

ctex_tile_backing_key = struct_ctex_tile_backing_key
ctex_tile_backing_store_callback = CFUNCTYPE(UNCHECKED(uint32_t), ctex_tile_backing_key, POINTER(None), c_size_t, POINTER(None))
ctex_tile_backing_load_callback = CFUNCTYPE(UNCHECKED(uint32_t), ctex_tile_backing_key, POINTER(None), c_size_t, POINTER(None))
ctex_tile_backing_discard_callback = CFUNCTYPE(UNCHECKED(None), ctex_tile_backing_key, POINTER(None))
ctex_tile_backing_release_callback = CFUNCTYPE(UNCHECKED(None), uint64_t, POINTER(None))

class struct_ctex_tile_backing_store_descriptor(Structure):
    pass

struct_ctex_tile_backing_store_descriptor.__slots__ = [
    'size',
    'store',
    'load',
    'discard',
    'release_namespace',
    'user_data',
]
struct_ctex_tile_backing_store_descriptor._fields_ = [
    ('size', uint32_t),
    ('store', ctex_tile_backing_store_callback),
    ('load', ctex_tile_backing_load_callback),
    ('discard', ctex_tile_backing_discard_callback),
    ('release_namespace', ctex_tile_backing_release_callback),
    ('user_data', POINTER(None)),
]

ctex_tile_backing_store_descriptor = struct_ctex_tile_backing_store_descriptor
enum_ctex_tile_eviction_status = c_int
CTEX_TILE_EVICTED = 0
CTEX_TILE_SPARSE = 1
CTEX_TILE_ALREADY_EVICTED = 2
CTEX_TILE_PINNED = 3
CTEX_TILE_NO_BACKING_STORE = 4
CTEX_TILE_BACKING_STORE_FAILED = 5
ctex_tile_eviction_status = enum_ctex_tile_eviction_status

class struct_ctex_tile_eviction_report(Structure):
    pass

struct_ctex_tile_eviction_report.__slots__ = [
    'size',
    'status',
    'resident_bytes_released',
    'backing_bytes_written',
    'resident_pixel_bytes',
    'backed_pixel_bytes',
]
struct_ctex_tile_eviction_report._fields_ = [
    ('size', uint32_t),
    ('status', uint32_t),
    ('resident_bytes_released', c_size_t),
    ('backing_bytes_written', c_size_t),
    ('resident_pixel_bytes', c_size_t),
    ('backed_pixel_bytes', c_size_t),
]

ctex_tile_eviction_report = struct_ctex_tile_eviction_report
enum_ctex_transport_channel_order = c_int
CTEX_TRANSPORT_CHANNEL_ORDER_R = 0
CTEX_TRANSPORT_CHANNEL_ORDER_RG = 1
CTEX_TRANSPORT_CHANNEL_ORDER_RGB = 2
CTEX_TRANSPORT_CHANNEL_ORDER_RGBA = 3
ctex_transport_channel_order = enum_ctex_transport_channel_order
enum_ctex_transport_component_byte_order = c_int
CTEX_TRANSPORT_COMPONENT_BYTE_ORDER_NATIVE = 0
ctex_transport_component_byte_order = enum_ctex_transport_component_byte_order
enum_ctex_transport_tile_contiguity = c_int
CTEX_TRANSPORT_SEPARATE_TILE_BUFFERS = 0
ctex_transport_tile_contiguity = enum_ctex_transport_tile_contiguity

class struct_ctex_transport_tile_memory_layout(Structure):
    pass

struct_ctex_transport_tile_memory_layout.__slots__ = [
    'size',
    'width',
    'height',
    'row_pitch_bytes',
    'pixel_stride_bytes',
    'channel_order',
    'component_type',
    'component_byte_order',
    'tile_contiguity',
    'byte_size',
]
struct_ctex_transport_tile_memory_layout._fields_ = [
    ('size', uint32_t),
    ('width', uint32_t),
    ('height', uint32_t),
    ('row_pitch_bytes', c_size_t),
    ('pixel_stride_bytes', c_size_t),
    ('channel_order', uint32_t),
    ('component_type', uint32_t),
    ('component_byte_order', uint32_t),
    ('tile_contiguity', uint32_t),
    ('byte_size', c_size_t),
]

ctex_transport_tile_memory_layout = struct_ctex_transport_tile_memory_layout

class struct_ctex_transport_tile_readback_destination(Structure):
    pass

struct_ctex_transport_tile_readback_destination.__slots__ = [
    'size',
    'version',
    'layout',
    'output',
    'output_size',
]
struct_ctex_transport_tile_readback_destination._fields_ = [
    ('size', uint32_t),
    ('version', ctex_transport_tile_version),
    ('layout', ctex_transport_tile_memory_layout),
    ('output', POINTER(None)),
    ('output_size', c_size_t),
]

ctex_transport_tile_readback_destination = struct_ctex_transport_tile_readback_destination
enum_ctex_transport_readback_status = c_int
CTEX_TRANSPORT_READBACK_PENDING = 0
CTEX_TRANSPORT_READBACK_COMPLETE = 1
CTEX_TRANSPORT_READBACK_CANCELLED = 2
CTEX_TRANSPORT_READBACK_FAILED = 3
ctex_transport_readback_status = enum_ctex_transport_readback_status

class struct_ctex_transport_host_tile_completion(Structure):
    pass

struct_ctex_transport_host_tile_completion.__slots__ = [
    'size',
    'version',
    'layout',
    'bytes',
    'byte_size',
]
struct_ctex_transport_host_tile_completion._fields_ = [
    ('size', uint32_t),
    ('version', ctex_transport_tile_version),
    ('layout', ctex_transport_tile_memory_layout),
    ('bytes', POINTER(None)),
    ('byte_size', c_size_t),
]

ctex_transport_host_tile_completion = struct_ctex_transport_host_tile_completion

class struct_ctex_transport_readback_info(Structure):
    pass

struct_ctex_transport_readback_info.__slots__ = [
    'size',
    'status',
    'output_readable',
    'tile_count',
    'required_detail_size',
]
struct_ctex_transport_readback_info._fields_ = [
    ('size', uint32_t),
    ('status', uint32_t),
    ('output_readable', uint32_t),
    ('tile_count', c_size_t),
    ('required_detail_size', c_size_t),
]

ctex_transport_readback_info = struct_ctex_transport_readback_info
enum_ctex_mesh_map_kind = c_int
CTEX_MESH_MAP_TANGENT_SPACE_NORMAL = 0
CTEX_MESH_MAP_OBJECT_SPACE_NORMAL = 1
CTEX_MESH_MAP_WORLD_SPACE_DIRECTION = 2
CTEX_MESH_MAP_AMBIENT_OCCLUSION = 3
CTEX_MESH_MAP_CURVATURE = 4
CTEX_MESH_MAP_THICKNESS = 5
CTEX_MESH_MAP_POSITION = 6
CTEX_MESH_MAP_HEIGHT = 7
CTEX_MESH_MAP_BENT_NORMAL = 8
CTEX_MESH_MAP_MATERIAL_ID = 9
CTEX_MESH_MAP_OBJECT_ID = 10
CTEX_MESH_MAP_UV_DENSITY = 11
CTEX_MESH_MAP_VERTEX_COLOUR = 12
ctex_mesh_map_kind = enum_ctex_mesh_map_kind
enum_ctex_mesh_map_channel_meaning = c_int
CTEX_MESH_MAP_SCALAR_DATA = 0
CTEX_MESH_MAP_NORMAL_XYZ = 1
CTEX_MESH_MAP_DIRECTION_XYZ = 2
CTEX_MESH_MAP_POSITION_XYZ = 3
CTEX_MESH_MAP_IDENTIFIER = 4
CTEX_MESH_MAP_COLOUR_RGB = 5
CTEX_MESH_MAP_COLOUR_RGBA = 6
ctex_mesh_map_channel_meaning = enum_ctex_mesh_map_channel_meaning
enum_ctex_mesh_map_normal_convention = c_int
CTEX_MESH_MAP_NORMAL_OPENGL = 0
CTEX_MESH_MAP_NORMAL_DIRECTX = 1
ctex_mesh_map_normal_convention = enum_ctex_mesh_map_normal_convention
enum_ctex_tangent_basis_algorithm = c_int
CTEX_TANGENT_BASIS_UV_DERIVATIVE = 0
CTEX_TANGENT_BASIS_LENGYEL_ORTHONORMALIZED = 1
CTEX_TANGENT_BASIS_MIKKTSPACE = 2
ctex_tangent_basis_algorithm = enum_ctex_tangent_basis_algorithm
enum_ctex_tangent_normal_orientation = c_int
CTEX_TANGENT_NORMAL_VERTEX = 0
CTEX_TANGENT_NORMAL_INVERTED_VERTEX = 1
ctex_tangent_normal_orientation = enum_ctex_tangent_normal_orientation
enum_ctex_coordinate_handedness = c_int
CTEX_COORDINATE_RIGHT_HANDED = 0
CTEX_COORDINATE_LEFT_HANDED = 1
ctex_coordinate_handedness = enum_ctex_coordinate_handedness
enum_ctex_uv_v_axis = c_int
CTEX_UV_V_AXIS_UPWARD = 0
CTEX_UV_V_AXIS_DOWNWARD = 1
ctex_uv_v_axis = enum_ctex_uv_v_axis
enum_ctex_tangent_handedness_encoding = c_int
CTEX_TANGENT_HANDEDNESS_W_SIGN = 0
ctex_tangent_handedness_encoding = enum_ctex_tangent_handedness_encoding
enum_ctex_tangent_frame_source = c_int
CTEX_TANGENT_FRAME_SUPPLIED = 0
CTEX_TANGENT_FRAME_GENERATED = 1
ctex_tangent_frame_source = enum_ctex_tangent_frame_source

class struct_ctex_tangent_frame_descriptor(Structure):
    pass

struct_ctex_tangent_frame_descriptor.__slots__ = [
    'size',
    'algorithm',
    'algorithm_version',
    'normal_orientation',
    'coordinate_handedness',
    'uv_v_axis',
    'handedness_encoding',
    'uv_set',
]
struct_ctex_tangent_frame_descriptor._fields_ = [
    ('size', uint32_t),
    ('algorithm', uint32_t),
    ('algorithm_version', uint32_t),
    ('normal_orientation', uint32_t),
    ('coordinate_handedness', uint32_t),
    ('uv_v_axis', uint32_t),
    ('handedness_encoding', uint32_t),
    ('uv_set', String),
]

ctex_tangent_frame_descriptor = struct_ctex_tangent_frame_descriptor

class struct_ctex_mesh_tangent_data_descriptor(Structure):
    pass

struct_ctex_mesh_tangent_data_descriptor.__slots__ = [
    'size',
    'frame',
    'corner_tangents',
    'corner_tangent_count',
]
struct_ctex_mesh_tangent_data_descriptor._fields_ = [
    ('size', uint32_t),
    ('frame', ctex_tangent_frame_descriptor),
    ('corner_tangents', POINTER(ctex_vec4f)),
    ('corner_tangent_count', c_size_t),
]

ctex_mesh_tangent_data_descriptor = struct_ctex_mesh_tangent_data_descriptor

class struct_ctex_mesh_tangent_frame_info(Structure):
    pass

struct_ctex_mesh_tangent_frame_info.__slots__ = [
    'size',
    'source',
    'algorithm',
    'algorithm_version',
    'normal_orientation',
    'coordinate_handedness',
    'uv_v_axis',
    'handedness_encoding',
    'corner_tangent_count',
    'required_uv_set_size',
]
struct_ctex_mesh_tangent_frame_info._fields_ = [
    ('size', uint32_t),
    ('source', uint32_t),
    ('algorithm', uint32_t),
    ('algorithm_version', uint32_t),
    ('normal_orientation', uint32_t),
    ('coordinate_handedness', uint32_t),
    ('uv_v_axis', uint32_t),
    ('handedness_encoding', uint32_t),
    ('corner_tangent_count', c_size_t),
    ('required_uv_set_size', c_size_t),
]

ctex_mesh_tangent_frame_info = struct_ctex_mesh_tangent_frame_info

class struct_ctex_mesh_map_pixel_buffer_descriptor(Structure):
    pass

struct_ctex_mesh_map_pixel_buffer_descriptor.__slots__ = [
    'size',
    'width',
    'height',
    'component_type',
    'component_count',
    'row_stride_bytes',
    'pixels',
    'pixel_bytes',
]
struct_ctex_mesh_map_pixel_buffer_descriptor._fields_ = [
    ('size', uint32_t),
    ('width', uint32_t),
    ('height', uint32_t),
    ('component_type', uint32_t),
    ('component_count', uint32_t),
    ('row_stride_bytes', c_size_t),
    ('pixels', POINTER(None)),
    ('pixel_bytes', c_size_t),
]

ctex_mesh_map_pixel_buffer_descriptor = struct_ctex_mesh_map_pixel_buffer_descriptor

class struct_ctex_mesh_map_import_descriptor(Structure):
    pass

struct_ctex_mesh_map_import_descriptor.__slots__ = [
    'size',
    'kind',
    'channel_meaning',
    'color_space',
    'has_normal_convention',
    'normal_convention',
    'tangent_frame',
    'buffer',
]
struct_ctex_mesh_map_import_descriptor._fields_ = [
    ('size', uint32_t),
    ('kind', uint32_t),
    ('channel_meaning', uint32_t),
    ('color_space', uint32_t),
    ('has_normal_convention', uint32_t),
    ('normal_convention', uint32_t),
    ('tangent_frame', POINTER(ctex_tangent_frame_descriptor)),
    ('buffer', ctex_mesh_map_pixel_buffer_descriptor),
]

ctex_mesh_map_import_descriptor = struct_ctex_mesh_map_import_descriptor

class struct_ctex_mesh_map_import_info(Structure):
    pass

struct_ctex_mesh_map_import_info.__slots__ = [
    'size',
    'replaced_existing',
    'resolution_mismatch',
    'stale',
    'converted_to_working_space',
    'storage_color_space',
    'channel_meaning',
    'map_width',
    'map_height',
    'texture_set_width',
    'texture_set_height',
    'produced_mesh_revision',
    'current_mesh_revision',
]
struct_ctex_mesh_map_import_info._fields_ = [
    ('size', uint32_t),
    ('replaced_existing', uint32_t),
    ('resolution_mismatch', uint32_t),
    ('stale', uint32_t),
    ('converted_to_working_space', uint32_t),
    ('storage_color_space', uint32_t),
    ('channel_meaning', uint32_t),
    ('map_width', uint32_t),
    ('map_height', uint32_t),
    ('texture_set_width', uint32_t),
    ('texture_set_height', uint32_t),
    ('produced_mesh_revision', uint64_t),
    ('current_mesh_revision', uint64_t),
]

ctex_mesh_map_import_info = struct_ctex_mesh_map_import_info

class struct_ctex_mesh_map_set_info(Structure):
    pass

struct_ctex_mesh_map_set_info.__slots__ = [
    'size',
    'texture_set_width',
    'texture_set_height',
    'mesh_revision',
    'bound_map_count',
    'resident_pixel_bytes',
    'required_texture_set_id_size',
    'required_uv_set_size',
]
struct_ctex_mesh_map_set_info._fields_ = [
    ('size', uint32_t),
    ('texture_set_width', uint32_t),
    ('texture_set_height', uint32_t),
    ('mesh_revision', uint64_t),
    ('bound_map_count', c_size_t),
    ('resident_pixel_bytes', c_size_t),
    ('required_texture_set_id_size', c_size_t),
    ('required_uv_set_size', c_size_t),
]

ctex_mesh_map_set_info = struct_ctex_mesh_map_set_info

class struct_ctex_mesh_map_entry_info(Structure):
    pass

struct_ctex_mesh_map_entry_info.__slots__ = [
    'kind',
    'width',
    'height',
    'resident_pixel_bytes',
    'produced_mesh_revision',
    'stale',
    'has_normal_convention',
    'normal_convention',
    'has_tangent_frame',
    'tangent_algorithm',
    'tangent_algorithm_version',
    'tangent_normal_orientation',
    'tangent_coordinate_handedness',
    'tangent_uv_v_axis',
    'tangent_handedness_encoding',
]
struct_ctex_mesh_map_entry_info._fields_ = [
    ('kind', uint32_t),
    ('width', uint32_t),
    ('height', uint32_t),
    ('resident_pixel_bytes', c_size_t),
    ('produced_mesh_revision', uint64_t),
    ('stale', uint32_t),
    ('has_normal_convention', uint32_t),
    ('normal_convention', uint32_t),
    ('has_tangent_frame', uint32_t),
    ('tangent_algorithm', uint32_t),
    ('tangent_algorithm_version', uint32_t),
    ('tangent_normal_orientation', uint32_t),
    ('tangent_coordinate_handedness', uint32_t),
    ('tangent_uv_v_axis', uint32_t),
    ('tangent_handedness_encoding', uint32_t),
]

ctex_mesh_map_entry_info = struct_ctex_mesh_map_entry_info

class struct_ctex_mesh_map_sample_info(Structure):
    pass

struct_ctex_mesh_map_sample_info.__slots__ = [
    'size',
    'component_count',
    'values',
    'stale',
    'produced_mesh_revision',
    'current_mesh_revision',
]
struct_ctex_mesh_map_sample_info._fields_ = [
    ('size', uint32_t),
    ('component_count', uint32_t),
    ('values', c_double * int(4)),
    ('stale', uint32_t),
    ('produced_mesh_revision', uint64_t),
    ('current_mesh_revision', uint64_t),
]

ctex_mesh_map_sample_info = struct_ctex_mesh_map_sample_info

class struct_ctex_mesh_map_staleness(Structure):
    pass

struct_ctex_mesh_map_staleness.__slots__ = [
    'kind',
    'produced_mesh_revision',
    'current_mesh_revision',
]
struct_ctex_mesh_map_staleness._fields_ = [
    ('kind', uint32_t),
    ('produced_mesh_revision', uint64_t),
    ('current_mesh_revision', uint64_t),
]

ctex_mesh_map_staleness = struct_ctex_mesh_map_staleness

class struct_ctex_mesh_map_requirement_info(Structure):
    pass

struct_ctex_mesh_map_requirement_info.__slots__ = [
    'size',
    'required_missing_map_count',
    'required_stale_map_count',
    'required_message_size',
]
struct_ctex_mesh_map_requirement_info._fields_ = [
    ('size', uint32_t),
    ('required_missing_map_count', c_size_t),
    ('required_stale_map_count', c_size_t),
    ('required_message_size', c_size_t),
]

ctex_mesh_map_requirement_info = struct_ctex_mesh_map_requirement_info

class struct_ctex_mesh_map_release_info(Structure):
    pass

struct_ctex_mesh_map_release_info.__slots__ = [
    'size',
    'released_map_count',
    'resident_pixel_bytes_released',
]
struct_ctex_mesh_map_release_info._fields_ = [
    ('size', uint32_t),
    ('released_map_count', c_size_t),
    ('resident_pixel_bytes_released', c_size_t),
]

ctex_mesh_map_release_info = struct_ctex_mesh_map_release_info
enum_ctex_mesh_map_generator_kind = c_int
CTEX_MESH_MAP_GENERATOR_AMBIENT_OCCLUSION = 0
CTEX_MESH_MAP_GENERATOR_CURVATURE = 1
CTEX_MESH_MAP_GENERATOR_THICKNESS = 2
CTEX_MESH_MAP_GENERATOR_POSITION_GRADIENT = 3
CTEX_MESH_MAP_GENERATOR_WORLD_SPACE_DIRECTION = 4
CTEX_MESH_MAP_GENERATOR_DIRT = 5
CTEX_MESH_MAP_GENERATOR_EDGE_WEAR = 6
CTEX_MESH_MAP_GENERATOR_SCRATCHES = 7
ctex_mesh_map_generator_kind = enum_ctex_mesh_map_generator_kind

class struct_ctex_mesh_map_generator_parameter_descriptor(Structure):
    pass

struct_ctex_mesh_map_generator_parameter_descriptor.__slots__ = [
    'size',
    'name_offset',
    'name_size',
    'default_value',
    'minimum',
    'maximum',
    'meaning_offset',
    'meaning_size',
]
struct_ctex_mesh_map_generator_parameter_descriptor._fields_ = [
    ('size', uint32_t),
    ('name_offset', c_size_t),
    ('name_size', c_size_t),
    ('default_value', c_double),
    ('minimum', c_double),
    ('maximum', c_double),
    ('meaning_offset', c_size_t),
    ('meaning_size', c_size_t),
]

ctex_mesh_map_generator_parameter_descriptor = struct_ctex_mesh_map_generator_parameter_descriptor

class struct_ctex_mesh_map_generator_info(Structure):
    pass

struct_ctex_mesh_map_generator_info.__slots__ = [
    'size',
    'kind',
    'name_offset',
    'name_size',
    'required_map_count',
    'parameter_count',
    'required_string_size',
]
struct_ctex_mesh_map_generator_info._fields_ = [
    ('size', uint32_t),
    ('kind', uint32_t),
    ('name_offset', c_size_t),
    ('name_size', c_size_t),
    ('required_map_count', c_size_t),
    ('parameter_count', c_size_t),
    ('required_string_size', c_size_t),
]

ctex_mesh_map_generator_info = struct_ctex_mesh_map_generator_info

class struct_ctex_mesh_map_generator_parameter(Structure):
    pass

struct_ctex_mesh_map_generator_parameter.__slots__ = [
    'size',
    'name',
    'value',
]
struct_ctex_mesh_map_generator_parameter._fields_ = [
    ('size', uint32_t),
    ('name', String),
    ('value', c_double),
]

ctex_mesh_map_generator_parameter = struct_ctex_mesh_map_generator_parameter

class struct_ctex_mesh_map_generator_resolved_parameter(Structure):
    pass

struct_ctex_mesh_map_generator_resolved_parameter.__slots__ = [
    'name_offset',
    'name_size',
    'value',
]
struct_ctex_mesh_map_generator_resolved_parameter._fields_ = [
    ('name_offset', c_size_t),
    ('name_size', c_size_t),
    ('value', c_double),
]

ctex_mesh_map_generator_resolved_parameter = struct_ctex_mesh_map_generator_resolved_parameter

class struct_ctex_mesh_map_generator_parameter_clamp(Structure):
    pass

struct_ctex_mesh_map_generator_parameter_clamp.__slots__ = [
    'name_offset',
    'name_size',
    'supplied',
    'resolved',
]
struct_ctex_mesh_map_generator_parameter_clamp._fields_ = [
    ('name_offset', c_size_t),
    ('name_size', c_size_t),
    ('supplied', c_double),
    ('resolved', c_double),
]

ctex_mesh_map_generator_parameter_clamp = struct_ctex_mesh_map_generator_parameter_clamp

class struct_ctex_mesh_map_generator_result_info(Structure):
    pass

struct_ctex_mesh_map_generator_result_info.__slots__ = [
    'size',
    'width',
    'height',
    'row_stride_bytes',
    'required_mask_value_count',
    'required_resolved_parameter_count',
    'required_parameter_clamp_count',
    'required_stale_map_count',
    'message_offset',
    'message_size',
    'required_string_size',
]
struct_ctex_mesh_map_generator_result_info._fields_ = [
    ('size', uint32_t),
    ('width', uint32_t),
    ('height', uint32_t),
    ('row_stride_bytes', c_size_t),
    ('required_mask_value_count', c_size_t),
    ('required_resolved_parameter_count', c_size_t),
    ('required_parameter_clamp_count', c_size_t),
    ('required_stale_map_count', c_size_t),
    ('message_offset', c_size_t),
    ('message_size', c_size_t),
    ('required_string_size', c_size_t),
]

ctex_mesh_map_generator_result_info = struct_ctex_mesh_map_generator_result_info
enum_ctex_mesh_map_bake_provider_status = c_int
CTEX_MESH_MAP_BAKE_PROVIDER_COMPLETED = 0
CTEX_MESH_MAP_BAKE_PROVIDER_CANCELLED = 1
CTEX_MESH_MAP_BAKE_PROVIDER_FAILED = 2
ctex_mesh_map_bake_provider_status = enum_ctex_mesh_map_bake_provider_status
enum_ctex_mesh_map_bake_request_status = c_int
CTEX_MESH_MAP_BAKE_COMPLETED = 0
CTEX_MESH_MAP_BAKE_CANCELLED = 1
CTEX_MESH_MAP_BAKE_UNSUPPORTED = 2
CTEX_MESH_MAP_BAKE_REQUEST_PROVIDER_FAILED = 3
ctex_mesh_map_bake_request_status = enum_ctex_mesh_map_bake_request_status
enum_ctex_mesh_map_bake_completion_disposition = c_int
CTEX_MESH_MAP_BAKE_BOUND = 0
CTEX_MESH_MAP_BAKE_STALE = 1
CTEX_MESH_MAP_BAKE_COMPLETION_CANCELLED = 2
CTEX_MESH_MAP_BAKE_INVALID_OUTPUT = 3
CTEX_MESH_MAP_BAKE_UNKNOWN_TOKEN = 4
ctex_mesh_map_bake_completion_disposition = enum_ctex_mesh_map_bake_completion_disposition
ctex_mesh_map_bake_can_produce_fn = CFUNCTYPE(UNCHECKED(uint32_t), POINTER(None), uint32_t)
ctex_mesh_map_bake_is_cancelled_fn = CFUNCTYPE(UNCHECKED(uint32_t), POINTER(None))
ctex_mesh_map_bake_report_progress_fn = CFUNCTYPE(UNCHECKED(None), POINTER(None), c_double)

class struct_ctex_mesh_map_bake_request_descriptor(Structure):
    pass

struct_ctex_mesh_map_bake_request_descriptor.__slots__ = [
    'size',
    'kind',
    'texture_set_id',
    'uv_set',
    'mesh_revision',
    'bake_settings_revision',
    'request_generation',
    'tangent_frame',
    'width',
    'height',
]
struct_ctex_mesh_map_bake_request_descriptor._fields_ = [
    ('size', uint32_t),
    ('kind', uint32_t),
    ('texture_set_id', String),
    ('uv_set', String),
    ('mesh_revision', uint64_t),
    ('bake_settings_revision', uint64_t),
    ('request_generation', uint64_t),
    ('tangent_frame', POINTER(ctex_tangent_frame_descriptor)),
    ('width', uint32_t),
    ('height', uint32_t),
]

ctex_mesh_map_bake_request_descriptor = struct_ctex_mesh_map_bake_request_descriptor

class struct_ctex_mesh_map_bake_control(Structure):
    pass

struct_ctex_mesh_map_bake_control.__slots__ = [
    'size',
    'user_data',
    'is_cancelled',
    'report_progress',
]
struct_ctex_mesh_map_bake_control._fields_ = [
    ('size', uint32_t),
    ('user_data', POINTER(None)),
    ('is_cancelled', ctex_mesh_map_bake_is_cancelled_fn),
    ('report_progress', ctex_mesh_map_bake_report_progress_fn),
]

ctex_mesh_map_bake_control = struct_ctex_mesh_map_bake_control

class struct_ctex_mesh_map_bake_output_descriptor(Structure):
    pass

struct_ctex_mesh_map_bake_output_descriptor.__slots__ = [
    'size',
    'buffer',
    'has_normal_convention',
    'normal_convention',
    'tangent_frame',
    'detail',
]
struct_ctex_mesh_map_bake_output_descriptor._fields_ = [
    ('size', uint32_t),
    ('buffer', ctex_mesh_map_pixel_buffer_descriptor),
    ('has_normal_convention', uint32_t),
    ('normal_convention', uint32_t),
    ('tangent_frame', POINTER(ctex_tangent_frame_descriptor)),
    ('detail', String),
]

ctex_mesh_map_bake_output_descriptor = struct_ctex_mesh_map_bake_output_descriptor
ctex_mesh_map_bake_request_fn = CFUNCTYPE(UNCHECKED(uint32_t), POINTER(None), POINTER(ctex_mesh_map_bake_request_descriptor), POINTER(ctex_mesh_map_bake_control), POINTER(ctex_mesh_map_bake_output_descriptor))

class struct_ctex_mesh_map_bake_provider_descriptor(Structure):
    pass

struct_ctex_mesh_map_bake_provider_descriptor.__slots__ = [
    'size',
    'name',
    'user_data',
    'can_produce',
    'request',
]
struct_ctex_mesh_map_bake_provider_descriptor._fields_ = [
    ('size', uint32_t),
    ('name', String),
    ('user_data', POINTER(None)),
    ('can_produce', ctex_mesh_map_bake_can_produce_fn),
    ('request', ctex_mesh_map_bake_request_fn),
]

ctex_mesh_map_bake_provider_descriptor = struct_ctex_mesh_map_bake_provider_descriptor

class struct_ctex_mesh_map_bake_control_descriptor(Structure):
    pass

struct_ctex_mesh_map_bake_control_descriptor.__slots__ = [
    'size',
    'user_data',
    'is_cancelled',
    'report_progress',
]
struct_ctex_mesh_map_bake_control_descriptor._fields_ = [
    ('size', uint32_t),
    ('user_data', POINTER(None)),
    ('is_cancelled', ctex_mesh_map_bake_is_cancelled_fn),
    ('report_progress', ctex_mesh_map_bake_report_progress_fn),
]

ctex_mesh_map_bake_control_descriptor = struct_ctex_mesh_map_bake_control_descriptor

class struct_ctex_mesh_map_bake_result_info(Structure):
    pass

struct_ctex_mesh_map_bake_result_info.__slots__ = [
    'size',
    'status',
    'has_binding',
    'replaced_existing',
    'resolution_mismatch',
    'stale',
    'produced_mesh_revision',
    'current_mesh_revision',
]
struct_ctex_mesh_map_bake_result_info._fields_ = [
    ('size', uint32_t),
    ('status', uint32_t),
    ('has_binding', uint32_t),
    ('replaced_existing', uint32_t),
    ('resolution_mismatch', uint32_t),
    ('stale', uint32_t),
    ('produced_mesh_revision', uint64_t),
    ('current_mesh_revision', uint64_t),
]

ctex_mesh_map_bake_result_info = struct_ctex_mesh_map_bake_result_info

class struct_ctex_mesh_map_bake_session_info(Structure):
    pass

struct_ctex_mesh_map_bake_session_info.__slots__ = [
    'size',
    'settings_revision',
    'pending_request_count',
    'undo_step_count',
]
struct_ctex_mesh_map_bake_session_info._fields_ = [
    ('size', uint32_t),
    ('settings_revision', uint64_t),
    ('pending_request_count', c_size_t),
    ('undo_step_count', c_size_t),
]

ctex_mesh_map_bake_session_info = struct_ctex_mesh_map_bake_session_info

class struct_ctex_mesh_map_bake_token_info(Structure):
    pass

struct_ctex_mesh_map_bake_token_info.__slots__ = [
    'size',
    'session_identity',
    'kind',
    'mesh_revision',
    'bake_settings_revision',
    'request_generation',
    'width',
    'height',
    'has_tangent_frame',
    'tangent_frame',
    'required_texture_set_id_size',
    'required_uv_set_size',
    'required_tangent_uv_set_size',
]
struct_ctex_mesh_map_bake_token_info._fields_ = [
    ('size', uint32_t),
    ('session_identity', uint64_t),
    ('kind', uint32_t),
    ('mesh_revision', uint64_t),
    ('bake_settings_revision', uint64_t),
    ('request_generation', uint64_t),
    ('width', uint32_t),
    ('height', uint32_t),
    ('has_tangent_frame', uint32_t),
    ('tangent_frame', ctex_tangent_frame_descriptor),
    ('required_texture_set_id_size', c_size_t),
    ('required_uv_set_size', c_size_t),
    ('required_tangent_uv_set_size', c_size_t),
]

ctex_mesh_map_bake_token_info = struct_ctex_mesh_map_bake_token_info

class struct_ctex_mesh_map_bake_completion_info(Structure):
    pass

struct_ctex_mesh_map_bake_completion_info.__slots__ = [
    'size',
    'disposition',
    'has_binding',
    'replaced_existing',
    'resolution_mismatch',
    'stale',
]
struct_ctex_mesh_map_bake_completion_info._fields_ = [
    ('size', uint32_t),
    ('disposition', uint32_t),
    ('has_binding', uint32_t),
    ('replaced_existing', uint32_t),
    ('resolution_mismatch', uint32_t),
    ('stale', uint32_t),
]

ctex_mesh_map_bake_completion_info = struct_ctex_mesh_map_bake_completion_info

class struct_ctex_mesh_map_bake_settings_edit_info(Structure):
    pass

struct_ctex_mesh_map_bake_settings_edit_info.__slots__ = [
    'size',
    'previous_revision',
    'current_revision',
    'invalidated_request_count',
]
struct_ctex_mesh_map_bake_settings_edit_info._fields_ = [
    ('size', uint32_t),
    ('previous_revision', uint64_t),
    ('current_revision', uint64_t),
    ('invalidated_request_count', c_size_t),
]

ctex_mesh_map_bake_settings_edit_info = struct_ctex_mesh_map_bake_settings_edit_info

class struct_ctex_mesh_map_bake_settings_undo_info(Structure):
    pass

struct_ctex_mesh_map_bake_settings_undo_info.__slots__ = [
    'size',
    'restored',
    'previous_revision',
    'restored_revision',
    'restored_map_count',
    'invalidated_request_count',
]
struct_ctex_mesh_map_bake_settings_undo_info._fields_ = [
    ('size', uint32_t),
    ('restored', uint32_t),
    ('previous_revision', uint64_t),
    ('restored_revision', uint64_t),
    ('restored_map_count', c_size_t),
    ('invalidated_request_count', c_size_t),
]

ctex_mesh_map_bake_settings_undo_info = struct_ctex_mesh_map_bake_settings_undo_info
enum_ctex_executor_route = c_int
CTEX_EXECUTOR_ROUTE_HOST_EXECUTED = 0
CTEX_EXECUTOR_ROUTE_CPU_REFERENCE = 1
CTEX_EXECUTOR_ROUTE_OWNED_GPU = 2
ctex_executor_route = enum_ctex_executor_route
enum_ctex_executor_availability = c_int
CTEX_EXECUTOR_AVAILABLE = 0
CTEX_EXECUTOR_DEVICE_UNAVAILABLE = 1
CTEX_EXECUTOR_HOST_NOT_ATTACHED = 2
ctex_executor_availability = enum_ctex_executor_availability
enum_ctex_executor_texture_format = c_int
CTEX_EXECUTOR_TEXTURE_R8_UNORM = 0
CTEX_EXECUTOR_TEXTURE_RG8_UNORM = 1
CTEX_EXECUTOR_TEXTURE_RGBA8_UNORM = 2
CTEX_EXECUTOR_TEXTURE_R16_UNORM = 3
CTEX_EXECUTOR_TEXTURE_RG16_UNORM = 4
CTEX_EXECUTOR_TEXTURE_RGBA16_UNORM = 5
CTEX_EXECUTOR_TEXTURE_R16_FLOAT = 6
CTEX_EXECUTOR_TEXTURE_RG16_FLOAT = 7
CTEX_EXECUTOR_TEXTURE_RGBA16_FLOAT = 8
CTEX_EXECUTOR_TEXTURE_R32_FLOAT = 9
CTEX_EXECUTOR_TEXTURE_RG32_FLOAT = 10
CTEX_EXECUTOR_TEXTURE_RGBA32_FLOAT = 11
CTEX_EXECUTOR_TEXTURE_DEPTH32_FLOAT = 12
ctex_executor_texture_format = enum_ctex_executor_texture_format

class struct_ctex_host_executor_descriptor(Structure):
    pass

struct_ctex_host_executor_descriptor.__slots__ = [
    'size',
    'device_name',
    'binding_budget',
    'maximum_texture_dimension',
    'supported_texture_formats',
    'supported_texture_format_count',
    'floating_point_filtering',
    'compute_available',
    'attached',
]
struct_ctex_host_executor_descriptor._fields_ = [
    ('size', uint32_t),
    ('device_name', String),
    ('binding_budget', uint32_t),
    ('maximum_texture_dimension', uint32_t),
    ('supported_texture_formats', POINTER(uint32_t)),
    ('supported_texture_format_count', c_size_t),
    ('floating_point_filtering', uint32_t),
    ('compute_available', uint32_t),
    ('attached', uint32_t),
]

ctex_host_executor_descriptor = struct_ctex_host_executor_descriptor

class struct_ctex_executor_info(Structure):
    pass

struct_ctex_executor_info.__slots__ = [
    'size',
    'route',
    'availability',
    'binding_budget',
    'maximum_texture_dimension',
    'supported_texture_format_count',
    'floating_point_filtering',
    'compute_available',
    'required_identifier_size',
    'required_display_name_size',
    'required_device_name_size',
]
struct_ctex_executor_info._fields_ = [
    ('size', uint32_t),
    ('route', uint32_t),
    ('availability', uint32_t),
    ('binding_budget', uint32_t),
    ('maximum_texture_dimension', uint32_t),
    ('supported_texture_format_count', c_size_t),
    ('floating_point_filtering', uint32_t),
    ('compute_available', uint32_t),
    ('required_identifier_size', c_size_t),
    ('required_display_name_size', c_size_t),
    ('required_device_name_size', c_size_t),
]

ctex_executor_info = struct_ctex_executor_info
enum_ctex_executor_selection_source = c_int
CTEX_EXECUTOR_SELECTION_AUTOMATIC = 0
CTEX_EXECUTOR_SELECTION_EXPLICIT = 1
CTEX_EXECUTOR_SELECTION_ENVIRONMENT = 2
ctex_executor_selection_source = enum_ctex_executor_selection_source

class struct_ctex_executor_selection_info(Structure):
    pass

struct_ctex_executor_selection_info.__slots__ = [
    'size',
    'source',
    'selected_executor_index',
    'required_requested_identifier_size',
    'required_message_size',
]
struct_ctex_executor_selection_info._fields_ = [
    ('size', uint32_t),
    ('source', uint32_t),
    ('selected_executor_index', c_size_t),
    ('required_requested_identifier_size', c_size_t),
    ('required_message_size', c_size_t),
]

ctex_executor_selection_info = struct_ctex_executor_selection_info
enum_ctex_execution_failure_code = c_int
CTEX_EXECUTION_FAILURE_DEVICE_UNAVAILABLE = 0
CTEX_EXECUTION_FAILURE_DEVICE_LOST = 1
CTEX_EXECUTION_FAILURE_OPERATION_FAILED = 2
CTEX_EXECUTION_FAILURE_CANCELLED = 3
ctex_execution_failure_code = enum_ctex_execution_failure_code
enum_ctex_executor_fallback_disposition = c_int
CTEX_EXECUTOR_NO_FALLBACK = 0
CTEX_EXECUTOR_CPU_FALLBACK = 1
CTEX_EXECUTOR_RECOVERY_REQUIRED = 2
ctex_executor_fallback_disposition = enum_ctex_executor_fallback_disposition

class struct_ctex_executor_fallback_descriptor(Structure):
    pass

struct_ctex_executor_fallback_descriptor.__slots__ = [
    'size',
    'failed_executor',
    'failure',
    'failure_detail',
    'disposition',
    'fallback_executor',
    'recovery_restored',
]
struct_ctex_executor_fallback_descriptor._fields_ = [
    ('size', uint32_t),
    ('failed_executor', String),
    ('failure', uint32_t),
    ('failure_detail', String),
    ('disposition', uint32_t),
    ('fallback_executor', String),
    ('recovery_restored', uint32_t),
]

ctex_executor_fallback_descriptor = struct_ctex_executor_fallback_descriptor

class struct_ctex_executor_fallback_info(Structure):
    pass

struct_ctex_executor_fallback_info.__slots__ = [
    'size',
    'disposition',
    'recovery_restored',
    'required_message_size',
]
struct_ctex_executor_fallback_info._fields_ = [
    ('size', uint32_t),
    ('disposition', uint32_t),
    ('recovery_restored', uint32_t),
    ('required_message_size', c_size_t),
]

ctex_executor_fallback_info = struct_ctex_executor_fallback_info
enum_ctex_cpu_execution_status = c_int
CTEX_CPU_EXECUTION_COMPLETED = 0
CTEX_CPU_EXECUTION_CANCELLED = 1
CTEX_CPU_EXECUTION_MEMORY_CEILING_EXCEEDED = 2
ctex_cpu_execution_status = enum_ctex_cpu_execution_status
ctex_cpu_work_item_callback = CFUNCTYPE(UNCHECKED(ctex_result), c_size_t, POINTER(None), c_size_t, POINTER(None), c_size_t, POINTER(None))
ctex_cpu_commit_callback = CFUNCTYPE(UNCHECKED(ctex_result), POINTER(None), c_size_t, POINTER(None))
ctex_cpu_cancel_callback = CFUNCTYPE(UNCHECKED(uint32_t), POINTER(None))
ctex_cpu_progress_callback = CFUNCTYPE(UNCHECKED(None), c_size_t, c_size_t, POINTER(None))

class struct_ctex_cpu_bounded_execution_descriptor(Structure):
    pass

struct_ctex_cpu_bounded_execution_descriptor.__slots__ = [
    'size',
    'operation',
    'work_item_count',
    'shared_working_memory_bytes',
    'working_memory_bytes_per_worker',
    'maximum_workers',
    'memory_ceiling_bytes',
    'progress_interval',
    'execute_work_item',
    'commit',
    'is_cancelled',
    'report_progress',
    'user_data',
]
struct_ctex_cpu_bounded_execution_descriptor._fields_ = [
    ('size', uint32_t),
    ('operation', String),
    ('work_item_count', c_size_t),
    ('shared_working_memory_bytes', c_size_t),
    ('working_memory_bytes_per_worker', c_size_t),
    ('maximum_workers', c_size_t),
    ('memory_ceiling_bytes', c_size_t),
    ('progress_interval', c_size_t),
    ('execute_work_item', ctex_cpu_work_item_callback),
    ('commit', ctex_cpu_commit_callback),
    ('is_cancelled', ctex_cpu_cancel_callback),
    ('report_progress', ctex_cpu_progress_callback),
    ('user_data', POINTER(None)),
]

ctex_cpu_bounded_execution_descriptor = struct_ctex_cpu_bounded_execution_descriptor

class struct_ctex_cpu_execution_info(Structure):
    pass

struct_ctex_cpu_execution_info.__slots__ = [
    'size',
    'status',
    'completed_work_items',
    'total_work_items',
    'required_memory_bytes',
    'worker_count',
    'required_message_size',
]
struct_ctex_cpu_execution_info._fields_ = [
    ('size', uint32_t),
    ('status', uint32_t),
    ('completed_work_items', c_size_t),
    ('total_work_items', c_size_t),
    ('required_memory_bytes', c_size_t),
    ('worker_count', c_size_t),
    ('required_message_size', c_size_t),
]

ctex_cpu_execution_info = struct_ctex_cpu_execution_info

class struct_ctex_cpu_raster_mesh_descriptor(Structure):
    pass

struct_ctex_cpu_raster_mesh_descriptor.__slots__ = [
    'size',
    'positions',
    'uv',
    'vertex_count',
    'triangle_indices',
    'triangle_index_count',
]
struct_ctex_cpu_raster_mesh_descriptor._fields_ = [
    ('size', uint32_t),
    ('positions', POINTER(ctex_vec3f)),
    ('uv', POINTER(ctex_vec2f)),
    ('vertex_count', c_size_t),
    ('triangle_indices', POINTER(uint32_t)),
    ('triangle_index_count', c_size_t),
]

ctex_cpu_raster_mesh_descriptor = struct_ctex_cpu_raster_mesh_descriptor

class struct_ctex_cpu_raster_camera_descriptor(Structure):
    pass

struct_ctex_cpu_raster_camera_descriptor.__slots__ = [
    'size',
    'view_projection',
    'width',
    'height',
]
struct_ctex_cpu_raster_camera_descriptor._fields_ = [
    ('size', uint32_t),
    ('view_projection', c_float * int(16)),
    ('width', uint32_t),
    ('height', uint32_t),
]

ctex_cpu_raster_camera_descriptor = struct_ctex_cpu_raster_camera_descriptor

class struct_ctex_cpu_viewport_raster_descriptor(Structure):
    pass

struct_ctex_cpu_viewport_raster_descriptor.__slots__ = [
    'size',
    'mesh',
    'camera',
    'maximum_output_pixels',
]
struct_ctex_cpu_viewport_raster_descriptor._fields_ = [
    ('size', uint32_t),
    ('mesh', POINTER(ctex_cpu_raster_mesh_descriptor)),
    ('camera', POINTER(ctex_cpu_raster_camera_descriptor)),
    ('maximum_output_pixels', c_size_t),
]

ctex_cpu_viewport_raster_descriptor = struct_ctex_cpu_viewport_raster_descriptor

class struct_ctex_cpu_uv_raster_descriptor(Structure):
    pass

struct_ctex_cpu_uv_raster_descriptor.__slots__ = [
    'size',
    'mesh',
    'camera',
    'width',
    'height',
    'tile_origin',
    'maximum_output_pixels',
]
struct_ctex_cpu_uv_raster_descriptor._fields_ = [
    ('size', uint32_t),
    ('mesh', POINTER(ctex_cpu_raster_mesh_descriptor)),
    ('camera', POINTER(ctex_cpu_raster_camera_descriptor)),
    ('width', uint32_t),
    ('height', uint32_t),
    ('tile_origin', ctex_vec2f),
    ('maximum_output_pixels', c_size_t),
]

ctex_cpu_uv_raster_descriptor = struct_ctex_cpu_uv_raster_descriptor

class struct_ctex_cpu_raster_info(Structure):
    pass

struct_ctex_cpu_raster_info.__slots__ = [
    'size',
    'width',
    'height',
    'pixel_count',
]
struct_ctex_cpu_raster_info._fields_ = [
    ('size', uint32_t),
    ('width', uint32_t),
    ('height', uint32_t),
    ('pixel_count', c_size_t),
]

ctex_cpu_raster_info = struct_ctex_cpu_raster_info

class struct_ctex_cpu_raster_outputs(Structure):
    pass

struct_ctex_cpu_raster_outputs.__slots__ = [
    'size',
    'depth',
    'depth_capacity',
    'coordinates',
    'coordinate_capacity',
    'coverage',
    'coverage_capacity',
    'triangle_identity',
    'triangle_identity_capacity',
]
struct_ctex_cpu_raster_outputs._fields_ = [
    ('size', uint32_t),
    ('depth', POINTER(c_float)),
    ('depth_capacity', c_size_t),
    ('coordinates', POINTER(ctex_vec2f)),
    ('coordinate_capacity', c_size_t),
    ('coverage', POINTER(uint8_t)),
    ('coverage_capacity', c_size_t),
    ('triangle_identity', POINTER(uint32_t)),
    ('triangle_identity_capacity', c_size_t),
]

ctex_cpu_raster_outputs = struct_ctex_cpu_raster_outputs
enum_ctex_parity_value_class = c_int
CTEX_PARITY_UNORM8 = 0
CTEX_PARITY_UNORM16 = 1
CTEX_PARITY_FLOATING_POINT = 2
ctex_parity_value_class = enum_ctex_parity_value_class

class struct_ctex_parity_tolerance_info(Structure):
    pass

struct_ctex_parity_tolerance_info.__slots__ = [
    'size',
    'absolute',
    'relative',
]
struct_ctex_parity_tolerance_info._fields_ = [
    ('size', uint32_t),
    ('absolute', c_double),
    ('relative', c_double),
]

ctex_parity_tolerance_info = struct_ctex_parity_tolerance_info

class struct_ctex_parity_comparison_info(Structure):
    pass

struct_ctex_parity_comparison_info.__slots__ = [
    'size',
    'matches',
    'compared_value_count',
    'maximum_absolute_deviation',
    'has_failure',
    'failure_value_index',
    'failure_reference',
    'failure_measured',
    'failure_absolute_deviation',
    'failure_allowed_deviation',
    'required_message_size',
]
struct_ctex_parity_comparison_info._fields_ = [
    ('size', uint32_t),
    ('matches', uint32_t),
    ('compared_value_count', c_size_t),
    ('maximum_absolute_deviation', c_double),
    ('has_failure', uint32_t),
    ('failure_value_index', c_size_t),
    ('failure_reference', c_double),
    ('failure_measured', c_double),
    ('failure_absolute_deviation', c_double),
    ('failure_allowed_deviation', c_double),
    ('required_message_size', c_size_t),
]

ctex_parity_comparison_info = struct_ctex_parity_comparison_info

class struct_ctex_parity_fixture_channel_descriptor(Structure):
    pass

struct_ctex_parity_fixture_channel_descriptor.__slots__ = [
    'size',
    'semantic',
    'value_class',
    'filtered',
    'component_count',
]
struct_ctex_parity_fixture_channel_descriptor._fields_ = [
    ('size', uint32_t),
    ('semantic', String),
    ('value_class', uint32_t),
    ('filtered', uint32_t),
    ('component_count', uint32_t),
]

ctex_parity_fixture_channel_descriptor = struct_ctex_parity_fixture_channel_descriptor

class struct_ctex_parity_fixture_descriptor(Structure):
    pass

struct_ctex_parity_fixture_descriptor.__slots__ = [
    'size',
    'identifier',
    'document',
    'stroke',
    'camera',
    'material',
    'channels',
    'channel_count',
]
struct_ctex_parity_fixture_descriptor._fields_ = [
    ('size', uint32_t),
    ('identifier', String),
    ('document', String),
    ('stroke', String),
    ('camera', String),
    ('material', String),
    ('channels', POINTER(ctex_parity_fixture_channel_descriptor)),
    ('channel_count', c_size_t),
]

ctex_parity_fixture_descriptor = struct_ctex_parity_fixture_descriptor

class struct_ctex_parity_rendered_channel_descriptor(Structure):
    pass

struct_ctex_parity_rendered_channel_descriptor.__slots__ = [
    'size',
    'semantic',
    'values',
    'value_count',
]
struct_ctex_parity_rendered_channel_descriptor._fields_ = [
    ('size', uint32_t),
    ('semantic', String),
    ('values', POINTER(c_double)),
    ('value_count', c_size_t),
]

ctex_parity_rendered_channel_descriptor = struct_ctex_parity_rendered_channel_descriptor

class struct_ctex_parity_rendered_fixture_descriptor(Structure):
    pass

struct_ctex_parity_rendered_fixture_descriptor.__slots__ = [
    'size',
    'width',
    'height',
    'channels',
    'channel_count',
]
struct_ctex_parity_rendered_fixture_descriptor._fields_ = [
    ('size', uint32_t),
    ('width', uint32_t),
    ('height', uint32_t),
    ('channels', POINTER(ctex_parity_rendered_channel_descriptor)),
    ('channel_count', c_size_t),
]

ctex_parity_rendered_fixture_descriptor = struct_ctex_parity_rendered_fixture_descriptor
ctex_parity_render_callback = CFUNCTYPE(UNCHECKED(ctex_result), POINTER(ctex_parity_fixture_descriptor), POINTER(ctex_parity_rendered_fixture_descriptor), POINTER(None))

class struct_ctex_parity_executor_binding_descriptor(Structure):
    pass

struct_ctex_parity_executor_binding_descriptor.__slots__ = [
    'size',
    'executor_index',
    'render',
    'user_data',
]
struct_ctex_parity_executor_binding_descriptor._fields_ = [
    ('size', uint32_t),
    ('executor_index', c_size_t),
    ('render', ctex_parity_render_callback),
    ('user_data', POINTER(None)),
]

ctex_parity_executor_binding_descriptor = struct_ctex_parity_executor_binding_descriptor

class struct_ctex_parity_gate_info(Structure):
    pass

struct_ctex_parity_gate_info.__slots__ = [
    'size',
    'passed',
    'executor_count',
    'reference_count',
    'passed_count',
    'failed_count',
    'unmeasured_count',
    'required_report_size',
]
struct_ctex_parity_gate_info._fields_ = [
    ('size', uint32_t),
    ('passed', uint32_t),
    ('executor_count', c_size_t),
    ('reference_count', c_size_t),
    ('passed_count', c_size_t),
    ('failed_count', c_size_t),
    ('unmeasured_count', c_size_t),
    ('required_report_size', c_size_t),
]

ctex_parity_gate_info = struct_ctex_parity_gate_info
enum_ctex_host_resource_owner = c_int
CTEX_HOST_RESOURCE_LIBRARY = 0
CTEX_HOST_RESOURCE_HOST = 1
ctex_host_resource_owner = enum_ctex_host_resource_owner
enum_ctex_host_resource_state = c_int
CTEX_HOST_RESOURCE_SHADER_READ = 0
CTEX_HOST_RESOURCE_STORAGE_READ = 1
CTEX_HOST_RESOURCE_STORAGE_WRITE = 2
CTEX_HOST_RESOURCE_RENDER_TARGET = 3
CTEX_HOST_RESOURCE_DEPTH_TARGET = 4
ctex_host_resource_state = enum_ctex_host_resource_state
enum_ctex_host_replay_semantics = c_int
CTEX_HOST_REPLAY_DETERMINISTIC = 0
CTEX_HOST_REPLAY_CHECKPOINT_ONLY = 1
ctex_host_replay_semantics = enum_ctex_host_replay_semantics

class struct_ctex_host_resource_descriptor(Structure):
    pass

struct_ctex_host_resource_descriptor.__slots__ = [
    'size',
    'logical_id',
    'generation',
    'role',
    'format',
    'width',
    'height',
    'layers',
    'mip_levels',
    'tile_width',
    'tile_height',
    'externally_initialized',
    'owner',
    'required_state',
    'output',
]
struct_ctex_host_resource_descriptor._fields_ = [
    ('size', uint32_t),
    ('logical_id', String),
    ('generation', uint64_t),
    ('role', String),
    ('format', uint32_t),
    ('width', uint32_t),
    ('height', uint32_t),
    ('layers', uint32_t),
    ('mip_levels', uint32_t),
    ('tile_width', uint32_t),
    ('tile_height', uint32_t),
    ('externally_initialized', uint32_t),
    ('owner', uint32_t),
    ('required_state', uint32_t),
    ('output', uint32_t),
]

ctex_host_resource_descriptor = struct_ctex_host_resource_descriptor

class struct_ctex_host_submission_descriptor(Structure):
    pass

struct_ctex_host_submission_descriptor.__slots__ = [
    'size',
    'operation',
    'base_revision',
    'resources',
    'resource_count',
    'replay_semantics',
]
struct_ctex_host_submission_descriptor._fields_ = [
    ('size', uint32_t),
    ('operation', String),
    ('base_revision', uint64_t),
    ('resources', POINTER(ctex_host_resource_descriptor)),
    ('resource_count', c_size_t),
    ('replay_semantics', uint32_t),
]

ctex_host_submission_descriptor = struct_ctex_host_submission_descriptor

class struct_ctex_host_submission_info(Structure):
    pass

struct_ctex_host_submission_info.__slots__ = [
    'size',
    'completion_token',
    'base_revision',
    'resource_count',
]
struct_ctex_host_submission_info._fields_ = [
    ('size', uint32_t),
    ('completion_token', uint64_t),
    ('base_revision', uint64_t),
    ('resource_count', c_size_t),
]

ctex_host_submission_info = struct_ctex_host_submission_info

class struct_ctex_host_execution_session_info(Structure):
    pass

struct_ctex_host_execution_session_info.__slots__ = [
    'size',
    'revision',
    'active_submission_count',
    'retained_recovery_bytes',
]
struct_ctex_host_execution_session_info._fields_ = [
    ('size', uint32_t),
    ('revision', uint64_t),
    ('active_submission_count', c_size_t),
    ('retained_recovery_bytes', c_size_t),
]

ctex_host_execution_session_info = struct_ctex_host_execution_session_info
enum_ctex_host_execution_status = c_int
CTEX_HOST_EXECUTION_SUCCEEDED = 0
CTEX_HOST_EXECUTION_FAILED = 1
CTEX_HOST_EXECUTION_CANCELLED = 2
ctex_host_execution_status = enum_ctex_host_execution_status

class struct_ctex_host_completed_resource_descriptor(Structure):
    pass

struct_ctex_host_completed_resource_descriptor.__slots__ = [
    'size',
    'logical_id',
    'generation',
    'format',
    'width',
    'height',
    'layers',
]
struct_ctex_host_completed_resource_descriptor._fields_ = [
    ('size', uint32_t),
    ('logical_id', String),
    ('generation', uint64_t),
    ('format', uint32_t),
    ('width', uint32_t),
    ('height', uint32_t),
    ('layers', uint32_t),
]

ctex_host_completed_resource_descriptor = struct_ctex_host_completed_resource_descriptor
enum_ctex_host_recovery_kind = c_int
CTEX_HOST_RECOVERY_RESULT_CHECKPOINT = 0
CTEX_HOST_RECOVERY_DETERMINISTIC_RECORD = 1
ctex_host_recovery_kind = enum_ctex_host_recovery_kind

class struct_ctex_host_recovery_descriptor(Structure):
    pass

struct_ctex_host_recovery_descriptor.__slots__ = [
    'size',
    'kind',
    'checkpoint_complete',
    'checkpoint_revision',
    'operation_record_version',
    'inputs_pinned',
    'retained_bytes',
]
struct_ctex_host_recovery_descriptor._fields_ = [
    ('size', uint32_t),
    ('kind', uint32_t),
    ('checkpoint_complete', uint32_t),
    ('checkpoint_revision', uint64_t),
    ('operation_record_version', String),
    ('inputs_pinned', uint32_t),
    ('retained_bytes', c_size_t),
]

ctex_host_recovery_descriptor = struct_ctex_host_recovery_descriptor

class struct_ctex_host_completion_descriptor(Structure):
    pass

struct_ctex_host_completion_descriptor.__slots__ = [
    'size',
    'completion_token',
    'status',
    'outputs',
    'output_count',
    'recovery',
    'detail',
]
struct_ctex_host_completion_descriptor._fields_ = [
    ('size', uint32_t),
    ('completion_token', uint64_t),
    ('status', uint32_t),
    ('outputs', POINTER(ctex_host_completed_resource_descriptor)),
    ('output_count', c_size_t),
    ('recovery', POINTER(ctex_host_recovery_descriptor)),
    ('detail', String),
]

ctex_host_completion_descriptor = struct_ctex_host_completion_descriptor
enum_ctex_host_completion_disposition = c_int
CTEX_HOST_COMPLETION_PUBLISHED = 0
CTEX_HOST_COMPLETION_AWAITING_RECOVERY = 1
CTEX_HOST_COMPLETION_STALE = 2
CTEX_HOST_COMPLETION_CANCELLED = 3
CTEX_HOST_COMPLETION_FAILED = 4
CTEX_HOST_COMPLETION_REJECTED = 5
CTEX_HOST_COMPLETION_DUPLICATE = 6
CTEX_HOST_COMPLETION_UNKNOWN_TOKEN = 7
ctex_host_completion_disposition = enum_ctex_host_completion_disposition

class struct_ctex_host_resource_version(Structure):
    pass

struct_ctex_host_resource_version.__slots__ = [
    'logical_id_offset',
    'generation',
]
struct_ctex_host_resource_version._fields_ = [
    ('logical_id_offset', c_size_t),
    ('generation', uint64_t),
]

ctex_host_resource_version = struct_ctex_host_resource_version

class struct_ctex_host_completion_result_info(Structure):
    pass

struct_ctex_host_completion_result_info.__slots__ = [
    'size',
    'disposition',
    'completion_token',
    'has_published_revision',
    'published_revision',
    'released_resource_count',
    'required_released_identity_size',
    'required_message_size',
]
struct_ctex_host_completion_result_info._fields_ = [
    ('size', uint32_t),
    ('disposition', uint32_t),
    ('completion_token', uint64_t),
    ('has_published_revision', uint32_t),
    ('published_revision', uint64_t),
    ('released_resource_count', c_size_t),
    ('required_released_identity_size', c_size_t),
    ('required_message_size', c_size_t),
]

ctex_host_completion_result_info = struct_ctex_host_completion_result_info

class struct_ctex_host_device_loss_info(Structure):
    pass

struct_ctex_host_device_loss_info.__slots__ = [
    'size',
    'recovered_revision',
    'cancelled_submission_count',
    'released_resource_count',
    'required_released_identity_size',
    'retained_recovery_bytes',
    'restored',
]
struct_ctex_host_device_loss_info._fields_ = [
    ('size', uint32_t),
    ('recovered_revision', uint64_t),
    ('cancelled_submission_count', c_size_t),
    ('released_resource_count', c_size_t),
    ('required_released_identity_size', c_size_t),
    ('retained_recovery_bytes', c_size_t),
    ('restored', uint32_t),
]

ctex_host_device_loss_info = struct_ctex_host_device_loss_info
enum_ctex_texture_export_texture_set_selection = c_int
CTEX_TEXTURE_EXPORT_TEXTURE_SET_ALL = 0
CTEX_TEXTURE_EXPORT_TEXTURE_SET_SELECTED = 1
ctex_texture_export_texture_set_selection = enum_ctex_texture_export_texture_set_selection
enum_ctex_texture_export_spatial_scope = c_int
CTEX_TEXTURE_EXPORT_SCOPE_TEXTURE_SET = 0
CTEX_TEXTURE_EXPORT_SCOPE_UDIM = 1
CTEX_TEXTURE_EXPORT_SCOPE_ATLAS = 2
ctex_texture_export_spatial_scope = enum_ctex_texture_export_spatial_scope
enum_ctex_texture_export_layer_scope = c_int
CTEX_TEXTURE_EXPORT_LAYER_FLATTEN_VISIBLE = 0
CTEX_TEXTURE_EXPORT_LAYER_FLATTEN_SELECTED = 1
CTEX_TEXTURE_EXPORT_LAYER_EACH_SELECTED = 2
ctex_texture_export_layer_scope = enum_ctex_texture_export_layer_scope
enum_ctex_texture_export_layer_kind = c_int
CTEX_TEXTURE_EXPORT_LAYER_CONTENT = 0
CTEX_TEXTURE_EXPORT_LAYER_GROUP = 1
ctex_texture_export_layer_kind = enum_ctex_texture_export_layer_kind

class struct_ctex_texture_export_layer_source_descriptor(Structure):
    pass

struct_ctex_texture_export_layer_source_descriptor.__slots__ = [
    'size',
    'identifier',
    'display_name',
    'parent_identifier',
    'kind',
    'visible',
]
struct_ctex_texture_export_layer_source_descriptor._fields_ = [
    ('size', uint32_t),
    ('identifier', String),
    ('display_name', String),
    ('parent_identifier', String),
    ('kind', uint32_t),
    ('visible', uint32_t),
]

ctex_texture_export_layer_source_descriptor = struct_ctex_texture_export_layer_source_descriptor

class struct_ctex_texture_export_texture_set_source_descriptor(Structure):
    pass

struct_ctex_texture_export_texture_set_source_descriptor.__slots__ = [
    'size',
    'identifier',
    'display_name',
    'width',
    'height',
    'occupied_udim_tiles',
    'occupied_udim_tile_count',
    'layers',
    'layer_count',
]
struct_ctex_texture_export_texture_set_source_descriptor._fields_ = [
    ('size', uint32_t),
    ('identifier', String),
    ('display_name', String),
    ('width', uint32_t),
    ('height', uint32_t),
    ('occupied_udim_tiles', POINTER(uint32_t)),
    ('occupied_udim_tile_count', c_size_t),
    ('layers', POINTER(ctex_texture_export_layer_source_descriptor)),
    ('layer_count', c_size_t),
]

ctex_texture_export_texture_set_source_descriptor = struct_ctex_texture_export_texture_set_source_descriptor

class struct_ctex_texture_export_atlas_source_descriptor(Structure):
    pass

struct_ctex_texture_export_atlas_source_descriptor.__slots__ = [
    'size',
    'identifier',
    'display_name',
    'width',
    'height',
    'texture_set_identifiers',
    'texture_set_identifier_count',
]
struct_ctex_texture_export_atlas_source_descriptor._fields_ = [
    ('size', uint32_t),
    ('identifier', String),
    ('display_name', String),
    ('width', uint32_t),
    ('height', uint32_t),
    ('texture_set_identifiers', POINTER(POINTER(c_char))),
    ('texture_set_identifier_count', c_size_t),
]

ctex_texture_export_atlas_source_descriptor = struct_ctex_texture_export_atlas_source_descriptor

class struct_ctex_texture_export_catalogue_descriptor(Structure):
    pass

struct_ctex_texture_export_catalogue_descriptor.__slots__ = [
    'size',
    'project_name',
    'texture_sets',
    'texture_set_count',
    'atlases',
    'atlas_count',
]
struct_ctex_texture_export_catalogue_descriptor._fields_ = [
    ('size', uint32_t),
    ('project_name', String),
    ('texture_sets', POINTER(ctex_texture_export_texture_set_source_descriptor)),
    ('texture_set_count', c_size_t),
    ('atlases', POINTER(ctex_texture_export_atlas_source_descriptor)),
    ('atlas_count', c_size_t),
]

ctex_texture_export_catalogue_descriptor = struct_ctex_texture_export_catalogue_descriptor

class struct_ctex_texture_export_layer_selection_descriptor(Structure):
    pass

struct_ctex_texture_export_layer_selection_descriptor.__slots__ = [
    'size',
    'texture_set_identifier',
    'layer_identifiers',
    'layer_identifier_count',
]
struct_ctex_texture_export_layer_selection_descriptor._fields_ = [
    ('size', uint32_t),
    ('texture_set_identifier', String),
    ('layer_identifiers', POINTER(POINTER(c_char))),
    ('layer_identifier_count', c_size_t),
]

ctex_texture_export_layer_selection_descriptor = struct_ctex_texture_export_layer_selection_descriptor

class struct_ctex_texture_export_plan_descriptor(Structure):
    pass

struct_ctex_texture_export_plan_descriptor.__slots__ = [
    'size',
    'texture_set_selection',
    'selected_texture_set_identifiers',
    'selected_texture_set_identifier_count',
    'spatial_scope',
    'layer_scope',
    'selected_layers',
    'selected_layer_count',
    'output_width',
    'output_height',
    'filename_pattern',
]
struct_ctex_texture_export_plan_descriptor._fields_ = [
    ('size', uint32_t),
    ('texture_set_selection', uint32_t),
    ('selected_texture_set_identifiers', POINTER(POINTER(c_char))),
    ('selected_texture_set_identifier_count', c_size_t),
    ('spatial_scope', uint32_t),
    ('layer_scope', uint32_t),
    ('selected_layers', POINTER(ctex_texture_export_layer_selection_descriptor)),
    ('selected_layer_count', c_size_t),
    ('output_width', uint32_t),
    ('output_height', uint32_t),
    ('filename_pattern', String),
]

ctex_texture_export_plan_descriptor = struct_ctex_texture_export_plan_descriptor

class struct_ctex_texture_export_texture_descriptor(Structure):
    pass

struct_ctex_texture_export_texture_descriptor.__slots__ = [
    'size',
    'suffix',
    'channel_tokens',
    'color_space',
    'bit_depth',
    'format',
]
struct_ctex_texture_export_texture_descriptor._fields_ = [
    ('size', uint32_t),
    ('suffix', String),
    ('channel_tokens', POINTER(c_char) * int(4)),
    ('color_space', uint32_t),
    ('bit_depth', uint32_t),
    ('format', uint32_t),
]

ctex_texture_export_texture_descriptor = struct_ctex_texture_export_texture_descriptor

class struct_ctex_texture_export_preset_descriptor(Structure):
    pass

struct_ctex_texture_export_preset_descriptor.__slots__ = [
    'size',
    'identifier',
    'display_name',
    'textures',
    'texture_count',
]
struct_ctex_texture_export_preset_descriptor._fields_ = [
    ('size', uint32_t),
    ('identifier', String),
    ('display_name', String),
    ('textures', POINTER(ctex_texture_export_texture_descriptor)),
    ('texture_count', c_size_t),
]

ctex_texture_export_preset_descriptor = struct_ctex_texture_export_preset_descriptor

class struct_ctex_texture_export_options_descriptor(Structure):
    pass

struct_ctex_texture_export_options_descriptor.__slots__ = [
    'size',
    'plan',
    'padding_radius',
    'jpeg_quality',
    'dry_run',
]
struct_ctex_texture_export_options_descriptor._fields_ = [
    ('size', uint32_t),
    ('plan', POINTER(ctex_texture_export_plan_descriptor)),
    ('padding_radius', uint32_t),
    ('jpeg_quality', uint32_t),
    ('dry_run', uint32_t),
]

ctex_texture_export_options_descriptor = struct_ctex_texture_export_options_descriptor

class struct_ctex_texture_export_named_value(Structure):
    pass

struct_ctex_texture_export_named_value.__slots__ = [
    'size',
    'identifier',
    'component_count',
    'components',
]
struct_ctex_texture_export_named_value._fields_ = [
    ('size', uint32_t),
    ('identifier', String),
    ('component_count', uint32_t),
    ('components', c_double * int(4)),
]

ctex_texture_export_named_value = struct_ctex_texture_export_named_value

class struct_ctex_texture_export_sample(Structure):
    pass

struct_ctex_texture_export_sample.__slots__ = [
    'size',
    'base_color',
    'opacity',
    'roughness',
    'metallic',
    'normal',
    'height',
    'occlusion',
    'emission',
    'subsurface',
    'mesh_maps',
    'mesh_map_count',
    'registered_channels',
    'registered_channel_count',
]
struct_ctex_texture_export_sample._fields_ = [
    ('size', uint32_t),
    ('base_color', c_double * int(3)),
    ('opacity', c_double),
    ('roughness', c_double),
    ('metallic', c_double),
    ('normal', c_double * int(3)),
    ('height', c_double),
    ('occlusion', c_double),
    ('emission', c_double * int(3)),
    ('subsurface', c_double),
    ('mesh_maps', POINTER(ctex_texture_export_named_value)),
    ('mesh_map_count', c_size_t),
    ('registered_channels', POINTER(ctex_texture_export_named_value)),
    ('registered_channel_count', c_size_t),
]

ctex_texture_export_sample = struct_ctex_texture_export_sample
ctex_texture_export_sample_callback = CFUNCTYPE(UNCHECKED(ctex_result), uint32_t, uint32_t, POINTER(ctex_texture_export_sample), POINTER(None))

class struct_ctex_texture_export_pixel_source_descriptor(Structure):
    pass

struct_ctex_texture_export_pixel_source_descriptor.__slots__ = [
    'size',
    'width',
    'height',
    'sample',
    'sample_user_data',
    'coverage',
    'coverage_count',
]
struct_ctex_texture_export_pixel_source_descriptor._fields_ = [
    ('size', uint32_t),
    ('width', uint32_t),
    ('height', uint32_t),
    ('sample', ctex_texture_export_sample_callback),
    ('sample_user_data', POINTER(None)),
    ('coverage', POINTER(uint8_t)),
    ('coverage_count', c_size_t),
]

ctex_texture_export_pixel_source_descriptor = struct_ctex_texture_export_pixel_source_descriptor

class struct_ctex_texture_export_layer_selection_view(Structure):
    pass

struct_ctex_texture_export_layer_selection_view.__slots__ = [
    'size',
    'texture_set_identifier',
    'layer_identifiers',
    'layer_identifier_count',
]
struct_ctex_texture_export_layer_selection_view._fields_ = [
    ('size', uint32_t),
    ('texture_set_identifier', String),
    ('layer_identifiers', POINTER(POINTER(c_char))),
    ('layer_identifier_count', c_size_t),
]

ctex_texture_export_layer_selection_view = struct_ctex_texture_export_layer_selection_view

class struct_ctex_texture_export_planned_output(Structure):
    pass

struct_ctex_texture_export_planned_output.__slots__ = [
    'size',
    'relative_path',
    'texture_set_identifiers',
    'texture_set_identifier_count',
    'has_udim_tile',
    'udim_tile',
    'atlas_identifier',
    'layer_selections',
    'layer_selection_count',
    'preset_texture_index',
    'width',
    'height',
    'format',
    'bit_depth',
    'color_space',
]
struct_ctex_texture_export_planned_output._fields_ = [
    ('size', uint32_t),
    ('relative_path', String),
    ('texture_set_identifiers', POINTER(POINTER(c_char))),
    ('texture_set_identifier_count', c_size_t),
    ('has_udim_tile', uint32_t),
    ('udim_tile', uint32_t),
    ('atlas_identifier', String),
    ('layer_selections', POINTER(ctex_texture_export_layer_selection_view)),
    ('layer_selection_count', c_size_t),
    ('preset_texture_index', c_size_t),
    ('width', uint32_t),
    ('height', uint32_t),
    ('format', uint32_t),
    ('bit_depth', uint32_t),
    ('color_space', uint32_t),
]

ctex_texture_export_planned_output = struct_ctex_texture_export_planned_output

class struct_ctex_texture_export_encoded_output(Structure):
    pass

struct_ctex_texture_export_encoded_output.__slots__ = [
    'size',
    'report_entry_index',
    'relative_path',
    'width',
    'height',
    'format',
    'bit_depth',
    'color_space',
    'bytes',
    'byte_count',
]
struct_ctex_texture_export_encoded_output._fields_ = [
    ('size', uint32_t),
    ('report_entry_index', c_size_t),
    ('relative_path', String),
    ('width', uint32_t),
    ('height', uint32_t),
    ('format', uint32_t),
    ('bit_depth', uint32_t),
    ('color_space', uint32_t),
    ('bytes', POINTER(None)),
    ('byte_count', c_size_t),
]

ctex_texture_export_encoded_output = struct_ctex_texture_export_encoded_output
ctex_texture_export_source_callback = CFUNCTYPE(UNCHECKED(ctex_result), POINTER(ctex_texture_export_planned_output), POINTER(ctex_texture_export_pixel_source_descriptor), POINTER(None))
ctex_texture_export_output_callback = CFUNCTYPE(UNCHECKED(ctex_result), POINTER(ctex_texture_export_encoded_output), POINTER(None))
ctex_texture_export_report_callback = CFUNCTYPE(UNCHECKED(ctex_result), String, c_size_t, POINTER(None))
ctex_texture_export_progress_callback = CFUNCTYPE(UNCHECKED(None), c_size_t, c_size_t, String, POINTER(None))
ctex_texture_export_cancel_callback = CFUNCTYPE(UNCHECKED(uint32_t), POINTER(None))

class struct_ctex_texture_export_callbacks_descriptor(Structure):
    pass

struct_ctex_texture_export_callbacks_descriptor.__slots__ = [
    'size',
    'source',
    'output',
    'report',
    'progress',
    'cancel',
    'user_data',
]
struct_ctex_texture_export_callbacks_descriptor._fields_ = [
    ('size', uint32_t),
    ('source', ctex_texture_export_source_callback),
    ('output', ctex_texture_export_output_callback),
    ('report', ctex_texture_export_report_callback),
    ('progress', ctex_texture_export_progress_callback),
    ('cancel', ctex_texture_export_cancel_callback),
    ('user_data', POINTER(None)),
]

ctex_texture_export_callbacks_descriptor = struct_ctex_texture_export_callbacks_descriptor

class struct_ctex_texture_export_info(Structure):
    pass

struct_ctex_texture_export_info.__slots__ = [
    'size',
    'planned_output_count',
    'encoded_output_count',
    'dry_run',
    'cancelled',
]
struct_ctex_texture_export_info._fields_ = [
    ('size', uint32_t),
    ('planned_output_count', c_size_t),
    ('encoded_output_count', c_size_t),
    ('dry_run', uint32_t),
    ('cancelled', uint32_t),
]

ctex_texture_export_info = struct_ctex_texture_export_info

class struct_ctex_project_container_read_limits_descriptor(Structure):
    pass

struct_ctex_project_container_read_limits_descriptor.__slots__ = [
    'size',
    'maximum_input_bytes',
    'maximum_total_allocation_bytes',
    'maximum_sections',
    'maximum_images',
    'maximum_tiles',
    'maximum_resources',
    'maximum_assets',
    'maximum_asset_dependencies',
    'maximum_string_bytes',
    'maximum_decoded_tile_bytes',
    'maximum_packed_resource_bytes',
    'maximum_asset_payload_bytes',
]
struct_ctex_project_container_read_limits_descriptor._fields_ = [
    ('size', uint32_t),
    ('maximum_input_bytes', c_size_t),
    ('maximum_total_allocation_bytes', c_size_t),
    ('maximum_sections', c_size_t),
    ('maximum_images', c_size_t),
    ('maximum_tiles', c_size_t),
    ('maximum_resources', c_size_t),
    ('maximum_assets', c_size_t),
    ('maximum_asset_dependencies', c_size_t),
    ('maximum_string_bytes', c_size_t),
    ('maximum_decoded_tile_bytes', c_size_t),
    ('maximum_packed_resource_bytes', c_size_t),
    ('maximum_asset_payload_bytes', c_size_t),
]

ctex_project_container_read_limits_descriptor = struct_ctex_project_container_read_limits_descriptor

class struct_ctex_project_container_version(Structure):
    pass

struct_ctex_project_container_version.__slots__ = [
    'size',
    'major',
    'minor',
    'patch',
]
struct_ctex_project_container_version._fields_ = [
    ('size', uint32_t),
    ('major', uint32_t),
    ('minor', uint32_t),
    ('patch', uint32_t),
]

ctex_project_container_version = struct_ctex_project_container_version

class struct_ctex_project_container_info(Structure):
    pass

struct_ctex_project_container_info.__slots__ = [
    'size',
    'source_schema',
    'newer_schema',
    'tiled_image_count',
    'resource_count',
    'asset_count',
    'opaque_section_count',
    'occupied_tile_count',
    'packed_resource_bytes',
    'canonical_size',
    'report_size',
]
struct_ctex_project_container_info._fields_ = [
    ('size', uint32_t),
    ('source_schema', ctex_project_container_version),
    ('newer_schema', uint32_t),
    ('tiled_image_count', c_size_t),
    ('resource_count', c_size_t),
    ('asset_count', c_size_t),
    ('opaque_section_count', c_size_t),
    ('occupied_tile_count', c_size_t),
    ('packed_resource_bytes', c_size_t),
    ('canonical_size', c_size_t),
    ('report_size', c_size_t),
]

ctex_project_container_info = struct_ctex_project_container_info

class struct_ctex_project_autosave_config_descriptor(Structure):
    pass

struct_ctex_project_autosave_config_descriptor.__slots__ = [
    'size',
    'recovery_directory',
    'recovery_key',
    'interval_milliseconds',
]
struct_ctex_project_autosave_config_descriptor._fields_ = [
    ('size', uint32_t),
    ('recovery_directory', String),
    ('recovery_key', String),
    ('interval_milliseconds', uint64_t),
]

ctex_project_autosave_config_descriptor = struct_ctex_project_autosave_config_descriptor
enum_ctex_project_autosave_submission_status = c_int
CTEX_PROJECT_AUTOSAVE_QUEUED = 0
CTEX_PROJECT_AUTOSAVE_STALE_REVISION = 1
ctex_project_autosave_submission_status = enum_ctex_project_autosave_submission_status

class struct_ctex_project_autosave_info(Structure):
    pass

struct_ctex_project_autosave_info.__slots__ = [
    'size',
    'has_last_saved_revision',
    'last_saved_revision',
    'has_pending_revision',
    'pending_revision',
    'has_saving_revision',
    'saving_revision',
    'successful_writes',
    'required_recovery_path_size',
    'required_last_error_size',
]
struct_ctex_project_autosave_info._fields_ = [
    ('size', uint32_t),
    ('has_last_saved_revision', uint32_t),
    ('last_saved_revision', uint64_t),
    ('has_pending_revision', uint32_t),
    ('pending_revision', uint64_t),
    ('has_saving_revision', uint32_t),
    ('saving_revision', uint64_t),
    ('successful_writes', uint64_t),
    ('required_recovery_path_size', c_size_t),
    ('required_last_error_size', c_size_t),
]

ctex_project_autosave_info = struct_ctex_project_autosave_info
ctex_project_quiesce_cancel_callback = CFUNCTYPE(UNCHECKED(None), POINTER(None))

class struct_ctex_project_quiesce_descriptor(Structure):
    pass

struct_ctex_project_quiesce_descriptor.__slots__ = [
    'size',
    'current_revision',
    'deadline_milliseconds',
    'request_cancel',
    'user_data',
]
struct_ctex_project_quiesce_descriptor._fields_ = [
    ('size', uint32_t),
    ('current_revision', uint64_t),
    ('deadline_milliseconds', uint64_t),
    ('request_cancel', ctex_project_quiesce_cancel_callback),
    ('user_data', POINTER(None)),
]

ctex_project_quiesce_descriptor = struct_ctex_project_quiesce_descriptor
enum_ctex_project_quiesce_status = c_int
CTEX_PROJECT_QUIESCE_DURABLE = 0
CTEX_PROJECT_QUIESCE_DEADLINE_EXCEEDED = 1
CTEX_PROJECT_QUIESCE_CHECKPOINT_FAILED = 2
ctex_project_quiesce_status = enum_ctex_project_quiesce_status

class struct_ctex_project_quiesce_report(Structure):
    pass

struct_ctex_project_quiesce_report.__slots__ = [
    'size',
    'status',
    'admissions_stopped',
    'cancellation_requested',
    'work_drained',
    'active_operation_count',
    'has_durable_revision',
    'durable_revision',
    'has_uncheckpointed_range',
    'uncheckpointed_first_revision',
    'uncheckpointed_last_revision',
]
struct_ctex_project_quiesce_report._fields_ = [
    ('size', uint32_t),
    ('status', uint32_t),
    ('admissions_stopped', uint32_t),
    ('cancellation_requested', uint32_t),
    ('work_drained', uint32_t),
    ('active_operation_count', c_size_t),
    ('has_durable_revision', uint32_t),
    ('durable_revision', uint64_t),
    ('has_uncheckpointed_range', uint32_t),
    ('uncheckpointed_first_revision', uint64_t),
    ('uncheckpointed_last_revision', uint64_t),
]

ctex_project_quiesce_report = struct_ctex_project_quiesce_report

class struct_ctex_project_recovery_checkpoint_info(Structure):
    pass

struct_ctex_project_recovery_checkpoint_info.__slots__ = [
    'size',
    'has_revision',
    'revision',
]
struct_ctex_project_recovery_checkpoint_info._fields_ = [
    ('size', uint32_t),
    ('has_revision', uint32_t),
    ('revision', uint64_t),
]

ctex_project_recovery_checkpoint_info = struct_ctex_project_recovery_checkpoint_info

class struct_ctex_project_recovery_entry(Structure):
    pass

struct_ctex_project_recovery_entry.__slots__ = [
    'recovery_key_offset',
    'recovery_key_size',
    'path_offset',
    'path_size',
    'schema',
    'file_bytes',
]
struct_ctex_project_recovery_entry._fields_ = [
    ('recovery_key_offset', c_size_t),
    ('recovery_key_size', c_size_t),
    ('path_offset', c_size_t),
    ('path_size', c_size_t),
    ('schema', ctex_project_container_version),
    ('file_bytes', uint64_t),
]

ctex_project_recovery_entry = struct_ctex_project_recovery_entry

class struct_ctex_project_recovery_rejection(Structure):
    pass

struct_ctex_project_recovery_rejection.__slots__ = [
    'path_offset',
    'path_size',
    'message_offset',
    'message_size',
]
struct_ctex_project_recovery_rejection._fields_ = [
    ('path_offset', c_size_t),
    ('path_size', c_size_t),
    ('message_offset', c_size_t),
    ('message_size', c_size_t),
]

ctex_project_recovery_rejection = struct_ctex_project_recovery_rejection

class struct_ctex_project_recovery_enumeration_info(Structure):
    pass

struct_ctex_project_recovery_enumeration_info.__slots__ = [
    'size',
    'required_recoverable_count',
    'required_rejected_count',
    'required_string_size',
]
struct_ctex_project_recovery_enumeration_info._fields_ = [
    ('size', uint32_t),
    ('required_recoverable_count', c_size_t),
    ('required_rejected_count', c_size_t),
    ('required_string_size', c_size_t),
]

ctex_project_recovery_enumeration_info = struct_ctex_project_recovery_enumeration_info

class struct_ctex_project_asset_export_options_descriptor(Structure):
    pass

struct_ctex_project_asset_export_options_descriptor.__slots__ = [
    'size',
    'self_contained',
    'source_directory',
]
struct_ctex_project_asset_export_options_descriptor._fields_ = [
    ('size', uint32_t),
    ('self_contained', uint32_t),
    ('source_directory', String),
]

ctex_project_asset_export_options_descriptor = struct_ctex_project_asset_export_options_descriptor

class struct_ctex_project_asset_search_paths_descriptor(Structure):
    pass

struct_ctex_project_asset_search_paths_descriptor.__slots__ = [
    'size',
    'paths',
    'path_count',
]
struct_ctex_project_asset_search_paths_descriptor._fields_ = [
    ('size', uint32_t),
    ('paths', POINTER(POINTER(c_char))),
    ('path_count', c_size_t),
]

ctex_project_asset_search_paths_descriptor = struct_ctex_project_asset_search_paths_descriptor

class struct_ctex_project_resource_descriptor(Structure):
    pass

struct_ctex_project_resource_descriptor.__slots__ = [
    'size',
    'identifier',
    'kind',
    'relative_path',
    'packed',
    'packed_bytes',
    'packed_byte_count',
]
struct_ctex_project_resource_descriptor._fields_ = [
    ('size', uint32_t),
    ('identifier', String),
    ('kind', String),
    ('relative_path', String),
    ('packed', uint32_t),
    ('packed_bytes', POINTER(None)),
    ('packed_byte_count', c_size_t),
]

ctex_project_resource_descriptor = struct_ctex_project_resource_descriptor
enum_ctex_operation_replay_class = c_int
CTEX_OPERATION_REPLAY_CHECKPOINT_ONLY = 0
CTEX_OPERATION_REPLAY_SAME_RESOLUTION = 1
CTEX_OPERATION_REPLAY_RESOLUTION_INDEPENDENT = 2
ctex_operation_replay_class = enum_ctex_operation_replay_class
enum_ctex_operation_payload_kind = c_int
CTEX_OPERATION_PAYLOAD_RESOLVED_STAMPS = 0
CTEX_OPERATION_PAYLOAD_EDITABLE_SOURCE_PATH = 1
CTEX_OPERATION_PAYLOAD_OPAQUE_ALGORITHM_DATA = 2
ctex_operation_payload_kind = enum_ctex_operation_payload_kind
enum_ctex_operation_replay_disposition = c_int
CTEX_OPERATION_REPLAY_SAME_RESOLUTION_AVAILABLE = 0
CTEX_OPERATION_REPLAY_RESOLUTION_INDEPENDENT_AVAILABLE = 1
CTEX_OPERATION_REPLAY_CHECKPOINT_ONLY_AVAILABLE = 2
CTEX_OPERATION_REPLAY_RESAMPLE_CHECKPOINT_REQUIRED = 3
CTEX_OPERATION_REPLAY_UNSUPPORTED_ALGORITHM = 4
ctex_operation_replay_disposition = enum_ctex_operation_replay_disposition

class struct_ctex_operation_channel_descriptor(Structure):
    pass

struct_ctex_operation_channel_descriptor.__slots__ = [
    'size',
    'semantic_id',
    'component_count',
    'scalar_representation',
    'bit_depth',
    'color_space',
    'default_value',
    'default_value_count',
]
struct_ctex_operation_channel_descriptor._fields_ = [
    ('size', uint32_t),
    ('semantic_id', String),
    ('component_count', uint32_t),
    ('scalar_representation', uint32_t),
    ('bit_depth', uint32_t),
    ('color_space', uint32_t),
    ('default_value', POINTER(c_double)),
    ('default_value_count', c_size_t),
]

ctex_operation_channel_descriptor = struct_ctex_operation_channel_descriptor

class struct_ctex_pinned_operation_resource_descriptor(Structure):
    pass

struct_ctex_pinned_operation_resource_descriptor.__slots__ = [
    'size',
    'role',
    'content_identity',
    'bytes',
    'byte_count',
]
struct_ctex_pinned_operation_resource_descriptor._fields_ = [
    ('size', uint32_t),
    ('role', String),
    ('content_identity', String),
    ('bytes', POINTER(None)),
    ('byte_count', c_size_t),
]

ctex_pinned_operation_resource_descriptor = struct_ctex_pinned_operation_resource_descriptor

class struct_ctex_operation_record_descriptor(Structure):
    pass

struct_ctex_operation_record_descriptor.__slots__ = [
    'size',
    'identifier',
    'algorithm_identifier',
    'algorithm_version',
    'preset_identifier',
    'preset_version',
    'replay_class',
    'input_document_revision',
    'seed',
    'mesh_content_identity',
    'coordinate_frame',
    'payload_kind',
    'payload_version',
    'channels',
    'channel_count',
    'pinned_resources',
    'pinned_resource_count',
    'checkpoint_image_identifiers',
    'checkpoint_image_count',
    'payload',
    'payload_size',
]
struct_ctex_operation_record_descriptor._fields_ = [
    ('size', uint32_t),
    ('identifier', String),
    ('algorithm_identifier', String),
    ('algorithm_version', uint32_t),
    ('preset_identifier', String),
    ('preset_version', uint32_t),
    ('replay_class', uint32_t),
    ('input_document_revision', uint64_t),
    ('seed', uint64_t),
    ('mesh_content_identity', String),
    ('coordinate_frame', c_double * int(16)),
    ('payload_kind', uint32_t),
    ('payload_version', uint32_t),
    ('channels', POINTER(ctex_operation_channel_descriptor)),
    ('channel_count', c_size_t),
    ('pinned_resources', POINTER(ctex_pinned_operation_resource_descriptor)),
    ('pinned_resource_count', c_size_t),
    ('checkpoint_image_identifiers', POINTER(POINTER(c_char))),
    ('checkpoint_image_count', c_size_t),
    ('payload', POINTER(None)),
    ('payload_size', c_size_t),
]

ctex_operation_record_descriptor = struct_ctex_operation_record_descriptor

class struct_ctex_operation_record_info(Structure):
    pass

struct_ctex_operation_record_info.__slots__ = [
    'size',
    'schema_version',
    'replay_class',
    'payload_kind',
    'payload_version',
    'input_document_revision',
    'seed',
    'channel_count',
    'pinned_resource_count',
    'checkpoint_image_count',
    'pinned_resource_bytes',
    'payload_size',
    'canonical_size',
    'report_size',
]
struct_ctex_operation_record_info._fields_ = [
    ('size', uint32_t),
    ('schema_version', uint32_t),
    ('replay_class', uint32_t),
    ('payload_kind', uint32_t),
    ('payload_version', uint32_t),
    ('input_document_revision', uint64_t),
    ('seed', uint64_t),
    ('channel_count', c_size_t),
    ('pinned_resource_count', c_size_t),
    ('checkpoint_image_count', c_size_t),
    ('pinned_resource_bytes', c_size_t),
    ('payload_size', c_size_t),
    ('canonical_size', c_size_t),
    ('report_size', c_size_t),
]

ctex_operation_record_info = struct_ctex_operation_record_info

class struct_ctex_operation_algorithm_support_descriptor(Structure):
    pass

struct_ctex_operation_algorithm_support_descriptor.__slots__ = [
    'size',
    'identifier',
    'minimum_version',
    'maximum_version',
]
struct_ctex_operation_algorithm_support_descriptor._fields_ = [
    ('size', uint32_t),
    ('identifier', String),
    ('minimum_version', uint32_t),
    ('maximum_version', uint32_t),
]

ctex_operation_algorithm_support_descriptor = struct_ctex_operation_algorithm_support_descriptor

class struct_ctex_operation_replay_assessment_descriptor(Structure):
    pass

struct_ctex_operation_replay_assessment_descriptor.__slots__ = [
    'size',
    'supported_algorithms',
    'supported_algorithm_count',
    'target_resolution_changed',
]
struct_ctex_operation_replay_assessment_descriptor._fields_ = [
    ('size', uint32_t),
    ('supported_algorithms', POINTER(ctex_operation_algorithm_support_descriptor)),
    ('supported_algorithm_count', c_size_t),
    ('target_resolution_changed', uint32_t),
]

ctex_operation_replay_assessment_descriptor = struct_ctex_operation_replay_assessment_descriptor

class struct_ctex_operation_replay_info(Structure):
    pass

struct_ctex_operation_replay_info.__slots__ = [
    'size',
    'disposition',
    'declared_replay_class',
    'replay_available',
    'checkpoint_available',
    'target_resolution_changed',
    'required_report_size',
]
struct_ctex_operation_replay_info._fields_ = [
    ('size', uint32_t),
    ('disposition', uint32_t),
    ('declared_replay_class', uint32_t),
    ('replay_available', uint32_t),
    ('checkpoint_available', uint32_t),
    ('target_resolution_changed', uint32_t),
    ('required_report_size', c_size_t),
]

ctex_operation_replay_info = struct_ctex_operation_replay_info

class struct_ctex_project_operation_replay_info(Structure):
    pass

struct_ctex_project_operation_replay_info.__slots__ = [
    'size',
    'record_count',
    'replay_available_count',
    'checkpoint_fallback_count',
    'unsupported_algorithm_count',
    'required_report_size',
]
struct_ctex_project_operation_replay_info._fields_ = [
    ('size', uint32_t),
    ('record_count', c_size_t),
    ('replay_available_count', c_size_t),
    ('checkpoint_fallback_count', c_size_t),
    ('unsupported_algorithm_count', c_size_t),
    ('required_report_size', c_size_t),
]

ctex_project_operation_replay_info = struct_ctex_project_operation_replay_info
enum_ctex_resolution_change_policy = c_int
CTEX_RESOLUTION_REPLAY_ELIGIBLE = 0
CTEX_RESOLUTION_RESAMPLE_ALL = 1
CTEX_RESOLUTION_CANCEL = 2
ctex_resolution_change_policy = enum_ctex_resolution_change_policy
enum_ctex_checkpoint_resample_policy = c_int
CTEX_CHECKPOINT_RESAMPLE_REFUSE = 0
CTEX_CHECKPOINT_RESAMPLE_NEAREST = 1
CTEX_CHECKPOINT_RESAMPLE_BILINEAR = 2
ctex_checkpoint_resample_policy = enum_ctex_checkpoint_resample_policy

class struct_ctex_resolution_operation_record_descriptor(Structure):
    pass

struct_ctex_resolution_operation_record_descriptor.__slots__ = [
    'size',
    'canonical_record',
    'canonical_record_size',
]
struct_ctex_resolution_operation_record_descriptor._fields_ = [
    ('size', uint32_t),
    ('canonical_record', POINTER(None)),
    ('canonical_record_size', c_size_t),
]

ctex_resolution_operation_record_descriptor = struct_ctex_resolution_operation_record_descriptor

class struct_ctex_resolution_replay_raster_descriptor(Structure):
    pass

struct_ctex_resolution_replay_raster_descriptor.__slots__ = [
    'size',
    'semantic_id',
    'udim_tile_number',
    'pixels',
    'pixel_bytes',
]
struct_ctex_resolution_replay_raster_descriptor._fields_ = [
    ('size', uint32_t),
    ('semantic_id', String),
    ('udim_tile_number', uint32_t),
    ('pixels', POINTER(None)),
    ('pixel_bytes', c_size_t),
]

ctex_resolution_replay_raster_descriptor = struct_ctex_resolution_replay_raster_descriptor

class struct_ctex_texture_set_resolution_change_descriptor(Structure):
    pass

struct_ctex_texture_set_resolution_change_descriptor.__slots__ = [
    'size',
    'width',
    'height',
    'policy',
    'checkpoint_policy',
    'operation_records',
    'operation_record_count',
    'supported_algorithms',
    'supported_algorithm_count',
    'replay_rasters',
    'replay_raster_count',
    'maximum_working_bytes',
    'maximum_history_bytes',
]
struct_ctex_texture_set_resolution_change_descriptor._fields_ = [
    ('size', uint32_t),
    ('width', uint32_t),
    ('height', uint32_t),
    ('policy', uint32_t),
    ('checkpoint_policy', uint32_t),
    ('operation_records', POINTER(ctex_resolution_operation_record_descriptor)),
    ('operation_record_count', c_size_t),
    ('supported_algorithms', POINTER(ctex_operation_algorithm_support_descriptor)),
    ('supported_algorithm_count', c_size_t),
    ('replay_rasters', POINTER(ctex_resolution_replay_raster_descriptor)),
    ('replay_raster_count', c_size_t),
    ('maximum_working_bytes', c_size_t),
    ('maximum_history_bytes', c_size_t),
]

ctex_texture_set_resolution_change_descriptor = struct_ctex_texture_set_resolution_change_descriptor

class struct_ctex_texture_set_resolution_change_info(Structure):
    pass

struct_ctex_texture_set_resolution_change_info.__slots__ = [
    'size',
    'committed',
    'policy',
    'source_width',
    'source_height',
    'target_width',
    'target_height',
    'replayed_source_count',
    'resampled_source_count',
    'procedural_entry_count',
    'raster_count',
    'staged_pixel_bytes',
    'retained_history_bytes',
]
struct_ctex_texture_set_resolution_change_info._fields_ = [
    ('size', uint32_t),
    ('committed', uint32_t),
    ('policy', uint32_t),
    ('source_width', uint32_t),
    ('source_height', uint32_t),
    ('target_width', uint32_t),
    ('target_height', uint32_t),
    ('replayed_source_count', c_size_t),
    ('resampled_source_count', c_size_t),
    ('procedural_entry_count', c_size_t),
    ('raster_count', c_size_t),
    ('staged_pixel_bytes', c_size_t),
    ('retained_history_bytes', c_size_t),
]

ctex_texture_set_resolution_change_info = struct_ctex_texture_set_resolution_change_info

class struct_ctex_texture_set_resolution_restore_info(Structure):
    pass

struct_ctex_texture_set_resolution_restore_info.__slots__ = [
    'size',
    'width',
    'height',
    'retained_history_bytes',
]
struct_ctex_texture_set_resolution_restore_info._fields_ = [
    ('size', uint32_t),
    ('width', uint32_t),
    ('height', uint32_t),
    ('retained_history_bytes', c_size_t),
]

ctex_texture_set_resolution_restore_info = struct_ctex_texture_set_resolution_restore_info
enum_ctex_editable_entry_kind = c_int
CTEX_EDITABLE_ENTRY_DECAL = 0
CTEX_EDITABLE_ENTRY_TEXT = 1
CTEX_EDITABLE_ENTRY_SURFACE_PATH = 2
ctex_editable_entry_kind = enum_ctex_editable_entry_kind

class struct_ctex_editable_placement_frame(Structure):
    pass

struct_ctex_editable_placement_frame.__slots__ = [
    'position',
    'normal',
    'rotation_radians',
    'uniform_scale',
    'axis_scale',
]
struct_ctex_editable_placement_frame._fields_ = [
    ('position', ctex_vec3d),
    ('normal', ctex_vec3d),
    ('rotation_radians', c_double),
    ('uniform_scale', c_double),
    ('axis_scale', ctex_vec2d),
]

ctex_editable_placement_frame = struct_ctex_editable_placement_frame

class struct_ctex_editable_material_parameter_descriptor(Structure):
    pass

struct_ctex_editable_material_parameter_descriptor.__slots__ = [
    'size',
    'identifier',
    'component_count',
    'value',
]
struct_ctex_editable_material_parameter_descriptor._fields_ = [
    ('size', uint32_t),
    ('identifier', String),
    ('component_count', uint32_t),
    ('value', c_double * int(4)),
]

ctex_editable_material_parameter_descriptor = struct_ctex_editable_material_parameter_descriptor

class struct_ctex_editable_tile_dependency_descriptor(Structure):
    pass

struct_ctex_editable_tile_dependency_descriptor.__slots__ = [
    'size',
    'semantic_id',
    'tile_x',
    'tile_y',
]
struct_ctex_editable_tile_dependency_descriptor._fields_ = [
    ('size', uint32_t),
    ('semantic_id', String),
    ('tile_x', uint32_t),
    ('tile_y', uint32_t),
]

ctex_editable_tile_dependency_descriptor = struct_ctex_editable_tile_dependency_descriptor

class struct_ctex_editable_surface_point_descriptor(Structure):
    pass

struct_ctex_editable_surface_point_descriptor.__slots__ = [
    'size',
    'position',
    'normal',
    'triangle',
    'barycentric',
    'width',
]
struct_ctex_editable_surface_point_descriptor._fields_ = [
    ('size', uint32_t),
    ('position', ctex_vec3d),
    ('normal', ctex_vec3d),
    ('triangle', uint32_t),
    ('barycentric', c_double * int(3)),
    ('width', c_double),
]

ctex_editable_surface_point_descriptor = struct_ctex_editable_surface_point_descriptor

class struct_ctex_editable_entry_descriptor(Structure):
    pass

struct_ctex_editable_entry_descriptor.__slots__ = [
    'size',
    'identifier',
    'kind',
    'expected_revision',
    'placement',
    'material_identity',
    'material_parameters',
    'material_parameter_count',
    'text',
    'font_identity',
    'mesh_revision',
    'surface_points',
    'surface_point_count',
    'dependent_tiles',
    'dependent_tile_count',
]
struct_ctex_editable_entry_descriptor._fields_ = [
    ('size', uint32_t),
    ('identifier', String),
    ('kind', uint32_t),
    ('expected_revision', uint64_t),
    ('placement', ctex_editable_placement_frame),
    ('material_identity', String),
    ('material_parameters', POINTER(ctex_editable_material_parameter_descriptor)),
    ('material_parameter_count', c_size_t),
    ('text', String),
    ('font_identity', String),
    ('mesh_revision', uint64_t),
    ('surface_points', POINTER(ctex_editable_surface_point_descriptor)),
    ('surface_point_count', c_size_t),
    ('dependent_tiles', POINTER(ctex_editable_tile_dependency_descriptor)),
    ('dependent_tile_count', c_size_t),
]

ctex_editable_entry_descriptor = struct_ctex_editable_entry_descriptor

class struct_ctex_editable_entry_info(Structure):
    pass

struct_ctex_editable_entry_info.__slots__ = [
    'size',
    'kind',
    'entry_present',
    'entry_revision',
    'document_revision',
    'entry_count',
    'material_parameter_count',
    'surface_point_count',
    'invalidated_tile_count',
    'undo_step_count',
    'redo_step_count',
    'required_report_size',
]
struct_ctex_editable_entry_info._fields_ = [
    ('size', uint32_t),
    ('kind', uint32_t),
    ('entry_present', uint32_t),
    ('entry_revision', uint64_t),
    ('document_revision', uint64_t),
    ('entry_count', c_size_t),
    ('material_parameter_count', c_size_t),
    ('surface_point_count', c_size_t),
    ('invalidated_tile_count', c_size_t),
    ('undo_step_count', c_size_t),
    ('redo_step_count', c_size_t),
    ('required_report_size', c_size_t),
]

ctex_editable_entry_info = struct_ctex_editable_entry_info

class struct_ctex_preset_shelf_entry_descriptor(Structure):
    pass

struct_ctex_preset_shelf_entry_descriptor.__slots__ = [
    'size',
    'asset_identifier',
    'display_name',
    'tags',
    'tag_count',
    'thumbnail_resource_identifier',
]
struct_ctex_preset_shelf_entry_descriptor._fields_ = [
    ('size', uint32_t),
    ('asset_identifier', String),
    ('display_name', String),
    ('tags', POINTER(POINTER(c_char))),
    ('tag_count', c_size_t),
    ('thumbnail_resource_identifier', String),
]

ctex_preset_shelf_entry_descriptor = struct_ctex_preset_shelf_entry_descriptor

class struct_ctex_preset_shelf_descriptor(Structure):
    pass

struct_ctex_preset_shelf_descriptor.__slots__ = [
    'size',
    'identifier',
    'display_name',
    'contents',
    'contents_size',
    'entries',
    'entry_count',
]
struct_ctex_preset_shelf_descriptor._fields_ = [
    ('size', uint32_t),
    ('identifier', String),
    ('display_name', String),
    ('contents', POINTER(None)),
    ('contents_size', c_size_t),
    ('entries', POINTER(ctex_preset_shelf_entry_descriptor)),
    ('entry_count', c_size_t),
]

ctex_preset_shelf_descriptor = struct_ctex_preset_shelf_descriptor

class struct_ctex_preset_library_descriptor(Structure):
    pass

struct_ctex_preset_library_descriptor.__slots__ = [
    'size',
    'shelves',
    'shelf_count',
    'read_limits',
]
struct_ctex_preset_library_descriptor._fields_ = [
    ('size', uint32_t),
    ('shelves', POINTER(ctex_preset_shelf_descriptor)),
    ('shelf_count', c_size_t),
    ('read_limits', POINTER(ctex_project_container_read_limits_descriptor)),
]

ctex_preset_library_descriptor = struct_ctex_preset_library_descriptor

class struct_ctex_preset_library_info(Structure):
    pass

struct_ctex_preset_library_info.__slots__ = [
    'size',
    'shelf_count',
    'preset_count',
    'report_size',
]
struct_ctex_preset_library_info._fields_ = [
    ('size', uint32_t),
    ('shelf_count', c_size_t),
    ('preset_count', c_size_t),
    ('report_size', c_size_t),
]

ctex_preset_library_info = struct_ctex_preset_library_info

class struct_ctex_material_graph_catalogue_info(Structure):
    pass

struct_ctex_material_graph_catalogue_info.__slots__ = [
    'size',
    'node_count',
    'input_node_count',
    'texture_node_count',
    'colour_filter_node_count',
    'vector_math_node_count',
    'math_operation_count',
    'vector_math_operation_count',
    'report_size',
]
struct_ctex_material_graph_catalogue_info._fields_ = [
    ('size', uint32_t),
    ('node_count', c_size_t),
    ('input_node_count', c_size_t),
    ('texture_node_count', c_size_t),
    ('colour_filter_node_count', c_size_t),
    ('vector_math_node_count', c_size_t),
    ('math_operation_count', c_size_t),
    ('vector_math_operation_count', c_size_t),
    ('report_size', c_size_t),
]

ctex_material_graph_catalogue_info = struct_ctex_material_graph_catalogue_info
enum_ctex_material_graph_socket_coercion = c_int
CTEX_MATERIAL_GRAPH_COERCION_IDENTITY = 0
CTEX_MATERIAL_GRAPH_COERCION_SCALAR_TO_VECTOR = 1
CTEX_MATERIAL_GRAPH_COERCION_VECTOR_TO_SCALAR = 2
CTEX_MATERIAL_GRAPH_COERCION_COLOUR_TO_VECTOR = 3
CTEX_MATERIAL_GRAPH_COERCION_COLOUR_TO_SCALAR = 4
ctex_material_graph_socket_coercion = enum_ctex_material_graph_socket_coercion
enum_ctex_material_graph_diagnostic_code = c_int
CTEX_MATERIAL_GRAPH_UNCONNECTED_REQUIRED_INPUT = 0
CTEX_MATERIAL_GRAPH_MISSING_MESH_MAP = 1
CTEX_MATERIAL_GRAPH_MISSING_IMAGE_RESOURCE = 2
CTEX_MATERIAL_GRAPH_MISSING_GROUP = 3
CTEX_MATERIAL_GRAPH_MISSING_NODE_TYPE = 4
CTEX_MATERIAL_GRAPH_INCOMPATIBLE_NODE_INTERFACE = 5
CTEX_MATERIAL_GRAPH_UNSUPPORTED_EMISSION_TARGET = 6
CTEX_MATERIAL_GRAPH_UNREACHABLE_NODE = 7
ctex_material_graph_diagnostic_code = enum_ctex_material_graph_diagnostic_code

class struct_ctex_material_graph_info(Structure):
    pass

struct_ctex_material_graph_info.__slots__ = [
    'size',
    'output_node_id',
    'node_count',
    'link_count',
    'output_channel_count',
    'canonical_size',
    'report_size',
]
struct_ctex_material_graph_info._fields_ = [
    ('size', uint32_t),
    ('output_node_id', uint64_t),
    ('node_count', c_size_t),
    ('link_count', c_size_t),
    ('output_channel_count', c_size_t),
    ('canonical_size', c_size_t),
    ('report_size', c_size_t),
]

ctex_material_graph_info = struct_ctex_material_graph_info

class struct_ctex_material_graph_link_descriptor(Structure):
    pass

struct_ctex_material_graph_link_descriptor.__slots__ = [
    'size',
    'source_node',
    'source_socket',
    'target_node',
    'target_socket',
]
struct_ctex_material_graph_link_descriptor._fields_ = [
    ('size', uint32_t),
    ('source_node', uint64_t),
    ('source_socket', String),
    ('target_node', uint64_t),
    ('target_socket', String),
]

ctex_material_graph_link_descriptor = struct_ctex_material_graph_link_descriptor

class struct_ctex_material_graph_link_info(Structure):
    pass

struct_ctex_material_graph_link_info.__slots__ = [
    'size',
    'graph',
    'coercion',
    'replaced',
    'replaced_source_node',
    'replaced_source_socket_size',
]
struct_ctex_material_graph_link_info._fields_ = [
    ('size', uint32_t),
    ('graph', ctex_material_graph_info),
    ('coercion', uint32_t),
    ('replaced', uint32_t),
    ('replaced_source_node', uint64_t),
    ('replaced_source_socket_size', c_size_t),
]

ctex_material_graph_link_info = struct_ctex_material_graph_link_info

class struct_ctex_material_graph_validation_resources_descriptor(Structure):
    pass

struct_ctex_material_graph_validation_resources_descriptor.__slots__ = [
    'size',
    'image_resources',
    'image_resource_count',
    'mesh_maps',
    'mesh_map_count',
]
struct_ctex_material_graph_validation_resources_descriptor._fields_ = [
    ('size', uint32_t),
    ('image_resources', POINTER(POINTER(c_char))),
    ('image_resource_count', c_size_t),
    ('mesh_maps', POINTER(POINTER(c_char))),
    ('mesh_map_count', c_size_t),
]

ctex_material_graph_validation_resources_descriptor = struct_ctex_material_graph_validation_resources_descriptor

class struct_ctex_material_graph_validation_info(Structure):
    pass

struct_ctex_material_graph_validation_info.__slots__ = [
    'size',
    'valid',
    'error_count',
    'warning_count',
    'diagnostic_count',
    'report_size',
]
struct_ctex_material_graph_validation_info._fields_ = [
    ('size', uint32_t),
    ('valid', uint32_t),
    ('error_count', c_size_t),
    ('warning_count', c_size_t),
    ('diagnostic_count', c_size_t),
    ('report_size', c_size_t),
]

ctex_material_graph_validation_info = struct_ctex_material_graph_validation_info

class struct_ctex_material_graph_library_info(Structure):
    pass

struct_ctex_material_graph_library_info.__slots__ = [
    'size',
    'preset_count',
    'canonical_size',
    'report_size',
]
struct_ctex_material_graph_library_info._fields_ = [
    ('size', uint32_t),
    ('preset_count', c_size_t),
    ('canonical_size', c_size_t),
    ('report_size', c_size_t),
]

ctex_material_graph_library_info = struct_ctex_material_graph_library_info

class struct_ctex_material_graph_preset_descriptor(Structure):
    pass

struct_ctex_material_graph_preset_descriptor.__slots__ = [
    'size',
    'stable_id',
    'name',
    'thumbnail_resource',
    'graph_serialized',
    'graph_serialized_size',
]
struct_ctex_material_graph_preset_descriptor._fields_ = [
    ('size', uint32_t),
    ('stable_id', String),
    ('name', String),
    ('thumbnail_resource', String),
    ('graph_serialized', POINTER(None)),
    ('graph_serialized_size', c_size_t),
]

ctex_material_graph_preset_descriptor = struct_ctex_material_graph_preset_descriptor
enum_ctex_smart_material_value_type = c_int
CTEX_SMART_MATERIAL_VALUE_SCALAR = 0
CTEX_SMART_MATERIAL_VALUE_VECTOR = 1
CTEX_SMART_MATERIAL_VALUE_COLOUR = 2
CTEX_SMART_MATERIAL_VALUE_STRING = 3
CTEX_SMART_MATERIAL_VALUE_IMAGE = 4
CTEX_SMART_MATERIAL_VALUE_BOOLEAN = 5
ctex_smart_material_value_type = enum_ctex_smart_material_value_type

class struct_ctex_smart_material_value_descriptor(Structure):
    pass

struct_ctex_smart_material_value_descriptor.__slots__ = [
    'size',
    'type',
    'scalar',
    'vector',
    'colour',
    'text',
    'boolean',
]
struct_ctex_smart_material_value_descriptor._fields_ = [
    ('size', uint32_t),
    ('type', uint32_t),
    ('scalar', c_double),
    ('vector', ctex_vec3f),
    ('colour', ctex_vec4f),
    ('text', String),
    ('boolean', uint32_t),
]

ctex_smart_material_value_descriptor = struct_ctex_smart_material_value_descriptor
enum_ctex_material_graph_owner_kind = c_int
CTEX_MATERIAL_GRAPH_OWNER_MATERIAL = 0
CTEX_MATERIAL_GRAPH_OWNER_GROUP = 1
ctex_material_graph_owner_kind = enum_ctex_material_graph_owner_kind

class struct_ctex_material_graph_socket_descriptor(Structure):
    pass

struct_ctex_material_graph_socket_descriptor.__slots__ = [
    'size',
    'identifier',
    'display_name',
    'type',
    'default_value',
]
struct_ctex_material_graph_socket_descriptor._fields_ = [
    ('size', uint32_t),
    ('identifier', String),
    ('display_name', String),
    ('type', uint32_t),
    ('default_value', POINTER(ctex_smart_material_value_descriptor)),
]

ctex_material_graph_socket_descriptor = struct_ctex_material_graph_socket_descriptor

class struct_ctex_material_graph_group_descriptor(Structure):
    pass

struct_ctex_material_graph_group_descriptor.__slots__ = [
    'size',
    'identifier',
    'display_name',
    'inputs',
    'input_count',
    'outputs',
    'output_count',
]
struct_ctex_material_graph_group_descriptor._fields_ = [
    ('size', uint32_t),
    ('identifier', String),
    ('display_name', String),
    ('inputs', POINTER(ctex_material_graph_socket_descriptor)),
    ('input_count', c_size_t),
    ('outputs', POINTER(ctex_material_graph_socket_descriptor)),
    ('output_count', c_size_t),
]

ctex_material_graph_group_descriptor = struct_ctex_material_graph_group_descriptor

class struct_ctex_material_graph_group_interface_descriptor(Structure):
    pass

struct_ctex_material_graph_group_interface_descriptor.__slots__ = [
    'size',
    'inputs',
    'input_count',
    'outputs',
    'output_count',
]
struct_ctex_material_graph_group_interface_descriptor._fields_ = [
    ('size', uint32_t),
    ('inputs', POINTER(ctex_material_graph_socket_descriptor)),
    ('input_count', c_size_t),
    ('outputs', POINTER(ctex_material_graph_socket_descriptor)),
    ('output_count', c_size_t),
]

ctex_material_graph_group_interface_descriptor = struct_ctex_material_graph_group_interface_descriptor

class struct_ctex_material_graph_workspace_info(Structure):
    pass

struct_ctex_material_graph_workspace_info.__slots__ = [
    'size',
    'material_count',
    'group_count',
]
struct_ctex_material_graph_workspace_info._fields_ = [
    ('size', uint32_t),
    ('material_count', c_size_t),
    ('group_count', c_size_t),
]

ctex_material_graph_workspace_info = struct_ctex_material_graph_workspace_info

class struct_ctex_material_graph_group_update_info(Structure):
    pass

struct_ctex_material_graph_group_update_info.__slots__ = [
    'size',
    'group_version',
    'instances_updated',
    'removed_link_count',
]
struct_ctex_material_graph_group_update_info._fields_ = [
    ('size', uint32_t),
    ('group_version', uint32_t),
    ('instances_updated', c_size_t),
    ('removed_link_count', c_size_t),
]

ctex_material_graph_group_update_info = struct_ctex_material_graph_group_update_info
enum_ctex_material_graph_emission_target = c_int
CTEX_MATERIAL_GRAPH_TARGET_WGSL = 0
CTEX_MATERIAL_GRAPH_TARGET_MSL = 1
CTEX_MATERIAL_GRAPH_TARGET_SPIRV = 2
CTEX_MATERIAL_GRAPH_TARGET_HLSL = 3
ctex_material_graph_emission_target = enum_ctex_material_graph_emission_target
enum_ctex_shader_filter_mode = c_int
CTEX_SHADER_FILTER_NEAREST = 0
CTEX_SHADER_FILTER_LINEAR = 1
ctex_shader_filter_mode = enum_ctex_shader_filter_mode
enum_ctex_shader_preview_kind = c_int
CTEX_SHADER_PREVIEW_LIT = 0
CTEX_SHADER_PREVIEW_CHANNEL_INSPECTION = 1
ctex_shader_preview_kind = enum_ctex_shader_preview_kind

class struct_ctex_shader_texture_descriptor(Structure):
    pass

struct_ctex_shader_texture_descriptor.__slots__ = [
    'size',
    'logical_id',
    'generation',
    'role',
    'format',
    'width',
    'height',
    'layers',
    'mip_levels',
    'tile_width',
    'tile_height',
    'externally_initialized',
]
struct_ctex_shader_texture_descriptor._fields_ = [
    ('size', uint32_t),
    ('logical_id', String),
    ('generation', uint64_t),
    ('role', String),
    ('format', uint32_t),
    ('width', uint32_t),
    ('height', uint32_t),
    ('layers', uint32_t),
    ('mip_levels', uint32_t),
    ('tile_width', uint32_t),
    ('tile_height', uint32_t),
    ('externally_initialized', uint32_t),
]

ctex_shader_texture_descriptor = struct_ctex_shader_texture_descriptor

class struct_ctex_shader_material_resource_descriptor(Structure):
    pass

struct_ctex_shader_material_resource_descriptor.__slots__ = [
    'size',
    'identifier',
    'texture',
]
struct_ctex_shader_material_resource_descriptor._fields_ = [
    ('size', uint32_t),
    ('identifier', String),
    ('texture', ctex_shader_texture_descriptor),
]

ctex_shader_material_resource_descriptor = struct_ctex_shader_material_resource_descriptor

class struct_ctex_shader_device_features_descriptor(Structure):
    pass

struct_ctex_shader_device_features_descriptor.__slots__ = [
    'size',
    'binding_budget',
    'maximum_texture_dimension',
    'supported_texture_formats',
    'supported_texture_format_count',
    'floating_point_filtering',
    'compute_available',
]
struct_ctex_shader_device_features_descriptor._fields_ = [
    ('size', uint32_t),
    ('binding_budget', uint32_t),
    ('maximum_texture_dimension', uint32_t),
    ('supported_texture_formats', POINTER(uint32_t)),
    ('supported_texture_format_count', c_size_t),
    ('floating_point_filtering', uint32_t),
    ('compute_available', uint32_t),
]

ctex_shader_device_features_descriptor = struct_ctex_shader_device_features_descriptor

class struct_ctex_shader_material_request(Structure):
    pass

struct_ctex_shader_material_request.__slots__ = [
    'size',
    'stable_identity',
    'target',
    'features',
    'resources',
    'resource_count',
    'output',
    'requested_filter',
    'vertex_count',
]
struct_ctex_shader_material_request._fields_ = [
    ('size', uint32_t),
    ('stable_identity', String),
    ('target', uint32_t),
    ('features', ctex_shader_device_features_descriptor),
    ('resources', POINTER(ctex_shader_material_resource_descriptor)),
    ('resource_count', c_size_t),
    ('output', ctex_shader_texture_descriptor),
    ('requested_filter', uint32_t),
    ('vertex_count', uint32_t),
]

ctex_shader_material_request = struct_ctex_shader_material_request

class struct_ctex_shader_material_info(Structure):
    pass

struct_ctex_shader_material_info.__slots__ = [
    'size',
    'target',
    'vertex_artifact_size',
    'fragment_artifact_size',
    'pass_plan_size',
    'workaround_report_size',
    'pass_count',
    'logical_resource_count',
    'binding_count',
    'workaround_count',
]
struct_ctex_shader_material_info._fields_ = [
    ('size', uint32_t),
    ('target', uint32_t),
    ('vertex_artifact_size', c_size_t),
    ('fragment_artifact_size', c_size_t),
    ('pass_plan_size', c_size_t),
    ('workaround_report_size', c_size_t),
    ('pass_count', c_size_t),
    ('logical_resource_count', c_size_t),
    ('binding_count', c_size_t),
    ('workaround_count', c_size_t),
]

ctex_shader_material_info = struct_ctex_shader_material_info
enum_ctex_shader_material_source_kind = c_int
CTEX_SHADER_MATERIAL_SOURCE_SERIALIZED_GRAPH = 0
CTEX_SHADER_MATERIAL_SOURCE_WORKSPACE = 1
ctex_shader_material_source_kind = enum_ctex_shader_material_source_kind

class struct_ctex_shader_material_source_descriptor(Structure):
    pass

struct_ctex_shader_material_source_descriptor.__slots__ = [
    'size',
    'kind',
    'graph_serialized',
    'graph_serialized_size',
    'workspace',
    'material_identifier',
]
struct_ctex_shader_material_source_descriptor._fields_ = [
    ('size', uint32_t),
    ('kind', uint32_t),
    ('graph_serialized', POINTER(None)),
    ('graph_serialized_size', c_size_t),
    ('workspace', POINTER(ctex_material_graph_workspace)),
    ('material_identifier', String),
]

ctex_shader_material_source_descriptor = struct_ctex_shader_material_source_descriptor

class struct_ctex_shader_material_debug_info(Structure):
    pass

struct_ctex_shader_material_debug_info.__slots__ = [
    'size',
    'node_attribution_count',
    'metadata_size',
    'binary_companion',
]
struct_ctex_shader_material_debug_info._fields_ = [
    ('size', uint32_t),
    ('node_attribution_count', c_size_t),
    ('metadata_size', c_size_t),
    ('binary_companion', uint32_t),
]

ctex_shader_material_debug_info = struct_ctex_shader_material_debug_info

class struct_ctex_shader_backend_attribution_info(Structure):
    pass

struct_ctex_shader_backend_attribution_info.__slots__ = [
    'size',
    'report_size',
]
struct_ctex_shader_backend_attribution_info._fields_ = [
    ('size', uint32_t),
    ('report_size', c_size_t),
]

ctex_shader_backend_attribution_info = struct_ctex_shader_backend_attribution_info

class struct_ctex_shader_layer_descriptor(Structure):
    pass

struct_ctex_shader_layer_descriptor.__slots__ = [
    'size',
    'identifier',
    'texture',
]
struct_ctex_shader_layer_descriptor._fields_ = [
    ('size', uint32_t),
    ('identifier', String),
    ('texture', ctex_shader_texture_descriptor),
]

ctex_shader_layer_descriptor = struct_ctex_shader_layer_descriptor

class struct_ctex_shader_layer_stack_request(Structure):
    pass

struct_ctex_shader_layer_stack_request.__slots__ = [
    'size',
    'stable_identity',
    'target',
    'features',
    'layers',
    'layer_count',
    'output',
    'requested_filter',
]
struct_ctex_shader_layer_stack_request._fields_ = [
    ('size', uint32_t),
    ('stable_identity', String),
    ('target', uint32_t),
    ('features', ctex_shader_device_features_descriptor),
    ('layers', POINTER(ctex_shader_layer_descriptor)),
    ('layer_count', c_size_t),
    ('output', ctex_shader_texture_descriptor),
    ('requested_filter', uint32_t),
]

ctex_shader_layer_stack_request = struct_ctex_shader_layer_stack_request

class struct_ctex_shader_layer_stack_info(Structure):
    pass

struct_ctex_shader_layer_stack_info.__slots__ = [
    'size',
    'target',
    'layer_count',
    'pass_count',
    'artifact_blob_size',
    'artifact_report_size',
    'pass_plan_size',
    'workaround_report_size',
    'workaround_count',
    'compute_used',
    'cache_hit',
]
struct_ctex_shader_layer_stack_info._fields_ = [
    ('size', uint32_t),
    ('target', uint32_t),
    ('layer_count', c_size_t),
    ('pass_count', c_size_t),
    ('artifact_blob_size', c_size_t),
    ('artifact_report_size', c_size_t),
    ('pass_plan_size', c_size_t),
    ('workaround_report_size', c_size_t),
    ('workaround_count', c_size_t),
    ('compute_used', uint32_t),
    ('cache_hit', uint32_t),
]

ctex_shader_layer_stack_info = struct_ctex_shader_layer_stack_info

class struct_ctex_shader_emission_cache_info(Structure):
    pass

struct_ctex_shader_emission_cache_info.__slots__ = [
    'size',
    'entry_count',
    'hit_count',
    'miss_count',
]
struct_ctex_shader_emission_cache_info._fields_ = [
    ('size', uint32_t),
    ('entry_count', c_size_t),
    ('hit_count', c_size_t),
    ('miss_count', c_size_t),
]

ctex_shader_emission_cache_info = struct_ctex_shader_emission_cache_info

class struct_ctex_shader_preview_channel_descriptor(Structure):
    pass

struct_ctex_shader_preview_channel_descriptor.__slots__ = [
    'size',
    'semantic_id',
    'component_count',
    'texture',
]
struct_ctex_shader_preview_channel_descriptor._fields_ = [
    ('size', uint32_t),
    ('semantic_id', String),
    ('component_count', uint32_t),
    ('texture', ctex_shader_texture_descriptor),
]

ctex_shader_preview_channel_descriptor = struct_ctex_shader_preview_channel_descriptor

class struct_ctex_shader_preview_environment_descriptor(Structure):
    pass

struct_ctex_shader_preview_environment_descriptor.__slots__ = [
    'size',
    'radiance',
    'diffuse_irradiance',
    'specular_brdf_lookup',
]
struct_ctex_shader_preview_environment_descriptor._fields_ = [
    ('size', uint32_t),
    ('radiance', ctex_shader_texture_descriptor),
    ('diffuse_irradiance', ctex_shader_texture_descriptor),
    ('specular_brdf_lookup', ctex_shader_texture_descriptor),
]

ctex_shader_preview_environment_descriptor = struct_ctex_shader_preview_environment_descriptor

class struct_ctex_shader_preview_request(Structure):
    pass

struct_ctex_shader_preview_request.__slots__ = [
    'size',
    'stable_identity',
    'target',
    'features',
    'channels',
    'channel_count',
    'output',
    'environment',
    'analytic_light_count',
    'vertex_count',
]
struct_ctex_shader_preview_request._fields_ = [
    ('size', uint32_t),
    ('stable_identity', String),
    ('target', uint32_t),
    ('features', ctex_shader_device_features_descriptor),
    ('channels', POINTER(ctex_shader_preview_channel_descriptor)),
    ('channel_count', c_size_t),
    ('output', ctex_shader_texture_descriptor),
    ('environment', POINTER(ctex_shader_preview_environment_descriptor)),
    ('analytic_light_count', c_size_t),
    ('vertex_count', uint32_t),
]

ctex_shader_preview_request = struct_ctex_shader_preview_request

class struct_ctex_shader_preview_info(Structure):
    pass

struct_ctex_shader_preview_info.__slots__ = [
    'size',
    'target',
    'kind',
    'fallback_lighting',
    'cache_hit',
    'vertex_artifact_size',
    'fragment_artifact_size',
    'pass_plan_size',
    'workaround_report_size',
    'pass_count',
    'logical_resource_count',
    'binding_count',
    'workaround_count',
]
struct_ctex_shader_preview_info._fields_ = [
    ('size', uint32_t),
    ('target', uint32_t),
    ('kind', uint32_t),
    ('fallback_lighting', uint32_t),
    ('cache_hit', uint32_t),
    ('vertex_artifact_size', c_size_t),
    ('fragment_artifact_size', c_size_t),
    ('pass_plan_size', c_size_t),
    ('workaround_report_size', c_size_t),
    ('pass_count', c_size_t),
    ('logical_resource_count', c_size_t),
    ('binding_count', c_size_t),
    ('workaround_count', c_size_t),
]

ctex_shader_preview_info = struct_ctex_shader_preview_info

class struct_ctex_material_graph_property_descriptor(Structure):
    pass

struct_ctex_material_graph_property_descriptor.__slots__ = [
    'size',
    'identifier',
    'display_name',
    'default_value',
    'allowed_values',
    'allowed_value_count',
]
struct_ctex_material_graph_property_descriptor._fields_ = [
    ('size', uint32_t),
    ('identifier', String),
    ('display_name', String),
    ('default_value', POINTER(ctex_smart_material_value_descriptor)),
    ('allowed_values', POINTER(POINTER(c_char))),
    ('allowed_value_count', c_size_t),
]

ctex_material_graph_property_descriptor = struct_ctex_material_graph_property_descriptor

class struct_ctex_material_graph_parity_fixture_descriptor(Structure):
    pass

struct_ctex_material_graph_parity_fixture_descriptor.__slots__ = [
    'size',
    'identifier',
    'inputs',
    'input_count',
    'expected_outputs',
    'expected_output_count',
    'tolerance',
]
struct_ctex_material_graph_parity_fixture_descriptor._fields_ = [
    ('size', uint32_t),
    ('identifier', String),
    ('inputs', POINTER(ctex_smart_material_value_descriptor)),
    ('input_count', c_size_t),
    ('expected_outputs', POINTER(ctex_smart_material_value_descriptor)),
    ('expected_output_count', c_size_t),
    ('tolerance', c_double),
]

ctex_material_graph_parity_fixture_descriptor = struct_ctex_material_graph_parity_fixture_descriptor

class struct_ctex_material_graph_host_property_value(Structure):
    pass

struct_ctex_material_graph_host_property_value.__slots__ = [
    'identifier',
    'value',
]
struct_ctex_material_graph_host_property_value._fields_ = [
    ('identifier', String),
    ('value', ctex_smart_material_value_descriptor),
]

ctex_material_graph_host_property_value = struct_ctex_material_graph_host_property_value

class struct_ctex_material_graph_host_evaluation_request(Structure):
    pass

struct_ctex_material_graph_host_evaluation_request.__slots__ = [
    'size',
    'node_id',
    'type_id',
    'type_version',
    'properties',
    'property_count',
    'inputs',
    'input_count',
]
struct_ctex_material_graph_host_evaluation_request._fields_ = [
    ('size', uint32_t),
    ('node_id', uint64_t),
    ('type_id', String),
    ('type_version', uint32_t),
    ('properties', POINTER(ctex_material_graph_host_property_value)),
    ('property_count', c_size_t),
    ('inputs', POINTER(ctex_smart_material_value_descriptor)),
    ('input_count', c_size_t),
]

ctex_material_graph_host_evaluation_request = struct_ctex_material_graph_host_evaluation_request
ctex_material_graph_cpu_evaluate_callback = CFUNCTYPE(UNCHECKED(ctex_result), POINTER(ctex_material_graph_host_evaluation_request), POINTER(ctex_smart_material_value_descriptor), c_size_t, POINTER(None))

class struct_ctex_material_graph_host_emission_request(Structure):
    pass

struct_ctex_material_graph_host_emission_request.__slots__ = [
    'size',
    'node_id',
    'type_id',
    'type_version',
    'target',
    'properties',
    'property_count',
    'input_expressions',
    'input_expression_count',
]
struct_ctex_material_graph_host_emission_request._fields_ = [
    ('size', uint32_t),
    ('node_id', uint64_t),
    ('type_id', String),
    ('type_version', uint32_t),
    ('target', uint32_t),
    ('properties', POINTER(ctex_material_graph_host_property_value)),
    ('property_count', c_size_t),
    ('input_expressions', POINTER(POINTER(c_char))),
    ('input_expression_count', c_size_t),
]

ctex_material_graph_host_emission_request = struct_ctex_material_graph_host_emission_request

class struct_ctex_material_graph_host_emission_result(Structure):
    pass

struct_ctex_material_graph_host_emission_result.__slots__ = [
    'size',
    'output_expressions',
    'output_expression_count',
    'resource_identifiers',
    'resource_identifier_count',
]
struct_ctex_material_graph_host_emission_result._fields_ = [
    ('size', uint32_t),
    ('output_expressions', POINTER(POINTER(c_char))),
    ('output_expression_count', c_size_t),
    ('resource_identifiers', POINTER(POINTER(c_char))),
    ('resource_identifier_count', c_size_t),
]

ctex_material_graph_host_emission_result = struct_ctex_material_graph_host_emission_result
ctex_material_graph_emit_callback = CFUNCTYPE(UNCHECKED(ctex_result), POINTER(ctex_material_graph_host_emission_request), POINTER(ctex_material_graph_host_emission_result), POINTER(None))

class struct_ctex_material_graph_host_node_registration_descriptor(Structure):
    pass

struct_ctex_material_graph_host_node_registration_descriptor.__slots__ = [
    'size',
    'type_id',
    'type_version',
    'display_name',
    'inputs',
    'input_count',
    'outputs',
    'output_count',
    'properties',
    'property_count',
    'cpu_evaluate',
    'emit',
    'user_data',
    'deterministic',
    'resource_dependencies',
    'resource_dependency_count',
    'supported_targets',
    'supported_target_count',
    'parity_fixtures',
    'parity_fixture_count',
]
struct_ctex_material_graph_host_node_registration_descriptor._fields_ = [
    ('size', uint32_t),
    ('type_id', String),
    ('type_version', uint32_t),
    ('display_name', String),
    ('inputs', POINTER(ctex_material_graph_socket_descriptor)),
    ('input_count', c_size_t),
    ('outputs', POINTER(ctex_material_graph_socket_descriptor)),
    ('output_count', c_size_t),
    ('properties', POINTER(ctex_material_graph_property_descriptor)),
    ('property_count', c_size_t),
    ('cpu_evaluate', ctex_material_graph_cpu_evaluate_callback),
    ('emit', ctex_material_graph_emit_callback),
    ('user_data', POINTER(None)),
    ('deterministic', uint32_t),
    ('resource_dependencies', POINTER(POINTER(c_char))),
    ('resource_dependency_count', c_size_t),
    ('supported_targets', POINTER(uint32_t)),
    ('supported_target_count', c_size_t),
    ('parity_fixtures', POINTER(ctex_material_graph_parity_fixture_descriptor)),
    ('parity_fixture_count', c_size_t),
]

ctex_material_graph_host_node_registration_descriptor = struct_ctex_material_graph_host_node_registration_descriptor

class struct_ctex_material_graph_node_registry_info(Structure):
    pass

struct_ctex_material_graph_node_registry_info.__slots__ = [
    'size',
    'registration_count',
]
struct_ctex_material_graph_node_registry_info._fields_ = [
    ('size', uint32_t),
    ('registration_count', c_size_t),
]

ctex_material_graph_node_registry_info = struct_ctex_material_graph_node_registry_info

class struct_ctex_material_graph_host_contract_info(Structure):
    pass

struct_ctex_material_graph_host_contract_info.__slots__ = [
    'size',
    'parity_passed',
    'parity_failure_count',
    'replay_eligible',
    'unpinned_dependency_count',
    'report_size',
]
struct_ctex_material_graph_host_contract_info._fields_ = [
    ('size', uint32_t),
    ('parity_passed', uint32_t),
    ('parity_failure_count', c_size_t),
    ('replay_eligible', uint32_t),
    ('unpinned_dependency_count', c_size_t),
    ('report_size', c_size_t),
]

ctex_material_graph_host_contract_info = struct_ctex_material_graph_host_contract_info

class struct_ctex_smart_material_info(Structure):
    pass

struct_ctex_smart_material_info.__slots__ = [
    'size',
    'source_schema_version',
    'canonical_schema_version',
    'entry_count',
    'derived_entry_count',
    'model_specific_entry_count',
    'model_specific_pixel_bytes',
    'exposed_parameter_count',
    'anchor_count',
    'anchor_reference_count',
    'resource_reference_count',
    'canonical_size',
    'report_size',
]
struct_ctex_smart_material_info._fields_ = [
    ('size', uint32_t),
    ('source_schema_version', uint32_t),
    ('canonical_schema_version', uint32_t),
    ('entry_count', c_size_t),
    ('derived_entry_count', c_size_t),
    ('model_specific_entry_count', c_size_t),
    ('model_specific_pixel_bytes', c_size_t),
    ('exposed_parameter_count', c_size_t),
    ('anchor_count', c_size_t),
    ('anchor_reference_count', c_size_t),
    ('resource_reference_count', c_size_t),
    ('canonical_size', c_size_t),
    ('report_size', c_size_t),
]

ctex_smart_material_info = struct_ctex_smart_material_info
enum_ctex_applied_preset_kind = c_int
CTEX_APPLIED_PRESET_SMART_MATERIAL = 0
CTEX_APPLIED_PRESET_SMART_MASK = 1
ctex_applied_preset_kind = enum_ctex_applied_preset_kind

class struct_ctex_preset_application_info(Structure):
    pass

struct_ctex_preset_application_info.__slots__ = [
    'size',
    'kind',
    'schema_version',
    'entry_count',
    'application_count',
    'undo_step_count',
]
struct_ctex_preset_application_info._fields_ = [
    ('size', uint32_t),
    ('kind', uint32_t),
    ('schema_version', uint32_t),
    ('entry_count', c_size_t),
    ('application_count', c_size_t),
    ('undo_step_count', c_size_t),
]

ctex_preset_application_info = struct_ctex_preset_application_info

class struct_ctex_preset_undo_info(Structure):
    pass

struct_ctex_preset_undo_info.__slots__ = [
    'size',
    'removed',
    'removed_entry_count',
    'application_count',
    'undo_step_count',
]
struct_ctex_preset_undo_info._fields_ = [
    ('size', uint32_t),
    ('removed', uint32_t),
    ('removed_entry_count', c_size_t),
    ('application_count', c_size_t),
    ('undo_step_count', c_size_t),
]

ctex_preset_undo_info = struct_ctex_preset_undo_info
enum_ctex_color_space = c_int
CTEX_COLOR_SPACE_LINEAR_REC709 = 0
CTEX_COLOR_SPACE_SRGB_REC709 = 1
ctex_color_space = enum_ctex_color_space
enum_ctex_input_color_space = c_int
CTEX_INPUT_COLOR_SPACE_AUTOMATIC = 0
CTEX_INPUT_COLOR_SPACE_LINEAR_REC709 = 1
CTEX_INPUT_COLOR_SPACE_SRGB_REC709 = 2
ctex_input_color_space = enum_ctex_input_color_space
enum_ctex_channel_semantic = c_int
CTEX_CHANNEL_SEMANTIC_BASE_COLOR = 0
CTEX_CHANNEL_SEMANTIC_OPACITY = 1
CTEX_CHANNEL_SEMANTIC_ROUGHNESS = 2
CTEX_CHANNEL_SEMANTIC_METALLIC = 3
CTEX_CHANNEL_SEMANTIC_NORMAL = 4
CTEX_CHANNEL_SEMANTIC_HEIGHT = 5
CTEX_CHANNEL_SEMANTIC_OCCLUSION = 6
CTEX_CHANNEL_SEMANTIC_EMISSION = 7
CTEX_CHANNEL_SEMANTIC_SUBSURFACE = 8
ctex_channel_semantic = enum_ctex_channel_semantic

class struct_ctex_rgb_color(Structure):
    pass

struct_ctex_rgb_color.__slots__ = [
    'red',
    'green',
    'blue',
    'color_space',
]
struct_ctex_rgb_color._fields_ = [
    ('red', c_double),
    ('green', c_double),
    ('blue', c_double),
    ('color_space', uint32_t),
]

ctex_rgb_color = struct_ctex_rgb_color

class struct_ctex_channel_color_policy(Structure):
    pass

struct_ctex_channel_color_policy.__slots__ = [
    'size',
    'color_valued',
    'recommended_bit_depth',
]
struct_ctex_channel_color_policy._fields_ = [
    ('size', uint32_t),
    ('color_valued', uint32_t),
    ('recommended_bit_depth', uint32_t),
]

ctex_channel_color_policy = struct_ctex_channel_color_policy

class struct_ctex_resolved_input_color_space(Structure):
    pass

struct_ctex_resolved_input_color_space.__slots__ = [
    'size',
    'color_space',
    'inferred',
]
struct_ctex_resolved_input_color_space._fields_ = [
    ('size', uint32_t),
    ('color_space', uint32_t),
    ('inferred', uint32_t),
]

ctex_resolved_input_color_space = struct_ctex_resolved_input_color_space

class struct_ctex_bit_depth_warning(Structure):
    pass

struct_ctex_bit_depth_warning.__slots__ = [
    'size',
    'warning',
    'selected_bit_depth',
    'recommended_bit_depth',
]
struct_ctex_bit_depth_warning._fields_ = [
    ('size', uint32_t),
    ('warning', uint32_t),
    ('selected_bit_depth', uint32_t),
    ('recommended_bit_depth', uint32_t),
]

ctex_bit_depth_warning = struct_ctex_bit_depth_warning

class struct_ctex_version(Structure):
    pass

struct_ctex_version.__slots__ = [
    'major',
    'minor',
    'patch',
    'string',
]
struct_ctex_version._fields_ = [
    ('major', uint32_t),
    ('minor', uint32_t),
    ('patch', uint32_t),
    ('string', String),
]

ctex_version = struct_ctex_version

for _lib in _libs.values():
    if not _lib.has("ctex_get_version", "cdecl"):
        continue
    ctex_get_version = _lib.get("ctex_get_version", "cdecl")
    ctex_get_version.argtypes = []
    ctex_get_version.restype = ctex_version
    break


for _lib in _libs.values():
    if not _lib.has("ctex_get_abi_version", "cdecl"):
        continue
    ctex_get_abi_version = _lib.get("ctex_get_abi_version", "cdecl")
    ctex_get_abi_version.argtypes = []
    ctex_get_abi_version.restype = ctex_version
    break


for _lib in _libs.values():
    if not _lib.has("ctex_get_working_color_space", "cdecl"):
        continue
    ctex_get_working_color_space = _lib.get("ctex_get_working_color_space", "cdecl")
    ctex_get_working_color_space.argtypes = []
    ctex_get_working_color_space.restype = ctex_color_space
    break


for _lib in _libs.values():
    if not _lib.has("ctex_color_space_get_name", "cdecl"):
        continue
    ctex_color_space_get_name = _lib.get("ctex_color_space_get_name", "cdecl")
    ctex_color_space_get_name.argtypes = [uint32_t, String, c_size_t, POINTER(c_size_t)]
    ctex_color_space_get_name.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_channel_get_color_policy", "cdecl"):
        continue
    ctex_channel_get_color_policy = _lib.get("ctex_channel_get_color_policy", "cdecl")
    ctex_channel_get_color_policy.argtypes = [uint32_t, POINTER(ctex_channel_color_policy)]
    ctex_channel_get_color_policy.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_resolve_input_color_space", "cdecl"):
        continue
    ctex_resolve_input_color_space = _lib.get("ctex_resolve_input_color_space", "cdecl")
    ctex_resolve_input_color_space.argtypes = [uint32_t, uint32_t, POINTER(ctex_resolved_input_color_space)]
    ctex_resolve_input_color_space.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_color_convert", "cdecl"):
        continue
    ctex_color_convert = _lib.get("ctex_color_convert", "cdecl")
    ctex_color_convert.argtypes = [POINTER(ctex_rgb_color), uint32_t, POINTER(ctex_rgb_color)]
    ctex_color_convert.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_color_input_to_working", "cdecl"):
        continue
    ctex_color_input_to_working = _lib.get("ctex_color_input_to_working", "cdecl")
    ctex_color_input_to_working.argtypes = [POINTER(ctex_rgb_color), uint32_t, POINTER(ctex_rgb_color)]
    ctex_color_input_to_working.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_channel_get_bit_depth_warning", "cdecl"):
        continue
    ctex_channel_get_bit_depth_warning = _lib.get("ctex_channel_get_bit_depth_warning", "cdecl")
    ctex_channel_get_bit_depth_warning.argtypes = [uint32_t, uint32_t, POINTER(ctex_bit_depth_warning)]
    ctex_channel_get_bit_depth_warning.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_accumulate_height", "cdecl"):
        continue
    ctex_accumulate_height = _lib.get("ctex_accumulate_height", "cdecl")
    ctex_accumulate_height.argtypes = [POINTER(c_double), c_size_t, uint32_t, POINTER(c_double)]
    ctex_accumulate_height.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_quantize_unorm8", "cdecl"):
        continue
    ctex_quantize_unorm8 = _lib.get("ctex_quantize_unorm8", "cdecl")
    ctex_quantize_unorm8.argtypes = [c_double, uint32_t, uint32_t, uint32_t, POINTER(uint8_t)]
    ctex_quantize_unorm8.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_cube_lut_create", "cdecl"):
        continue
    ctex_cube_lut_create = _lib.get("ctex_cube_lut_create", "cdecl")
    ctex_cube_lut_create.argtypes = [String, c_size_t, POINTER(POINTER(ctex_cube_lut))]
    ctex_cube_lut_create.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_cube_lut_destroy", "cdecl"):
        continue
    ctex_cube_lut_destroy = _lib.get("ctex_cube_lut_destroy", "cdecl")
    ctex_cube_lut_destroy.argtypes = [POINTER(ctex_cube_lut)]
    ctex_cube_lut_destroy.restype = None
    break


for _lib in _libs.values():
    if not _lib.has("ctex_cube_lut_apply_preview", "cdecl"):
        continue
    ctex_cube_lut_apply_preview = _lib.get("ctex_cube_lut_apply_preview", "cdecl")
    ctex_cube_lut_apply_preview.argtypes = [POINTER(ctex_cube_lut), POINTER(ctex_rgb_color), POINTER(ctex_rgb_color)]
    ctex_cube_lut_apply_preview.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_image_decode_memory", "cdecl"):
        continue
    ctex_image_decode_memory = _lib.get("ctex_image_decode_memory", "cdecl")
    ctex_image_decode_memory.argtypes = [POINTER(None), c_size_t, String, uint32_t, uint32_t, POINTER(ctex_image_decode_limits_descriptor), POINTER(ctex_decoded_image_info), POINTER(None), c_size_t, POINTER(c_size_t)]
    ctex_image_decode_memory.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_image_decode_memory_bounded", "cdecl"):
        continue
    ctex_image_decode_memory_bounded = _lib.get("ctex_image_decode_memory_bounded", "cdecl")
    ctex_image_decode_memory_bounded.argtypes = [POINTER(None), c_size_t, String, uint32_t, uint32_t, POINTER(ctex_image_decode_limits_descriptor), POINTER(ctex_image_decode_control_descriptor), POINTER(ctex_image_decode_execution_info), POINTER(ctex_decoded_image_info), POINTER(None), c_size_t, POINTER(c_size_t)]
    ctex_image_decode_memory_bounded.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_image_decode_layered_memory", "cdecl"):
        continue
    ctex_image_decode_layered_memory = _lib.get("ctex_image_decode_layered_memory", "cdecl")
    ctex_image_decode_layered_memory.argtypes = [POINTER(None), c_size_t, String, POINTER(ctex_layered_image_decode_descriptor), POINTER(ctex_image_decode_limits_descriptor), POINTER(ctex_image_decode_control_descriptor), POINTER(ctex_image_decode_execution_info), POINTER(ctex_layered_image_decode_info), POINTER(ctex_layered_decoded_image_info), c_size_t, String, c_size_t, POINTER(None), c_size_t]
    ctex_image_decode_layered_memory.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_image_expand_channels", "cdecl"):
        continue
    ctex_image_expand_channels = _lib.get("ctex_image_expand_channels", "cdecl")
    ctex_image_expand_channels.argtypes = [POINTER(None), c_size_t, POINTER(ctex_image_channel_expansion_descriptor), POINTER(ctex_image_channel_expansion_info), POINTER(None), c_size_t]
    ctex_image_expand_channels.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_image_resample", "cdecl"):
        continue
    ctex_image_resample = _lib.get("ctex_image_resample", "cdecl")
    ctex_image_resample.argtypes = [POINTER(None), c_size_t, POINTER(ctex_image_resample_descriptor), POINTER(ctex_image_resample_info), POINTER(None), c_size_t]
    ctex_image_resample.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_image_encode_memory", "cdecl"):
        continue
    ctex_image_encode_memory = _lib.get("ctex_image_encode_memory", "cdecl")
    ctex_image_encode_memory.argtypes = [POINTER(None), c_size_t, POINTER(ctex_image_encode_descriptor), POINTER(None), c_size_t, POINTER(c_size_t)]
    ctex_image_encode_memory.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_texture_export_get_built_in_preset_ids", "cdecl"):
        continue
    ctex_texture_export_get_built_in_preset_ids = _lib.get("ctex_texture_export_get_built_in_preset_ids", "cdecl")
    ctex_texture_export_get_built_in_preset_ids.argtypes = [String, c_size_t, POINTER(c_size_t), POINTER(c_size_t)]
    ctex_texture_export_get_built_in_preset_ids.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_texture_export_run", "cdecl"):
        continue
    ctex_texture_export_run = _lib.get("ctex_texture_export_run", "cdecl")
    ctex_texture_export_run.argtypes = [POINTER(ctex_texture_export_catalogue_descriptor), POINTER(ctex_texture_export_preset_descriptor), POINTER(ctex_texture_export_options_descriptor), POINTER(ctex_texture_export_callbacks_descriptor), POINTER(ctex_texture_export_info)]
    ctex_texture_export_run.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_project_container_create_empty", "cdecl"):
        continue
    ctex_project_container_create_empty = _lib.get("ctex_project_container_create_empty", "cdecl")
    ctex_project_container_create_empty.argtypes = [POINTER(None), c_size_t, POINTER(c_size_t)]
    ctex_project_container_create_empty.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_project_container_probe_version", "cdecl"):
        continue
    ctex_project_container_probe_version = _lib.get("ctex_project_container_probe_version", "cdecl")
    ctex_project_container_probe_version.argtypes = [POINTER(None), c_size_t, POINTER(ctex_project_container_version)]
    ctex_project_container_probe_version.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_project_container_normalize", "cdecl"):
        continue
    ctex_project_container_normalize = _lib.get("ctex_project_container_normalize", "cdecl")
    ctex_project_container_normalize.argtypes = [POINTER(None), c_size_t, POINTER(ctex_project_container_read_limits_descriptor), POINTER(ctex_project_container_info), POINTER(None), c_size_t, String, c_size_t]
    ctex_project_container_normalize.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_project_container_save_atomic", "cdecl"):
        continue
    ctex_project_container_save_atomic = _lib.get("ctex_project_container_save_atomic", "cdecl")
    ctex_project_container_save_atomic.argtypes = [POINTER(None), c_size_t, POINTER(ctex_project_container_read_limits_descriptor), String, POINTER(ctex_project_container_info)]
    ctex_project_container_save_atomic.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_project_autosave_session_create", "cdecl"):
        continue
    ctex_project_autosave_session_create = _lib.get("ctex_project_autosave_session_create", "cdecl")
    ctex_project_autosave_session_create.argtypes = [POINTER(ctex_project_autosave_config_descriptor), POINTER(POINTER(ctex_project_autosave_session))]
    ctex_project_autosave_session_create.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_project_autosave_session_destroy", "cdecl"):
        continue
    ctex_project_autosave_session_destroy = _lib.get("ctex_project_autosave_session_destroy", "cdecl")
    ctex_project_autosave_session_destroy.argtypes = [POINTER(ctex_project_autosave_session)]
    ctex_project_autosave_session_destroy.restype = None
    break


for _lib in _libs.values():
    if not _lib.has("ctex_project_autosave_session_submit", "cdecl"):
        continue
    ctex_project_autosave_session_submit = _lib.get("ctex_project_autosave_session_submit", "cdecl")
    ctex_project_autosave_session_submit.argtypes = [POINTER(ctex_project_autosave_session), uint64_t, POINTER(None), c_size_t, POINTER(ctex_project_container_read_limits_descriptor), POINTER(uint32_t)]
    ctex_project_autosave_session_submit.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_project_autosave_session_wait", "cdecl"):
        continue
    ctex_project_autosave_session_wait = _lib.get("ctex_project_autosave_session_wait", "cdecl")
    ctex_project_autosave_session_wait.argtypes = [POINTER(ctex_project_autosave_session), uint64_t, POINTER(uint32_t)]
    ctex_project_autosave_session_wait.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_project_autosave_session_flush", "cdecl"):
        continue
    ctex_project_autosave_session_flush = _lib.get("ctex_project_autosave_session_flush", "cdecl")
    ctex_project_autosave_session_flush.argtypes = [POINTER(ctex_project_autosave_session)]
    ctex_project_autosave_session_flush.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_project_autosave_session_get_info", "cdecl"):
        continue
    ctex_project_autosave_session_get_info = _lib.get("ctex_project_autosave_session_get_info", "cdecl")
    ctex_project_autosave_session_get_info.argtypes = [POINTER(ctex_project_autosave_session), POINTER(ctex_project_autosave_info), String, c_size_t, String, c_size_t]
    ctex_project_autosave_session_get_info.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_project_lifecycle_quiesce", "cdecl"):
        continue
    ctex_project_lifecycle_quiesce = _lib.get("ctex_project_lifecycle_quiesce", "cdecl")
    ctex_project_lifecycle_quiesce.argtypes = [POINTER(ctex_resource_ledger), POINTER(ctex_project_autosave_session), POINTER(ctex_project_quiesce_descriptor), POINTER(ctex_project_quiesce_report)]
    ctex_project_lifecycle_quiesce.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_project_lifecycle_resume", "cdecl"):
        continue
    ctex_project_lifecycle_resume = _lib.get("ctex_project_lifecycle_resume", "cdecl")
    ctex_project_lifecycle_resume.argtypes = [POINTER(ctex_resource_ledger)]
    ctex_project_lifecycle_resume.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_project_recovery_enumerate", "cdecl"):
        continue
    ctex_project_recovery_enumerate = _lib.get("ctex_project_recovery_enumerate", "cdecl")
    ctex_project_recovery_enumerate.argtypes = [String, POINTER(ctex_project_recovery_enumeration_info), POINTER(ctex_project_recovery_entry), c_size_t, POINTER(ctex_project_recovery_rejection), c_size_t, String, c_size_t]
    ctex_project_recovery_enumerate.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_project_recovery_read", "cdecl"):
        continue
    ctex_project_recovery_read = _lib.get("ctex_project_recovery_read", "cdecl")
    ctex_project_recovery_read.argtypes = [String, POINTER(ctex_project_container_read_limits_descriptor), POINTER(ctex_project_container_info), POINTER(None), c_size_t, String, c_size_t]
    ctex_project_recovery_read.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_project_recovery_resume", "cdecl"):
        continue
    ctex_project_recovery_resume = _lib.get("ctex_project_recovery_resume", "cdecl")
    ctex_project_recovery_resume.argtypes = [String, POINTER(ctex_project_container_read_limits_descriptor), POINTER(ctex_project_recovery_checkpoint_info), POINTER(ctex_project_container_info), POINTER(None), c_size_t, String, c_size_t]
    ctex_project_recovery_resume.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_project_asset_export", "cdecl"):
        continue
    ctex_project_asset_export = _lib.get("ctex_project_asset_export", "cdecl")
    ctex_project_asset_export.argtypes = [POINTER(None), c_size_t, POINTER(ctex_project_container_read_limits_descriptor), String, POINTER(ctex_project_asset_export_options_descriptor), POINTER(ctex_project_container_info), POINTER(None), c_size_t, String, c_size_t]
    ctex_project_asset_export.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_project_asset_install", "cdecl"):
        continue
    ctex_project_asset_install = _lib.get("ctex_project_asset_install", "cdecl")
    ctex_project_asset_install.argtypes = [POINTER(None), c_size_t, POINTER(None), c_size_t, POINTER(ctex_project_container_read_limits_descriptor), POINTER(ctex_project_asset_search_paths_descriptor), POINTER(ctex_project_container_info), POINTER(None), c_size_t, String, c_size_t]
    ctex_project_asset_install.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_operation_record_create", "cdecl"):
        continue
    ctex_operation_record_create = _lib.get("ctex_operation_record_create", "cdecl")
    ctex_operation_record_create.argtypes = [POINTER(ctex_operation_record_descriptor), POINTER(ctex_operation_record_info), POINTER(None), c_size_t, String, c_size_t]
    ctex_operation_record_create.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_operation_record_inspect", "cdecl"):
        continue
    ctex_operation_record_inspect = _lib.get("ctex_operation_record_inspect", "cdecl")
    ctex_operation_record_inspect.argtypes = [POINTER(None), c_size_t, POINTER(ctex_operation_record_info), POINTER(None), c_size_t, String, c_size_t]
    ctex_operation_record_inspect.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_operation_record_assess_replay", "cdecl"):
        continue
    ctex_operation_record_assess_replay = _lib.get("ctex_operation_record_assess_replay", "cdecl")
    ctex_operation_record_assess_replay.argtypes = [POINTER(None), c_size_t, POINTER(ctex_operation_replay_assessment_descriptor), POINTER(ctex_operation_replay_info), String, c_size_t]
    ctex_operation_record_assess_replay.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_project_container_upsert_operation_record", "cdecl"):
        continue
    ctex_project_container_upsert_operation_record = _lib.get("ctex_project_container_upsert_operation_record", "cdecl")
    ctex_project_container_upsert_operation_record.argtypes = [POINTER(None), c_size_t, POINTER(ctex_project_container_read_limits_descriptor), POINTER(None), c_size_t, POINTER(ctex_project_container_info), POINTER(None), c_size_t, String, c_size_t]
    ctex_project_container_upsert_operation_record.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_project_container_get_operation_record", "cdecl"):
        continue
    ctex_project_container_get_operation_record = _lib.get("ctex_project_container_get_operation_record", "cdecl")
    ctex_project_container_get_operation_record.argtypes = [POINTER(None), c_size_t, POINTER(ctex_project_container_read_limits_descriptor), String, POINTER(None), c_size_t, POINTER(c_size_t)]
    ctex_project_container_get_operation_record.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_project_container_assess_operation_replay", "cdecl"):
        continue
    ctex_project_container_assess_operation_replay = _lib.get("ctex_project_container_assess_operation_replay", "cdecl")
    ctex_project_container_assess_operation_replay.argtypes = [POINTER(None), c_size_t, POINTER(ctex_project_container_read_limits_descriptor), POINTER(ctex_operation_replay_assessment_descriptor), POINTER(ctex_project_operation_replay_info), String, c_size_t]
    ctex_project_container_assess_operation_replay.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_texture_set_change_resolution", "cdecl"):
        continue
    ctex_texture_set_change_resolution = _lib.get("ctex_texture_set_change_resolution", "cdecl")
    ctex_texture_set_change_resolution.argtypes = [POINTER(ctex_document), String, POINTER(ctex_texture_set_resolution_change_descriptor), POINTER(ctex_texture_set_resolution_change_info)]
    ctex_texture_set_change_resolution.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_texture_set_undo_resolution_change", "cdecl"):
        continue
    ctex_texture_set_undo_resolution_change = _lib.get("ctex_texture_set_undo_resolution_change", "cdecl")
    ctex_texture_set_undo_resolution_change.argtypes = [POINTER(ctex_document), String, POINTER(ctex_texture_set_resolution_restore_info)]
    ctex_texture_set_undo_resolution_change.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_texture_set_redo_resolution_change", "cdecl"):
        continue
    ctex_texture_set_redo_resolution_change = _lib.get("ctex_texture_set_redo_resolution_change", "cdecl")
    ctex_texture_set_redo_resolution_change.argtypes = [POINTER(ctex_document), String, POINTER(ctex_texture_set_resolution_restore_info)]
    ctex_texture_set_redo_resolution_change.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_texture_set_editable_entry_add", "cdecl"):
        continue
    ctex_texture_set_editable_entry_add = _lib.get("ctex_texture_set_editable_entry_add", "cdecl")
    ctex_texture_set_editable_entry_add.argtypes = [POINTER(ctex_document), String, POINTER(ctex_editable_entry_descriptor), POINTER(ctex_editable_entry_info), String, c_size_t]
    ctex_texture_set_editable_entry_add.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_texture_set_editable_entry_edit", "cdecl"):
        continue
    ctex_texture_set_editable_entry_edit = _lib.get("ctex_texture_set_editable_entry_edit", "cdecl")
    ctex_texture_set_editable_entry_edit.argtypes = [POINTER(ctex_document), String, POINTER(ctex_editable_entry_descriptor), POINTER(ctex_editable_entry_info), String, c_size_t]
    ctex_texture_set_editable_entry_edit.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_texture_set_editable_entry_inspect", "cdecl"):
        continue
    ctex_texture_set_editable_entry_inspect = _lib.get("ctex_texture_set_editable_entry_inspect", "cdecl")
    ctex_texture_set_editable_entry_inspect.argtypes = [POINTER(ctex_document), String, String, POINTER(ctex_editable_entry_info), String, c_size_t]
    ctex_texture_set_editable_entry_inspect.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_texture_set_editable_entry_plan_rasterization", "cdecl"):
        continue
    ctex_texture_set_editable_entry_plan_rasterization = _lib.get("ctex_texture_set_editable_entry_plan_rasterization", "cdecl")
    ctex_texture_set_editable_entry_plan_rasterization.argtypes = [POINTER(ctex_document), String, String, POINTER(ctex_editable_entry_info), String, c_size_t]
    ctex_texture_set_editable_entry_plan_rasterization.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_texture_set_editable_entry_undo", "cdecl"):
        continue
    ctex_texture_set_editable_entry_undo = _lib.get("ctex_texture_set_editable_entry_undo", "cdecl")
    ctex_texture_set_editable_entry_undo.argtypes = [POINTER(ctex_document), String, POINTER(ctex_editable_entry_info), String, c_size_t]
    ctex_texture_set_editable_entry_undo.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_texture_set_editable_entry_redo", "cdecl"):
        continue
    ctex_texture_set_editable_entry_redo = _lib.get("ctex_texture_set_editable_entry_redo", "cdecl")
    ctex_texture_set_editable_entry_redo.argtypes = [POINTER(ctex_document), String, POINTER(ctex_editable_entry_info), String, c_size_t]
    ctex_texture_set_editable_entry_redo.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_texture_set_editable_surface_path_resolve", "cdecl"):
        continue
    ctex_texture_set_editable_surface_path_resolve = _lib.get("ctex_texture_set_editable_surface_path_resolve", "cdecl")
    ctex_texture_set_editable_surface_path_resolve.argtypes = [POINTER(ctex_document), String, String, POINTER(ctex_stroke_settings_descriptor), POINTER(ctex_resolved_stroke_info), POINTER(ctex_resolved_stamp), c_size_t, POINTER(c_size_t), POINTER(ctex_swept_segment), c_size_t, POINTER(c_size_t)]
    ctex_texture_set_editable_surface_path_resolve.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_project_container_upsert_editable_authoring", "cdecl"):
        continue
    ctex_project_container_upsert_editable_authoring = _lib.get("ctex_project_container_upsert_editable_authoring", "cdecl")
    ctex_project_container_upsert_editable_authoring.argtypes = [POINTER(None), c_size_t, POINTER(ctex_project_container_read_limits_descriptor), POINTER(ctex_document), String, String, POINTER(ctex_project_container_info), POINTER(None), c_size_t, String, c_size_t]
    ctex_project_container_upsert_editable_authoring.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_project_container_restore_editable_authoring", "cdecl"):
        continue
    ctex_project_container_restore_editable_authoring = _lib.get("ctex_project_container_restore_editable_authoring", "cdecl")
    ctex_project_container_restore_editable_authoring.argtypes = [POINTER(None), c_size_t, POINTER(ctex_project_container_read_limits_descriptor), String, POINTER(ctex_document), String, POINTER(ctex_editable_entry_info), String, c_size_t]
    ctex_project_container_restore_editable_authoring.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_material_graph_get_builtin_catalogue", "cdecl"):
        continue
    ctex_material_graph_get_builtin_catalogue = _lib.get("ctex_material_graph_get_builtin_catalogue", "cdecl")
    ctex_material_graph_get_builtin_catalogue.argtypes = [POINTER(ctex_material_graph_catalogue_info), String, c_size_t]
    ctex_material_graph_get_builtin_catalogue.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_material_graph_create_default", "cdecl"):
        continue
    ctex_material_graph_create_default = _lib.get("ctex_material_graph_create_default", "cdecl")
    ctex_material_graph_create_default.argtypes = [POINTER(ctex_material_graph_info), POINTER(None), c_size_t, String, c_size_t]
    ctex_material_graph_create_default.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_material_graph_inspect", "cdecl"):
        continue
    ctex_material_graph_inspect = _lib.get("ctex_material_graph_inspect", "cdecl")
    ctex_material_graph_inspect.argtypes = [POINTER(None), c_size_t, POINTER(ctex_material_graph_info), POINTER(None), c_size_t, String, c_size_t]
    ctex_material_graph_inspect.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_material_graph_compare", "cdecl"):
        continue
    ctex_material_graph_compare = _lib.get("ctex_material_graph_compare", "cdecl")
    ctex_material_graph_compare.argtypes = [POINTER(None), c_size_t, POINTER(None), c_size_t, POINTER(uint32_t)]
    ctex_material_graph_compare.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_material_graph_add_builtin_node", "cdecl"):
        continue
    ctex_material_graph_add_builtin_node = _lib.get("ctex_material_graph_add_builtin_node", "cdecl")
    ctex_material_graph_add_builtin_node.argtypes = [POINTER(None), c_size_t, String, ctex_vec2f, POINTER(ctex_material_graph_info), POINTER(uint64_t), POINTER(None), c_size_t]
    ctex_material_graph_add_builtin_node.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_material_graph_set_input_value", "cdecl"):
        continue
    ctex_material_graph_set_input_value = _lib.get("ctex_material_graph_set_input_value", "cdecl")
    ctex_material_graph_set_input_value.argtypes = [POINTER(None), c_size_t, uint64_t, String, POINTER(ctex_smart_material_value_descriptor), POINTER(ctex_material_graph_info), POINTER(None), c_size_t]
    ctex_material_graph_set_input_value.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_material_graph_set_property_value", "cdecl"):
        continue
    ctex_material_graph_set_property_value = _lib.get("ctex_material_graph_set_property_value", "cdecl")
    ctex_material_graph_set_property_value.argtypes = [POINTER(None), c_size_t, uint64_t, String, POINTER(ctex_smart_material_value_descriptor), POINTER(ctex_material_graph_info), POINTER(None), c_size_t]
    ctex_material_graph_set_property_value.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_material_graph_add_link", "cdecl"):
        continue
    ctex_material_graph_add_link = _lib.get("ctex_material_graph_add_link", "cdecl")
    ctex_material_graph_add_link.argtypes = [POINTER(None), c_size_t, POINTER(ctex_material_graph_link_descriptor), POINTER(ctex_material_graph_link_info), POINTER(None), c_size_t, String, c_size_t]
    ctex_material_graph_add_link.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_material_graph_validate", "cdecl"):
        continue
    ctex_material_graph_validate = _lib.get("ctex_material_graph_validate", "cdecl")
    ctex_material_graph_validate.argtypes = [POINTER(None), c_size_t, POINTER(ctex_material_graph_validation_resources_descriptor), POINTER(ctex_material_graph_validation_info), String, c_size_t]
    ctex_material_graph_validate.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_material_graph_library_create_empty", "cdecl"):
        continue
    ctex_material_graph_library_create_empty = _lib.get("ctex_material_graph_library_create_empty", "cdecl")
    ctex_material_graph_library_create_empty.argtypes = [POINTER(ctex_material_graph_library_info), POINTER(None), c_size_t, String, c_size_t]
    ctex_material_graph_library_create_empty.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_material_graph_library_inspect", "cdecl"):
        continue
    ctex_material_graph_library_inspect = _lib.get("ctex_material_graph_library_inspect", "cdecl")
    ctex_material_graph_library_inspect.argtypes = [POINTER(None), c_size_t, POINTER(ctex_material_graph_library_info), POINTER(None), c_size_t, String, c_size_t]
    ctex_material_graph_library_inspect.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_material_graph_library_add_preset", "cdecl"):
        continue
    ctex_material_graph_library_add_preset = _lib.get("ctex_material_graph_library_add_preset", "cdecl")
    ctex_material_graph_library_add_preset.argtypes = [POINTER(None), c_size_t, POINTER(ctex_material_graph_preset_descriptor), POINTER(ctex_material_graph_library_info), POINTER(None), c_size_t, String, c_size_t]
    ctex_material_graph_library_add_preset.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_material_graph_library_resolve_preset", "cdecl"):
        continue
    ctex_material_graph_library_resolve_preset = _lib.get("ctex_material_graph_library_resolve_preset", "cdecl")
    ctex_material_graph_library_resolve_preset.argtypes = [POINTER(None), c_size_t, String, POINTER(ctex_material_graph_info), POINTER(None), c_size_t, String, c_size_t]
    ctex_material_graph_library_resolve_preset.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_material_graph_workspace_create", "cdecl"):
        continue
    ctex_material_graph_workspace_create = _lib.get("ctex_material_graph_workspace_create", "cdecl")
    ctex_material_graph_workspace_create.argtypes = [POINTER(POINTER(ctex_material_graph_workspace))]
    ctex_material_graph_workspace_create.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_material_graph_workspace_destroy", "cdecl"):
        continue
    ctex_material_graph_workspace_destroy = _lib.get("ctex_material_graph_workspace_destroy", "cdecl")
    ctex_material_graph_workspace_destroy.argtypes = [POINTER(ctex_material_graph_workspace)]
    ctex_material_graph_workspace_destroy.restype = None
    break


for _lib in _libs.values():
    if not _lib.has("ctex_material_graph_workspace_get_info", "cdecl"):
        continue
    ctex_material_graph_workspace_get_info = _lib.get("ctex_material_graph_workspace_get_info", "cdecl")
    ctex_material_graph_workspace_get_info.argtypes = [POINTER(ctex_material_graph_workspace), POINTER(ctex_material_graph_workspace_info)]
    ctex_material_graph_workspace_get_info.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_material_graph_workspace_add_material", "cdecl"):
        continue
    ctex_material_graph_workspace_add_material = _lib.get("ctex_material_graph_workspace_add_material", "cdecl")
    ctex_material_graph_workspace_add_material.argtypes = [POINTER(ctex_material_graph_workspace), String, POINTER(None), c_size_t]
    ctex_material_graph_workspace_add_material.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_material_graph_workspace_create_group", "cdecl"):
        continue
    ctex_material_graph_workspace_create_group = _lib.get("ctex_material_graph_workspace_create_group", "cdecl")
    ctex_material_graph_workspace_create_group.argtypes = [POINTER(ctex_material_graph_workspace), POINTER(ctex_material_graph_group_descriptor)]
    ctex_material_graph_workspace_create_group.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_material_graph_workspace_instantiate_group", "cdecl"):
        continue
    ctex_material_graph_workspace_instantiate_group = _lib.get("ctex_material_graph_workspace_instantiate_group", "cdecl")
    ctex_material_graph_workspace_instantiate_group.argtypes = [POINTER(ctex_material_graph_workspace), String, uint32_t, String, ctex_vec2f, POINTER(uint64_t)]
    ctex_material_graph_workspace_instantiate_group.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_material_graph_workspace_update_group_interface", "cdecl"):
        continue
    ctex_material_graph_workspace_update_group_interface = _lib.get("ctex_material_graph_workspace_update_group_interface", "cdecl")
    ctex_material_graph_workspace_update_group_interface.argtypes = [POINTER(ctex_material_graph_workspace), String, POINTER(ctex_material_graph_group_interface_descriptor), POINTER(ctex_material_graph_group_update_info)]
    ctex_material_graph_workspace_update_group_interface.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_material_graph_workspace_get_graph", "cdecl"):
        continue
    ctex_material_graph_workspace_get_graph = _lib.get("ctex_material_graph_workspace_get_graph", "cdecl")
    ctex_material_graph_workspace_get_graph.argtypes = [POINTER(ctex_material_graph_workspace), uint32_t, String, POINTER(ctex_material_graph_info), POINTER(None), c_size_t, String, c_size_t]
    ctex_material_graph_workspace_get_graph.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_material_graph_workspace_add_builtin_node", "cdecl"):
        continue
    ctex_material_graph_workspace_add_builtin_node = _lib.get("ctex_material_graph_workspace_add_builtin_node", "cdecl")
    ctex_material_graph_workspace_add_builtin_node.argtypes = [POINTER(ctex_material_graph_workspace), uint32_t, String, String, ctex_vec2f, POINTER(uint64_t)]
    ctex_material_graph_workspace_add_builtin_node.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_material_graph_workspace_set_input_value", "cdecl"):
        continue
    ctex_material_graph_workspace_set_input_value = _lib.get("ctex_material_graph_workspace_set_input_value", "cdecl")
    ctex_material_graph_workspace_set_input_value.argtypes = [POINTER(ctex_material_graph_workspace), uint32_t, String, uint64_t, String, POINTER(ctex_smart_material_value_descriptor)]
    ctex_material_graph_workspace_set_input_value.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_material_graph_workspace_set_property_value", "cdecl"):
        continue
    ctex_material_graph_workspace_set_property_value = _lib.get("ctex_material_graph_workspace_set_property_value", "cdecl")
    ctex_material_graph_workspace_set_property_value.argtypes = [POINTER(ctex_material_graph_workspace), uint32_t, String, uint64_t, String, POINTER(ctex_smart_material_value_descriptor)]
    ctex_material_graph_workspace_set_property_value.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_material_graph_workspace_add_link", "cdecl"):
        continue
    ctex_material_graph_workspace_add_link = _lib.get("ctex_material_graph_workspace_add_link", "cdecl")
    ctex_material_graph_workspace_add_link.argtypes = [POINTER(ctex_material_graph_workspace), uint32_t, String, POINTER(ctex_material_graph_link_descriptor), POINTER(ctex_material_graph_link_info), String, c_size_t]
    ctex_material_graph_workspace_add_link.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_material_graph_node_registry_create", "cdecl"):
        continue
    ctex_material_graph_node_registry_create = _lib.get("ctex_material_graph_node_registry_create", "cdecl")
    ctex_material_graph_node_registry_create.argtypes = [POINTER(POINTER(ctex_material_graph_node_registry))]
    ctex_material_graph_node_registry_create.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_material_graph_node_registry_destroy", "cdecl"):
        continue
    ctex_material_graph_node_registry_destroy = _lib.get("ctex_material_graph_node_registry_destroy", "cdecl")
    ctex_material_graph_node_registry_destroy.argtypes = [POINTER(ctex_material_graph_node_registry)]
    ctex_material_graph_node_registry_destroy.restype = None
    break


for _lib in _libs.values():
    if not _lib.has("ctex_material_graph_node_registry_get_info", "cdecl"):
        continue
    ctex_material_graph_node_registry_get_info = _lib.get("ctex_material_graph_node_registry_get_info", "cdecl")
    ctex_material_graph_node_registry_get_info.argtypes = [POINTER(ctex_material_graph_node_registry), POINTER(ctex_material_graph_node_registry_info)]
    ctex_material_graph_node_registry_get_info.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_material_graph_node_registry_register", "cdecl"):
        continue
    ctex_material_graph_node_registry_register = _lib.get("ctex_material_graph_node_registry_register", "cdecl")
    ctex_material_graph_node_registry_register.argtypes = [POINTER(ctex_material_graph_node_registry), POINTER(ctex_material_graph_host_node_registration_descriptor)]
    ctex_material_graph_node_registry_register.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_material_graph_add_registered_node", "cdecl"):
        continue
    ctex_material_graph_add_registered_node = _lib.get("ctex_material_graph_add_registered_node", "cdecl")
    ctex_material_graph_add_registered_node.argtypes = [POINTER(ctex_material_graph_node_registry), POINTER(None), c_size_t, String, uint32_t, ctex_vec2f, POINTER(ctex_material_graph_info), POINTER(uint64_t), POINTER(None), c_size_t]
    ctex_material_graph_add_registered_node.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_material_graph_workspace_add_registered_node", "cdecl"):
        continue
    ctex_material_graph_workspace_add_registered_node = _lib.get("ctex_material_graph_workspace_add_registered_node", "cdecl")
    ctex_material_graph_workspace_add_registered_node.argtypes = [POINTER(ctex_material_graph_workspace), POINTER(ctex_material_graph_node_registry), uint32_t, String, String, uint32_t, ctex_vec2f, POINTER(uint64_t)]
    ctex_material_graph_workspace_add_registered_node.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_material_graph_validate_registered", "cdecl"):
        continue
    ctex_material_graph_validate_registered = _lib.get("ctex_material_graph_validate_registered", "cdecl")
    ctex_material_graph_validate_registered.argtypes = [POINTER(ctex_material_graph_node_registry), POINTER(None), c_size_t, uint32_t, POINTER(ctex_material_graph_validation_resources_descriptor), POINTER(ctex_material_graph_validation_info), String, c_size_t]
    ctex_material_graph_validate_registered.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_material_graph_node_registry_verify_contract", "cdecl"):
        continue
    ctex_material_graph_node_registry_verify_contract = _lib.get("ctex_material_graph_node_registry_verify_contract", "cdecl")
    ctex_material_graph_node_registry_verify_contract.argtypes = [POINTER(ctex_material_graph_node_registry), String, uint32_t, POINTER(POINTER(c_char)), c_size_t, POINTER(ctex_material_graph_host_contract_info), String, c_size_t]
    ctex_material_graph_node_registry_verify_contract.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_shader_emit_material", "cdecl"):
        continue
    ctex_shader_emit_material = _lib.get("ctex_shader_emit_material", "cdecl")
    ctex_shader_emit_material.argtypes = [POINTER(ctex_material_graph_node_registry), POINTER(None), c_size_t, POINTER(ctex_shader_material_request), POINTER(ctex_shader_material_info), POINTER(None), c_size_t, POINTER(None), c_size_t, String, c_size_t, String, c_size_t]
    ctex_shader_emit_material.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_shader_emission_cache_create", "cdecl"):
        continue
    ctex_shader_emission_cache_create = _lib.get("ctex_shader_emission_cache_create", "cdecl")
    ctex_shader_emission_cache_create.argtypes = [POINTER(POINTER(ctex_shader_emission_cache))]
    ctex_shader_emission_cache_create.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_shader_emission_cache_destroy", "cdecl"):
        continue
    ctex_shader_emission_cache_destroy = _lib.get("ctex_shader_emission_cache_destroy", "cdecl")
    ctex_shader_emission_cache_destroy.argtypes = [POINTER(ctex_shader_emission_cache)]
    ctex_shader_emission_cache_destroy.restype = None
    break


for _lib in _libs.values():
    if not _lib.has("ctex_shader_emission_cache_get_info", "cdecl"):
        continue
    ctex_shader_emission_cache_get_info = _lib.get("ctex_shader_emission_cache_get_info", "cdecl")
    ctex_shader_emission_cache_get_info.argtypes = [POINTER(ctex_shader_emission_cache), POINTER(ctex_shader_emission_cache_info)]
    ctex_shader_emission_cache_get_info.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_shader_emission_cache_clear", "cdecl"):
        continue
    ctex_shader_emission_cache_clear = _lib.get("ctex_shader_emission_cache_clear", "cdecl")
    ctex_shader_emission_cache_clear.argtypes = [POINTER(ctex_shader_emission_cache)]
    ctex_shader_emission_cache_clear.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_shader_emit_material_cached", "cdecl"):
        continue
    ctex_shader_emit_material_cached = _lib.get("ctex_shader_emit_material_cached", "cdecl")
    ctex_shader_emit_material_cached.argtypes = [POINTER(ctex_shader_emission_cache), POINTER(ctex_material_graph_node_registry), POINTER(None), c_size_t, POINTER(ctex_shader_material_request), POINTER(ctex_shader_material_info), POINTER(None), c_size_t, POINTER(None), c_size_t, String, c_size_t, String, c_size_t, POINTER(uint32_t)]
    ctex_shader_emit_material_cached.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_shader_emit_material_inspectable", "cdecl"):
        continue
    ctex_shader_emit_material_inspectable = _lib.get("ctex_shader_emit_material_inspectable", "cdecl")
    ctex_shader_emit_material_inspectable.argtypes = [POINTER(ctex_shader_emission_cache), POINTER(ctex_material_graph_node_registry), POINTER(ctex_shader_material_source_descriptor), POINTER(ctex_shader_material_request), POINTER(ctex_shader_material_info), POINTER(None), c_size_t, POINTER(None), c_size_t, String, c_size_t, String, c_size_t, POINTER(ctex_shader_material_debug_info), String, c_size_t, POINTER(uint32_t)]
    ctex_shader_emit_material_inspectable.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_shader_get_backend_attribution", "cdecl"):
        continue
    ctex_shader_get_backend_attribution = _lib.get("ctex_shader_get_backend_attribution", "cdecl")
    ctex_shader_get_backend_attribution.argtypes = [POINTER(ctex_shader_backend_attribution_info), String, c_size_t]
    ctex_shader_get_backend_attribution.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_shader_emit_layer_stack", "cdecl"):
        continue
    ctex_shader_emit_layer_stack = _lib.get("ctex_shader_emit_layer_stack", "cdecl")
    ctex_shader_emit_layer_stack.argtypes = [POINTER(ctex_shader_emission_cache), POINTER(ctex_shader_layer_stack_request), POINTER(ctex_shader_layer_stack_info), POINTER(None), c_size_t, String, c_size_t, String, c_size_t, String, c_size_t]
    ctex_shader_emit_layer_stack.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_shader_emit_lit_preview", "cdecl"):
        continue
    ctex_shader_emit_lit_preview = _lib.get("ctex_shader_emit_lit_preview", "cdecl")
    ctex_shader_emit_lit_preview.argtypes = [POINTER(ctex_shader_emission_cache), POINTER(ctex_shader_preview_request), POINTER(ctex_shader_preview_info), POINTER(None), c_size_t, POINTER(None), c_size_t, String, c_size_t, String, c_size_t]
    ctex_shader_emit_lit_preview.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_shader_emit_channel_inspection", "cdecl"):
        continue
    ctex_shader_emit_channel_inspection = _lib.get("ctex_shader_emit_channel_inspection", "cdecl")
    ctex_shader_emit_channel_inspection.argtypes = [POINTER(ctex_shader_emission_cache), POINTER(ctex_shader_preview_request), String, POINTER(ctex_shader_preview_info), POINTER(None), c_size_t, POINTER(None), c_size_t, String, c_size_t, String, c_size_t]
    ctex_shader_emit_channel_inspection.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_smart_material_inspect", "cdecl"):
        continue
    ctex_smart_material_inspect = _lib.get("ctex_smart_material_inspect", "cdecl")
    ctex_smart_material_inspect.argtypes = [POINTER(None), c_size_t, POINTER(ctex_smart_material_info), POINTER(None), c_size_t, String, c_size_t]
    ctex_smart_material_inspect.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_smart_material_set_parameter", "cdecl"):
        continue
    ctex_smart_material_set_parameter = _lib.get("ctex_smart_material_set_parameter", "cdecl")
    ctex_smart_material_set_parameter.argtypes = [POINTER(None), c_size_t, String, POINTER(ctex_smart_material_value_descriptor), POINTER(ctex_smart_material_info), POINTER(None), c_size_t, String, c_size_t]
    ctex_smart_material_set_parameter.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_smart_material_set_anchor", "cdecl"):
        continue
    ctex_smart_material_set_anchor = _lib.get("ctex_smart_material_set_anchor", "cdecl")
    ctex_smart_material_set_anchor.argtypes = [POINTER(None), c_size_t, String, uint32_t, POINTER(ctex_smart_material_info), POINTER(None), c_size_t, String, c_size_t]
    ctex_smart_material_set_anchor.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_smart_material_add_anchor_reference", "cdecl"):
        continue
    ctex_smart_material_add_anchor_reference = _lib.get("ctex_smart_material_add_anchor_reference", "cdecl")
    ctex_smart_material_add_anchor_reference.argtypes = [POINTER(None), c_size_t, String, String, uint64_t, String, POINTER(ctex_smart_material_info), POINTER(None), c_size_t, String, c_size_t]
    ctex_smart_material_add_anchor_reference.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_smart_material_plan_anchor_evaluation", "cdecl"):
        continue
    ctex_smart_material_plan_anchor_evaluation = _lib.get("ctex_smart_material_plan_anchor_evaluation", "cdecl")
    ctex_smart_material_plan_anchor_evaluation.argtypes = [POINTER(None), c_size_t, POINTER(POINTER(c_char)), c_size_t, String, c_size_t, POINTER(c_size_t)]
    ctex_smart_material_plan_anchor_evaluation.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_smart_material_package", "cdecl"):
        continue
    ctex_smart_material_package = _lib.get("ctex_smart_material_package", "cdecl")
    ctex_smart_material_package.argtypes = [POINTER(None), c_size_t, POINTER(ctex_project_resource_descriptor), c_size_t, POINTER(ctex_project_asset_export_options_descriptor), POINTER(ctex_project_container_info), POINTER(None), c_size_t, String, c_size_t]
    ctex_smart_material_package.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_smart_material_import", "cdecl"):
        continue
    ctex_smart_material_import = _lib.get("ctex_smart_material_import", "cdecl")
    ctex_smart_material_import.argtypes = [POINTER(None), c_size_t, POINTER(ctex_project_container_read_limits_descriptor), POINTER(ctex_project_asset_search_paths_descriptor), POINTER(ctex_smart_material_info), POINTER(None), c_size_t, String, c_size_t]
    ctex_smart_material_import.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_preset_library_enumerate", "cdecl"):
        continue
    ctex_preset_library_enumerate = _lib.get("ctex_preset_library_enumerate", "cdecl")
    ctex_preset_library_enumerate.argtypes = [POINTER(ctex_preset_library_descriptor), POINTER(ctex_preset_library_info), String, c_size_t]
    ctex_preset_library_enumerate.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_preset_library_resolve", "cdecl"):
        continue
    ctex_preset_library_resolve = _lib.get("ctex_preset_library_resolve", "cdecl")
    ctex_preset_library_resolve.argtypes = [POINTER(ctex_preset_library_descriptor), String, POINTER(ctex_project_container_info), POINTER(None), c_size_t, String, c_size_t]
    ctex_preset_library_resolve.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_stroke_settings_init", "cdecl"):
        continue
    ctex_stroke_settings_init = _lib.get("ctex_stroke_settings_init", "cdecl")
    ctex_stroke_settings_init.argtypes = [POINTER(ctex_stroke_settings_descriptor)]
    ctex_stroke_settings_init.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_stroke_resolve", "cdecl"):
        continue
    ctex_stroke_resolve = _lib.get("ctex_stroke_resolve", "cdecl")
    ctex_stroke_resolve.argtypes = [POINTER(ctex_stroke_settings_descriptor), POINTER(ctex_stroke_input_sample), c_size_t, POINTER(ctex_resolved_stroke_info), POINTER(ctex_resolved_stamp), c_size_t, POINTER(c_size_t), POINTER(ctex_swept_segment), c_size_t, POINTER(c_size_t)]
    ctex_stroke_resolve.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_stroke_preset_serialize", "cdecl"):
        continue
    ctex_stroke_preset_serialize = _lib.get("ctex_stroke_preset_serialize", "cdecl")
    ctex_stroke_preset_serialize.argtypes = [String, POINTER(ctex_stroke_settings_descriptor), String, c_size_t, POINTER(c_size_t)]
    ctex_stroke_preset_serialize.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_stroke_preset_deserialize", "cdecl"):
        continue
    ctex_stroke_preset_deserialize = _lib.get("ctex_stroke_preset_deserialize", "cdecl")
    ctex_stroke_preset_deserialize.argtypes = [String, c_size_t, POINTER(ctex_stroke_preset_info), POINTER(ctex_stroke_settings_descriptor), POINTER(ctex_stroke_preset_buffers_descriptor)]
    ctex_stroke_preset_deserialize.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_paint_evaluate_tile_coverage", "cdecl"):
        continue
    ctex_paint_evaluate_tile_coverage = _lib.get("ctex_paint_evaluate_tile_coverage", "cdecl")
    ctex_paint_evaluate_tile_coverage.argtypes = [POINTER(ctex_mesh), POINTER(ctex_paint_tile_coverage_descriptor), POINTER(ctex_resolved_stroke_descriptor), POINTER(c_double), c_size_t, POINTER(c_size_t)]
    ctex_paint_evaluate_tile_coverage.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_paint_evaluate_material_coordinates", "cdecl"):
        continue
    ctex_paint_evaluate_material_coordinates = _lib.get("ctex_paint_evaluate_material_coordinates", "cdecl")
    ctex_paint_evaluate_material_coordinates.argtypes = [POINTER(ctex_mesh), POINTER(ctex_paint_tile_coverage_descriptor), POINTER(ctex_paint_material_coordinate_descriptor), POINTER(ctex_paint_material_coordinate_sample), c_size_t, POINTER(c_size_t)]
    ctex_paint_evaluate_material_coordinates.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_paint_rejection_init", "cdecl"):
        continue
    ctex_paint_rejection_init = _lib.get("ctex_paint_rejection_init", "cdecl")
    ctex_paint_rejection_init.argtypes = [POINTER(ctex_paint_rejection_descriptor)]
    ctex_paint_rejection_init.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_paint_evaluate_rejected_coverage", "cdecl"):
        continue
    ctex_paint_evaluate_rejected_coverage = _lib.get("ctex_paint_evaluate_rejected_coverage", "cdecl")
    ctex_paint_evaluate_rejected_coverage.argtypes = [POINTER(ctex_mesh), POINTER(ctex_paint_tile_coverage_descriptor), POINTER(ctex_resolved_stroke_descriptor), POINTER(ctex_paint_rejection_descriptor), POINTER(ctex_paint_rejection_info), POINTER(c_double), c_size_t, POINTER(c_size_t)]
    ctex_paint_evaluate_rejected_coverage.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_paint_work_init", "cdecl"):
        continue
    ctex_paint_work_init = _lib.get("ctex_paint_work_init", "cdecl")
    ctex_paint_work_init.argtypes = [POINTER(ctex_paint_work_descriptor)]
    ctex_paint_work_init.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_paint_plan_work", "cdecl"):
        continue
    ctex_paint_plan_work = _lib.get("ctex_paint_plan_work", "cdecl")
    ctex_paint_plan_work.argtypes = [POINTER(ctex_paint_work_descriptor), POINTER(ctex_paint_work_info), POINTER(ctex_paint_tile_coordinate), c_size_t, POINTER(c_size_t)]
    ctex_paint_plan_work.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_paint_seam_dilation_init", "cdecl"):
        continue
    ctex_paint_seam_dilation_init = _lib.get("ctex_paint_seam_dilation_init", "cdecl")
    ctex_paint_seam_dilation_init.argtypes = [POINTER(ctex_paint_seam_dilation_descriptor)]
    ctex_paint_seam_dilation_init.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_paint_dilate_uv_seams", "cdecl"):
        continue
    ctex_paint_dilate_uv_seams = _lib.get("ctex_paint_dilate_uv_seams", "cdecl")
    ctex_paint_dilate_uv_seams.argtypes = [POINTER(ctex_paint_seam_dilation_descriptor), POINTER(ctex_paint_seam_dilation_info), POINTER(c_double), c_size_t, POINTER(c_size_t)]
    ctex_paint_dilate_uv_seams.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_paint_dilation_session_create", "cdecl"):
        continue
    ctex_paint_dilation_session_create = _lib.get("ctex_paint_dilation_session_create", "cdecl")
    ctex_paint_dilation_session_create.argtypes = [uint32_t, POINTER(POINTER(ctex_paint_dilation_session))]
    ctex_paint_dilation_session_create.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_paint_dilation_session_destroy", "cdecl"):
        continue
    ctex_paint_dilation_session_destroy = _lib.get("ctex_paint_dilation_session_destroy", "cdecl")
    ctex_paint_dilation_session_destroy.argtypes = [POINTER(ctex_paint_dilation_session)]
    ctex_paint_dilation_session_destroy.restype = None
    break


for _lib in _libs.values():
    if not _lib.has("ctex_paint_dilation_session_stage_tile", "cdecl"):
        continue
    ctex_paint_dilation_session_stage_tile = _lib.get("ctex_paint_dilation_session_stage_tile", "cdecl")
    ctex_paint_dilation_session_stage_tile.argtypes = [POINTER(ctex_paint_dilation_session), POINTER(ctex_paint_dilation_tile_descriptor)]
    ctex_paint_dilation_session_stage_tile.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_paint_dilation_session_get_preview", "cdecl"):
        continue
    ctex_paint_dilation_session_get_preview = _lib.get("ctex_paint_dilation_session_get_preview", "cdecl")
    ctex_paint_dilation_session_get_preview.argtypes = [POINTER(ctex_paint_dilation_session), POINTER(ctex_paint_dilation_session_info), POINTER(ctex_paint_dilation_tile_info), c_size_t, POINTER(c_size_t), POINTER(c_double), c_size_t, POINTER(c_size_t)]
    ctex_paint_dilation_session_get_preview.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_paint_dilation_session_finish", "cdecl"):
        continue
    ctex_paint_dilation_session_finish = _lib.get("ctex_paint_dilation_session_finish", "cdecl")
    ctex_paint_dilation_session_finish.argtypes = [POINTER(ctex_paint_dilation_session), POINTER(ctex_paint_dilation_session_info), POINTER(ctex_paint_dilation_tile_info), c_size_t, POINTER(c_size_t), POINTER(c_double), c_size_t, POINTER(c_size_t)]
    ctex_paint_dilation_session_finish.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_paint_filter_surface_scalar", "cdecl"):
        continue
    ctex_paint_filter_surface_scalar = _lib.get("ctex_paint_filter_surface_scalar", "cdecl")
    ctex_paint_filter_surface_scalar.argtypes = [POINTER(ctex_paint_surface_filter_descriptor), POINTER(c_double), c_size_t, POINTER(c_double)]
    ctex_paint_filter_surface_scalar.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_paint_filter_surface_tangent_vector", "cdecl"):
        continue
    ctex_paint_filter_surface_tangent_vector = _lib.get("ctex_paint_filter_surface_tangent_vector", "cdecl")
    ctex_paint_filter_surface_tangent_vector.argtypes = [POINTER(ctex_paint_surface_filter_descriptor), POINTER(ctex_vec3d), c_size_t, POINTER(ctex_vec3d)]
    ctex_paint_filter_surface_tangent_vector.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_paint_plan_island_padding", "cdecl"):
        continue
    ctex_paint_plan_island_padding = _lib.get("ctex_paint_plan_island_padding", "cdecl")
    ctex_paint_plan_island_padding.argtypes = [POINTER(ctex_paint_island_padding_descriptor), POINTER(ctex_paint_island_padding_info), POINTER(uint32_t), c_size_t, POINTER(c_size_t), POINTER(ctex_paint_unsupported_mip_level), c_size_t, POINTER(c_size_t), POINTER(uint32_t), c_size_t, POINTER(c_size_t)]
    ctex_paint_plan_island_padding.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_paint_apply_island_padding", "cdecl"):
        continue
    ctex_paint_apply_island_padding = _lib.get("ctex_paint_apply_island_padding", "cdecl")
    ctex_paint_apply_island_padding.argtypes = [POINTER(ctex_paint_island_padding_descriptor), POINTER(ctex_paint_island_padding_info), POINTER(c_double), c_size_t, POINTER(c_size_t)]
    ctex_paint_apply_island_padding.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_paint_surface_map_cache_create", "cdecl"):
        continue
    ctex_paint_surface_map_cache_create = _lib.get("ctex_paint_surface_map_cache_create", "cdecl")
    ctex_paint_surface_map_cache_create.argtypes = [POINTER(POINTER(ctex_paint_surface_map_cache))]
    ctex_paint_surface_map_cache_create.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_paint_surface_map_cache_destroy", "cdecl"):
        continue
    ctex_paint_surface_map_cache_destroy = _lib.get("ctex_paint_surface_map_cache_destroy", "cdecl")
    ctex_paint_surface_map_cache_destroy.argtypes = [POINTER(ctex_paint_surface_map_cache)]
    ctex_paint_surface_map_cache_destroy.restype = None
    break


for _lib in _libs.values():
    if not _lib.has("ctex_paint_surface_map_cache_clear", "cdecl"):
        continue
    ctex_paint_surface_map_cache_clear = _lib.get("ctex_paint_surface_map_cache_clear", "cdecl")
    ctex_paint_surface_map_cache_clear.argtypes = [POINTER(ctex_paint_surface_map_cache)]
    ctex_paint_surface_map_cache_clear.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_paint_surface_map_cache_get_statistics", "cdecl"):
        continue
    ctex_paint_surface_map_cache_get_statistics = _lib.get("ctex_paint_surface_map_cache_get_statistics", "cdecl")
    ctex_paint_surface_map_cache_get_statistics.argtypes = [POINTER(ctex_paint_surface_map_cache), POINTER(ctex_paint_surface_map_statistics)]
    ctex_paint_surface_map_cache_get_statistics.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_paint_surface_map_cache_lookup", "cdecl"):
        continue
    ctex_paint_surface_map_cache_lookup = _lib.get("ctex_paint_surface_map_cache_lookup", "cdecl")
    ctex_paint_surface_map_cache_lookup.argtypes = [POINTER(ctex_paint_surface_map_cache), POINTER(ctex_mesh), POINTER(ctex_paint_surface_map_request), POINTER(ctex_paint_surface_map_info), POINTER(ctex_paint_surface_map_buffers)]
    ctex_paint_surface_map_cache_lookup.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_paint_combine_masks", "cdecl"):
        continue
    ctex_paint_combine_masks = _lib.get("ctex_paint_combine_masks", "cdecl")
    ctex_paint_combine_masks.argtypes = [uint32_t, uint32_t, POINTER(ctex_paint_mask_inputs_descriptor), POINTER(ctex_paint_mask_info), POINTER(c_double), c_size_t, POINTER(c_size_t)]
    ctex_paint_combine_masks.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_paint_evaluate_tile_deposition", "cdecl"):
        continue
    ctex_paint_evaluate_tile_deposition = _lib.get("ctex_paint_evaluate_tile_deposition", "cdecl")
    ctex_paint_evaluate_tile_deposition.argtypes = [POINTER(ctex_mesh), POINTER(ctex_paint_tile_coverage_descriptor), POINTER(ctex_resolved_stroke_descriptor), POINTER(ctex_paint_deposition_descriptor), POINTER(ctex_paint_deposition_info), POINTER(ctex_paint_deposition_sample), c_size_t, POINTER(c_size_t)]
    ctex_paint_evaluate_tile_deposition.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_paint_blend_snapshot", "cdecl"):
        continue
    ctex_paint_blend_snapshot = _lib.get("ctex_paint_blend_snapshot", "cdecl")
    ctex_paint_blend_snapshot.argtypes = [POINTER(ctex_paint_blend_descriptor), POINTER(ctex_vec4f), c_size_t, POINTER(c_size_t)]
    ctex_paint_blend_snapshot.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_paint_apply_brush", "cdecl"):
        continue
    ctex_paint_apply_brush = _lib.get("ctex_paint_apply_brush", "cdecl")
    ctex_paint_apply_brush.argtypes = [POINTER(ctex_paint_brush_descriptor), POINTER(ctex_paint_brush_info), POINTER(ctex_paint_tool_channel_output), c_size_t]
    ctex_paint_apply_brush.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_paint_apply_eraser", "cdecl"):
        continue
    ctex_paint_apply_eraser = _lib.get("ctex_paint_apply_eraser", "cdecl")
    ctex_paint_apply_eraser.argtypes = [POINTER(ctex_paint_eraser_descriptor), POINTER(ctex_paint_eraser_info), POINTER(c_double), c_size_t]
    ctex_paint_apply_eraser.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_paint_apply_fill", "cdecl"):
        continue
    ctex_paint_apply_fill = _lib.get("ctex_paint_apply_fill", "cdecl")
    ctex_paint_apply_fill.argtypes = [POINTER(ctex_paint_fill_descriptor), POINTER(ctex_paint_fill_info), POINTER(ctex_paint_fill_outputs)]
    ctex_paint_apply_fill.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_paint_apply_clone", "cdecl"):
        continue
    ctex_paint_apply_clone = _lib.get("ctex_paint_apply_clone", "cdecl")
    ctex_paint_apply_clone.argtypes = [POINTER(ctex_paint_clone_descriptor), POINTER(ctex_paint_clone_info), POINTER(ctex_paint_clone_outputs)]
    ctex_paint_apply_clone.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_paint_apply_blur", "cdecl"):
        continue
    ctex_paint_apply_blur = _lib.get("ctex_paint_apply_blur", "cdecl")
    ctex_paint_apply_blur.argtypes = [POINTER(ctex_paint_blur_descriptor), POINTER(ctex_paint_blur_info), POINTER(ctex_paint_tool_channel_output), c_size_t]
    ctex_paint_apply_blur.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_paint_apply_smear", "cdecl"):
        continue
    ctex_paint_apply_smear = _lib.get("ctex_paint_apply_smear", "cdecl")
    ctex_paint_apply_smear.argtypes = [POINTER(ctex_paint_smear_descriptor), POINTER(ctex_paint_smear_info), POINTER(ctex_paint_tool_channel_output), c_size_t]
    ctex_paint_apply_smear.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_paint_resolve_stencil_mask", "cdecl"):
        continue
    ctex_paint_resolve_stencil_mask = _lib.get("ctex_paint_resolve_stencil_mask", "cdecl")
    ctex_paint_resolve_stencil_mask.argtypes = [POINTER(ctex_paint_stencil_descriptor), POINTER(ctex_paint_stencil_info), POINTER(c_double), c_size_t]
    ctex_paint_resolve_stencil_mask.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_paint_rasterize_decal", "cdecl"):
        continue
    ctex_paint_rasterize_decal = _lib.get("ctex_paint_rasterize_decal", "cdecl")
    ctex_paint_rasterize_decal.argtypes = [POINTER(ctex_paint_decal_descriptor), POINTER(ctex_paint_decal_info), POINTER(ctex_paint_decal_outputs)]
    ctex_paint_rasterize_decal.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_paint_apply_projection", "cdecl"):
        continue
    ctex_paint_apply_projection = _lib.get("ctex_paint_apply_projection", "cdecl")
    ctex_paint_apply_projection.argtypes = [POINTER(ctex_paint_projection_descriptor), POINTER(ctex_paint_projection_info), POINTER(ctex_paint_projection_outputs)]
    ctex_paint_apply_projection.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_paint_apply_text", "cdecl"):
        continue
    ctex_paint_apply_text = _lib.get("ctex_paint_apply_text", "cdecl")
    ctex_paint_apply_text.argtypes = [POINTER(ctex_paint_text_descriptor), POINTER(ctex_paint_text_info), POINTER(ctex_paint_text_outputs)]
    ctex_paint_apply_text.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_paint_apply_particles", "cdecl"):
        continue
    ctex_paint_apply_particles = _lib.get("ctex_paint_apply_particles", "cdecl")
    ctex_paint_apply_particles.argtypes = [POINTER(ctex_pick_index), POINTER(ctex_paint_particle_descriptor), POINTER(ctex_paint_particle_info), POINTER(ctex_paint_particle_outputs)]
    ctex_paint_apply_particles.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_paint_pick_enabled_channels", "cdecl"):
        continue
    ctex_paint_pick_enabled_channels = _lib.get("ctex_paint_pick_enabled_channels", "cdecl")
    ctex_paint_pick_enabled_channels.argtypes = [POINTER(ctex_paint_picker_descriptor), POINTER(ctex_paint_picker_info), POINTER(ctex_paint_picker_channel_value), c_size_t, String, c_size_t]
    ctex_paint_pick_enabled_channels.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_paint_select_colour_id", "cdecl"):
        continue
    ctex_paint_select_colour_id = _lib.get("ctex_paint_select_colour_id", "cdecl")
    ctex_paint_select_colour_id.argtypes = [POINTER(ctex_paint_colour_id_descriptor), POINTER(ctex_paint_colour_id_info), POINTER(c_double), c_size_t]
    ctex_paint_select_colour_id.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_paint_get_parameter_catalogue", "cdecl"):
        continue
    ctex_paint_get_parameter_catalogue = _lib.get("ctex_paint_get_parameter_catalogue", "cdecl")
    ctex_paint_get_parameter_catalogue.argtypes = [POINTER(ctex_paint_parameter_descriptor), c_size_t, String, c_size_t, POINTER(ctex_paint_parameter_catalogue_info)]
    ctex_paint_get_parameter_catalogue.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_paint_validate_parameter", "cdecl"):
        continue
    ctex_paint_validate_parameter = _lib.get("ctex_paint_validate_parameter", "cdecl")
    ctex_paint_validate_parameter.argtypes = [String, uint32_t, c_double, POINTER(ctex_paint_parameter_validation_info)]
    ctex_paint_validate_parameter.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_paint_select_screen", "cdecl"):
        continue
    ctex_paint_select_screen = _lib.get("ctex_paint_select_screen", "cdecl")
    ctex_paint_select_screen.argtypes = [POINTER(ctex_pick_index), POINTER(ctex_paint_screen_selection_descriptor), POINTER(ctex_paint_selection_info), POINTER(ctex_paint_selection_outputs)]
    ctex_paint_select_screen.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_paint_select_polygon", "cdecl"):
        continue
    ctex_paint_select_polygon = _lib.get("ctex_paint_select_polygon", "cdecl")
    ctex_paint_select_polygon.argtypes = [POINTER(ctex_paint_polygon_selection_descriptor), POINTER(ctex_paint_selection_info), POINTER(ctex_paint_selection_outputs)]
    ctex_paint_select_polygon.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_set_log_sink", "cdecl"):
        continue
    ctex_set_log_sink = _lib.get("ctex_set_log_sink", "cdecl")
    ctex_set_log_sink.argtypes = [POINTER(ctex_log_sink_descriptor)]
    ctex_set_log_sink.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_set_allocator", "cdecl"):
        continue
    ctex_set_allocator = _lib.get("ctex_set_allocator", "cdecl")
    ctex_set_allocator.argtypes = [POINTER(ctex_allocator_descriptor)]
    ctex_set_allocator.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_mesh_create", "cdecl"):
        continue
    ctex_mesh_create = _lib.get("ctex_mesh_create", "cdecl")
    ctex_mesh_create.argtypes = [POINTER(ctex_mesh_descriptor), POINTER(POINTER(ctex_mesh))]
    ctex_mesh_create.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_mesh_destroy", "cdecl"):
        continue
    ctex_mesh_destroy = _lib.get("ctex_mesh_destroy", "cdecl")
    ctex_mesh_destroy.argtypes = [POINTER(ctex_mesh)]
    ctex_mesh_destroy.restype = None
    break


for _lib in _libs.values():
    if not _lib.has("ctex_mesh_replace", "cdecl"):
        continue
    ctex_mesh_replace = _lib.get("ctex_mesh_replace", "cdecl")
    ctex_mesh_replace.argtypes = [POINTER(ctex_mesh), POINTER(ctex_mesh_descriptor)]
    ctex_mesh_replace.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_mesh_get_info", "cdecl"):
        continue
    ctex_mesh_get_info = _lib.get("ctex_mesh_get_info", "cdecl")
    ctex_mesh_get_info.argtypes = [POINTER(ctex_mesh), POINTER(ctex_mesh_info)]
    ctex_mesh_get_info.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_mesh_get_uv_set_names", "cdecl"):
        continue
    ctex_mesh_get_uv_set_names = _lib.get("ctex_mesh_get_uv_set_names", "cdecl")
    ctex_mesh_get_uv_set_names.argtypes = [POINTER(ctex_mesh), String, c_size_t, POINTER(c_size_t), POINTER(c_size_t)]
    ctex_mesh_get_uv_set_names.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_mesh_analyze_uv_overlaps", "cdecl"):
        continue
    ctex_mesh_analyze_uv_overlaps = _lib.get("ctex_mesh_analyze_uv_overlaps", "cdecl")
    ctex_mesh_analyze_uv_overlaps.argtypes = [POINTER(ctex_mesh), String, uint32_t, POINTER(ctex_mesh_uv_overlap_info), POINTER(uint32_t), c_size_t]
    ctex_mesh_analyze_uv_overlaps.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_mesh_analyze_uv_coverage", "cdecl"):
        continue
    ctex_mesh_analyze_uv_coverage = _lib.get("ctex_mesh_analyze_uv_coverage", "cdecl")
    ctex_mesh_analyze_uv_coverage.argtypes = [POINTER(ctex_mesh), String, uint32_t, uint32_t, uint32_t, POINTER(ctex_mesh_uv_coverage_info), POINTER(uint32_t), c_size_t]
    ctex_mesh_analyze_uv_coverage.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_mesh_replacement_plan_create", "cdecl"):
        continue
    ctex_mesh_replacement_plan_create = _lib.get("ctex_mesh_replacement_plan_create", "cdecl")
    ctex_mesh_replacement_plan_create.argtypes = [POINTER(ctex_document), POINTER(ctex_mesh), POINTER(ctex_mesh_descriptor), POINTER(POINTER(ctex_mesh_replacement_plan))]
    ctex_mesh_replacement_plan_create.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_mesh_replacement_plan_create_with_tangent_data", "cdecl"):
        continue
    ctex_mesh_replacement_plan_create_with_tangent_data = _lib.get("ctex_mesh_replacement_plan_create_with_tangent_data", "cdecl")
    ctex_mesh_replacement_plan_create_with_tangent_data.argtypes = [POINTER(ctex_document), POINTER(ctex_mesh), POINTER(ctex_mesh_descriptor), POINTER(ctex_mesh_tangent_data_descriptor), POINTER(POINTER(ctex_mesh_replacement_plan))]
    ctex_mesh_replacement_plan_create_with_tangent_data.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_mesh_replacement_plan_destroy", "cdecl"):
        continue
    ctex_mesh_replacement_plan_destroy = _lib.get("ctex_mesh_replacement_plan_destroy", "cdecl")
    ctex_mesh_replacement_plan_destroy.argtypes = [POINTER(ctex_mesh_replacement_plan)]
    ctex_mesh_replacement_plan_destroy.restype = None
    break


for _lib in _libs.values():
    if not _lib.has("ctex_mesh_replacement_plan_get_info", "cdecl"):
        continue
    ctex_mesh_replacement_plan_get_info = _lib.get("ctex_mesh_replacement_plan_get_info", "cdecl")
    ctex_mesh_replacement_plan_get_info.argtypes = [POINTER(ctex_mesh_replacement_plan), POINTER(ctex_mesh_replacement_plan_info), POINTER(ctex_mesh_replacement_entry), c_size_t, String, c_size_t]
    ctex_mesh_replacement_plan_get_info.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_mesh_replacement_plan_apply", "cdecl"):
        continue
    ctex_mesh_replacement_plan_apply = _lib.get("ctex_mesh_replacement_plan_apply", "cdecl")
    ctex_mesh_replacement_plan_apply.argtypes = [POINTER(ctex_mesh_replacement_plan), POINTER(ctex_mesh_replacement_decision), c_size_t, POINTER(ctex_mesh_replacement_apply_info)]
    ctex_mesh_replacement_plan_apply.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_mesh_replacement_plan_preflight_reprojection", "cdecl"):
        continue
    ctex_mesh_replacement_plan_preflight_reprojection = _lib.get("ctex_mesh_replacement_plan_preflight_reprojection", "cdecl")
    ctex_mesh_replacement_plan_preflight_reprojection.argtypes = [POINTER(ctex_mesh_replacement_plan), POINTER(ctex_mesh_reprojection_descriptor), POINTER(ctex_mesh_reprojection_preflight_info), String, c_size_t]
    ctex_mesh_replacement_plan_preflight_reprojection.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_mesh_replacement_plan_commit_reprojection", "cdecl"):
        continue
    ctex_mesh_replacement_plan_commit_reprojection = _lib.get("ctex_mesh_replacement_plan_commit_reprojection", "cdecl")
    ctex_mesh_replacement_plan_commit_reprojection.argtypes = [POINTER(ctex_mesh_replacement_plan), uint32_t, uint32_t, POINTER(ctex_mesh_reprojection_commit_info)]
    ctex_mesh_replacement_plan_commit_reprojection.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_mesh_create_with_tangent_data", "cdecl"):
        continue
    ctex_mesh_create_with_tangent_data = _lib.get("ctex_mesh_create_with_tangent_data", "cdecl")
    ctex_mesh_create_with_tangent_data.argtypes = [POINTER(ctex_mesh_descriptor), POINTER(ctex_mesh_tangent_data_descriptor), POINTER(POINTER(ctex_mesh))]
    ctex_mesh_create_with_tangent_data.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_mesh_replace_with_tangent_data", "cdecl"):
        continue
    ctex_mesh_replace_with_tangent_data = _lib.get("ctex_mesh_replace_with_tangent_data", "cdecl")
    ctex_mesh_replace_with_tangent_data.argtypes = [POINTER(ctex_mesh), POINTER(ctex_mesh_descriptor), POINTER(ctex_mesh_tangent_data_descriptor)]
    ctex_mesh_replace_with_tangent_data.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_mesh_get_tangent_frame", "cdecl"):
        continue
    ctex_mesh_get_tangent_frame = _lib.get("ctex_mesh_get_tangent_frame", "cdecl")
    ctex_mesh_get_tangent_frame.argtypes = [POINTER(ctex_mesh), POINTER(ctex_mesh_tangent_frame_info), String, c_size_t]
    ctex_mesh_get_tangent_frame.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_mesh_map_set_create", "cdecl"):
        continue
    ctex_mesh_map_set_create = _lib.get("ctex_mesh_map_set_create", "cdecl")
    ctex_mesh_map_set_create.argtypes = [POINTER(ctex_document), String, POINTER(ctex_mesh), POINTER(POINTER(ctex_mesh_map_set))]
    ctex_mesh_map_set_create.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_mesh_map_set_destroy", "cdecl"):
        continue
    ctex_mesh_map_set_destroy = _lib.get("ctex_mesh_map_set_destroy", "cdecl")
    ctex_mesh_map_set_destroy.argtypes = [POINTER(ctex_mesh_map_set)]
    ctex_mesh_map_set_destroy.restype = None
    break


for _lib in _libs.values():
    if not _lib.has("ctex_mesh_map_kind_get_name", "cdecl"):
        continue
    ctex_mesh_map_kind_get_name = _lib.get("ctex_mesh_map_kind_get_name", "cdecl")
    ctex_mesh_map_kind_get_name.argtypes = [uint32_t, String, c_size_t, POINTER(c_size_t)]
    ctex_mesh_map_kind_get_name.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_mesh_map_set_get_info", "cdecl"):
        continue
    ctex_mesh_map_set_get_info = _lib.get("ctex_mesh_map_set_get_info", "cdecl")
    ctex_mesh_map_set_get_info.argtypes = [POINTER(ctex_mesh_map_set), POINTER(ctex_mesh_map_set_info), String, c_size_t, String, c_size_t]
    ctex_mesh_map_set_get_info.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_mesh_map_set_get_entries", "cdecl"):
        continue
    ctex_mesh_map_set_get_entries = _lib.get("ctex_mesh_map_set_get_entries", "cdecl")
    ctex_mesh_map_set_get_entries.argtypes = [POINTER(ctex_mesh_map_set), POINTER(ctex_mesh_map_entry_info), c_size_t, POINTER(c_size_t)]
    ctex_mesh_map_set_get_entries.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_mesh_map_set_import_external", "cdecl"):
        continue
    ctex_mesh_map_set_import_external = _lib.get("ctex_mesh_map_set_import_external", "cdecl")
    ctex_mesh_map_set_import_external.argtypes = [POINTER(ctex_mesh_map_set), POINTER(ctex_mesh_map_import_descriptor), POINTER(ctex_mesh_map_import_info)]
    ctex_mesh_map_set_import_external.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_mesh_map_set_sample", "cdecl"):
        continue
    ctex_mesh_map_set_sample = _lib.get("ctex_mesh_map_set_sample", "cdecl")
    ctex_mesh_map_set_sample.argtypes = [POINTER(ctex_mesh_map_set), uint32_t, c_double, c_double, POINTER(ctex_mesh_map_sample_info)]
    ctex_mesh_map_set_sample.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_mesh_map_set_check_requirements", "cdecl"):
        continue
    ctex_mesh_map_set_check_requirements = _lib.get("ctex_mesh_map_set_check_requirements", "cdecl")
    ctex_mesh_map_set_check_requirements.argtypes = [POINTER(ctex_mesh_map_set), String, POINTER(uint32_t), c_size_t, POINTER(uint32_t), c_size_t, POINTER(ctex_mesh_map_staleness), c_size_t, POINTER(ctex_mesh_map_requirement_info), String, c_size_t]
    ctex_mesh_map_set_check_requirements.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_mesh_map_set_synchronize_mesh", "cdecl"):
        continue
    ctex_mesh_map_set_synchronize_mesh = _lib.get("ctex_mesh_map_set_synchronize_mesh", "cdecl")
    ctex_mesh_map_set_synchronize_mesh.argtypes = [POINTER(ctex_mesh_map_set), POINTER(ctex_mesh), POINTER(ctex_mesh_map_staleness), c_size_t, POINTER(c_size_t)]
    ctex_mesh_map_set_synchronize_mesh.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_mesh_map_set_release", "cdecl"):
        continue
    ctex_mesh_map_set_release = _lib.get("ctex_mesh_map_set_release", "cdecl")
    ctex_mesh_map_set_release.argtypes = [POINTER(ctex_mesh_map_set), uint32_t, POINTER(ctex_mesh_map_release_info)]
    ctex_mesh_map_set_release.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_mesh_map_set_release_all", "cdecl"):
        continue
    ctex_mesh_map_set_release_all = _lib.get("ctex_mesh_map_set_release_all", "cdecl")
    ctex_mesh_map_set_release_all.argtypes = [POINTER(ctex_mesh_map_set), POINTER(ctex_mesh_map_release_info)]
    ctex_mesh_map_set_release_all.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_mesh_map_generator_get_info", "cdecl"):
        continue
    ctex_mesh_map_generator_get_info = _lib.get("ctex_mesh_map_generator_get_info", "cdecl")
    ctex_mesh_map_generator_get_info.argtypes = [uint32_t, POINTER(ctex_mesh_map_generator_info), POINTER(uint32_t), c_size_t, POINTER(ctex_mesh_map_generator_parameter_descriptor), c_size_t, String, c_size_t]
    ctex_mesh_map_generator_get_info.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_mesh_map_generator_generate", "cdecl"):
        continue
    ctex_mesh_map_generator_generate = _lib.get("ctex_mesh_map_generator_generate", "cdecl")
    ctex_mesh_map_generator_generate.argtypes = [POINTER(ctex_mesh_map_set), uint32_t, uint32_t, uint32_t, POINTER(ctex_mesh_map_generator_parameter), c_size_t, POINTER(ctex_mesh_map_generator_result_info), POINTER(c_float), c_size_t, POINTER(ctex_mesh_map_generator_resolved_parameter), c_size_t, POINTER(ctex_mesh_map_generator_parameter_clamp), c_size_t, POINTER(ctex_mesh_map_staleness), c_size_t, String, c_size_t]
    ctex_mesh_map_generator_generate.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_mesh_map_set_request_bake", "cdecl"):
        continue
    ctex_mesh_map_set_request_bake = _lib.get("ctex_mesh_map_set_request_bake", "cdecl")
    ctex_mesh_map_set_request_bake.argtypes = [POINTER(ctex_mesh_map_set), POINTER(ctex_mesh_map_bake_provider_descriptor), uint32_t, uint32_t, uint32_t, uint64_t, uint64_t, POINTER(ctex_mesh_map_bake_control_descriptor), POINTER(ctex_mesh_map_bake_result_info)]
    ctex_mesh_map_set_request_bake.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_mesh_map_bake_session_create", "cdecl"):
        continue
    ctex_mesh_map_bake_session_create = _lib.get("ctex_mesh_map_bake_session_create", "cdecl")
    ctex_mesh_map_bake_session_create.argtypes = [POINTER(ctex_mesh_map_set), uint64_t, POINTER(POINTER(ctex_mesh_map_bake_session))]
    ctex_mesh_map_bake_session_create.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_mesh_map_bake_session_destroy", "cdecl"):
        continue
    ctex_mesh_map_bake_session_destroy = _lib.get("ctex_mesh_map_bake_session_destroy", "cdecl")
    ctex_mesh_map_bake_session_destroy.argtypes = [POINTER(ctex_mesh_map_bake_session)]
    ctex_mesh_map_bake_session_destroy.restype = None
    break


for _lib in _libs.values():
    if not _lib.has("ctex_mesh_map_bake_session_get_info", "cdecl"):
        continue
    ctex_mesh_map_bake_session_get_info = _lib.get("ctex_mesh_map_bake_session_get_info", "cdecl")
    ctex_mesh_map_bake_session_get_info.argtypes = [POINTER(ctex_mesh_map_bake_session), POINTER(ctex_mesh_map_bake_session_info)]
    ctex_mesh_map_bake_session_get_info.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_mesh_map_bake_session_begin", "cdecl"):
        continue
    ctex_mesh_map_bake_session_begin = _lib.get("ctex_mesh_map_bake_session_begin", "cdecl")
    ctex_mesh_map_bake_session_begin.argtypes = [POINTER(ctex_mesh_map_bake_session), uint32_t, uint32_t, uint32_t, POINTER(POINTER(ctex_mesh_map_bake_request_token))]
    ctex_mesh_map_bake_session_begin.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_mesh_map_bake_session_cancel", "cdecl"):
        continue
    ctex_mesh_map_bake_session_cancel = _lib.get("ctex_mesh_map_bake_session_cancel", "cdecl")
    ctex_mesh_map_bake_session_cancel.argtypes = [POINTER(ctex_mesh_map_bake_session), POINTER(ctex_mesh_map_bake_request_token), POINTER(uint32_t)]
    ctex_mesh_map_bake_session_cancel.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_mesh_map_bake_session_complete", "cdecl"):
        continue
    ctex_mesh_map_bake_session_complete = _lib.get("ctex_mesh_map_bake_session_complete", "cdecl")
    ctex_mesh_map_bake_session_complete.argtypes = [POINTER(ctex_mesh_map_bake_session), POINTER(ctex_mesh_map_bake_request_token), POINTER(ctex_mesh_map_bake_output_descriptor), POINTER(ctex_mesh_map_bake_completion_info)]
    ctex_mesh_map_bake_session_complete.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_mesh_map_bake_session_edit_settings", "cdecl"):
        continue
    ctex_mesh_map_bake_session_edit_settings = _lib.get("ctex_mesh_map_bake_session_edit_settings", "cdecl")
    ctex_mesh_map_bake_session_edit_settings.argtypes = [POINTER(ctex_mesh_map_bake_session), uint64_t, POINTER(ctex_mesh_map_bake_settings_edit_info)]
    ctex_mesh_map_bake_session_edit_settings.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_mesh_map_bake_session_undo_settings", "cdecl"):
        continue
    ctex_mesh_map_bake_session_undo_settings = _lib.get("ctex_mesh_map_bake_session_undo_settings", "cdecl")
    ctex_mesh_map_bake_session_undo_settings.argtypes = [POINTER(ctex_mesh_map_bake_session), POINTER(ctex_mesh_map_bake_settings_undo_info)]
    ctex_mesh_map_bake_session_undo_settings.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_mesh_map_bake_request_token_destroy", "cdecl"):
        continue
    ctex_mesh_map_bake_request_token_destroy = _lib.get("ctex_mesh_map_bake_request_token_destroy", "cdecl")
    ctex_mesh_map_bake_request_token_destroy.argtypes = [POINTER(ctex_mesh_map_bake_request_token)]
    ctex_mesh_map_bake_request_token_destroy.restype = None
    break


for _lib in _libs.values():
    if not _lib.has("ctex_mesh_map_bake_request_token_get_info", "cdecl"):
        continue
    ctex_mesh_map_bake_request_token_get_info = _lib.get("ctex_mesh_map_bake_request_token_get_info", "cdecl")
    ctex_mesh_map_bake_request_token_get_info.argtypes = [POINTER(ctex_mesh_map_bake_request_token), POINTER(ctex_mesh_map_bake_token_info), String, c_size_t, String, c_size_t, String, c_size_t]
    ctex_mesh_map_bake_request_token_get_info.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_pick_index_create", "cdecl"):
        continue
    ctex_pick_index_create = _lib.get("ctex_pick_index_create", "cdecl")
    ctex_pick_index_create.argtypes = [POINTER(ctex_mesh), POINTER(POINTER(ctex_pick_index))]
    ctex_pick_index_create.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_pick_index_destroy", "cdecl"):
        continue
    ctex_pick_index_destroy = _lib.get("ctex_pick_index_destroy", "cdecl")
    ctex_pick_index_destroy.argtypes = [POINTER(ctex_pick_index)]
    ctex_pick_index_destroy.restype = None
    break


for _lib in _libs.values():
    if not _lib.has("ctex_pick_index_get_info", "cdecl"):
        continue
    ctex_pick_index_get_info = _lib.get("ctex_pick_index_get_info", "cdecl")
    ctex_pick_index_get_info.argtypes = [POINTER(ctex_pick_index), POINTER(ctex_pick_index_info)]
    ctex_pick_index_get_info.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_uv_pick_index_create", "cdecl"):
        continue
    ctex_uv_pick_index_create = _lib.get("ctex_uv_pick_index_create", "cdecl")
    ctex_uv_pick_index_create.argtypes = [POINTER(ctex_mesh), String, POINTER(POINTER(ctex_uv_pick_index))]
    ctex_uv_pick_index_create.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_uv_pick_index_destroy", "cdecl"):
        continue
    ctex_uv_pick_index_destroy = _lib.get("ctex_uv_pick_index_destroy", "cdecl")
    ctex_uv_pick_index_destroy.argtypes = [POINTER(ctex_uv_pick_index)]
    ctex_uv_pick_index_destroy.restype = None
    break


for _lib in _libs.values():
    if not _lib.has("ctex_uv_pick_index_get_info", "cdecl"):
        continue
    ctex_uv_pick_index_get_info = _lib.get("ctex_uv_pick_index_get_info", "cdecl")
    ctex_uv_pick_index_get_info.argtypes = [POINTER(ctex_uv_pick_index), POINTER(ctex_pick_index_info)]
    ctex_uv_pick_index_get_info.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_pick_ray_from_screen", "cdecl"):
        continue
    ctex_pick_ray_from_screen = _lib.get("ctex_pick_ray_from_screen", "cdecl")
    ctex_pick_ray_from_screen.argtypes = [ctex_vec2f, POINTER(ctex_pick_screen_view_descriptor), uint32_t, POINTER(ctex_pick_ray)]
    ctex_pick_ray_from_screen.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_pick_ray_query", "cdecl"):
        continue
    ctex_pick_ray_query = _lib.get("ctex_pick_ray_query", "cdecl")
    ctex_pick_ray_query.argtypes = [POINTER(ctex_pick_index), ctex_pick_ray, POINTER(ctex_pick_options_descriptor), POINTER(ctex_pick_texture_set_binding_descriptor), c_size_t, POINTER(ctex_pick_hit), c_size_t, String, c_size_t, POINTER(ctex_pick_query_info)]
    ctex_pick_ray_query.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_pick_uv_query", "cdecl"):
        continue
    ctex_pick_uv_query = _lib.get("ctex_pick_uv_query", "cdecl")
    ctex_pick_uv_query.argtypes = [POINTER(ctex_uv_pick_index), ctex_vec2f, POINTER(ctex_pick_texture_set_binding_descriptor), POINTER(ctex_pick_hit), String, c_size_t, POINTER(ctex_pick_query_info)]
    ctex_pick_uv_query.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_pick_snap_to_surface", "cdecl"):
        continue
    ctex_pick_snap_to_surface = _lib.get("ctex_pick_snap_to_surface", "cdecl")
    ctex_pick_snap_to_surface.argtypes = [POINTER(ctex_pick_index), ctex_vec3f, c_float, POINTER(ctex_pick_texture_set_binding_descriptor), c_size_t, POINTER(ctex_pick_hit), String, c_size_t, POINTER(ctex_pick_query_info)]
    ctex_pick_snap_to_surface.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_pick_query_screen_rectangle", "cdecl"):
        continue
    ctex_pick_query_screen_rectangle = _lib.get("ctex_pick_query_screen_rectangle", "cdecl")
    ctex_pick_query_screen_rectangle.argtypes = [POINTER(ctex_pick_index), ctex_vec2f, ctex_vec2f, POINTER(ctex_pick_screen_view_descriptor), POINTER(uint32_t), c_size_t, POINTER(ctex_pick_query_info)]
    ctex_pick_query_screen_rectangle.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_pick_query_screen_lasso", "cdecl"):
        continue
    ctex_pick_query_screen_lasso = _lib.get("ctex_pick_query_screen_lasso", "cdecl")
    ctex_pick_query_screen_lasso.argtypes = [POINTER(ctex_pick_index), POINTER(ctex_vec2f), c_size_t, POINTER(ctex_pick_screen_view_descriptor), POINTER(uint32_t), c_size_t, POINTER(ctex_pick_query_info)]
    ctex_pick_query_screen_lasso.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_pick_query_world_sphere", "cdecl"):
        continue
    ctex_pick_query_world_sphere = _lib.get("ctex_pick_query_world_sphere", "cdecl")
    ctex_pick_query_world_sphere.argtypes = [POINTER(ctex_pick_index), ctex_vec3f, c_float, POINTER(uint32_t), c_size_t, POINTER(ctex_pick_query_info)]
    ctex_pick_query_world_sphere.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_pick_query_world_box", "cdecl"):
        continue
    ctex_pick_query_world_box = _lib.get("ctex_pick_query_world_box", "cdecl")
    ctex_pick_query_world_box.argtypes = [POINTER(ctex_pick_index), ctex_vec3f, ctex_vec3f, POINTER(uint32_t), c_size_t, POINTER(ctex_pick_query_info)]
    ctex_pick_query_world_box.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_pick_nearest_batch", "cdecl"):
        continue
    ctex_pick_nearest_batch = _lib.get("ctex_pick_nearest_batch", "cdecl")
    ctex_pick_nearest_batch.argtypes = [POINTER(ctex_pick_index), POINTER(ctex_pick_ray), c_size_t, c_float, uint32_t, POINTER(ctex_pick_texture_set_binding_descriptor), c_size_t, POINTER(ctex_pick_batch_control_descriptor), POINTER(ctex_pick_hit), c_size_t, String, c_size_t, POINTER(ctex_pick_batch_info)]
    ctex_pick_nearest_batch.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_document_create", "cdecl"):
        continue
    ctex_document_create = _lib.get("ctex_document_create", "cdecl")
    ctex_document_create.argtypes = [POINTER(POINTER(ctex_document))]
    ctex_document_create.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_document_destroy", "cdecl"):
        continue
    ctex_document_destroy = _lib.get("ctex_document_destroy", "cdecl")
    ctex_document_destroy.argtypes = [POINTER(ctex_document)]
    ctex_document_destroy.restype = None
    break


for _lib in _libs.values():
    if not _lib.has("ctex_document_create_texture_set", "cdecl"):
        continue
    ctex_document_create_texture_set = _lib.get("ctex_document_create_texture_set", "cdecl")
    ctex_document_create_texture_set.argtypes = [POINTER(ctex_document), POINTER(ctex_texture_set_descriptor)]
    ctex_document_create_texture_set.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_document_create_texture_sets_from_mesh", "cdecl"):
        continue
    ctex_document_create_texture_sets_from_mesh = _lib.get("ctex_document_create_texture_sets_from_mesh", "cdecl")
    ctex_document_create_texture_sets_from_mesh.argtypes = [POINTER(ctex_document), POINTER(ctex_mesh), String, uint32_t, uint32_t, uint8_t]
    ctex_document_create_texture_sets_from_mesh.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_document_get_texture_set_ids", "cdecl"):
        continue
    ctex_document_get_texture_set_ids = _lib.get("ctex_document_get_texture_set_ids", "cdecl")
    ctex_document_get_texture_set_ids.argtypes = [POINTER(ctex_document), String, c_size_t, POINTER(c_size_t), POINTER(c_size_t)]
    ctex_document_get_texture_set_ids.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_document_create_atlas", "cdecl"):
        continue
    ctex_document_create_atlas = _lib.get("ctex_document_create_atlas", "cdecl")
    ctex_document_create_atlas.argtypes = [POINTER(ctex_document), POINTER(ctex_atlas_descriptor)]
    ctex_document_create_atlas.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_document_get_atlas_ids", "cdecl"):
        continue
    ctex_document_get_atlas_ids = _lib.get("ctex_document_get_atlas_ids", "cdecl")
    ctex_document_get_atlas_ids.argtypes = [POINTER(ctex_document), String, c_size_t, POINTER(c_size_t), POINTER(c_size_t)]
    ctex_document_get_atlas_ids.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_document_get_atlas", "cdecl"):
        continue
    ctex_document_get_atlas = _lib.get("ctex_document_get_atlas", "cdecl")
    ctex_document_get_atlas.argtypes = [POINTER(ctex_document), String, POINTER(ctex_atlas_info), POINTER(ctex_atlas_region), c_size_t, String, c_size_t, String, c_size_t]
    ctex_document_get_atlas.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_texture_set_get_channel_ids", "cdecl"):
        continue
    ctex_texture_set_get_channel_ids = _lib.get("ctex_texture_set_get_channel_ids", "cdecl")
    ctex_texture_set_get_channel_ids.argtypes = [POINTER(ctex_document), String, String, c_size_t, POINTER(c_size_t), POINTER(c_size_t)]
    ctex_texture_set_get_channel_ids.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_texture_set_register_channel", "cdecl"):
        continue
    ctex_texture_set_register_channel = _lib.get("ctex_texture_set_register_channel", "cdecl")
    ctex_texture_set_register_channel.argtypes = [POINTER(ctex_document), String, POINTER(ctex_channel_descriptor)]
    ctex_texture_set_register_channel.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_texture_set_set_channel_enabled", "cdecl"):
        continue
    ctex_texture_set_set_channel_enabled = _lib.get("ctex_texture_set_set_channel_enabled", "cdecl")
    ctex_texture_set_set_channel_enabled.argtypes = [POINTER(ctex_document), String, String, uint32_t, uint32_t]
    ctex_texture_set_set_channel_enabled.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_texture_set_get_channel_info", "cdecl"):
        continue
    ctex_texture_set_get_channel_info = _lib.get("ctex_texture_set_get_channel_info", "cdecl")
    ctex_texture_set_get_channel_info.argtypes = [POINTER(ctex_document), String, String, POINTER(ctex_channel_info), String, c_size_t, POINTER(c_size_t)]
    ctex_texture_set_get_channel_info.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_texture_set_get_memory_report", "cdecl"):
        continue
    ctex_texture_set_get_memory_report = _lib.get("ctex_texture_set_get_memory_report", "cdecl")
    ctex_texture_set_get_memory_report.argtypes = [POINTER(ctex_document), String, POINTER(ctex_texture_set_memory_report)]
    ctex_texture_set_get_memory_report.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_texture_set_set_channel_backing_store", "cdecl"):
        continue
    ctex_texture_set_set_channel_backing_store = _lib.get("ctex_texture_set_set_channel_backing_store", "cdecl")
    ctex_texture_set_set_channel_backing_store.argtypes = [POINTER(ctex_document), String, String, uint32_t, POINTER(ctex_tile_backing_store_descriptor)]
    ctex_texture_set_set_channel_backing_store.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_texture_set_evict_channel_tile", "cdecl"):
        continue
    ctex_texture_set_evict_channel_tile = _lib.get("ctex_texture_set_evict_channel_tile", "cdecl")
    ctex_texture_set_evict_channel_tile.argtypes = [POINTER(ctex_document), String, String, uint32_t, uint32_t, uint32_t, POINTER(ctex_tile_eviction_report)]
    ctex_texture_set_evict_channel_tile.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_document_get_memory_report", "cdecl"):
        continue
    ctex_document_get_memory_report = _lib.get("ctex_document_get_memory_report", "cdecl")
    ctex_document_get_memory_report.argtypes = [POINTER(ctex_document), POINTER(ctex_document_memory_info), POINTER(ctex_document_texture_set_memory_info), c_size_t, String, c_size_t]
    ctex_document_get_memory_report.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_texture_set_layer_append", "cdecl"):
        continue
    ctex_texture_set_layer_append = _lib.get("ctex_texture_set_layer_append", "cdecl")
    ctex_texture_set_layer_append.argtypes = [POINTER(ctex_document), String, POINTER(ctex_layer_entry_descriptor), c_size_t]
    ctex_texture_set_layer_append.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_texture_set_layer_inspect", "cdecl"):
        continue
    ctex_texture_set_layer_inspect = _lib.get("ctex_texture_set_layer_inspect", "cdecl")
    ctex_texture_set_layer_inspect.argtypes = [POINTER(ctex_document), String, String, c_size_t, POINTER(c_size_t)]
    ctex_texture_set_layer_inspect.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_texture_set_layer_set_state", "cdecl"):
        continue
    ctex_texture_set_layer_set_state = _lib.get("ctex_texture_set_layer_set_state", "cdecl")
    ctex_texture_set_layer_set_state.argtypes = [POINTER(ctex_document), String, String, String, uint32_t, c_double, String]
    ctex_texture_set_layer_set_state.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_texture_set_layer_set_layout", "cdecl"):
        continue
    ctex_texture_set_layer_set_layout = _lib.get("ctex_texture_set_layer_set_layout", "cdecl")
    ctex_texture_set_layer_set_layout.argtypes = [POINTER(ctex_document), String, String, String, String]
    ctex_texture_set_layer_set_layout.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_texture_set_layer_set_channel", "cdecl"):
        continue
    ctex_texture_set_layer_set_channel = _lib.get("ctex_texture_set_layer_set_channel", "cdecl")
    ctex_texture_set_layer_set_channel.argtypes = [POINTER(ctex_document), String, String, POINTER(ctex_layer_channel_descriptor)]
    ctex_texture_set_layer_set_channel.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_texture_set_layer_record_paint", "cdecl"):
        continue
    ctex_texture_set_layer_record_paint = _lib.get("ctex_texture_set_layer_record_paint", "cdecl")
    ctex_texture_set_layer_record_paint.argtypes = [POINTER(ctex_document), String, String]
    ctex_texture_set_layer_record_paint.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_texture_set_layer_remove", "cdecl"):
        continue
    ctex_texture_set_layer_remove = _lib.get("ctex_texture_set_layer_remove", "cdecl")
    ctex_texture_set_layer_remove.argtypes = [POINTER(ctex_document), String, POINTER(POINTER(c_char)), c_size_t, uint32_t]
    ctex_texture_set_layer_remove.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_texture_set_layer_evaluate_blend", "cdecl"):
        continue
    ctex_texture_set_layer_evaluate_blend = _lib.get("ctex_texture_set_layer_evaluate_blend", "cdecl")
    ctex_texture_set_layer_evaluate_blend.argtypes = [POINTER(ctex_document), String, String, ctex_vec4f, ctex_vec4f, c_double, POINTER(ctex_vec4f)]
    ctex_texture_set_layer_evaluate_blend.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_texture_set_layer_get_applicable_masks", "cdecl"):
        continue
    ctex_texture_set_layer_get_applicable_masks = _lib.get("ctex_texture_set_layer_get_applicable_masks", "cdecl")
    ctex_texture_set_layer_get_applicable_masks.argtypes = [POINTER(ctex_document), String, String, String, c_size_t, POINTER(c_size_t), POINTER(c_size_t)]
    ctex_texture_set_layer_get_applicable_masks.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_texture_set_layer_get_participation", "cdecl"):
        continue
    ctex_texture_set_layer_get_participation = _lib.get("ctex_texture_set_layer_get_participation", "cdecl")
    ctex_texture_set_layer_get_participation.argtypes = [POINTER(ctex_document), String, String, String, POINTER(ctex_layer_mask_sample), c_size_t, POINTER(ctex_layer_participation_info), String, c_size_t]
    ctex_texture_set_layer_get_participation.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_texture_set_layer_composite_cpu", "cdecl"):
        continue
    ctex_texture_set_layer_composite_cpu = _lib.get("ctex_texture_set_layer_composite_cpu", "cdecl")
    ctex_texture_set_layer_composite_cpu.argtypes = [POINTER(ctex_document), String, uint32_t, uint32_t, POINTER(ctex_layer_composite_raster_descriptor), c_size_t, POINTER(ctex_layer_composite_mask_descriptor), c_size_t, POINTER(ctex_layer_composite_info), POINTER(ctex_layer_composite_channel_info), c_size_t, String, c_size_t, POINTER(ctex_vec4f), c_size_t]
    ctex_texture_set_layer_composite_cpu.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_layer_snapshot_create", "cdecl"):
        continue
    ctex_layer_snapshot_create = _lib.get("ctex_layer_snapshot_create", "cdecl")
    ctex_layer_snapshot_create.argtypes = [uint32_t, uint32_t, POINTER(ctex_layer_composite_raster_descriptor), c_size_t, POINTER(ctex_layer_composite_mask_descriptor), c_size_t, POINTER(POINTER(ctex_layer_snapshot))]
    ctex_layer_snapshot_create.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_layer_snapshot_destroy", "cdecl"):
        continue
    ctex_layer_snapshot_destroy = _lib.get("ctex_layer_snapshot_destroy", "cdecl")
    ctex_layer_snapshot_destroy.argtypes = [POINTER(ctex_layer_snapshot)]
    ctex_layer_snapshot_destroy.restype = None
    break


for _lib in _libs.values():
    if not _lib.has("ctex_layer_snapshot_read", "cdecl"):
        continue
    ctex_layer_snapshot_read = _lib.get("ctex_layer_snapshot_read", "cdecl")
    ctex_layer_snapshot_read.argtypes = [POINTER(ctex_layer_snapshot), POINTER(ctex_layer_snapshot_info), POINTER(ctex_layer_snapshot_content_info), c_size_t, POINTER(ctex_layer_snapshot_mask_info), c_size_t, String, c_size_t, POINTER(ctex_vec4f), c_size_t, POINTER(c_float), c_size_t, POINTER(c_double), c_size_t]
    ctex_layer_snapshot_read.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_texture_set_layer_composite_snapshot_cpu", "cdecl"):
        continue
    ctex_texture_set_layer_composite_snapshot_cpu = _lib.get("ctex_texture_set_layer_composite_snapshot_cpu", "cdecl")
    ctex_texture_set_layer_composite_snapshot_cpu.argtypes = [POINTER(ctex_document), String, POINTER(ctex_layer_snapshot), POINTER(ctex_layer_composite_info), POINTER(ctex_layer_composite_channel_info), c_size_t, String, c_size_t, POINTER(ctex_vec4f), c_size_t]
    ctex_texture_set_layer_composite_snapshot_cpu.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_texture_set_apply_layer_operation", "cdecl"):
        continue
    ctex_texture_set_apply_layer_operation = _lib.get("ctex_texture_set_apply_layer_operation", "cdecl")
    ctex_texture_set_apply_layer_operation.argtypes = [POINTER(ctex_document), String, POINTER(ctex_layer_snapshot), POINTER(ctex_layer_operation_descriptor), POINTER(ctex_layer_operation_info), String, c_size_t]
    ctex_texture_set_apply_layer_operation.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_texture_set_begin_transaction", "cdecl"):
        continue
    ctex_texture_set_begin_transaction = _lib.get("ctex_texture_set_begin_transaction", "cdecl")
    ctex_texture_set_begin_transaction.argtypes = [POINTER(ctex_document), String, String, POINTER(ctex_tile_history_target_descriptor), c_size_t, POINTER(ctex_layer_snapshot), POINTER(POINTER(ctex_texture_set_transaction))]
    ctex_texture_set_begin_transaction.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_texture_set_transaction_destroy", "cdecl"):
        continue
    ctex_texture_set_transaction_destroy = _lib.get("ctex_texture_set_transaction_destroy", "cdecl")
    ctex_texture_set_transaction_destroy.argtypes = [POINTER(ctex_texture_set_transaction)]
    ctex_texture_set_transaction_destroy.restype = None
    break


for _lib in _libs.values():
    if not _lib.has("ctex_texture_set_transaction_write_pixel", "cdecl"):
        continue
    ctex_texture_set_transaction_write_pixel = _lib.get("ctex_texture_set_transaction_write_pixel", "cdecl")
    ctex_texture_set_transaction_write_pixel.argtypes = [POINTER(ctex_texture_set_transaction), String, uint32_t, uint32_t, POINTER(None), c_size_t]
    ctex_texture_set_transaction_write_pixel.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_texture_set_transaction_apply_layer_operation", "cdecl"):
        continue
    ctex_texture_set_transaction_apply_layer_operation = _lib.get("ctex_texture_set_transaction_apply_layer_operation", "cdecl")
    ctex_texture_set_transaction_apply_layer_operation.argtypes = [POINTER(ctex_texture_set_transaction), POINTER(ctex_layer_operation_descriptor)]
    ctex_texture_set_transaction_apply_layer_operation.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_texture_set_transaction_set_layer_state", "cdecl"):
        continue
    ctex_texture_set_transaction_set_layer_state = _lib.get("ctex_texture_set_transaction_set_layer_state", "cdecl")
    ctex_texture_set_transaction_set_layer_state.argtypes = [POINTER(ctex_texture_set_transaction), String, String, uint32_t, c_double, String]
    ctex_texture_set_transaction_set_layer_state.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_texture_set_transaction_set_layer_layout", "cdecl"):
        continue
    ctex_texture_set_transaction_set_layer_layout = _lib.get("ctex_texture_set_transaction_set_layer_layout", "cdecl")
    ctex_texture_set_transaction_set_layer_layout.argtypes = [POINTER(ctex_texture_set_transaction), String, String, String]
    ctex_texture_set_transaction_set_layer_layout.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_texture_set_transaction_set_layer_channel", "cdecl"):
        continue
    ctex_texture_set_transaction_set_layer_channel = _lib.get("ctex_texture_set_transaction_set_layer_channel", "cdecl")
    ctex_texture_set_transaction_set_layer_channel.argtypes = [POINTER(ctex_texture_set_transaction), String, POINTER(ctex_layer_channel_descriptor)]
    ctex_texture_set_transaction_set_layer_channel.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_texture_set_transaction_set_fill_graph", "cdecl"):
        continue
    ctex_texture_set_transaction_set_fill_graph = _lib.get("ctex_texture_set_transaction_set_fill_graph", "cdecl")
    ctex_texture_set_transaction_set_fill_graph.argtypes = [POINTER(ctex_texture_set_transaction), String, POINTER(None), c_size_t]
    ctex_texture_set_transaction_set_fill_graph.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_texture_set_transaction_commit", "cdecl"):
        continue
    ctex_texture_set_transaction_commit = _lib.get("ctex_texture_set_transaction_commit", "cdecl")
    ctex_texture_set_transaction_commit.argtypes = [POINTER(ctex_texture_set_transaction), POINTER(ctex_tile_history_commit_info)]
    ctex_texture_set_transaction_commit.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_texture_set_transaction_cancel", "cdecl"):
        continue
    ctex_texture_set_transaction_cancel = _lib.get("ctex_texture_set_transaction_cancel", "cdecl")
    ctex_texture_set_transaction_cancel.argtypes = [POINTER(ctex_texture_set_transaction)]
    ctex_texture_set_transaction_cancel.restype = None
    break


for _lib in _libs.values():
    if not _lib.has("ctex_texture_set_configure_tile_history", "cdecl"):
        continue
    ctex_texture_set_configure_tile_history = _lib.get("ctex_texture_set_configure_tile_history", "cdecl")
    ctex_texture_set_configure_tile_history.argtypes = [POINTER(ctex_document), String, c_size_t]
    ctex_texture_set_configure_tile_history.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_texture_set_get_tile_history_budget", "cdecl"):
        continue
    ctex_texture_set_get_tile_history_budget = _lib.get("ctex_texture_set_get_tile_history_budget", "cdecl")
    ctex_texture_set_get_tile_history_budget.argtypes = [POINTER(ctex_document), String, c_size_t, POINTER(ctex_tile_history_budget_report)]
    ctex_texture_set_get_tile_history_budget.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_texture_set_begin_tile_history", "cdecl"):
        continue
    ctex_texture_set_begin_tile_history = _lib.get("ctex_texture_set_begin_tile_history", "cdecl")
    ctex_texture_set_begin_tile_history.argtypes = [POINTER(ctex_document), String, String, POINTER(ctex_tile_history_target_descriptor), c_size_t, POINTER(POINTER(ctex_tile_history_capture))]
    ctex_texture_set_begin_tile_history.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_tile_history_capture_destroy", "cdecl"):
        continue
    ctex_tile_history_capture_destroy = _lib.get("ctex_tile_history_capture_destroy", "cdecl")
    ctex_tile_history_capture_destroy.argtypes = [POINTER(ctex_tile_history_capture)]
    ctex_tile_history_capture_destroy.restype = None
    break


for _lib in _libs.values():
    if not _lib.has("ctex_tile_history_capture_commit", "cdecl"):
        continue
    ctex_tile_history_capture_commit = _lib.get("ctex_tile_history_capture_commit", "cdecl")
    ctex_tile_history_capture_commit.argtypes = [POINTER(ctex_tile_history_capture), POINTER(ctex_tile_history_commit_info)]
    ctex_tile_history_capture_commit.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_texture_set_undo_tiles", "cdecl"):
        continue
    ctex_texture_set_undo_tiles = _lib.get("ctex_texture_set_undo_tiles", "cdecl")
    ctex_texture_set_undo_tiles.argtypes = [POINTER(ctex_document), String, POINTER(ctex_tile_history_restore_info)]
    ctex_texture_set_undo_tiles.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_texture_set_redo_tiles", "cdecl"):
        continue
    ctex_texture_set_redo_tiles = _lib.get("ctex_texture_set_redo_tiles", "cdecl")
    ctex_texture_set_redo_tiles.argtypes = [POINTER(ctex_document), String, POINTER(ctex_tile_history_restore_info)]
    ctex_texture_set_redo_tiles.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_texture_set_get_udim_tiles", "cdecl"):
        continue
    ctex_texture_set_get_udim_tiles = _lib.get("ctex_texture_set_get_udim_tiles", "cdecl")
    ctex_texture_set_get_udim_tiles.argtypes = [POINTER(ctex_document), String, POINTER(uint32_t), c_size_t, POINTER(c_size_t)]
    ctex_texture_set_get_udim_tiles.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_texture_set_ensure_udim_tiles", "cdecl"):
        continue
    ctex_texture_set_ensure_udim_tiles = _lib.get("ctex_texture_set_ensure_udim_tiles", "cdecl")
    ctex_texture_set_ensure_udim_tiles.argtypes = [POINTER(ctex_document), String, POINTER(uint32_t), c_size_t, POINTER(c_size_t)]
    ctex_texture_set_ensure_udim_tiles.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_texture_set_write_udim_pixels", "cdecl"):
        continue
    ctex_texture_set_write_udim_pixels = _lib.get("ctex_texture_set_write_udim_pixels", "cdecl")
    ctex_texture_set_write_udim_pixels.argtypes = [POINTER(ctex_document), String, String, POINTER(ctex_udim_pixel_write_descriptor), c_size_t, POINTER(ctex_udim_write_info)]
    ctex_texture_set_write_udim_pixels.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_texture_set_read_udim_pixel", "cdecl"):
        continue
    ctex_texture_set_read_udim_pixel = _lib.get("ctex_texture_set_read_udim_pixel", "cdecl")
    ctex_texture_set_read_udim_pixel.argtypes = [POINTER(ctex_document), String, String, uint32_t, uint32_t, uint32_t, POINTER(None), c_size_t, POINTER(c_size_t)]
    ctex_texture_set_read_udim_pixel.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_texture_set_query_channel_delta", "cdecl"):
        continue
    ctex_texture_set_query_channel_delta = _lib.get("ctex_texture_set_query_channel_delta", "cdecl")
    ctex_texture_set_query_channel_delta.argtypes = [POINTER(ctex_document), String, String, ctex_transport_revision_cursor, POINTER(ctex_transport_tile_version), c_size_t, POINTER(ctex_transport_delta_info)]
    ctex_texture_set_query_channel_delta.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_texture_set_reset_channel_revision_history", "cdecl"):
        continue
    ctex_texture_set_reset_channel_revision_history = _lib.get("ctex_texture_set_reset_channel_revision_history", "cdecl")
    ctex_texture_set_reset_channel_revision_history.argtypes = [POINTER(ctex_document), String, String, POINTER(ctex_transport_revision_cursor)]
    ctex_texture_set_reset_channel_revision_history.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_transport_snapshot_pool_create", "cdecl"):
        continue
    ctex_transport_snapshot_pool_create = _lib.get("ctex_transport_snapshot_pool_create", "cdecl")
    ctex_transport_snapshot_pool_create.argtypes = [c_size_t, POINTER(POINTER(ctex_transport_snapshot_pool))]
    ctex_transport_snapshot_pool_create.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_transport_snapshot_pool_destroy", "cdecl"):
        continue
    ctex_transport_snapshot_pool_destroy = _lib.get("ctex_transport_snapshot_pool_destroy", "cdecl")
    ctex_transport_snapshot_pool_destroy.argtypes = [POINTER(ctex_transport_snapshot_pool)]
    ctex_transport_snapshot_pool_destroy.restype = None
    break


for _lib in _libs.values():
    if not _lib.has("ctex_transport_snapshot_pool_get_memory_report", "cdecl"):
        continue
    ctex_transport_snapshot_pool_get_memory_report = _lib.get("ctex_transport_snapshot_pool_get_memory_report", "cdecl")
    ctex_transport_snapshot_pool_get_memory_report.argtypes = [POINTER(ctex_transport_snapshot_pool), POINTER(ctex_transport_snapshot_memory_report)]
    ctex_transport_snapshot_pool_get_memory_report.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_resource_ledger_create", "cdecl"):
        continue
    ctex_resource_ledger_create = _lib.get("ctex_resource_ledger_create", "cdecl")
    ctex_resource_ledger_create.argtypes = [POINTER(POINTER(ctex_resource_ledger))]
    ctex_resource_ledger_create.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_resource_ledger_destroy", "cdecl"):
        continue
    ctex_resource_ledger_destroy = _lib.get("ctex_resource_ledger_destroy", "cdecl")
    ctex_resource_ledger_destroy.argtypes = [POINTER(ctex_resource_ledger)]
    ctex_resource_ledger_destroy.restype = None
    break


for _lib in _libs.values():
    if not _lib.has("ctex_resource_ledger_upsert", "cdecl"):
        continue
    ctex_resource_ledger_upsert = _lib.get("ctex_resource_ledger_upsert", "cdecl")
    ctex_resource_ledger_upsert.argtypes = [POINTER(ctex_resource_ledger), POINTER(ctex_resource_allocation_descriptor)]
    ctex_resource_ledger_upsert.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_resource_ledger_remove", "cdecl"):
        continue
    ctex_resource_ledger_remove = _lib.get("ctex_resource_ledger_remove", "cdecl")
    ctex_resource_ledger_remove.argtypes = [POINTER(ctex_resource_ledger), uint64_t, POINTER(uint32_t)]
    ctex_resource_ledger_remove.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_resource_ledger_get_report", "cdecl"):
        continue
    ctex_resource_ledger_get_report = _lib.get("ctex_resource_ledger_get_report", "cdecl")
    ctex_resource_ledger_get_report.argtypes = [POINTER(ctex_resource_ledger), POINTER(ctex_resource_accounting_report), POINTER(ctex_resource_category_report), c_size_t]
    ctex_resource_ledger_get_report.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_resource_ledger_set_cache_eviction_callback", "cdecl"):
        continue
    ctex_resource_ledger_set_cache_eviction_callback = _lib.get("ctex_resource_ledger_set_cache_eviction_callback", "cdecl")
    ctex_resource_ledger_set_cache_eviction_callback.argtypes = [POINTER(ctex_resource_ledger), ctex_resource_cache_eviction_callback, POINTER(None)]
    ctex_resource_ledger_set_cache_eviction_callback.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_resource_ledger_admit", "cdecl"):
        continue
    ctex_resource_ledger_admit = _lib.get("ctex_resource_ledger_admit", "cdecl")
    ctex_resource_ledger_admit.argtypes = [POINTER(ctex_resource_ledger), POINTER(ctex_resource_admission_descriptor), POINTER(POINTER(ctex_resource_reservation)), POINTER(ctex_resource_admission_report)]
    ctex_resource_ledger_admit.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_resource_ledger_admit_operation_recovery", "cdecl"):
        continue
    ctex_resource_ledger_admit_operation_recovery = _lib.get("ctex_resource_ledger_admit_operation_recovery", "cdecl")
    ctex_resource_ledger_admit_operation_recovery.argtypes = [POINTER(ctex_resource_ledger), POINTER(ctex_resource_budget_limits), POINTER(None), c_size_t, c_size_t, POINTER(POINTER(ctex_resource_reservation)), POINTER(ctex_resource_admission_report)]
    ctex_resource_ledger_admit_operation_recovery.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_resource_ledger_admit_preview_quality", "cdecl"):
        continue
    ctex_resource_ledger_admit_preview_quality = _lib.get("ctex_resource_ledger_admit_preview_quality", "cdecl")
    ctex_resource_ledger_admit_preview_quality.argtypes = [POINTER(ctex_resource_ledger), POINTER(ctex_preview_quality_admission_descriptor), POINTER(POINTER(ctex_resource_reservation)), POINTER(ctex_preview_quality_admission_report)]
    ctex_resource_ledger_admit_preview_quality.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_resource_reservation_destroy", "cdecl"):
        continue
    ctex_resource_reservation_destroy = _lib.get("ctex_resource_reservation_destroy", "cdecl")
    ctex_resource_reservation_destroy.argtypes = [POINTER(ctex_resource_reservation)]
    ctex_resource_reservation_destroy.restype = None
    break


for _lib in _libs.values():
    if not _lib.has("ctex_resource_reservation_release", "cdecl"):
        continue
    ctex_resource_reservation_release = _lib.get("ctex_resource_reservation_release", "cdecl")
    ctex_resource_reservation_release.argtypes = [POINTER(ctex_resource_reservation)]
    ctex_resource_reservation_release.restype = None
    break


for _lib in _libs.values():
    if not _lib.has("ctex_resource_reservation_get_evicted_allocations", "cdecl"):
        continue
    ctex_resource_reservation_get_evicted_allocations = _lib.get("ctex_resource_reservation_get_evicted_allocations", "cdecl")
    ctex_resource_reservation_get_evicted_allocations.argtypes = [POINTER(ctex_resource_reservation), POINTER(uint64_t), c_size_t, POINTER(c_size_t)]
    ctex_resource_reservation_get_evicted_allocations.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_texture_set_query_channel_snapshot", "cdecl"):
        continue
    ctex_texture_set_query_channel_snapshot = _lib.get("ctex_texture_set_query_channel_snapshot", "cdecl")
    ctex_texture_set_query_channel_snapshot.argtypes = [POINTER(ctex_transport_snapshot_pool), POINTER(ctex_document), String, String, ctex_transport_revision_cursor, POINTER(POINTER(ctex_transport_snapshot)), POINTER(ctex_transport_snapshot_query_info)]
    ctex_texture_set_query_channel_snapshot.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_paint_preview_session_query_snapshot", "cdecl"):
        continue
    ctex_paint_preview_session_query_snapshot = _lib.get("ctex_paint_preview_session_query_snapshot", "cdecl")
    ctex_paint_preview_session_query_snapshot.argtypes = [POINTER(ctex_transport_snapshot_pool), POINTER(ctex_paint_preview_session), ctex_transport_revision_cursor, POINTER(POINTER(ctex_transport_snapshot)), POINTER(ctex_transport_snapshot_query_info)]
    ctex_paint_preview_session_query_snapshot.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_transport_snapshot_destroy", "cdecl"):
        continue
    ctex_transport_snapshot_destroy = _lib.get("ctex_transport_snapshot_destroy", "cdecl")
    ctex_transport_snapshot_destroy.argtypes = [POINTER(ctex_transport_snapshot)]
    ctex_transport_snapshot_destroy.restype = None
    break


for _lib in _libs.values():
    if not _lib.has("ctex_transport_snapshot_get_tile_versions", "cdecl"):
        continue
    ctex_transport_snapshot_get_tile_versions = _lib.get("ctex_transport_snapshot_get_tile_versions", "cdecl")
    ctex_transport_snapshot_get_tile_versions.argtypes = [POINTER(ctex_transport_snapshot), POINTER(ctex_transport_tile_version), c_size_t, POINTER(c_size_t)]
    ctex_transport_snapshot_get_tile_versions.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_transport_snapshot_negotiate_format", "cdecl"):
        continue
    ctex_transport_snapshot_negotiate_format = _lib.get("ctex_transport_snapshot_negotiate_format", "cdecl")
    ctex_transport_snapshot_negotiate_format.argtypes = [POINTER(ctex_transport_snapshot), POINTER(ctex_transport_pixel_format), c_size_t, uint32_t, POINTER(ctex_transport_format_selection)]
    ctex_transport_snapshot_negotiate_format.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_transport_snapshot_get_tile_memory_layout", "cdecl"):
        continue
    ctex_transport_snapshot_get_tile_memory_layout = _lib.get("ctex_transport_snapshot_get_tile_memory_layout", "cdecl")
    ctex_transport_snapshot_get_tile_memory_layout.argtypes = [POINTER(ctex_transport_snapshot), ctex_transport_tile_version, POINTER(ctex_transport_format_selection), POINTER(ctex_transport_tile_memory_layout)]
    ctex_transport_snapshot_get_tile_memory_layout.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_transport_snapshot_read_tiles", "cdecl"):
        continue
    ctex_transport_snapshot_read_tiles = _lib.get("ctex_transport_snapshot_read_tiles", "cdecl")
    ctex_transport_snapshot_read_tiles.argtypes = [POINTER(ctex_transport_snapshot), POINTER(ctex_transport_format_selection), POINTER(ctex_transport_tile_readback_destination), c_size_t]
    ctex_transport_snapshot_read_tiles.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_transport_snapshot_begin_readback", "cdecl"):
        continue
    ctex_transport_snapshot_begin_readback = _lib.get("ctex_transport_snapshot_begin_readback", "cdecl")
    ctex_transport_snapshot_begin_readback.argtypes = [POINTER(ctex_transport_snapshot), POINTER(ctex_transport_format_selection), POINTER(ctex_transport_tile_readback_destination), c_size_t, POINTER(POINTER(ctex_transport_readback))]
    ctex_transport_snapshot_begin_readback.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_transport_snapshot_begin_host_readback", "cdecl"):
        continue
    ctex_transport_snapshot_begin_host_readback = _lib.get("ctex_transport_snapshot_begin_host_readback", "cdecl")
    ctex_transport_snapshot_begin_host_readback.argtypes = [POINTER(ctex_transport_snapshot), POINTER(ctex_transport_format_selection), POINTER(ctex_transport_tile_readback_destination), c_size_t, POINTER(POINTER(ctex_transport_readback))]
    ctex_transport_snapshot_begin_host_readback.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_transport_readback_destroy", "cdecl"):
        continue
    ctex_transport_readback_destroy = _lib.get("ctex_transport_readback_destroy", "cdecl")
    ctex_transport_readback_destroy.argtypes = [POINTER(ctex_transport_readback)]
    ctex_transport_readback_destroy.restype = None
    break


for _lib in _libs.values():
    if not _lib.has("ctex_transport_readback_get_info", "cdecl"):
        continue
    ctex_transport_readback_get_info = _lib.get("ctex_transport_readback_get_info", "cdecl")
    ctex_transport_readback_get_info.argtypes = [POINTER(ctex_transport_readback), POINTER(ctex_transport_readback_info), String, c_size_t]
    ctex_transport_readback_get_info.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_transport_readback_complete_host", "cdecl"):
        continue
    ctex_transport_readback_complete_host = _lib.get("ctex_transport_readback_complete_host", "cdecl")
    ctex_transport_readback_complete_host.argtypes = [POINTER(ctex_transport_readback), POINTER(ctex_transport_host_tile_completion), c_size_t]
    ctex_transport_readback_complete_host.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_transport_readback_cancel", "cdecl"):
        continue
    ctex_transport_readback_cancel = _lib.get("ctex_transport_readback_cancel", "cdecl")
    ctex_transport_readback_cancel.argtypes = [POINTER(ctex_transport_readback)]
    ctex_transport_readback_cancel.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_transport_readback_fail", "cdecl"):
        continue
    ctex_transport_readback_fail = _lib.get("ctex_transport_readback_fail", "cdecl")
    ctex_transport_readback_fail.argtypes = [POINTER(ctex_transport_readback), String]
    ctex_transport_readback_fail.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_executor_registry_create", "cdecl"):
        continue
    ctex_executor_registry_create = _lib.get("ctex_executor_registry_create", "cdecl")
    ctex_executor_registry_create.argtypes = [POINTER(ctex_host_executor_descriptor), POINTER(POINTER(ctex_executor_registry))]
    ctex_executor_registry_create.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_executor_registry_destroy", "cdecl"):
        continue
    ctex_executor_registry_destroy = _lib.get("ctex_executor_registry_destroy", "cdecl")
    ctex_executor_registry_destroy.argtypes = [POINTER(ctex_executor_registry)]
    ctex_executor_registry_destroy.restype = None
    break


for _lib in _libs.values():
    if not _lib.has("ctex_executor_registry_get_count", "cdecl"):
        continue
    ctex_executor_registry_get_count = _lib.get("ctex_executor_registry_get_count", "cdecl")
    ctex_executor_registry_get_count.argtypes = [POINTER(ctex_executor_registry), POINTER(c_size_t)]
    ctex_executor_registry_get_count.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_executor_registry_get_info", "cdecl"):
        continue
    ctex_executor_registry_get_info = _lib.get("ctex_executor_registry_get_info", "cdecl")
    ctex_executor_registry_get_info.argtypes = [POINTER(ctex_executor_registry), c_size_t, POINTER(ctex_executor_info), POINTER(uint32_t), c_size_t, String, c_size_t, String, c_size_t, String, c_size_t]
    ctex_executor_registry_get_info.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_executor_registry_select", "cdecl"):
        continue
    ctex_executor_registry_select = _lib.get("ctex_executor_registry_select", "cdecl")
    ctex_executor_registry_select.argtypes = [POINTER(ctex_executor_registry), String, POINTER(ctex_executor_selection_info), String, c_size_t, String, c_size_t]
    ctex_executor_registry_select.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_executor_registry_pin_default", "cdecl"):
        continue
    ctex_executor_registry_pin_default = _lib.get("ctex_executor_registry_pin_default", "cdecl")
    ctex_executor_registry_pin_default.argtypes = [POINTER(ctex_executor_registry), String]
    ctex_executor_registry_pin_default.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_executor_registry_clear_default", "cdecl"):
        continue
    ctex_executor_registry_clear_default = _lib.get("ctex_executor_registry_clear_default", "cdecl")
    ctex_executor_registry_clear_default.argtypes = [POINTER(ctex_executor_registry)]
    ctex_executor_registry_clear_default.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_executor_make_fallback_report", "cdecl"):
        continue
    ctex_executor_make_fallback_report = _lib.get("ctex_executor_make_fallback_report", "cdecl")
    ctex_executor_make_fallback_report.argtypes = [POINTER(ctex_executor_fallback_descriptor), POINTER(ctex_executor_fallback_info), String, c_size_t]
    ctex_executor_make_fallback_report.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_cpu_execute_bounded", "cdecl"):
        continue
    ctex_cpu_execute_bounded = _lib.get("ctex_cpu_execute_bounded", "cdecl")
    ctex_cpu_execute_bounded.argtypes = [POINTER(ctex_cpu_bounded_execution_descriptor), POINTER(POINTER(ctex_cpu_execution_result))]
    ctex_cpu_execute_bounded.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_cpu_execution_result_destroy", "cdecl"):
        continue
    ctex_cpu_execution_result_destroy = _lib.get("ctex_cpu_execution_result_destroy", "cdecl")
    ctex_cpu_execution_result_destroy.argtypes = [POINTER(ctex_cpu_execution_result)]
    ctex_cpu_execution_result_destroy.restype = None
    break


for _lib in _libs.values():
    if not _lib.has("ctex_cpu_execution_result_get_info", "cdecl"):
        continue
    ctex_cpu_execution_result_get_info = _lib.get("ctex_cpu_execution_result_get_info", "cdecl")
    ctex_cpu_execution_result_get_info.argtypes = [POINTER(ctex_cpu_execution_result), POINTER(ctex_cpu_execution_info), String, c_size_t]
    ctex_cpu_execution_result_get_info.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_cpu_reference_rasterize_viewport", "cdecl"):
        continue
    ctex_cpu_reference_rasterize_viewport = _lib.get("ctex_cpu_reference_rasterize_viewport", "cdecl")
    ctex_cpu_reference_rasterize_viewport.argtypes = [POINTER(ctex_cpu_viewport_raster_descriptor), POINTER(ctex_cpu_raster_info), POINTER(ctex_cpu_raster_outputs)]
    ctex_cpu_reference_rasterize_viewport.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_cpu_reference_rasterize_uv", "cdecl"):
        continue
    ctex_cpu_reference_rasterize_uv = _lib.get("ctex_cpu_reference_rasterize_uv", "cdecl")
    ctex_cpu_reference_rasterize_uv.argtypes = [POINTER(ctex_cpu_uv_raster_descriptor), POINTER(ctex_cpu_raster_info), POINTER(ctex_cpu_raster_outputs)]
    ctex_cpu_reference_rasterize_uv.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_executor_parity_get_tolerance", "cdecl"):
        continue
    ctex_executor_parity_get_tolerance = _lib.get("ctex_executor_parity_get_tolerance", "cdecl")
    ctex_executor_parity_get_tolerance.argtypes = [uint32_t, uint32_t, POINTER(ctex_parity_tolerance_info)]
    ctex_executor_parity_get_tolerance.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_executor_compare_parity", "cdecl"):
        continue
    ctex_executor_compare_parity = _lib.get("ctex_executor_compare_parity", "cdecl")
    ctex_executor_compare_parity.argtypes = [POINTER(c_double), POINTER(c_double), c_size_t, uint32_t, uint32_t, POINTER(ctex_parity_comparison_info), String, c_size_t]
    ctex_executor_compare_parity.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_executor_run_parity_gate", "cdecl"):
        continue
    ctex_executor_run_parity_gate = _lib.get("ctex_executor_run_parity_gate", "cdecl")
    ctex_executor_run_parity_gate.argtypes = [POINTER(ctex_executor_registry), POINTER(ctex_parity_fixture_descriptor), c_size_t, POINTER(ctex_parity_executor_binding_descriptor), c_size_t, POINTER(POINTER(ctex_parity_gate_result))]
    ctex_executor_run_parity_gate.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_parity_gate_result_destroy", "cdecl"):
        continue
    ctex_parity_gate_result_destroy = _lib.get("ctex_parity_gate_result_destroy", "cdecl")
    ctex_parity_gate_result_destroy.argtypes = [POINTER(ctex_parity_gate_result)]
    ctex_parity_gate_result_destroy.restype = None
    break


for _lib in _libs.values():
    if not _lib.has("ctex_parity_gate_result_get_info", "cdecl"):
        continue
    ctex_parity_gate_result_get_info = _lib.get("ctex_parity_gate_result_get_info", "cdecl")
    ctex_parity_gate_result_get_info.argtypes = [POINTER(ctex_parity_gate_result), POINTER(ctex_parity_gate_info), String, c_size_t]
    ctex_parity_gate_result_get_info.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_host_execution_session_create", "cdecl"):
        continue
    ctex_host_execution_session_create = _lib.get("ctex_host_execution_session_create", "cdecl")
    ctex_host_execution_session_create.argtypes = [uint64_t, POINTER(POINTER(ctex_host_execution_session))]
    ctex_host_execution_session_create.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_host_execution_session_destroy", "cdecl"):
        continue
    ctex_host_execution_session_destroy = _lib.get("ctex_host_execution_session_destroy", "cdecl")
    ctex_host_execution_session_destroy.argtypes = [POINTER(ctex_host_execution_session)]
    ctex_host_execution_session_destroy.restype = None
    break


for _lib in _libs.values():
    if not _lib.has("ctex_host_execution_session_get_info", "cdecl"):
        continue
    ctex_host_execution_session_get_info = _lib.get("ctex_host_execution_session_get_info", "cdecl")
    ctex_host_execution_session_get_info.argtypes = [POINTER(ctex_host_execution_session), POINTER(ctex_host_execution_session_info)]
    ctex_host_execution_session_get_info.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_host_execution_session_submit", "cdecl"):
        continue
    ctex_host_execution_session_submit = _lib.get("ctex_host_execution_session_submit", "cdecl")
    ctex_host_execution_session_submit.argtypes = [POINTER(ctex_host_execution_session), POINTER(ctex_host_submission_descriptor), POINTER(ctex_host_submission_info)]
    ctex_host_execution_session_submit.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_host_execution_session_cancel", "cdecl"):
        continue
    ctex_host_execution_session_cancel = _lib.get("ctex_host_execution_session_cancel", "cdecl")
    ctex_host_execution_session_cancel.argtypes = [POINTER(ctex_host_execution_session), uint64_t, POINTER(uint32_t)]
    ctex_host_execution_session_cancel.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_host_execution_session_complete", "cdecl"):
        continue
    ctex_host_execution_session_complete = _lib.get("ctex_host_execution_session_complete", "cdecl")
    ctex_host_execution_session_complete.argtypes = [POINTER(ctex_host_execution_session), POINTER(ctex_host_completion_descriptor), POINTER(POINTER(ctex_host_completion_result))]
    ctex_host_execution_session_complete.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_host_execution_session_establish_recovery", "cdecl"):
        continue
    ctex_host_execution_session_establish_recovery = _lib.get("ctex_host_execution_session_establish_recovery", "cdecl")
    ctex_host_execution_session_establish_recovery.argtypes = [POINTER(ctex_host_execution_session), uint64_t, POINTER(ctex_host_recovery_descriptor), POINTER(POINTER(ctex_host_completion_result))]
    ctex_host_execution_session_establish_recovery.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_host_execution_session_get_committed_resource", "cdecl"):
        continue
    ctex_host_execution_session_get_committed_resource = _lib.get("ctex_host_execution_session_get_committed_resource", "cdecl")
    ctex_host_execution_session_get_committed_resource.argtypes = [POINTER(ctex_host_execution_session), String, POINTER(uint32_t), POINTER(uint64_t)]
    ctex_host_execution_session_get_committed_resource.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_host_execution_session_resource_is_held", "cdecl"):
        continue
    ctex_host_execution_session_resource_is_held = _lib.get("ctex_host_execution_session_resource_is_held", "cdecl")
    ctex_host_execution_session_resource_is_held.argtypes = [POINTER(ctex_host_execution_session), String, uint64_t, POINTER(uint32_t)]
    ctex_host_execution_session_resource_is_held.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_host_execution_session_report_device_loss", "cdecl"):
        continue
    ctex_host_execution_session_report_device_loss = _lib.get("ctex_host_execution_session_report_device_loss", "cdecl")
    ctex_host_execution_session_report_device_loss.argtypes = [POINTER(ctex_host_execution_session), POINTER(POINTER(ctex_host_recovery_report))]
    ctex_host_execution_session_report_device_loss.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_host_completion_result_destroy", "cdecl"):
        continue
    ctex_host_completion_result_destroy = _lib.get("ctex_host_completion_result_destroy", "cdecl")
    ctex_host_completion_result_destroy.argtypes = [POINTER(ctex_host_completion_result)]
    ctex_host_completion_result_destroy.restype = None
    break


for _lib in _libs.values():
    if not _lib.has("ctex_host_completion_result_get_info", "cdecl"):
        continue
    ctex_host_completion_result_get_info = _lib.get("ctex_host_completion_result_get_info", "cdecl")
    ctex_host_completion_result_get_info.argtypes = [POINTER(ctex_host_completion_result), POINTER(ctex_host_completion_result_info), POINTER(ctex_host_resource_version), c_size_t, String, c_size_t, String, c_size_t]
    ctex_host_completion_result_get_info.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_host_recovery_report_destroy", "cdecl"):
        continue
    ctex_host_recovery_report_destroy = _lib.get("ctex_host_recovery_report_destroy", "cdecl")
    ctex_host_recovery_report_destroy.argtypes = [POINTER(ctex_host_recovery_report)]
    ctex_host_recovery_report_destroy.restype = None
    break


for _lib in _libs.values():
    if not _lib.has("ctex_host_recovery_report_get_info", "cdecl"):
        continue
    ctex_host_recovery_report_get_info = _lib.get("ctex_host_recovery_report_get_info", "cdecl")
    ctex_host_recovery_report_get_info.argtypes = [POINTER(ctex_host_recovery_report), POINTER(ctex_host_device_loss_info), POINTER(uint64_t), c_size_t, POINTER(ctex_host_resource_version), c_size_t, String, c_size_t]
    ctex_host_recovery_report_get_info.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_texture_set_apply_smart_material", "cdecl"):
        continue
    ctex_texture_set_apply_smart_material = _lib.get("ctex_texture_set_apply_smart_material", "cdecl")
    ctex_texture_set_apply_smart_material.argtypes = [POINTER(ctex_document), String, POINTER(None), c_size_t, String, POINTER(ctex_preset_application_info)]
    ctex_texture_set_apply_smart_material.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_texture_set_apply_smart_mask", "cdecl"):
        continue
    ctex_texture_set_apply_smart_mask = _lib.get("ctex_texture_set_apply_smart_mask", "cdecl")
    ctex_texture_set_apply_smart_mask.argtypes = [POINTER(ctex_document), String, POINTER(None), c_size_t, String, String, POINTER(ctex_preset_application_info)]
    ctex_texture_set_apply_smart_mask.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_texture_set_get_preset_applications", "cdecl"):
        continue
    ctex_texture_set_get_preset_applications = _lib.get("ctex_texture_set_get_preset_applications", "cdecl")
    ctex_texture_set_get_preset_applications.argtypes = [POINTER(ctex_document), String, String, c_size_t, POINTER(c_size_t)]
    ctex_texture_set_get_preset_applications.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_texture_set_set_applied_entry_state", "cdecl"):
        continue
    ctex_texture_set_set_applied_entry_state = _lib.get("ctex_texture_set_set_applied_entry_state", "cdecl")
    ctex_texture_set_set_applied_entry_state.argtypes = [POINTER(ctex_document), String, String, uint32_t, c_double]
    ctex_texture_set_set_applied_entry_state.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_texture_set_undo_last_preset_application", "cdecl"):
        continue
    ctex_texture_set_undo_last_preset_application = _lib.get("ctex_texture_set_undo_last_preset_application", "cdecl")
    ctex_texture_set_undo_last_preset_application.argtypes = [POINTER(ctex_document), String, POINTER(ctex_preset_undo_info)]
    ctex_texture_set_undo_last_preset_application.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_paint_preview_session_create", "cdecl"):
        continue
    ctex_paint_preview_session_create = _lib.get("ctex_paint_preview_session_create", "cdecl")
    ctex_paint_preview_session_create.argtypes = [POINTER(ctex_document), String, String, POINTER(POINTER(ctex_paint_preview_session))]
    ctex_paint_preview_session_create.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_paint_preview_session_destroy", "cdecl"):
        continue
    ctex_paint_preview_session_destroy = _lib.get("ctex_paint_preview_session_destroy", "cdecl")
    ctex_paint_preview_session_destroy.argtypes = [POINTER(ctex_paint_preview_session)]
    ctex_paint_preview_session_destroy.restype = None
    break


for _lib in _libs.values():
    if not _lib.has("ctex_paint_preview_session_write_pixel", "cdecl"):
        continue
    ctex_paint_preview_session_write_pixel = _lib.get("ctex_paint_preview_session_write_pixel", "cdecl")
    ctex_paint_preview_session_write_pixel.argtypes = [POINTER(ctex_paint_preview_session), uint32_t, uint32_t, POINTER(None), c_size_t]
    ctex_paint_preview_session_write_pixel.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_paint_preview_session_get_info", "cdecl"):
        continue
    ctex_paint_preview_session_get_info = _lib.get("ctex_paint_preview_session_get_info", "cdecl")
    ctex_paint_preview_session_get_info.argtypes = [POINTER(ctex_paint_preview_session), POINTER(ctex_paint_preview_info)]
    ctex_paint_preview_session_get_info.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_paint_preview_session_get_pixels", "cdecl"):
        continue
    ctex_paint_preview_session_get_pixels = _lib.get("ctex_paint_preview_session_get_pixels", "cdecl")
    ctex_paint_preview_session_get_pixels.argtypes = [POINTER(ctex_paint_preview_session), POINTER(None), c_size_t, POINTER(c_size_t)]
    ctex_paint_preview_session_get_pixels.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_paint_preview_session_get_changed_tiles", "cdecl"):
        continue
    ctex_paint_preview_session_get_changed_tiles = _lib.get("ctex_paint_preview_session_get_changed_tiles", "cdecl")
    ctex_paint_preview_session_get_changed_tiles.argtypes = [POINTER(ctex_paint_preview_session), POINTER(ctex_paint_tile_coordinate), c_size_t, POINTER(c_size_t)]
    ctex_paint_preview_session_get_changed_tiles.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_paint_preview_session_finalize", "cdecl"):
        continue
    ctex_paint_preview_session_finalize = _lib.get("ctex_paint_preview_session_finalize", "cdecl")
    ctex_paint_preview_session_finalize.argtypes = [POINTER(ctex_paint_preview_session), POINTER(uint8_t), c_size_t, uint32_t, POINTER(ctex_paint_preview_info)]
    ctex_paint_preview_session_finalize.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_paint_preview_session_commit", "cdecl"):
        continue
    ctex_paint_preview_session_commit = _lib.get("ctex_paint_preview_session_commit", "cdecl")
    ctex_paint_preview_session_commit.argtypes = [POINTER(ctex_paint_preview_session), POINTER(ctex_paint_preview_info)]
    ctex_paint_preview_session_commit.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_paint_preview_session_cancel", "cdecl"):
        continue
    ctex_paint_preview_session_cancel = _lib.get("ctex_paint_preview_session_cancel", "cdecl")
    ctex_paint_preview_session_cancel.argtypes = [POINTER(ctex_paint_preview_session)]
    ctex_paint_preview_session_cancel.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_get_last_result", "cdecl"):
        continue
    ctex_get_last_result = _lib.get("ctex_get_last_result", "cdecl")
    ctex_get_last_result.argtypes = []
    ctex_get_last_result.restype = ctex_result
    break


for _lib in _libs.values():
    if not _lib.has("ctex_get_last_diagnostic_code", "cdecl"):
        continue
    ctex_get_last_diagnostic_code = _lib.get("ctex_get_last_diagnostic_code", "cdecl")
    ctex_get_last_diagnostic_code.argtypes = []
    ctex_get_last_diagnostic_code.restype = ctex_diagnostic_code
    break


for _lib in _libs.values():
    if not _lib.has("ctex_get_last_diagnostic", "cdecl"):
        continue
    ctex_get_last_diagnostic = _lib.get("ctex_get_last_diagnostic", "cdecl")
    ctex_get_last_diagnostic.argtypes = []
    ctex_get_last_diagnostic.restype = c_char_p
    break


try:
    UINT32_MAX = 4294967295
except:
    pass


try:
    CTEX_LOG_SINK_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_log_sink_descriptor)))).value
except:
    pass


try:
    CTEX_LOG_SINK_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_log_sink_descriptor)))).value
except:
    pass


try:
    CTEX_ALLOCATOR_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_allocator_descriptor)))).value
except:
    pass


try:
    CTEX_ALLOCATOR_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_allocator_descriptor)))).value
except:
    pass


try:
    CTEX_MAX_MESH_VERTEX_COUNT = (c_size_t (ord_if_char(100000000))).value
except:
    pass


try:
    CTEX_MAX_MESH_TRIANGLE_COUNT = (c_size_t (ord_if_char(100000000))).value
except:
    pass


try:
    CTEX_DEFAULT_TILE_SIZE = (uint32_t (ord_if_char(64))).value
except:
    pass


try:
    CTEX_NO_SURFACE_TRIANGLE = UINT32_MAX
except:
    pass


try:
    CTEX_NO_UV_ISLAND = UINT32_MAX
except:
    pass


try:
    CTEX_MAX_MATERIAL_GRAPH_SERIALIZED_SIZE = (c_size_t (ord_if_char(67108864))).value
except:
    pass


try:
    CTEX_MAX_MATERIAL_GRAPH_LIBRARY_SERIALIZED_SIZE = (c_size_t (ord_if_char(268435456))).value
except:
    pass


try:
    CTEX_STROKE_INPUT_SAMPLE_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_stroke_input_sample)))).value
except:
    pass


try:
    CTEX_STROKE_INPUT_SAMPLE_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_stroke_input_sample)))).value
except:
    pass


try:
    CTEX_RESPONSE_MAPPING_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_response_mapping_descriptor)))).value
except:
    pass


try:
    CTEX_RESPONSE_MAPPING_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_response_mapping_descriptor)))).value
except:
    pass


try:
    CTEX_STROKE_STABILIZER_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_stroke_stabilizer_descriptor)))).value
except:
    pass


try:
    CTEX_STROKE_STABILIZER_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_stroke_stabilizer_descriptor)))).value
except:
    pass


try:
    CTEX_STROKE_JITTER_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_stroke_jitter_descriptor)))).value
except:
    pass


try:
    CTEX_STROKE_JITTER_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_stroke_jitter_descriptor)))).value
except:
    pass


try:
    CTEX_STROKE_TAPER_SPAN_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_stroke_taper_span_descriptor)))).value
except:
    pass


try:
    CTEX_STROKE_TAPER_SPAN_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_stroke_taper_span_descriptor)))).value
except:
    pass


try:
    CTEX_STROKE_TAPER_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_stroke_taper_descriptor)))).value
except:
    pass


try:
    CTEX_STROKE_TAPER_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_stroke_taper_descriptor)))).value
except:
    pass


try:
    CTEX_STROKE_CONSTRAINT_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_stroke_constraint_descriptor)))).value
except:
    pass


try:
    CTEX_STROKE_CONSTRAINT_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_stroke_constraint_descriptor)))).value
except:
    pass


try:
    CTEX_STROKE_SYMMETRY_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_stroke_symmetry_descriptor)))).value
except:
    pass


try:
    CTEX_STROKE_SYMMETRY_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_stroke_symmetry_descriptor)))).value
except:
    pass


try:
    CTEX_STROKE_SETTINGS_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_stroke_settings_descriptor)))).value
except:
    pass


try:
    CTEX_STROKE_SETTINGS_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_stroke_settings_descriptor)))).value
except:
    pass


try:
    CTEX_RESOLVED_STROKE_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_resolved_stroke_info)))).value
except:
    pass


try:
    CTEX_RESOLVED_STROKE_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_resolved_stroke_info)))).value
except:
    pass


try:
    CTEX_RESOLVED_STROKE_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_resolved_stroke_descriptor)))).value
except:
    pass


try:
    CTEX_RESOLVED_STROKE_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_resolved_stroke_descriptor)))).value
except:
    pass


try:
    CTEX_MAX_PAINT_TILE_TEXEL_COUNT = (c_size_t (ord_if_char(1048576))).value
except:
    pass


try:
    CTEX_PAINT_TILE_COVERAGE_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_tile_coverage_descriptor)))).value
except:
    pass


try:
    CTEX_PAINT_TILE_COVERAGE_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_tile_coverage_descriptor)))).value
except:
    pass


try:
    CTEX_PAINT_MASK_INPUTS_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_mask_inputs_descriptor)))).value
except:
    pass


try:
    CTEX_PAINT_MASK_INPUTS_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_mask_inputs_descriptor)))).value
except:
    pass


try:
    CTEX_PAINT_MASK_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_mask_info)))).value
except:
    pass


try:
    CTEX_PAINT_MASK_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_mask_info)))).value
except:
    pass


try:
    CTEX_PAINT_MATERIAL_COORDINATE_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_material_coordinate_descriptor)))).value
except:
    pass


try:
    CTEX_PAINT_MATERIAL_COORDINATE_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_material_coordinate_descriptor)))).value
except:
    pass


try:
    CTEX_PAINT_DEPTH_CONTEXT_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_depth_context_descriptor)))).value
except:
    pass


try:
    CTEX_PAINT_DEPTH_CONTEXT_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_depth_context_descriptor)))).value
except:
    pass


try:
    CTEX_PAINT_REJECTION_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_rejection_descriptor)))).value
except:
    pass


try:
    CTEX_PAINT_REJECTION_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_rejection_descriptor)))).value
except:
    pass


try:
    CTEX_PAINT_REJECTION_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_rejection_info)))).value
except:
    pass


try:
    CTEX_PAINT_REJECTION_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_rejection_info)))).value
except:
    pass


try:
    CTEX_PAINT_PREVIEW_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_preview_info)))).value
except:
    pass


try:
    CTEX_PAINT_PREVIEW_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_preview_info)))).value
except:
    pass


try:
    CTEX_PAINT_WORK_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_work_descriptor)))).value
except:
    pass


try:
    CTEX_PAINT_WORK_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_work_descriptor)))).value
except:
    pass


try:
    CTEX_PAINT_WORK_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_work_info)))).value
except:
    pass


try:
    CTEX_PAINT_WORK_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_work_info)))).value
except:
    pass


try:
    CTEX_PAINT_SEAM_DILATION_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_seam_dilation_descriptor)))).value
except:
    pass


try:
    CTEX_PAINT_SEAM_DILATION_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_seam_dilation_descriptor)))).value
except:
    pass


try:
    CTEX_PAINT_SEAM_DILATION_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_seam_dilation_info)))).value
except:
    pass


try:
    CTEX_PAINT_SEAM_DILATION_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_seam_dilation_info)))).value
except:
    pass


try:
    CTEX_PAINT_DILATION_TILE_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_dilation_tile_descriptor)))).value
except:
    pass


try:
    CTEX_PAINT_DILATION_TILE_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_dilation_tile_descriptor)))).value
except:
    pass


try:
    CTEX_PAINT_DILATION_SESSION_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_dilation_session_info)))).value
except:
    pass


try:
    CTEX_PAINT_DILATION_SESSION_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_dilation_session_info)))).value
except:
    pass


try:
    CTEX_PAINT_SURFACE_FILTER_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_surface_filter_descriptor)))).value
except:
    pass


try:
    CTEX_PAINT_SURFACE_FILTER_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_surface_filter_descriptor)))).value
except:
    pass


try:
    CTEX_PAINT_ISLAND_PADDING_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_island_padding_descriptor)))).value
except:
    pass


try:
    CTEX_PAINT_ISLAND_PADDING_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_island_padding_descriptor)))).value
except:
    pass


try:
    CTEX_PAINT_ISLAND_PADDING_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_island_padding_info)))).value
except:
    pass


try:
    CTEX_PAINT_ISLAND_PADDING_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_island_padding_info)))).value
except:
    pass


try:
    CTEX_PAINT_SURFACE_MAP_REQUEST_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_surface_map_request)))).value
except:
    pass


try:
    CTEX_PAINT_SURFACE_MAP_REQUEST_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_surface_map_request)))).value
except:
    pass


try:
    CTEX_PAINT_SURFACE_MAP_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_surface_map_info)))).value
except:
    pass


try:
    CTEX_PAINT_SURFACE_MAP_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_surface_map_info)))).value
except:
    pass


try:
    CTEX_PAINT_SURFACE_MAP_BUFFERS_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_surface_map_buffers)))).value
except:
    pass


try:
    CTEX_PAINT_SURFACE_MAP_BUFFERS_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_surface_map_buffers)))).value
except:
    pass


try:
    CTEX_PAINT_SURFACE_MAP_STATISTICS_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_surface_map_statistics)))).value
except:
    pass


try:
    CTEX_PAINT_SURFACE_MAP_STATISTICS_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_surface_map_statistics)))).value
except:
    pass


try:
    CTEX_PAINT_DEPOSITION_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_deposition_descriptor)))).value
except:
    pass


try:
    CTEX_PAINT_DEPOSITION_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_deposition_info)))).value
except:
    pass


try:
    CTEX_PAINT_DEPOSITION_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_deposition_info)))).value
except:
    pass


try:
    CTEX_PAINT_BLEND_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_blend_descriptor)))).value
except:
    pass


try:
    CTEX_PAINT_BLEND_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_blend_descriptor)))).value
except:
    pass


try:
    CTEX_PAINT_TOOL_CHANNEL_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_tool_channel_descriptor)))).value
except:
    pass


try:
    CTEX_PAINT_TOOL_CHANNEL_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_tool_channel_descriptor)))).value
except:
    pass


try:
    CTEX_PAINT_TOOL_CHANNEL_OUTPUT_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_tool_channel_output)))).value
except:
    pass


try:
    CTEX_PAINT_TOOL_CHANNEL_OUTPUT_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_tool_channel_output)))).value
except:
    pass


try:
    CTEX_PAINT_BRUSH_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_brush_descriptor)))).value
except:
    pass


try:
    CTEX_PAINT_BRUSH_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_brush_descriptor)))).value
except:
    pass


try:
    CTEX_PAINT_BRUSH_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_brush_info)))).value
except:
    pass


try:
    CTEX_PAINT_BRUSH_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_brush_info)))).value
except:
    pass


try:
    CTEX_PAINT_ERASER_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_eraser_descriptor)))).value
except:
    pass


try:
    CTEX_PAINT_ERASER_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_eraser_descriptor)))).value
except:
    pass


try:
    CTEX_PAINT_ERASER_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_eraser_info)))).value
except:
    pass


try:
    CTEX_PAINT_ERASER_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_eraser_info)))).value
except:
    pass


try:
    CTEX_PAINT_FILL_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_fill_descriptor)))).value
except:
    pass


try:
    CTEX_PAINT_FILL_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_fill_descriptor)))).value
except:
    pass


try:
    CTEX_PAINT_FILL_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_fill_info)))).value
except:
    pass


try:
    CTEX_PAINT_FILL_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_fill_info)))).value
except:
    pass


try:
    CTEX_PAINT_FILL_OUTPUTS_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_fill_outputs)))).value
except:
    pass


try:
    CTEX_PAINT_FILL_OUTPUTS_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_fill_outputs)))).value
except:
    pass


try:
    CTEX_PAINT_CLONE_SOURCE_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_clone_source_descriptor)))).value
except:
    pass


try:
    CTEX_PAINT_CLONE_SOURCE_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_clone_source_descriptor)))).value
except:
    pass


try:
    CTEX_PAINT_CLONE_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_clone_descriptor)))).value
except:
    pass


try:
    CTEX_PAINT_CLONE_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_clone_descriptor)))).value
except:
    pass


try:
    CTEX_PAINT_CLONE_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_clone_info)))).value
except:
    pass


try:
    CTEX_PAINT_CLONE_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_clone_info)))).value
except:
    pass


try:
    CTEX_PAINT_CLONE_OUTPUTS_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_clone_outputs)))).value
except:
    pass


try:
    CTEX_PAINT_CLONE_OUTPUTS_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_clone_outputs)))).value
except:
    pass


try:
    CTEX_PAINT_NO_CLONE_SAMPLE = (c_size_t (ord_if_char((-1)))).value
except:
    pass


try:
    CTEX_PAINT_BLUR_NEIGHBORHOOD_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_blur_neighborhood_descriptor)))).value
except:
    pass


try:
    CTEX_PAINT_BLUR_NEIGHBORHOOD_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_blur_neighborhood_descriptor)))).value
except:
    pass


try:
    CTEX_PAINT_BLUR_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_blur_descriptor)))).value
except:
    pass


try:
    CTEX_PAINT_BLUR_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_blur_descriptor)))).value
except:
    pass


try:
    CTEX_PAINT_BLUR_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_blur_info)))).value
except:
    pass


try:
    CTEX_PAINT_BLUR_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_blur_info)))).value
except:
    pass


try:
    CTEX_PAINT_SMEAR_MAPPING_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_smear_mapping_descriptor)))).value
except:
    pass


try:
    CTEX_PAINT_SMEAR_MAPPING_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_smear_mapping_descriptor)))).value
except:
    pass


try:
    CTEX_PAINT_SMEAR_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_smear_descriptor)))).value
except:
    pass


try:
    CTEX_PAINT_SMEAR_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_smear_descriptor)))).value
except:
    pass


try:
    CTEX_PAINT_SMEAR_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_smear_info)))).value
except:
    pass


try:
    CTEX_PAINT_SMEAR_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_smear_info)))).value
except:
    pass


try:
    CTEX_PAINT_STENCIL_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_stencil_descriptor)))).value
except:
    pass


try:
    CTEX_PAINT_STENCIL_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_stencil_descriptor)))).value
except:
    pass


try:
    CTEX_PAINT_STENCIL_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_stencil_info)))).value
except:
    pass


try:
    CTEX_PAINT_STENCIL_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_stencil_info)))).value
except:
    pass


try:
    CTEX_PAINT_DECAL_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_decal_descriptor)))).value
except:
    pass


try:
    CTEX_PAINT_DECAL_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_decal_descriptor)))).value
except:
    pass


try:
    CTEX_PAINT_DECAL_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_decal_info)))).value
except:
    pass


try:
    CTEX_PAINT_DECAL_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_decal_info)))).value
except:
    pass


try:
    CTEX_PAINT_DECAL_OUTPUTS_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_decal_outputs)))).value
except:
    pass


try:
    CTEX_PAINT_DECAL_OUTPUTS_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_decal_outputs)))).value
except:
    pass


try:
    CTEX_PAINT_NO_DECAL_SAMPLE = (c_size_t (ord_if_char((-1)))).value
except:
    pass


try:
    CTEX_PAINT_PROJECTION_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_projection_descriptor)))).value
except:
    pass


try:
    CTEX_PAINT_PROJECTION_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_projection_descriptor)))).value
except:
    pass


try:
    CTEX_PAINT_PROJECTION_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_projection_info)))).value
except:
    pass


try:
    CTEX_PAINT_PROJECTION_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_projection_info)))).value
except:
    pass


try:
    CTEX_PAINT_PROJECTION_OUTPUTS_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_projection_outputs)))).value
except:
    pass


try:
    CTEX_PAINT_PROJECTION_OUTPUTS_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_projection_outputs)))).value
except:
    pass


try:
    CTEX_PAINT_NO_PROJECTION_SAMPLE = (c_size_t (ord_if_char((-1)))).value
except:
    pass


try:
    CTEX_PAINT_FONT_GLYPH_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_font_glyph_descriptor)))).value
except:
    pass


try:
    CTEX_PAINT_FONT_GLYPH_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_font_glyph_descriptor)))).value
except:
    pass


try:
    CTEX_PAINT_FONT_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_font_descriptor)))).value
except:
    pass


try:
    CTEX_PAINT_FONT_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_font_descriptor)))).value
except:
    pass


try:
    CTEX_PAINT_TEXT_MATERIAL_VALUE_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_text_material_value)))).value
except:
    pass


try:
    CTEX_PAINT_TEXT_MATERIAL_VALUE_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_text_material_value)))).value
except:
    pass


try:
    CTEX_PAINT_TEXT_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_text_descriptor)))).value
except:
    pass


try:
    CTEX_PAINT_TEXT_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_text_descriptor)))).value
except:
    pass


try:
    CTEX_PAINT_TEXT_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_text_info)))).value
except:
    pass


try:
    CTEX_PAINT_TEXT_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_text_info)))).value
except:
    pass


try:
    CTEX_PAINT_TEXT_OUTPUTS_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_text_outputs)))).value
except:
    pass


try:
    CTEX_PAINT_TEXT_OUTPUTS_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_text_outputs)))).value
except:
    pass


try:
    CTEX_PAINT_PARTICLE_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_particle_descriptor)))).value
except:
    pass


try:
    CTEX_PAINT_PARTICLE_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_particle_descriptor)))).value
except:
    pass


try:
    CTEX_PAINT_PARTICLE_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_particle_info)))).value
except:
    pass


try:
    CTEX_PAINT_PARTICLE_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_particle_info)))).value
except:
    pass


try:
    CTEX_PAINT_PARTICLE_OUTPUTS_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_particle_outputs)))).value
except:
    pass


try:
    CTEX_PAINT_PARTICLE_OUTPUTS_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_particle_outputs)))).value
except:
    pass


try:
    CTEX_PAINT_NO_PARTICLE_TEXEL = (c_size_t (ord_if_char((-1)))).value
except:
    pass


try:
    CTEX_STROKE_PRESET_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_stroke_preset_info)))).value
except:
    pass


try:
    CTEX_STROKE_PRESET_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_stroke_preset_info)))).value
except:
    pass


try:
    CTEX_STROKE_PRESET_BUFFERS_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_stroke_preset_buffers_descriptor)))).value
except:
    pass


try:
    CTEX_STROKE_PRESET_BUFFERS_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_stroke_preset_buffers_descriptor)))).value
except:
    pass


try:
    CTEX_UV_SET_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_uv_set_descriptor)))).value
except:
    pass


try:
    CTEX_UV_SET_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_uv_set_descriptor)))).value
except:
    pass


try:
    CTEX_MESH_PARTITION_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_mesh_partition_descriptor)))).value
except:
    pass


try:
    CTEX_MESH_PARTITION_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_mesh_partition_descriptor)))).value
except:
    pass


try:
    CTEX_MESH_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_mesh_descriptor)))).value
except:
    pass


try:
    CTEX_MESH_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_mesh_descriptor)))).value
except:
    pass


try:
    CTEX_MESH_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_mesh_info)))).value
except:
    pass


try:
    CTEX_MESH_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_mesh_info)))).value
except:
    pass


try:
    CTEX_MESH_UV_OVERLAP_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_mesh_uv_overlap_info)))).value
except:
    pass


try:
    CTEX_MESH_UV_OVERLAP_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_mesh_uv_overlap_info)))).value
except:
    pass


try:
    CTEX_MESH_UV_COVERAGE_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_mesh_uv_coverage_info)))).value
except:
    pass


try:
    CTEX_MESH_UV_COVERAGE_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_mesh_uv_coverage_info)))).value
except:
    pass


try:
    CTEX_MESH_REPLACEMENT_NO_PARTITION = UINT32_MAX
except:
    pass


try:
    CTEX_MESH_REPLACEMENT_PLAN_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_mesh_replacement_plan_info)))).value
except:
    pass


try:
    CTEX_MESH_REPLACEMENT_PLAN_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_mesh_replacement_plan_info)))).value
except:
    pass


try:
    CTEX_MESH_REPLACEMENT_DECISION_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_mesh_replacement_decision)))).value
except:
    pass


try:
    CTEX_MESH_REPLACEMENT_DECISION_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_mesh_replacement_decision)))).value
except:
    pass


try:
    CTEX_MESH_REPLACEMENT_APPLY_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_mesh_replacement_apply_info)))).value
except:
    pass


try:
    CTEX_MESH_REPLACEMENT_APPLY_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_mesh_replacement_apply_info)))).value
except:
    pass


try:
    CTEX_MESH_REPROJECTION_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_mesh_reprojection_descriptor)))).value
except:
    pass


try:
    CTEX_MESH_REPROJECTION_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_mesh_reprojection_descriptor)))).value
except:
    pass


try:
    CTEX_MESH_REPROJECTION_PREFLIGHT_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_mesh_reprojection_preflight_info)))).value
except:
    pass


try:
    CTEX_MESH_REPROJECTION_PREFLIGHT_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_mesh_reprojection_preflight_info)))).value
except:
    pass


try:
    CTEX_MESH_REPROJECTION_COMMIT_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_mesh_reprojection_commit_info)))).value
except:
    pass


try:
    CTEX_MESH_REPROJECTION_COMMIT_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_mesh_reprojection_commit_info)))).value
except:
    pass


try:
    CTEX_PICK_TEXTURE_SET_BINDING_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_pick_texture_set_binding_descriptor)))).value
except:
    pass


try:
    CTEX_PICK_TEXTURE_SET_BINDING_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_pick_texture_set_binding_descriptor)))).value
except:
    pass


try:
    CTEX_PICK_OPTIONS_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_pick_options_descriptor)))).value
except:
    pass


try:
    CTEX_PICK_OPTIONS_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_pick_options_descriptor)))).value
except:
    pass


try:
    CTEX_PICK_SCREEN_VIEW_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_pick_screen_view_descriptor)))).value
except:
    pass


try:
    CTEX_PICK_SCREEN_VIEW_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_pick_screen_view_descriptor)))).value
except:
    pass


try:
    CTEX_PICK_INDEX_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_pick_index_info)))).value
except:
    pass


try:
    CTEX_PICK_INDEX_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_pick_index_info)))).value
except:
    pass


try:
    CTEX_PICK_QUERY_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_pick_query_info)))).value
except:
    pass


try:
    CTEX_PICK_QUERY_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_pick_query_info)))).value
except:
    pass


try:
    CTEX_PAINT_PICKER_TEXTURE_VIEW_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_picker_texture_view_descriptor)))).value
except:
    pass


try:
    CTEX_PAINT_PICKER_TEXTURE_VIEW_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_picker_texture_view_descriptor)))).value
except:
    pass


try:
    CTEX_PAINT_PICKER_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_picker_descriptor)))).value
except:
    pass


try:
    CTEX_PAINT_PICKER_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_picker_descriptor)))).value
except:
    pass


try:
    CTEX_PAINT_PICKER_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_picker_info)))).value
except:
    pass


try:
    CTEX_PAINT_PICKER_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_picker_info)))).value
except:
    pass


try:
    CTEX_PAINT_COLOUR_ID_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_colour_id_descriptor)))).value
except:
    pass


try:
    CTEX_PAINT_COLOUR_ID_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_colour_id_descriptor)))).value
except:
    pass


try:
    CTEX_PAINT_COLOUR_ID_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_colour_id_info)))).value
except:
    pass


try:
    CTEX_PAINT_COLOUR_ID_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_colour_id_info)))).value
except:
    pass


try:
    CTEX_PAINT_PARAMETER_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_parameter_descriptor)))).value
except:
    pass


try:
    CTEX_PAINT_PARAMETER_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_parameter_descriptor)))).value
except:
    pass


try:
    CTEX_PAINT_PARAMETER_CATALOGUE_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_parameter_catalogue_info)))).value
except:
    pass


try:
    CTEX_PAINT_PARAMETER_CATALOGUE_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_parameter_catalogue_info)))).value
except:
    pass


try:
    CTEX_PAINT_PARAMETER_VALIDATION_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_parameter_validation_info)))).value
except:
    pass


try:
    CTEX_PAINT_PARAMETER_VALIDATION_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_parameter_validation_info)))).value
except:
    pass


try:
    CTEX_PAINT_SELECTION_SURFACE_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_selection_surface_descriptor)))).value
except:
    pass


try:
    CTEX_PAINT_SELECTION_SURFACE_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_selection_surface_descriptor)))).value
except:
    pass


try:
    CTEX_PAINT_SCREEN_SELECTION_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_screen_selection_descriptor)))).value
except:
    pass


try:
    CTEX_PAINT_SCREEN_SELECTION_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_screen_selection_descriptor)))).value
except:
    pass


try:
    CTEX_PAINT_POLYGON_SELECTION_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_polygon_selection_descriptor)))).value
except:
    pass


try:
    CTEX_PAINT_POLYGON_SELECTION_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_polygon_selection_descriptor)))).value
except:
    pass


try:
    CTEX_PAINT_SELECTION_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_selection_info)))).value
except:
    pass


try:
    CTEX_PAINT_SELECTION_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_selection_info)))).value
except:
    pass


try:
    CTEX_PAINT_SELECTION_OUTPUTS_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_selection_outputs)))).value
except:
    pass


try:
    CTEX_PAINT_SELECTION_OUTPUTS_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_paint_selection_outputs)))).value
except:
    pass


try:
    CTEX_PICK_BATCH_CONTROL_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_pick_batch_control_descriptor)))).value
except:
    pass


try:
    CTEX_PICK_BATCH_CONTROL_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_pick_batch_control_descriptor)))).value
except:
    pass


try:
    CTEX_PICK_BATCH_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_pick_batch_info)))).value
except:
    pass


try:
    CTEX_PICK_BATCH_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_pick_batch_info)))).value
except:
    pass


try:
    CTEX_TEXTURE_SET_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_texture_set_descriptor)))).value
except:
    pass


try:
    CTEX_UDIM_PIXEL_WRITE_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_udim_pixel_write_descriptor)))).value
except:
    pass


try:
    CTEX_UDIM_PIXEL_WRITE_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_udim_pixel_write_descriptor)))).value
except:
    pass


try:
    CTEX_UDIM_WRITE_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_udim_write_info)))).value
except:
    pass


try:
    CTEX_UDIM_WRITE_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_udim_write_info)))).value
except:
    pass


try:
    CTEX_ATLAS_REGION_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_atlas_region_descriptor)))).value
except:
    pass


try:
    CTEX_ATLAS_REGION_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_atlas_region_descriptor)))).value
except:
    pass


try:
    CTEX_ATLAS_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_atlas_descriptor)))).value
except:
    pass


try:
    CTEX_ATLAS_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_atlas_descriptor)))).value
except:
    pass


try:
    CTEX_ATLAS_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_atlas_info)))).value
except:
    pass


try:
    CTEX_ATLAS_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_atlas_info)))).value
except:
    pass


try:
    CTEX_LAYER_CHANNEL_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_layer_channel_descriptor)))).value
except:
    pass


try:
    CTEX_LAYER_CHANNEL_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_layer_channel_descriptor)))).value
except:
    pass


try:
    CTEX_LAYER_ENTRY_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_layer_entry_descriptor)))).value
except:
    pass


try:
    CTEX_LAYER_ENTRY_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_layer_entry_descriptor)))).value
except:
    pass


try:
    CTEX_LAYER_MASK_SAMPLE_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_layer_mask_sample)))).value
except:
    pass


try:
    CTEX_LAYER_MASK_SAMPLE_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_layer_mask_sample)))).value
except:
    pass


try:
    CTEX_LAYER_PARTICIPATION_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_layer_participation_info)))).value
except:
    pass


try:
    CTEX_LAYER_PARTICIPATION_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_layer_participation_info)))).value
except:
    pass


try:
    CTEX_TILE_HISTORY_TARGET_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_tile_history_target_descriptor)))).value
except:
    pass


try:
    CTEX_TILE_HISTORY_TARGET_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_tile_history_target_descriptor)))).value
except:
    pass


try:
    CTEX_TILE_HISTORY_BUDGET_REPORT_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_tile_history_budget_report)))).value
except:
    pass


try:
    CTEX_TILE_HISTORY_BUDGET_REPORT_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_tile_history_budget_report)))).value
except:
    pass


try:
    CTEX_TILE_HISTORY_COMMIT_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_tile_history_commit_info)))).value
except:
    pass


try:
    CTEX_TILE_HISTORY_COMMIT_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_tile_history_commit_info)))).value
except:
    pass


try:
    CTEX_TILE_HISTORY_RESTORE_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_tile_history_restore_info)))).value
except:
    pass


try:
    CTEX_TILE_HISTORY_RESTORE_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_tile_history_restore_info)))).value
except:
    pass


try:
    CTEX_LAYER_COMPOSITE_RASTER_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_layer_composite_raster_descriptor)))).value
except:
    pass


try:
    CTEX_LAYER_COMPOSITE_RASTER_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_layer_composite_raster_descriptor)))).value
except:
    pass


try:
    CTEX_LAYER_COMPOSITE_MASK_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_layer_composite_mask_descriptor)))).value
except:
    pass


try:
    CTEX_LAYER_COMPOSITE_MASK_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_layer_composite_mask_descriptor)))).value
except:
    pass


try:
    CTEX_LAYER_COMPOSITE_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_layer_composite_info)))).value
except:
    pass


try:
    CTEX_LAYER_COMPOSITE_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_layer_composite_info)))).value
except:
    pass


try:
    CTEX_LAYER_SNAPSHOT_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_layer_snapshot_info)))).value
except:
    pass


try:
    CTEX_LAYER_SNAPSHOT_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_layer_snapshot_info)))).value
except:
    pass


try:
    CTEX_LAYER_OPERATION_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_layer_operation_descriptor)))).value
except:
    pass


try:
    CTEX_LAYER_OPERATION_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_layer_operation_descriptor)))).value
except:
    pass


try:
    CTEX_LAYER_OPERATION_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_layer_operation_info)))).value
except:
    pass


try:
    CTEX_LAYER_OPERATION_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_layer_operation_info)))).value
except:
    pass


try:
    CTEX_IMAGE_DECODE_LIMITS_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_image_decode_limits_descriptor)))).value
except:
    pass


try:
    CTEX_IMAGE_DECODE_LIMITS_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_image_decode_limits_descriptor)))).value
except:
    pass


try:
    CTEX_IMAGE_DECODE_PROGRESS_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_image_decode_progress_info)))).value
except:
    pass


try:
    CTEX_IMAGE_DECODE_PROGRESS_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_image_decode_progress_info)))).value
except:
    pass


try:
    CTEX_IMAGE_DECODE_CONTROL_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_image_decode_control_descriptor)))).value
except:
    pass


try:
    CTEX_IMAGE_DECODE_CONTROL_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_image_decode_control_descriptor)))).value
except:
    pass


try:
    CTEX_IMAGE_DECODE_EXECUTION_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_image_decode_execution_info)))).value
except:
    pass


try:
    CTEX_IMAGE_DECODE_EXECUTION_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_image_decode_execution_info)))).value
except:
    pass


try:
    CTEX_DECODED_IMAGE_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_decoded_image_info)))).value
except:
    pass


try:
    CTEX_DECODED_IMAGE_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_decoded_image_info)))).value
except:
    pass


try:
    CTEX_LAYERED_IMAGE_DECODE_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_layered_image_decode_descriptor)))).value
except:
    pass


try:
    CTEX_LAYERED_IMAGE_DECODE_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_layered_image_decode_descriptor)))).value
except:
    pass


try:
    CTEX_LAYERED_IMAGE_DECODE_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_layered_image_decode_info)))).value
except:
    pass


try:
    CTEX_LAYERED_IMAGE_DECODE_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_layered_image_decode_info)))).value
except:
    pass


try:
    CTEX_LAYERED_DECODED_IMAGE_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_layered_decoded_image_info)))).value
except:
    pass


try:
    CTEX_LAYERED_DECODED_IMAGE_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_layered_decoded_image_info)))).value
except:
    pass


try:
    CTEX_IMAGE_CHANNEL_EXPANSION_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_image_channel_expansion_descriptor)))).value
except:
    pass


try:
    CTEX_IMAGE_CHANNEL_EXPANSION_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_image_channel_expansion_descriptor)))).value
except:
    pass


try:
    CTEX_IMAGE_CHANNEL_EXPANSION_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_image_channel_expansion_info)))).value
except:
    pass


try:
    CTEX_IMAGE_CHANNEL_EXPANSION_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_image_channel_expansion_info)))).value
except:
    pass


try:
    CTEX_IMAGE_RESAMPLE_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_image_resample_descriptor)))).value
except:
    pass


try:
    CTEX_IMAGE_RESAMPLE_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_image_resample_descriptor)))).value
except:
    pass


try:
    CTEX_IMAGE_RESAMPLE_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_image_resample_info)))).value
except:
    pass


try:
    CTEX_IMAGE_RESAMPLE_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_image_resample_info)))).value
except:
    pass


try:
    CTEX_IMAGE_ENCODE_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_image_encode_descriptor)))).value
except:
    pass


try:
    CTEX_IMAGE_ENCODE_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_image_encode_descriptor)))).value
except:
    pass


try:
    CTEX_CHANNEL_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_channel_descriptor)))).value
except:
    pass


try:
    CTEX_CHANNEL_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_channel_descriptor)))).value
except:
    pass


try:
    CTEX_CHANNEL_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_channel_info)))).value
except:
    pass


try:
    CTEX_CHANNEL_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_channel_info)))).value
except:
    pass


try:
    CTEX_TEXTURE_SET_MEMORY_REPORT_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_texture_set_memory_report)))).value
except:
    pass


try:
    CTEX_TEXTURE_SET_MEMORY_REPORT_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_texture_set_memory_report)))).value
except:
    pass


try:
    CTEX_DOCUMENT_MEMORY_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_document_memory_info)))).value
except:
    pass


try:
    CTEX_DOCUMENT_MEMORY_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_document_memory_info)))).value
except:
    pass


try:
    CTEX_TRANSPORT_DELTA_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_transport_delta_info)))).value
except:
    pass


try:
    CTEX_TRANSPORT_DELTA_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_transport_delta_info)))).value
except:
    pass


try:
    CTEX_TRANSPORT_FORMAT_SELECTION_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_transport_format_selection)))).value
except:
    pass


try:
    CTEX_TRANSPORT_FORMAT_SELECTION_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_transport_format_selection)))).value
except:
    pass


try:
    CTEX_TRANSPORT_SNAPSHOT_QUERY_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_transport_snapshot_query_info)))).value
except:
    pass


try:
    CTEX_TRANSPORT_SNAPSHOT_QUERY_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_transport_snapshot_query_info)))).value
except:
    pass


try:
    CTEX_TRANSPORT_SNAPSHOT_MEMORY_REPORT_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_transport_snapshot_memory_report)))).value
except:
    pass


try:
    CTEX_TRANSPORT_SNAPSHOT_MEMORY_REPORT_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_transport_snapshot_memory_report)))).value
except:
    pass


try:
    CTEX_RESOURCE_ALLOCATION_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_resource_allocation_descriptor)))).value
except:
    pass


try:
    CTEX_RESOURCE_ALLOCATION_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_resource_allocation_descriptor)))).value
except:
    pass


try:
    CTEX_RESOURCE_CATEGORY_REPORT_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_resource_category_report)))).value
except:
    pass


try:
    CTEX_RESOURCE_CATEGORY_REPORT_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_resource_category_report)))).value
except:
    pass


try:
    CTEX_RESOURCE_ACCOUNTING_REPORT_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_resource_accounting_report)))).value
except:
    pass


try:
    CTEX_RESOURCE_ACCOUNTING_REPORT_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_resource_accounting_report)))).value
except:
    pass


try:
    CTEX_RESOURCE_BUDGET_LIMITS_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_resource_budget_limits)))).value
except:
    pass


try:
    CTEX_RESOURCE_BUDGET_LIMITS_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_resource_budget_limits)))).value
except:
    pass


try:
    CTEX_RESOURCE_REQUIREMENT_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_resource_requirement)))).value
except:
    pass


try:
    CTEX_RESOURCE_REQUIREMENT_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_resource_requirement)))).value
except:
    pass


try:
    CTEX_RESOURCE_ADMISSION_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_resource_admission_descriptor)))).value
except:
    pass


try:
    CTEX_RESOURCE_ADMISSION_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_resource_admission_descriptor)))).value
except:
    pass


try:
    CTEX_RESOURCE_ADMISSION_REPORT_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_resource_admission_report)))).value
except:
    pass


try:
    CTEX_RESOURCE_ADMISSION_REPORT_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_resource_admission_report)))).value
except:
    pass


try:
    CTEX_PREVIEW_QUALITY_OPTION_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_preview_quality_option)))).value
except:
    pass


try:
    CTEX_PREVIEW_QUALITY_OPTION_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_preview_quality_option)))).value
except:
    pass


try:
    CTEX_PREVIEW_QUALITY_ADMISSION_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_preview_quality_admission_descriptor)))).value
except:
    pass


try:
    CTEX_PREVIEW_QUALITY_ADMISSION_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_preview_quality_admission_descriptor)))).value
except:
    pass


try:
    CTEX_PREVIEW_QUALITY_ADMISSION_REPORT_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_preview_quality_admission_report)))).value
except:
    pass


try:
    CTEX_PREVIEW_QUALITY_ADMISSION_REPORT_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_preview_quality_admission_report)))).value
except:
    pass


try:
    CTEX_TILE_BACKING_STORE_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_tile_backing_store_descriptor)))).value
except:
    pass


try:
    CTEX_TILE_BACKING_STORE_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_tile_backing_store_descriptor)))).value
except:
    pass


try:
    CTEX_TILE_EVICTION_REPORT_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_tile_eviction_report)))).value
except:
    pass


try:
    CTEX_TILE_EVICTION_REPORT_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_tile_eviction_report)))).value
except:
    pass


try:
    CTEX_TRANSPORT_TILE_MEMORY_LAYOUT_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_transport_tile_memory_layout)))).value
except:
    pass


try:
    CTEX_TRANSPORT_TILE_MEMORY_LAYOUT_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_transport_tile_memory_layout)))).value
except:
    pass


try:
    CTEX_TRANSPORT_TILE_READBACK_DESTINATION_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_transport_tile_readback_destination)))).value
except:
    pass


try:
    CTEX_TRANSPORT_TILE_READBACK_DESTINATION_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_transport_tile_readback_destination)))).value
except:
    pass


try:
    CTEX_TRANSPORT_HOST_TILE_COMPLETION_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_transport_host_tile_completion)))).value
except:
    pass


try:
    CTEX_TRANSPORT_HOST_TILE_COMPLETION_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_transport_host_tile_completion)))).value
except:
    pass


try:
    CTEX_TRANSPORT_READBACK_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_transport_readback_info)))).value
except:
    pass


try:
    CTEX_TRANSPORT_READBACK_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_transport_readback_info)))).value
except:
    pass


try:
    CTEX_TANGENT_FRAME_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_tangent_frame_descriptor)))).value
except:
    pass


try:
    CTEX_TANGENT_FRAME_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_tangent_frame_descriptor)))).value
except:
    pass


try:
    CTEX_MESH_TANGENT_DATA_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_mesh_tangent_data_descriptor)))).value
except:
    pass


try:
    CTEX_MESH_TANGENT_DATA_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_mesh_tangent_data_descriptor)))).value
except:
    pass


try:
    CTEX_MESH_TANGENT_FRAME_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_mesh_tangent_frame_info)))).value
except:
    pass


try:
    CTEX_MESH_TANGENT_FRAME_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_mesh_tangent_frame_info)))).value
except:
    pass


try:
    CTEX_MESH_MAP_PIXEL_BUFFER_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_mesh_map_pixel_buffer_descriptor)))).value
except:
    pass


try:
    CTEX_MESH_MAP_PIXEL_BUFFER_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_mesh_map_pixel_buffer_descriptor)))).value
except:
    pass


try:
    CTEX_MESH_MAP_IMPORT_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_mesh_map_import_descriptor)))).value
except:
    pass


try:
    CTEX_MESH_MAP_IMPORT_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_mesh_map_import_descriptor)))).value
except:
    pass


try:
    CTEX_MESH_MAP_IMPORT_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_mesh_map_import_info)))).value
except:
    pass


try:
    CTEX_MESH_MAP_IMPORT_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_mesh_map_import_info)))).value
except:
    pass


try:
    CTEX_MESH_MAP_SET_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_mesh_map_set_info)))).value
except:
    pass


try:
    CTEX_MESH_MAP_SET_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_mesh_map_set_info)))).value
except:
    pass


try:
    CTEX_MESH_MAP_SAMPLE_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_mesh_map_sample_info)))).value
except:
    pass


try:
    CTEX_MESH_MAP_SAMPLE_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_mesh_map_sample_info)))).value
except:
    pass


try:
    CTEX_MESH_MAP_REQUIREMENT_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_mesh_map_requirement_info)))).value
except:
    pass


try:
    CTEX_MESH_MAP_REQUIREMENT_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_mesh_map_requirement_info)))).value
except:
    pass


try:
    CTEX_MESH_MAP_RELEASE_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_mesh_map_release_info)))).value
except:
    pass


try:
    CTEX_MESH_MAP_RELEASE_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_mesh_map_release_info)))).value
except:
    pass


try:
    CTEX_MESH_MAP_GENERATOR_PARAMETER_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_mesh_map_generator_parameter_descriptor)))).value
except:
    pass


try:
    CTEX_MESH_MAP_GENERATOR_PARAMETER_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_mesh_map_generator_parameter_descriptor)))).value
except:
    pass


try:
    CTEX_MESH_MAP_GENERATOR_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_mesh_map_generator_info)))).value
except:
    pass


try:
    CTEX_MESH_MAP_GENERATOR_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_mesh_map_generator_info)))).value
except:
    pass


try:
    CTEX_MESH_MAP_GENERATOR_PARAMETER_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_mesh_map_generator_parameter)))).value
except:
    pass


try:
    CTEX_MESH_MAP_GENERATOR_PARAMETER_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_mesh_map_generator_parameter)))).value
except:
    pass


try:
    CTEX_MESH_MAP_GENERATOR_RESULT_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_mesh_map_generator_result_info)))).value
except:
    pass


try:
    CTEX_MESH_MAP_GENERATOR_RESULT_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_mesh_map_generator_result_info)))).value
except:
    pass


try:
    CTEX_MESH_MAP_BAKE_REQUEST_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_mesh_map_bake_request_descriptor)))).value
except:
    pass


try:
    CTEX_MESH_MAP_BAKE_REQUEST_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_mesh_map_bake_request_descriptor)))).value
except:
    pass


try:
    CTEX_MESH_MAP_BAKE_CONTROL_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_mesh_map_bake_control)))).value
except:
    pass


try:
    CTEX_MESH_MAP_BAKE_CONTROL_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_mesh_map_bake_control)))).value
except:
    pass


try:
    CTEX_MESH_MAP_BAKE_OUTPUT_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_mesh_map_bake_output_descriptor)))).value
except:
    pass


try:
    CTEX_MESH_MAP_BAKE_OUTPUT_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_mesh_map_bake_output_descriptor)))).value
except:
    pass


try:
    CTEX_MESH_MAP_BAKE_PROVIDER_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_mesh_map_bake_provider_descriptor)))).value
except:
    pass


try:
    CTEX_MESH_MAP_BAKE_PROVIDER_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_mesh_map_bake_provider_descriptor)))).value
except:
    pass


try:
    CTEX_MESH_MAP_BAKE_CONTROL_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_mesh_map_bake_control_descriptor)))).value
except:
    pass


try:
    CTEX_MESH_MAP_BAKE_CONTROL_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_mesh_map_bake_control_descriptor)))).value
except:
    pass


try:
    CTEX_MESH_MAP_BAKE_RESULT_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_mesh_map_bake_result_info)))).value
except:
    pass


try:
    CTEX_MESH_MAP_BAKE_RESULT_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_mesh_map_bake_result_info)))).value
except:
    pass


try:
    CTEX_MESH_MAP_BAKE_SESSION_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_mesh_map_bake_session_info)))).value
except:
    pass


try:
    CTEX_MESH_MAP_BAKE_SESSION_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_mesh_map_bake_session_info)))).value
except:
    pass


try:
    CTEX_MESH_MAP_BAKE_TOKEN_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_mesh_map_bake_token_info)))).value
except:
    pass


try:
    CTEX_MESH_MAP_BAKE_TOKEN_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_mesh_map_bake_token_info)))).value
except:
    pass


try:
    CTEX_MESH_MAP_BAKE_COMPLETION_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_mesh_map_bake_completion_info)))).value
except:
    pass


try:
    CTEX_MESH_MAP_BAKE_COMPLETION_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_mesh_map_bake_completion_info)))).value
except:
    pass


try:
    CTEX_MESH_MAP_BAKE_SETTINGS_EDIT_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_mesh_map_bake_settings_edit_info)))).value
except:
    pass


try:
    CTEX_MESH_MAP_BAKE_SETTINGS_EDIT_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_mesh_map_bake_settings_edit_info)))).value
except:
    pass


try:
    CTEX_MESH_MAP_BAKE_SETTINGS_UNDO_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_mesh_map_bake_settings_undo_info)))).value
except:
    pass


try:
    CTEX_MESH_MAP_BAKE_SETTINGS_UNDO_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_mesh_map_bake_settings_undo_info)))).value
except:
    pass


try:
    CTEX_HOST_EXECUTOR_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_host_executor_descriptor)))).value
except:
    pass


try:
    CTEX_HOST_EXECUTOR_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_host_executor_descriptor)))).value
except:
    pass


try:
    CTEX_EXECUTOR_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_executor_info)))).value
except:
    pass


try:
    CTEX_EXECUTOR_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_executor_info)))).value
except:
    pass


try:
    CTEX_EXECUTOR_SELECTION_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_executor_selection_info)))).value
except:
    pass


try:
    CTEX_EXECUTOR_SELECTION_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_executor_selection_info)))).value
except:
    pass


try:
    CTEX_EXECUTOR_FALLBACK_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_executor_fallback_descriptor)))).value
except:
    pass


try:
    CTEX_EXECUTOR_FALLBACK_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_executor_fallback_descriptor)))).value
except:
    pass


try:
    CTEX_EXECUTOR_FALLBACK_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_executor_fallback_info)))).value
except:
    pass


try:
    CTEX_EXECUTOR_FALLBACK_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_executor_fallback_info)))).value
except:
    pass


try:
    CTEX_CPU_BOUNDED_EXECUTION_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_cpu_bounded_execution_descriptor)))).value
except:
    pass


try:
    CTEX_CPU_BOUNDED_EXECUTION_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_cpu_bounded_execution_descriptor)))).value
except:
    pass


try:
    CTEX_CPU_EXECUTION_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_cpu_execution_info)))).value
except:
    pass


try:
    CTEX_CPU_EXECUTION_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_cpu_execution_info)))).value
except:
    pass


try:
    CTEX_CPU_RASTER_MESH_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_cpu_raster_mesh_descriptor)))).value
except:
    pass


try:
    CTEX_CPU_RASTER_MESH_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_cpu_raster_mesh_descriptor)))).value
except:
    pass


try:
    CTEX_CPU_RASTER_CAMERA_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_cpu_raster_camera_descriptor)))).value
except:
    pass


try:
    CTEX_CPU_RASTER_CAMERA_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_cpu_raster_camera_descriptor)))).value
except:
    pass


try:
    CTEX_CPU_VIEWPORT_RASTER_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_cpu_viewport_raster_descriptor)))).value
except:
    pass


try:
    CTEX_CPU_VIEWPORT_RASTER_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_cpu_viewport_raster_descriptor)))).value
except:
    pass


try:
    CTEX_CPU_UV_RASTER_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_cpu_uv_raster_descriptor)))).value
except:
    pass


try:
    CTEX_CPU_UV_RASTER_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_cpu_uv_raster_descriptor)))).value
except:
    pass


try:
    CTEX_CPU_RASTER_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_cpu_raster_info)))).value
except:
    pass


try:
    CTEX_CPU_RASTER_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_cpu_raster_info)))).value
except:
    pass


try:
    CTEX_CPU_RASTER_OUTPUTS_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_cpu_raster_outputs)))).value
except:
    pass


try:
    CTEX_CPU_RASTER_OUTPUTS_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_cpu_raster_outputs)))).value
except:
    pass


try:
    CTEX_PARITY_TOLERANCE_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_parity_tolerance_info)))).value
except:
    pass


try:
    CTEX_PARITY_TOLERANCE_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_parity_tolerance_info)))).value
except:
    pass


try:
    CTEX_PARITY_COMPARISON_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_parity_comparison_info)))).value
except:
    pass


try:
    CTEX_PARITY_COMPARISON_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_parity_comparison_info)))).value
except:
    pass


try:
    CTEX_PARITY_FIXTURE_CHANNEL_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_parity_fixture_channel_descriptor)))).value
except:
    pass


try:
    CTEX_PARITY_FIXTURE_CHANNEL_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_parity_fixture_channel_descriptor)))).value
except:
    pass


try:
    CTEX_PARITY_FIXTURE_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_parity_fixture_descriptor)))).value
except:
    pass


try:
    CTEX_PARITY_FIXTURE_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_parity_fixture_descriptor)))).value
except:
    pass


try:
    CTEX_PARITY_RENDERED_CHANNEL_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_parity_rendered_channel_descriptor)))).value
except:
    pass


try:
    CTEX_PARITY_RENDERED_CHANNEL_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_parity_rendered_channel_descriptor)))).value
except:
    pass


try:
    CTEX_PARITY_RENDERED_FIXTURE_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_parity_rendered_fixture_descriptor)))).value
except:
    pass


try:
    CTEX_PARITY_RENDERED_FIXTURE_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_parity_rendered_fixture_descriptor)))).value
except:
    pass


try:
    CTEX_PARITY_EXECUTOR_BINDING_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_parity_executor_binding_descriptor)))).value
except:
    pass


try:
    CTEX_PARITY_EXECUTOR_BINDING_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_parity_executor_binding_descriptor)))).value
except:
    pass


try:
    CTEX_PARITY_GATE_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_parity_gate_info)))).value
except:
    pass


try:
    CTEX_PARITY_GATE_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_parity_gate_info)))).value
except:
    pass


try:
    CTEX_HOST_RESOURCE_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_host_resource_descriptor)))).value
except:
    pass


try:
    CTEX_HOST_RESOURCE_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_host_resource_descriptor)))).value
except:
    pass


try:
    CTEX_HOST_SUBMISSION_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_host_submission_descriptor)))).value
except:
    pass


try:
    CTEX_HOST_SUBMISSION_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_host_submission_descriptor)))).value
except:
    pass


try:
    CTEX_HOST_SUBMISSION_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_host_submission_info)))).value
except:
    pass


try:
    CTEX_HOST_SUBMISSION_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_host_submission_info)))).value
except:
    pass


try:
    CTEX_HOST_EXECUTION_SESSION_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_host_execution_session_info)))).value
except:
    pass


try:
    CTEX_HOST_EXECUTION_SESSION_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_host_execution_session_info)))).value
except:
    pass


try:
    CTEX_HOST_COMPLETED_RESOURCE_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_host_completed_resource_descriptor)))).value
except:
    pass


try:
    CTEX_HOST_COMPLETED_RESOURCE_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_host_completed_resource_descriptor)))).value
except:
    pass


try:
    CTEX_HOST_RECOVERY_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_host_recovery_descriptor)))).value
except:
    pass


try:
    CTEX_HOST_RECOVERY_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_host_recovery_descriptor)))).value
except:
    pass


try:
    CTEX_HOST_COMPLETION_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_host_completion_descriptor)))).value
except:
    pass


try:
    CTEX_HOST_COMPLETION_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_host_completion_descriptor)))).value
except:
    pass


try:
    CTEX_HOST_COMPLETION_RESULT_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_host_completion_result_info)))).value
except:
    pass


try:
    CTEX_HOST_COMPLETION_RESULT_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_host_completion_result_info)))).value
except:
    pass


try:
    CTEX_HOST_DEVICE_LOSS_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_host_device_loss_info)))).value
except:
    pass


try:
    CTEX_HOST_DEVICE_LOSS_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_host_device_loss_info)))).value
except:
    pass


try:
    CTEX_TEXTURE_EXPORT_LAYER_SOURCE_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_texture_export_layer_source_descriptor)))).value
except:
    pass


try:
    CTEX_TEXTURE_EXPORT_LAYER_SOURCE_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_texture_export_layer_source_descriptor)))).value
except:
    pass


try:
    CTEX_TEXTURE_EXPORT_TEXTURE_SET_SOURCE_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_texture_export_texture_set_source_descriptor)))).value
except:
    pass


try:
    CTEX_TEXTURE_EXPORT_TEXTURE_SET_SOURCE_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_texture_export_texture_set_source_descriptor)))).value
except:
    pass


try:
    CTEX_TEXTURE_EXPORT_ATLAS_SOURCE_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_texture_export_atlas_source_descriptor)))).value
except:
    pass


try:
    CTEX_TEXTURE_EXPORT_ATLAS_SOURCE_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_texture_export_atlas_source_descriptor)))).value
except:
    pass


try:
    CTEX_TEXTURE_EXPORT_CATALOGUE_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_texture_export_catalogue_descriptor)))).value
except:
    pass


try:
    CTEX_TEXTURE_EXPORT_CATALOGUE_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_texture_export_catalogue_descriptor)))).value
except:
    pass


try:
    CTEX_TEXTURE_EXPORT_LAYER_SELECTION_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_texture_export_layer_selection_descriptor)))).value
except:
    pass


try:
    CTEX_TEXTURE_EXPORT_LAYER_SELECTION_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_texture_export_layer_selection_descriptor)))).value
except:
    pass


try:
    CTEX_TEXTURE_EXPORT_PLAN_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_texture_export_plan_descriptor)))).value
except:
    pass


try:
    CTEX_TEXTURE_EXPORT_PLAN_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_texture_export_plan_descriptor)))).value
except:
    pass


try:
    CTEX_TEXTURE_EXPORT_TEXTURE_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_texture_export_texture_descriptor)))).value
except:
    pass


try:
    CTEX_TEXTURE_EXPORT_TEXTURE_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_texture_export_texture_descriptor)))).value
except:
    pass


try:
    CTEX_TEXTURE_EXPORT_PRESET_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_texture_export_preset_descriptor)))).value
except:
    pass


try:
    CTEX_TEXTURE_EXPORT_PRESET_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_texture_export_preset_descriptor)))).value
except:
    pass


try:
    CTEX_TEXTURE_EXPORT_OPTIONS_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_texture_export_options_descriptor)))).value
except:
    pass


try:
    CTEX_TEXTURE_EXPORT_OPTIONS_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_texture_export_options_descriptor)))).value
except:
    pass


try:
    CTEX_TEXTURE_EXPORT_NAMED_VALUE_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_texture_export_named_value)))).value
except:
    pass


try:
    CTEX_TEXTURE_EXPORT_NAMED_VALUE_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_texture_export_named_value)))).value
except:
    pass


try:
    CTEX_TEXTURE_EXPORT_SAMPLE_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_texture_export_sample)))).value
except:
    pass


try:
    CTEX_TEXTURE_EXPORT_SAMPLE_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_texture_export_sample)))).value
except:
    pass


try:
    CTEX_TEXTURE_EXPORT_PIXEL_SOURCE_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_texture_export_pixel_source_descriptor)))).value
except:
    pass


try:
    CTEX_TEXTURE_EXPORT_PIXEL_SOURCE_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_texture_export_pixel_source_descriptor)))).value
except:
    pass


try:
    CTEX_TEXTURE_EXPORT_LAYER_SELECTION_VIEW_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_texture_export_layer_selection_view)))).value
except:
    pass


try:
    CTEX_TEXTURE_EXPORT_LAYER_SELECTION_VIEW_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_texture_export_layer_selection_view)))).value
except:
    pass


try:
    CTEX_TEXTURE_EXPORT_PLANNED_OUTPUT_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_texture_export_planned_output)))).value
except:
    pass


try:
    CTEX_TEXTURE_EXPORT_PLANNED_OUTPUT_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_texture_export_planned_output)))).value
except:
    pass


try:
    CTEX_TEXTURE_EXPORT_ENCODED_OUTPUT_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_texture_export_encoded_output)))).value
except:
    pass


try:
    CTEX_TEXTURE_EXPORT_ENCODED_OUTPUT_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_texture_export_encoded_output)))).value
except:
    pass


try:
    CTEX_TEXTURE_EXPORT_CALLBACKS_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_texture_export_callbacks_descriptor)))).value
except:
    pass


try:
    CTEX_TEXTURE_EXPORT_CALLBACKS_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_texture_export_callbacks_descriptor)))).value
except:
    pass


try:
    CTEX_TEXTURE_EXPORT_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_texture_export_info)))).value
except:
    pass


try:
    CTEX_TEXTURE_EXPORT_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_texture_export_info)))).value
except:
    pass


try:
    CTEX_PROJECT_CONTAINER_READ_LIMITS_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_project_container_read_limits_descriptor)))).value
except:
    pass


try:
    CTEX_PROJECT_CONTAINER_READ_LIMITS_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_project_container_read_limits_descriptor)))).value
except:
    pass


try:
    CTEX_PROJECT_CONTAINER_VERSION_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_project_container_version)))).value
except:
    pass


try:
    CTEX_PROJECT_CONTAINER_VERSION_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_project_container_version)))).value
except:
    pass


try:
    CTEX_PROJECT_CONTAINER_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_project_container_info)))).value
except:
    pass


try:
    CTEX_PROJECT_CONTAINER_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_project_container_info)))).value
except:
    pass


try:
    CTEX_PROJECT_AUTOSAVE_CONFIG_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_project_autosave_config_descriptor)))).value
except:
    pass


try:
    CTEX_PROJECT_AUTOSAVE_CONFIG_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_project_autosave_config_descriptor)))).value
except:
    pass


try:
    CTEX_PROJECT_AUTOSAVE_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_project_autosave_info)))).value
except:
    pass


try:
    CTEX_PROJECT_AUTOSAVE_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_project_autosave_info)))).value
except:
    pass


try:
    CTEX_PROJECT_QUIESCE_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_project_quiesce_descriptor)))).value
except:
    pass


try:
    CTEX_PROJECT_QUIESCE_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_project_quiesce_descriptor)))).value
except:
    pass


try:
    CTEX_PROJECT_QUIESCE_REPORT_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_project_quiesce_report)))).value
except:
    pass


try:
    CTEX_PROJECT_QUIESCE_REPORT_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_project_quiesce_report)))).value
except:
    pass


try:
    CTEX_PROJECT_RECOVERY_CHECKPOINT_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_project_recovery_checkpoint_info)))).value
except:
    pass


try:
    CTEX_PROJECT_RECOVERY_CHECKPOINT_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_project_recovery_checkpoint_info)))).value
except:
    pass


try:
    CTEX_PROJECT_RECOVERY_ENUMERATION_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_project_recovery_enumeration_info)))).value
except:
    pass


try:
    CTEX_PROJECT_RECOVERY_ENUMERATION_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_project_recovery_enumeration_info)))).value
except:
    pass


try:
    CTEX_PROJECT_ASSET_EXPORT_OPTIONS_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_project_asset_export_options_descriptor)))).value
except:
    pass


try:
    CTEX_PROJECT_ASSET_EXPORT_OPTIONS_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_project_asset_export_options_descriptor)))).value
except:
    pass


try:
    CTEX_PROJECT_ASSET_SEARCH_PATHS_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_project_asset_search_paths_descriptor)))).value
except:
    pass


try:
    CTEX_PROJECT_ASSET_SEARCH_PATHS_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_project_asset_search_paths_descriptor)))).value
except:
    pass


try:
    CTEX_PROJECT_RESOURCE_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_project_resource_descriptor)))).value
except:
    pass


try:
    CTEX_PROJECT_RESOURCE_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_project_resource_descriptor)))).value
except:
    pass


try:
    CTEX_OPERATION_CHANNEL_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_operation_channel_descriptor)))).value
except:
    pass


try:
    CTEX_OPERATION_CHANNEL_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_operation_channel_descriptor)))).value
except:
    pass


try:
    CTEX_PINNED_OPERATION_RESOURCE_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_pinned_operation_resource_descriptor)))).value
except:
    pass


try:
    CTEX_PINNED_OPERATION_RESOURCE_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_pinned_operation_resource_descriptor)))).value
except:
    pass


try:
    CTEX_OPERATION_RECORD_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_operation_record_descriptor)))).value
except:
    pass


try:
    CTEX_OPERATION_RECORD_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_operation_record_descriptor)))).value
except:
    pass


try:
    CTEX_OPERATION_RECORD_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_operation_record_info)))).value
except:
    pass


try:
    CTEX_OPERATION_RECORD_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_operation_record_info)))).value
except:
    pass


try:
    CTEX_OPERATION_ALGORITHM_SUPPORT_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_operation_algorithm_support_descriptor)))).value
except:
    pass


try:
    CTEX_OPERATION_ALGORITHM_SUPPORT_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_operation_algorithm_support_descriptor)))).value
except:
    pass


try:
    CTEX_OPERATION_REPLAY_ASSESSMENT_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_operation_replay_assessment_descriptor)))).value
except:
    pass


try:
    CTEX_OPERATION_REPLAY_ASSESSMENT_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_operation_replay_assessment_descriptor)))).value
except:
    pass


try:
    CTEX_OPERATION_REPLAY_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_operation_replay_info)))).value
except:
    pass


try:
    CTEX_OPERATION_REPLAY_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_operation_replay_info)))).value
except:
    pass


try:
    CTEX_PROJECT_OPERATION_REPLAY_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_project_operation_replay_info)))).value
except:
    pass


try:
    CTEX_PROJECT_OPERATION_REPLAY_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_project_operation_replay_info)))).value
except:
    pass


try:
    CTEX_RESOLUTION_OPERATION_RECORD_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_resolution_operation_record_descriptor)))).value
except:
    pass


try:
    CTEX_RESOLUTION_OPERATION_RECORD_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_resolution_operation_record_descriptor)))).value
except:
    pass


try:
    CTEX_RESOLUTION_REPLAY_RASTER_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_resolution_replay_raster_descriptor)))).value
except:
    pass


try:
    CTEX_RESOLUTION_REPLAY_RASTER_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_resolution_replay_raster_descriptor)))).value
except:
    pass


try:
    CTEX_TEXTURE_SET_RESOLUTION_CHANGE_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_texture_set_resolution_change_descriptor)))).value
except:
    pass


try:
    CTEX_TEXTURE_SET_RESOLUTION_CHANGE_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_texture_set_resolution_change_descriptor)))).value
except:
    pass


try:
    CTEX_TEXTURE_SET_RESOLUTION_CHANGE_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_texture_set_resolution_change_info)))).value
except:
    pass


try:
    CTEX_TEXTURE_SET_RESOLUTION_CHANGE_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_texture_set_resolution_change_info)))).value
except:
    pass


try:
    CTEX_TEXTURE_SET_RESOLUTION_RESTORE_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_texture_set_resolution_restore_info)))).value
except:
    pass


try:
    CTEX_TEXTURE_SET_RESOLUTION_RESTORE_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_texture_set_resolution_restore_info)))).value
except:
    pass


try:
    CTEX_EDITABLE_MATERIAL_PARAMETER_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_editable_material_parameter_descriptor)))).value
except:
    pass


try:
    CTEX_EDITABLE_MATERIAL_PARAMETER_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_editable_material_parameter_descriptor)))).value
except:
    pass


try:
    CTEX_EDITABLE_TILE_DEPENDENCY_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_editable_tile_dependency_descriptor)))).value
except:
    pass


try:
    CTEX_EDITABLE_TILE_DEPENDENCY_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_editable_tile_dependency_descriptor)))).value
except:
    pass


try:
    CTEX_EDITABLE_SURFACE_POINT_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_editable_surface_point_descriptor)))).value
except:
    pass


try:
    CTEX_EDITABLE_SURFACE_POINT_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_editable_surface_point_descriptor)))).value
except:
    pass


try:
    CTEX_EDITABLE_ENTRY_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_editable_entry_descriptor)))).value
except:
    pass


try:
    CTEX_EDITABLE_ENTRY_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_editable_entry_descriptor)))).value
except:
    pass


try:
    CTEX_EDITABLE_ENTRY_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_editable_entry_info)))).value
except:
    pass


try:
    CTEX_EDITABLE_ENTRY_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_editable_entry_info)))).value
except:
    pass


try:
    CTEX_PRESET_SHELF_ENTRY_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_preset_shelf_entry_descriptor)))).value
except:
    pass


try:
    CTEX_PRESET_SHELF_ENTRY_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_preset_shelf_entry_descriptor)))).value
except:
    pass


try:
    CTEX_PRESET_SHELF_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_preset_shelf_descriptor)))).value
except:
    pass


try:
    CTEX_PRESET_SHELF_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_preset_shelf_descriptor)))).value
except:
    pass


try:
    CTEX_PRESET_LIBRARY_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_preset_library_descriptor)))).value
except:
    pass


try:
    CTEX_PRESET_LIBRARY_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_preset_library_descriptor)))).value
except:
    pass


try:
    CTEX_PRESET_LIBRARY_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_preset_library_info)))).value
except:
    pass


try:
    CTEX_PRESET_LIBRARY_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_preset_library_info)))).value
except:
    pass


try:
    CTEX_MATERIAL_GRAPH_CATALOGUE_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_material_graph_catalogue_info)))).value
except:
    pass


try:
    CTEX_MATERIAL_GRAPH_CATALOGUE_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_material_graph_catalogue_info)))).value
except:
    pass


try:
    CTEX_MATERIAL_GRAPH_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_material_graph_info)))).value
except:
    pass


try:
    CTEX_MATERIAL_GRAPH_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_material_graph_info)))).value
except:
    pass


try:
    CTEX_MATERIAL_GRAPH_LINK_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_material_graph_link_descriptor)))).value
except:
    pass


try:
    CTEX_MATERIAL_GRAPH_LINK_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_material_graph_link_descriptor)))).value
except:
    pass


try:
    CTEX_MATERIAL_GRAPH_LINK_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_material_graph_link_info)))).value
except:
    pass


try:
    CTEX_MATERIAL_GRAPH_LINK_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_material_graph_link_info)))).value
except:
    pass


try:
    CTEX_MATERIAL_GRAPH_VALIDATION_RESOURCES_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_material_graph_validation_resources_descriptor)))).value
except:
    pass


try:
    CTEX_MATERIAL_GRAPH_VALIDATION_RESOURCES_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_material_graph_validation_resources_descriptor)))).value
except:
    pass


try:
    CTEX_MATERIAL_GRAPH_VALIDATION_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_material_graph_validation_info)))).value
except:
    pass


try:
    CTEX_MATERIAL_GRAPH_VALIDATION_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_material_graph_validation_info)))).value
except:
    pass


try:
    CTEX_MATERIAL_GRAPH_LIBRARY_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_material_graph_library_info)))).value
except:
    pass


try:
    CTEX_MATERIAL_GRAPH_LIBRARY_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_material_graph_library_info)))).value
except:
    pass


try:
    CTEX_MATERIAL_GRAPH_PRESET_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_material_graph_preset_descriptor)))).value
except:
    pass


try:
    CTEX_MATERIAL_GRAPH_PRESET_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_material_graph_preset_descriptor)))).value
except:
    pass


try:
    CTEX_SMART_MATERIAL_VALUE_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_smart_material_value_descriptor)))).value
except:
    pass


try:
    CTEX_SMART_MATERIAL_VALUE_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_smart_material_value_descriptor)))).value
except:
    pass


try:
    CTEX_MATERIAL_GRAPH_SOCKET_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_material_graph_socket_descriptor)))).value
except:
    pass


try:
    CTEX_MATERIAL_GRAPH_SOCKET_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_material_graph_socket_descriptor)))).value
except:
    pass


try:
    CTEX_MATERIAL_GRAPH_GROUP_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_material_graph_group_descriptor)))).value
except:
    pass


try:
    CTEX_MATERIAL_GRAPH_GROUP_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_material_graph_group_descriptor)))).value
except:
    pass


try:
    CTEX_MATERIAL_GRAPH_GROUP_INTERFACE_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_material_graph_group_interface_descriptor)))).value
except:
    pass


try:
    CTEX_MATERIAL_GRAPH_GROUP_INTERFACE_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_material_graph_group_interface_descriptor)))).value
except:
    pass


try:
    CTEX_MATERIAL_GRAPH_WORKSPACE_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_material_graph_workspace_info)))).value
except:
    pass


try:
    CTEX_MATERIAL_GRAPH_WORKSPACE_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_material_graph_workspace_info)))).value
except:
    pass


try:
    CTEX_MATERIAL_GRAPH_GROUP_UPDATE_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_material_graph_group_update_info)))).value
except:
    pass


try:
    CTEX_MATERIAL_GRAPH_GROUP_UPDATE_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_material_graph_group_update_info)))).value
except:
    pass


try:
    CTEX_SHADER_TEXTURE_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_shader_texture_descriptor)))).value
except:
    pass


try:
    CTEX_SHADER_TEXTURE_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_shader_texture_descriptor)))).value
except:
    pass


try:
    CTEX_SHADER_MATERIAL_RESOURCE_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_shader_material_resource_descriptor)))).value
except:
    pass


try:
    CTEX_SHADER_MATERIAL_RESOURCE_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_shader_material_resource_descriptor)))).value
except:
    pass


try:
    CTEX_SHADER_DEVICE_FEATURES_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_shader_device_features_descriptor)))).value
except:
    pass


try:
    CTEX_SHADER_DEVICE_FEATURES_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_shader_device_features_descriptor)))).value
except:
    pass


try:
    CTEX_SHADER_MATERIAL_REQUEST_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_shader_material_request)))).value
except:
    pass


try:
    CTEX_SHADER_MATERIAL_REQUEST_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_shader_material_request)))).value
except:
    pass


try:
    CTEX_SHADER_MATERIAL_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_shader_material_info)))).value
except:
    pass


try:
    CTEX_SHADER_MATERIAL_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_shader_material_info)))).value
except:
    pass


try:
    CTEX_SHADER_MATERIAL_SOURCE_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_shader_material_source_descriptor)))).value
except:
    pass


try:
    CTEX_SHADER_MATERIAL_SOURCE_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_shader_material_source_descriptor)))).value
except:
    pass


try:
    CTEX_SHADER_MATERIAL_DEBUG_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_shader_material_debug_info)))).value
except:
    pass


try:
    CTEX_SHADER_MATERIAL_DEBUG_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_shader_material_debug_info)))).value
except:
    pass


try:
    CTEX_SHADER_BACKEND_ATTRIBUTION_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_shader_backend_attribution_info)))).value
except:
    pass


try:
    CTEX_SHADER_BACKEND_ATTRIBUTION_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_shader_backend_attribution_info)))).value
except:
    pass


try:
    CTEX_SHADER_LAYER_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_shader_layer_descriptor)))).value
except:
    pass


try:
    CTEX_SHADER_LAYER_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_shader_layer_descriptor)))).value
except:
    pass


try:
    CTEX_SHADER_LAYER_STACK_REQUEST_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_shader_layer_stack_request)))).value
except:
    pass


try:
    CTEX_SHADER_LAYER_STACK_REQUEST_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_shader_layer_stack_request)))).value
except:
    pass


try:
    CTEX_SHADER_LAYER_STACK_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_shader_layer_stack_info)))).value
except:
    pass


try:
    CTEX_SHADER_LAYER_STACK_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_shader_layer_stack_info)))).value
except:
    pass


try:
    CTEX_SHADER_EMISSION_CACHE_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_shader_emission_cache_info)))).value
except:
    pass


try:
    CTEX_SHADER_EMISSION_CACHE_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_shader_emission_cache_info)))).value
except:
    pass


try:
    CTEX_SHADER_PREVIEW_CHANNEL_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_shader_preview_channel_descriptor)))).value
except:
    pass


try:
    CTEX_SHADER_PREVIEW_CHANNEL_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_shader_preview_channel_descriptor)))).value
except:
    pass


try:
    CTEX_SHADER_PREVIEW_ENVIRONMENT_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_shader_preview_environment_descriptor)))).value
except:
    pass


try:
    CTEX_SHADER_PREVIEW_ENVIRONMENT_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_shader_preview_environment_descriptor)))).value
except:
    pass


try:
    CTEX_SHADER_PREVIEW_REQUEST_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_shader_preview_request)))).value
except:
    pass


try:
    CTEX_SHADER_PREVIEW_REQUEST_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_shader_preview_request)))).value
except:
    pass


try:
    CTEX_SHADER_PREVIEW_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_shader_preview_info)))).value
except:
    pass


try:
    CTEX_SHADER_PREVIEW_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_shader_preview_info)))).value
except:
    pass


try:
    CTEX_MATERIAL_GRAPH_PROPERTY_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_material_graph_property_descriptor)))).value
except:
    pass


try:
    CTEX_MATERIAL_GRAPH_PROPERTY_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_material_graph_property_descriptor)))).value
except:
    pass


try:
    CTEX_MATERIAL_GRAPH_PARITY_FIXTURE_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_material_graph_parity_fixture_descriptor)))).value
except:
    pass


try:
    CTEX_MATERIAL_GRAPH_PARITY_FIXTURE_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_material_graph_parity_fixture_descriptor)))).value
except:
    pass


try:
    CTEX_MATERIAL_GRAPH_HOST_EVALUATION_REQUEST_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_material_graph_host_evaluation_request)))).value
except:
    pass


try:
    CTEX_MATERIAL_GRAPH_HOST_EVALUATION_REQUEST_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_material_graph_host_evaluation_request)))).value
except:
    pass


try:
    CTEX_MATERIAL_GRAPH_HOST_EMISSION_REQUEST_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_material_graph_host_emission_request)))).value
except:
    pass


try:
    CTEX_MATERIAL_GRAPH_HOST_EMISSION_REQUEST_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_material_graph_host_emission_request)))).value
except:
    pass


try:
    CTEX_MATERIAL_GRAPH_HOST_EMISSION_RESULT_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_material_graph_host_emission_result)))).value
except:
    pass


try:
    CTEX_MATERIAL_GRAPH_HOST_EMISSION_RESULT_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_material_graph_host_emission_result)))).value
except:
    pass


try:
    CTEX_MATERIAL_GRAPH_HOST_NODE_REGISTRATION_DESCRIPTOR_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_material_graph_host_node_registration_descriptor)))).value
except:
    pass


try:
    CTEX_MATERIAL_GRAPH_HOST_NODE_REGISTRATION_DESCRIPTOR_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_material_graph_host_node_registration_descriptor)))).value
except:
    pass


try:
    CTEX_MATERIAL_GRAPH_NODE_REGISTRY_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_material_graph_node_registry_info)))).value
except:
    pass


try:
    CTEX_MATERIAL_GRAPH_NODE_REGISTRY_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_material_graph_node_registry_info)))).value
except:
    pass


try:
    CTEX_MATERIAL_GRAPH_HOST_CONTRACT_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_material_graph_host_contract_info)))).value
except:
    pass


try:
    CTEX_MATERIAL_GRAPH_HOST_CONTRACT_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_material_graph_host_contract_info)))).value
except:
    pass


try:
    CTEX_SMART_MATERIAL_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_smart_material_info)))).value
except:
    pass


try:
    CTEX_SMART_MATERIAL_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_smart_material_info)))).value
except:
    pass


try:
    CTEX_PRESET_APPLICATION_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_preset_application_info)))).value
except:
    pass


try:
    CTEX_PRESET_APPLICATION_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_preset_application_info)))).value
except:
    pass


try:
    CTEX_PRESET_UNDO_INFO_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_preset_undo_info)))).value
except:
    pass


try:
    CTEX_PRESET_UNDO_INFO_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_preset_undo_info)))).value
except:
    pass


try:
    CTEX_CHANNEL_COLOR_POLICY_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_channel_color_policy)))).value
except:
    pass


try:
    CTEX_CHANNEL_COLOR_POLICY_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_channel_color_policy)))).value
except:
    pass


try:
    CTEX_RESOLVED_INPUT_COLOR_SPACE_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_resolved_input_color_space)))).value
except:
    pass


try:
    CTEX_RESOLVED_INPUT_COLOR_SPACE_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_resolved_input_color_space)))).value
except:
    pass


try:
    CTEX_BIT_DEPTH_WARNING_V1_SIZE = (uint32_t (ord_if_char(sizeof(ctex_bit_depth_warning)))).value
except:
    pass


try:
    CTEX_BIT_DEPTH_WARNING_CURRENT_SIZE = (uint32_t (ord_if_char(sizeof(ctex_bit_depth_warning)))).value
except:
    pass

ctex_log_sink_descriptor = struct_ctex_log_sink_descriptor
ctex_allocator_descriptor = struct_ctex_allocator_descriptor
ctex_document = struct_ctex_document
ctex_cube_lut = struct_ctex_cube_lut
ctex_mesh = struct_ctex_mesh
ctex_mesh_replacement_plan = struct_ctex_mesh_replacement_plan
ctex_paint_dilation_session = struct_ctex_paint_dilation_session
ctex_paint_surface_map_cache = struct_ctex_paint_surface_map_cache
ctex_paint_preview_session = struct_ctex_paint_preview_session
ctex_pick_index = struct_ctex_pick_index
ctex_uv_pick_index = struct_ctex_uv_pick_index
ctex_transport_snapshot_pool = struct_ctex_transport_snapshot_pool
ctex_transport_snapshot = struct_ctex_transport_snapshot
ctex_transport_readback = struct_ctex_transport_readback
ctex_resource_ledger = struct_ctex_resource_ledger
ctex_resource_reservation = struct_ctex_resource_reservation
ctex_mesh_map_set = struct_ctex_mesh_map_set
ctex_mesh_map_bake_session = struct_ctex_mesh_map_bake_session
ctex_mesh_map_bake_request_token = struct_ctex_mesh_map_bake_request_token
ctex_project_autosave_session = struct_ctex_project_autosave_session
ctex_executor_registry = struct_ctex_executor_registry
ctex_cpu_execution_result = struct_ctex_cpu_execution_result
ctex_parity_gate_result = struct_ctex_parity_gate_result
ctex_host_execution_session = struct_ctex_host_execution_session
ctex_host_completion_result = struct_ctex_host_completion_result
ctex_host_recovery_report = struct_ctex_host_recovery_report
ctex_material_graph_workspace = struct_ctex_material_graph_workspace
ctex_material_graph_node_registry = struct_ctex_material_graph_node_registry
ctex_shader_emission_cache = struct_ctex_shader_emission_cache
ctex_tile_history_capture = struct_ctex_tile_history_capture
ctex_layer_snapshot = struct_ctex_layer_snapshot
ctex_texture_set_transaction = struct_ctex_texture_set_transaction
ctex_vec2f = struct_ctex_vec2f
ctex_vec3f = struct_ctex_vec3f
ctex_vec4f = struct_ctex_vec4f
ctex_vec2d = struct_ctex_vec2d
ctex_vec3d = struct_ctex_vec3d
ctex_stroke_frame = struct_ctex_stroke_frame
ctex_stroke_input_sample = struct_ctex_stroke_input_sample
ctex_response_curve_point = struct_ctex_response_curve_point
ctex_response_mapping_descriptor = struct_ctex_response_mapping_descriptor
ctex_stroke_stabilizer_descriptor = struct_ctex_stroke_stabilizer_descriptor
ctex_stroke_jitter_descriptor = struct_ctex_stroke_jitter_descriptor
ctex_stroke_taper_span_descriptor = struct_ctex_stroke_taper_span_descriptor
ctex_stroke_taper_descriptor = struct_ctex_stroke_taper_descriptor
ctex_stroke_constraint_descriptor = struct_ctex_stroke_constraint_descriptor
ctex_stroke_symmetry_descriptor = struct_ctex_stroke_symmetry_descriptor
ctex_stroke_settings_descriptor = struct_ctex_stroke_settings_descriptor
ctex_resolved_stamp = struct_ctex_resolved_stamp
ctex_swept_segment = struct_ctex_swept_segment
ctex_resolved_stroke_info = struct_ctex_resolved_stroke_info
ctex_resolved_stroke_descriptor = struct_ctex_resolved_stroke_descriptor
ctex_paint_tile_coverage_descriptor = struct_ctex_paint_tile_coverage_descriptor
ctex_paint_mask_view = struct_ctex_paint_mask_view
ctex_paint_mask_inputs_descriptor = struct_ctex_paint_mask_inputs_descriptor
ctex_paint_mask_info = struct_ctex_paint_mask_info
ctex_paint_material_coordinate_descriptor = struct_ctex_paint_material_coordinate_descriptor
ctex_paint_material_coordinate_sample = struct_ctex_paint_material_coordinate_sample
ctex_paint_depth_context_descriptor = struct_ctex_paint_depth_context_descriptor
ctex_paint_rejection_descriptor = struct_ctex_paint_rejection_descriptor
ctex_paint_rejection_info = struct_ctex_paint_rejection_info
ctex_paint_stamp_footprint = struct_ctex_paint_stamp_footprint
ctex_paint_tile_coordinate = struct_ctex_paint_tile_coordinate
ctex_paint_preview_info = struct_ctex_paint_preview_info
ctex_paint_work_descriptor = struct_ctex_paint_work_descriptor
ctex_paint_work_info = struct_ctex_paint_work_info
ctex_paint_seam_dilation_descriptor = struct_ctex_paint_seam_dilation_descriptor
ctex_paint_seam_dilation_info = struct_ctex_paint_seam_dilation_info
ctex_paint_dilation_tile_descriptor = struct_ctex_paint_dilation_tile_descriptor
ctex_paint_dilation_tile_info = struct_ctex_paint_dilation_tile_info
ctex_paint_dilation_session_info = struct_ctex_paint_dilation_session_info
ctex_paint_surface_filter_sample = struct_ctex_paint_surface_filter_sample
ctex_paint_surface_filter_descriptor = struct_ctex_paint_surface_filter_descriptor
ctex_paint_island_padding_descriptor = struct_ctex_paint_island_padding_descriptor
ctex_paint_unsupported_mip_level = struct_ctex_paint_unsupported_mip_level
ctex_paint_island_padding_info = struct_ctex_paint_island_padding_info
ctex_paint_surface_map_request = struct_ctex_paint_surface_map_request
ctex_paint_surface_texel = struct_ctex_paint_surface_texel
ctex_paint_surface_map_info = struct_ctex_paint_surface_map_info
ctex_paint_surface_map_buffers = struct_ctex_paint_surface_map_buffers
ctex_paint_surface_map_statistics = struct_ctex_paint_surface_map_statistics
ctex_paint_deposition_descriptor = struct_ctex_paint_deposition_descriptor
ctex_paint_deposition_info = struct_ctex_paint_deposition_info
ctex_paint_deposition_sample = struct_ctex_paint_deposition_sample
ctex_paint_blend_descriptor = struct_ctex_paint_blend_descriptor
ctex_paint_tool_channel_descriptor = struct_ctex_paint_tool_channel_descriptor
ctex_paint_tool_channel_output = struct_ctex_paint_tool_channel_output
ctex_paint_brush_descriptor = struct_ctex_paint_brush_descriptor
ctex_paint_brush_info = struct_ctex_paint_brush_info
ctex_paint_eraser_descriptor = struct_ctex_paint_eraser_descriptor
ctex_paint_eraser_info = struct_ctex_paint_eraser_info
ctex_paint_fill_triangle_topology = struct_ctex_paint_fill_triangle_topology
ctex_paint_fill_descriptor = struct_ctex_paint_fill_descriptor
ctex_paint_fill_info = struct_ctex_paint_fill_info
ctex_paint_fill_outputs = struct_ctex_paint_fill_outputs
ctex_paint_clone_source_descriptor = struct_ctex_paint_clone_source_descriptor
ctex_paint_clone_descriptor = struct_ctex_paint_clone_descriptor
ctex_paint_clone_info = struct_ctex_paint_clone_info
ctex_paint_clone_outputs = struct_ctex_paint_clone_outputs
ctex_paint_blur_neighborhood_descriptor = struct_ctex_paint_blur_neighborhood_descriptor
ctex_paint_blur_descriptor = struct_ctex_paint_blur_descriptor
ctex_paint_blur_info = struct_ctex_paint_blur_info
ctex_paint_smear_mapping_descriptor = struct_ctex_paint_smear_mapping_descriptor
ctex_paint_smear_descriptor = struct_ctex_paint_smear_descriptor
ctex_paint_smear_info = struct_ctex_paint_smear_info
ctex_paint_stencil_descriptor = struct_ctex_paint_stencil_descriptor
ctex_paint_stencil_info = struct_ctex_paint_stencil_info
ctex_paint_decal_transform = struct_ctex_paint_decal_transform
ctex_paint_decal_placement = struct_ctex_paint_decal_placement
ctex_paint_decal_descriptor = struct_ctex_paint_decal_descriptor
ctex_paint_decal_info = struct_ctex_paint_decal_info
ctex_paint_decal_outputs = struct_ctex_paint_decal_outputs
ctex_paint_projection_descriptor = struct_ctex_paint_projection_descriptor
ctex_paint_projection_sample = struct_ctex_paint_projection_sample
ctex_paint_projection_info = struct_ctex_paint_projection_info
ctex_paint_projection_outputs = struct_ctex_paint_projection_outputs
ctex_paint_font_glyph_descriptor = struct_ctex_paint_font_glyph_descriptor
ctex_paint_font_descriptor = struct_ctex_paint_font_descriptor
ctex_paint_text_material_value = struct_ctex_paint_text_material_value
ctex_paint_text_descriptor = struct_ctex_paint_text_descriptor
ctex_paint_text_info = struct_ctex_paint_text_info
ctex_paint_text_outputs = struct_ctex_paint_text_outputs
ctex_paint_particle_settings = struct_ctex_paint_particle_settings
ctex_paint_particle_contact = struct_ctex_paint_particle_contact
ctex_paint_particle_state = struct_ctex_paint_particle_state
ctex_pick_texture_set_binding_descriptor = struct_ctex_pick_texture_set_binding_descriptor
ctex_paint_particle_descriptor = struct_ctex_paint_particle_descriptor
ctex_paint_particle_info = struct_ctex_paint_particle_info
ctex_paint_particle_outputs = struct_ctex_paint_particle_outputs
ctex_stroke_preset_info = struct_ctex_stroke_preset_info
ctex_stroke_preset_buffers_descriptor = struct_ctex_stroke_preset_buffers_descriptor
ctex_uv_set_descriptor = struct_ctex_uv_set_descriptor
ctex_mesh_partition_descriptor = struct_ctex_mesh_partition_descriptor
ctex_mesh_descriptor = struct_ctex_mesh_descriptor
ctex_mesh_info = struct_ctex_mesh_info
ctex_mesh_uv_overlap_info = struct_ctex_mesh_uv_overlap_info
ctex_mesh_uv_coverage_info = struct_ctex_mesh_uv_coverage_info
ctex_mesh_replacement_entry = struct_ctex_mesh_replacement_entry
ctex_mesh_replacement_plan_info = struct_ctex_mesh_replacement_plan_info
ctex_mesh_replacement_decision = struct_ctex_mesh_replacement_decision
ctex_mesh_replacement_apply_info = struct_ctex_mesh_replacement_apply_info
ctex_mesh_reprojection_descriptor = struct_ctex_mesh_reprojection_descriptor
ctex_mesh_reprojection_preflight_info = struct_ctex_mesh_reprojection_preflight_info
ctex_mesh_reprojection_commit_info = struct_ctex_mesh_reprojection_commit_info
ctex_pick_ray = struct_ctex_pick_ray
ctex_pick_options_descriptor = struct_ctex_pick_options_descriptor
ctex_pick_screen_view_descriptor = struct_ctex_pick_screen_view_descriptor
ctex_pick_hit = struct_ctex_pick_hit
ctex_pick_index_info = struct_ctex_pick_index_info
ctex_pick_query_info = struct_ctex_pick_query_info
ctex_paint_picker_texture_view_descriptor = struct_ctex_paint_picker_texture_view_descriptor
ctex_paint_picker_descriptor = struct_ctex_paint_picker_descriptor
ctex_paint_picker_channel_value = struct_ctex_paint_picker_channel_value
ctex_paint_picker_info = struct_ctex_paint_picker_info
ctex_paint_colour_id_descriptor = struct_ctex_paint_colour_id_descriptor
ctex_paint_colour_id_info = struct_ctex_paint_colour_id_info
ctex_paint_parameter_descriptor = struct_ctex_paint_parameter_descriptor
ctex_paint_parameter_catalogue_info = struct_ctex_paint_parameter_catalogue_info
ctex_paint_parameter_validation_info = struct_ctex_paint_parameter_validation_info
ctex_paint_selection_surface_descriptor = struct_ctex_paint_selection_surface_descriptor
ctex_paint_screen_selection_descriptor = struct_ctex_paint_screen_selection_descriptor
ctex_paint_polygon_selection_descriptor = struct_ctex_paint_polygon_selection_descriptor
ctex_paint_selection_info = struct_ctex_paint_selection_info
ctex_paint_selection_outputs = struct_ctex_paint_selection_outputs
ctex_pick_batch_control_descriptor = struct_ctex_pick_batch_control_descriptor
ctex_pick_batch_info = struct_ctex_pick_batch_info
ctex_texture_set_descriptor = struct_ctex_texture_set_descriptor
ctex_udim_pixel_write_descriptor = struct_ctex_udim_pixel_write_descriptor
ctex_udim_write_info = struct_ctex_udim_write_info
ctex_atlas_region_descriptor = struct_ctex_atlas_region_descriptor
ctex_atlas_descriptor = struct_ctex_atlas_descriptor
ctex_atlas_region = struct_ctex_atlas_region
ctex_atlas_info = struct_ctex_atlas_info
ctex_layer_channel_descriptor = struct_ctex_layer_channel_descriptor
ctex_layer_entry_descriptor = struct_ctex_layer_entry_descriptor
ctex_layer_mask_sample = struct_ctex_layer_mask_sample
ctex_layer_participation_info = struct_ctex_layer_participation_info
ctex_tile_history_target_descriptor = struct_ctex_tile_history_target_descriptor
ctex_tile_history_budget_report = struct_ctex_tile_history_budget_report
ctex_tile_history_commit_info = struct_ctex_tile_history_commit_info
ctex_tile_history_restore_info = struct_ctex_tile_history_restore_info
ctex_layer_composite_raster_descriptor = struct_ctex_layer_composite_raster_descriptor
ctex_layer_composite_mask_descriptor = struct_ctex_layer_composite_mask_descriptor
ctex_layer_composite_info = struct_ctex_layer_composite_info
ctex_layer_composite_channel_info = struct_ctex_layer_composite_channel_info
ctex_layer_snapshot_info = struct_ctex_layer_snapshot_info
ctex_layer_snapshot_content_info = struct_ctex_layer_snapshot_content_info
ctex_layer_snapshot_mask_info = struct_ctex_layer_snapshot_mask_info
ctex_layer_operation_descriptor = struct_ctex_layer_operation_descriptor
ctex_layer_operation_info = struct_ctex_layer_operation_info
ctex_image_decode_limits_descriptor = struct_ctex_image_decode_limits_descriptor
ctex_image_decode_progress_info = struct_ctex_image_decode_progress_info
ctex_image_decode_control_descriptor = struct_ctex_image_decode_control_descriptor
ctex_image_decode_execution_info = struct_ctex_image_decode_execution_info
ctex_decoded_image_info = struct_ctex_decoded_image_info
ctex_layered_image_decode_descriptor = struct_ctex_layered_image_decode_descriptor
ctex_layered_image_decode_info = struct_ctex_layered_image_decode_info
ctex_layered_decoded_image_info = struct_ctex_layered_decoded_image_info
ctex_image_channel_expansion_descriptor = struct_ctex_image_channel_expansion_descriptor
ctex_image_channel_expansion_info = struct_ctex_image_channel_expansion_info
ctex_image_resample_descriptor = struct_ctex_image_resample_descriptor
ctex_image_resample_info = struct_ctex_image_resample_info
ctex_image_encode_descriptor = struct_ctex_image_encode_descriptor
ctex_channel_descriptor = struct_ctex_channel_descriptor
ctex_channel_info = struct_ctex_channel_info
ctex_texture_set_memory_report = struct_ctex_texture_set_memory_report
ctex_document_texture_set_memory_info = struct_ctex_document_texture_set_memory_info
ctex_document_memory_info = struct_ctex_document_memory_info
ctex_transport_revision_cursor = struct_ctex_transport_revision_cursor
ctex_transport_tile_version = struct_ctex_transport_tile_version
ctex_transport_delta_info = struct_ctex_transport_delta_info
ctex_transport_pixel_format = struct_ctex_transport_pixel_format
ctex_transport_format_selection = struct_ctex_transport_format_selection
ctex_transport_snapshot_query_info = struct_ctex_transport_snapshot_query_info
ctex_transport_snapshot_memory_report = struct_ctex_transport_snapshot_memory_report
ctex_resource_allocation_descriptor = struct_ctex_resource_allocation_descriptor
ctex_resource_category_report = struct_ctex_resource_category_report
ctex_resource_accounting_report = struct_ctex_resource_accounting_report
ctex_resource_budget_limits = struct_ctex_resource_budget_limits
ctex_resource_requirement = struct_ctex_resource_requirement
ctex_resource_admission_descriptor = struct_ctex_resource_admission_descriptor
ctex_resource_admission_report = struct_ctex_resource_admission_report
ctex_preview_quality_option = struct_ctex_preview_quality_option
ctex_preview_quality_admission_descriptor = struct_ctex_preview_quality_admission_descriptor
ctex_preview_quality_admission_report = struct_ctex_preview_quality_admission_report
ctex_tile_backing_key = struct_ctex_tile_backing_key
ctex_tile_backing_store_descriptor = struct_ctex_tile_backing_store_descriptor
ctex_tile_eviction_report = struct_ctex_tile_eviction_report
ctex_transport_tile_memory_layout = struct_ctex_transport_tile_memory_layout
ctex_transport_tile_readback_destination = struct_ctex_transport_tile_readback_destination
ctex_transport_host_tile_completion = struct_ctex_transport_host_tile_completion
ctex_transport_readback_info = struct_ctex_transport_readback_info
ctex_tangent_frame_descriptor = struct_ctex_tangent_frame_descriptor
ctex_mesh_tangent_data_descriptor = struct_ctex_mesh_tangent_data_descriptor
ctex_mesh_tangent_frame_info = struct_ctex_mesh_tangent_frame_info
ctex_mesh_map_pixel_buffer_descriptor = struct_ctex_mesh_map_pixel_buffer_descriptor
ctex_mesh_map_import_descriptor = struct_ctex_mesh_map_import_descriptor
ctex_mesh_map_import_info = struct_ctex_mesh_map_import_info
ctex_mesh_map_set_info = struct_ctex_mesh_map_set_info
ctex_mesh_map_entry_info = struct_ctex_mesh_map_entry_info
ctex_mesh_map_sample_info = struct_ctex_mesh_map_sample_info
ctex_mesh_map_staleness = struct_ctex_mesh_map_staleness
ctex_mesh_map_requirement_info = struct_ctex_mesh_map_requirement_info
ctex_mesh_map_release_info = struct_ctex_mesh_map_release_info
ctex_mesh_map_generator_parameter_descriptor = struct_ctex_mesh_map_generator_parameter_descriptor
ctex_mesh_map_generator_info = struct_ctex_mesh_map_generator_info
ctex_mesh_map_generator_parameter = struct_ctex_mesh_map_generator_parameter
ctex_mesh_map_generator_resolved_parameter = struct_ctex_mesh_map_generator_resolved_parameter
ctex_mesh_map_generator_parameter_clamp = struct_ctex_mesh_map_generator_parameter_clamp
ctex_mesh_map_generator_result_info = struct_ctex_mesh_map_generator_result_info
ctex_mesh_map_bake_request_descriptor = struct_ctex_mesh_map_bake_request_descriptor
ctex_mesh_map_bake_control = struct_ctex_mesh_map_bake_control
ctex_mesh_map_bake_output_descriptor = struct_ctex_mesh_map_bake_output_descriptor
ctex_mesh_map_bake_provider_descriptor = struct_ctex_mesh_map_bake_provider_descriptor
ctex_mesh_map_bake_control_descriptor = struct_ctex_mesh_map_bake_control_descriptor
ctex_mesh_map_bake_result_info = struct_ctex_mesh_map_bake_result_info
ctex_mesh_map_bake_session_info = struct_ctex_mesh_map_bake_session_info
ctex_mesh_map_bake_token_info = struct_ctex_mesh_map_bake_token_info
ctex_mesh_map_bake_completion_info = struct_ctex_mesh_map_bake_completion_info
ctex_mesh_map_bake_settings_edit_info = struct_ctex_mesh_map_bake_settings_edit_info
ctex_mesh_map_bake_settings_undo_info = struct_ctex_mesh_map_bake_settings_undo_info
ctex_host_executor_descriptor = struct_ctex_host_executor_descriptor
ctex_executor_info = struct_ctex_executor_info
ctex_executor_selection_info = struct_ctex_executor_selection_info
ctex_executor_fallback_descriptor = struct_ctex_executor_fallback_descriptor
ctex_executor_fallback_info = struct_ctex_executor_fallback_info
ctex_cpu_bounded_execution_descriptor = struct_ctex_cpu_bounded_execution_descriptor
ctex_cpu_execution_info = struct_ctex_cpu_execution_info
ctex_cpu_raster_mesh_descriptor = struct_ctex_cpu_raster_mesh_descriptor
ctex_cpu_raster_camera_descriptor = struct_ctex_cpu_raster_camera_descriptor
ctex_cpu_viewport_raster_descriptor = struct_ctex_cpu_viewport_raster_descriptor
ctex_cpu_uv_raster_descriptor = struct_ctex_cpu_uv_raster_descriptor
ctex_cpu_raster_info = struct_ctex_cpu_raster_info
ctex_cpu_raster_outputs = struct_ctex_cpu_raster_outputs
ctex_parity_tolerance_info = struct_ctex_parity_tolerance_info
ctex_parity_comparison_info = struct_ctex_parity_comparison_info
ctex_parity_fixture_channel_descriptor = struct_ctex_parity_fixture_channel_descriptor
ctex_parity_fixture_descriptor = struct_ctex_parity_fixture_descriptor
ctex_parity_rendered_channel_descriptor = struct_ctex_parity_rendered_channel_descriptor
ctex_parity_rendered_fixture_descriptor = struct_ctex_parity_rendered_fixture_descriptor
ctex_parity_executor_binding_descriptor = struct_ctex_parity_executor_binding_descriptor
ctex_parity_gate_info = struct_ctex_parity_gate_info
ctex_host_resource_descriptor = struct_ctex_host_resource_descriptor
ctex_host_submission_descriptor = struct_ctex_host_submission_descriptor
ctex_host_submission_info = struct_ctex_host_submission_info
ctex_host_execution_session_info = struct_ctex_host_execution_session_info
ctex_host_completed_resource_descriptor = struct_ctex_host_completed_resource_descriptor
ctex_host_recovery_descriptor = struct_ctex_host_recovery_descriptor
ctex_host_completion_descriptor = struct_ctex_host_completion_descriptor
ctex_host_resource_version = struct_ctex_host_resource_version
ctex_host_completion_result_info = struct_ctex_host_completion_result_info
ctex_host_device_loss_info = struct_ctex_host_device_loss_info
ctex_texture_export_layer_source_descriptor = struct_ctex_texture_export_layer_source_descriptor
ctex_texture_export_texture_set_source_descriptor = struct_ctex_texture_export_texture_set_source_descriptor
ctex_texture_export_atlas_source_descriptor = struct_ctex_texture_export_atlas_source_descriptor
ctex_texture_export_catalogue_descriptor = struct_ctex_texture_export_catalogue_descriptor
ctex_texture_export_layer_selection_descriptor = struct_ctex_texture_export_layer_selection_descriptor
ctex_texture_export_plan_descriptor = struct_ctex_texture_export_plan_descriptor
ctex_texture_export_texture_descriptor = struct_ctex_texture_export_texture_descriptor
ctex_texture_export_preset_descriptor = struct_ctex_texture_export_preset_descriptor
ctex_texture_export_options_descriptor = struct_ctex_texture_export_options_descriptor
ctex_texture_export_named_value = struct_ctex_texture_export_named_value
ctex_texture_export_sample = struct_ctex_texture_export_sample
ctex_texture_export_pixel_source_descriptor = struct_ctex_texture_export_pixel_source_descriptor
ctex_texture_export_layer_selection_view = struct_ctex_texture_export_layer_selection_view
ctex_texture_export_planned_output = struct_ctex_texture_export_planned_output
ctex_texture_export_encoded_output = struct_ctex_texture_export_encoded_output
ctex_texture_export_callbacks_descriptor = struct_ctex_texture_export_callbacks_descriptor
ctex_texture_export_info = struct_ctex_texture_export_info
ctex_project_container_read_limits_descriptor = struct_ctex_project_container_read_limits_descriptor
ctex_project_container_version = struct_ctex_project_container_version
ctex_project_container_info = struct_ctex_project_container_info
ctex_project_autosave_config_descriptor = struct_ctex_project_autosave_config_descriptor
ctex_project_autosave_info = struct_ctex_project_autosave_info
ctex_project_quiesce_descriptor = struct_ctex_project_quiesce_descriptor
ctex_project_quiesce_report = struct_ctex_project_quiesce_report
ctex_project_recovery_checkpoint_info = struct_ctex_project_recovery_checkpoint_info
ctex_project_recovery_entry = struct_ctex_project_recovery_entry
ctex_project_recovery_rejection = struct_ctex_project_recovery_rejection
ctex_project_recovery_enumeration_info = struct_ctex_project_recovery_enumeration_info
ctex_project_asset_export_options_descriptor = struct_ctex_project_asset_export_options_descriptor
ctex_project_asset_search_paths_descriptor = struct_ctex_project_asset_search_paths_descriptor
ctex_project_resource_descriptor = struct_ctex_project_resource_descriptor
ctex_operation_channel_descriptor = struct_ctex_operation_channel_descriptor
ctex_pinned_operation_resource_descriptor = struct_ctex_pinned_operation_resource_descriptor
ctex_operation_record_descriptor = struct_ctex_operation_record_descriptor
ctex_operation_record_info = struct_ctex_operation_record_info
ctex_operation_algorithm_support_descriptor = struct_ctex_operation_algorithm_support_descriptor
ctex_operation_replay_assessment_descriptor = struct_ctex_operation_replay_assessment_descriptor
ctex_operation_replay_info = struct_ctex_operation_replay_info
ctex_project_operation_replay_info = struct_ctex_project_operation_replay_info
ctex_resolution_operation_record_descriptor = struct_ctex_resolution_operation_record_descriptor
ctex_resolution_replay_raster_descriptor = struct_ctex_resolution_replay_raster_descriptor
ctex_texture_set_resolution_change_descriptor = struct_ctex_texture_set_resolution_change_descriptor
ctex_texture_set_resolution_change_info = struct_ctex_texture_set_resolution_change_info
ctex_texture_set_resolution_restore_info = struct_ctex_texture_set_resolution_restore_info
ctex_editable_placement_frame = struct_ctex_editable_placement_frame
ctex_editable_material_parameter_descriptor = struct_ctex_editable_material_parameter_descriptor
ctex_editable_tile_dependency_descriptor = struct_ctex_editable_tile_dependency_descriptor
ctex_editable_surface_point_descriptor = struct_ctex_editable_surface_point_descriptor
ctex_editable_entry_descriptor = struct_ctex_editable_entry_descriptor
ctex_editable_entry_info = struct_ctex_editable_entry_info
ctex_preset_shelf_entry_descriptor = struct_ctex_preset_shelf_entry_descriptor
ctex_preset_shelf_descriptor = struct_ctex_preset_shelf_descriptor
ctex_preset_library_descriptor = struct_ctex_preset_library_descriptor
ctex_preset_library_info = struct_ctex_preset_library_info
ctex_material_graph_catalogue_info = struct_ctex_material_graph_catalogue_info
ctex_material_graph_info = struct_ctex_material_graph_info
ctex_material_graph_link_descriptor = struct_ctex_material_graph_link_descriptor
ctex_material_graph_link_info = struct_ctex_material_graph_link_info
ctex_material_graph_validation_resources_descriptor = struct_ctex_material_graph_validation_resources_descriptor
ctex_material_graph_validation_info = struct_ctex_material_graph_validation_info
ctex_material_graph_library_info = struct_ctex_material_graph_library_info
ctex_material_graph_preset_descriptor = struct_ctex_material_graph_preset_descriptor
ctex_smart_material_value_descriptor = struct_ctex_smart_material_value_descriptor
ctex_material_graph_socket_descriptor = struct_ctex_material_graph_socket_descriptor
ctex_material_graph_group_descriptor = struct_ctex_material_graph_group_descriptor
ctex_material_graph_group_interface_descriptor = struct_ctex_material_graph_group_interface_descriptor
ctex_material_graph_workspace_info = struct_ctex_material_graph_workspace_info
ctex_material_graph_group_update_info = struct_ctex_material_graph_group_update_info
ctex_shader_texture_descriptor = struct_ctex_shader_texture_descriptor
ctex_shader_material_resource_descriptor = struct_ctex_shader_material_resource_descriptor
ctex_shader_device_features_descriptor = struct_ctex_shader_device_features_descriptor
ctex_shader_material_request = struct_ctex_shader_material_request
ctex_shader_material_info = struct_ctex_shader_material_info
ctex_shader_material_source_descriptor = struct_ctex_shader_material_source_descriptor
ctex_shader_material_debug_info = struct_ctex_shader_material_debug_info
ctex_shader_backend_attribution_info = struct_ctex_shader_backend_attribution_info
ctex_shader_layer_descriptor = struct_ctex_shader_layer_descriptor
ctex_shader_layer_stack_request = struct_ctex_shader_layer_stack_request
ctex_shader_layer_stack_info = struct_ctex_shader_layer_stack_info
ctex_shader_emission_cache_info = struct_ctex_shader_emission_cache_info
ctex_shader_preview_channel_descriptor = struct_ctex_shader_preview_channel_descriptor
ctex_shader_preview_environment_descriptor = struct_ctex_shader_preview_environment_descriptor
ctex_shader_preview_request = struct_ctex_shader_preview_request
ctex_shader_preview_info = struct_ctex_shader_preview_info
ctex_material_graph_property_descriptor = struct_ctex_material_graph_property_descriptor
ctex_material_graph_parity_fixture_descriptor = struct_ctex_material_graph_parity_fixture_descriptor
ctex_material_graph_host_property_value = struct_ctex_material_graph_host_property_value
ctex_material_graph_host_evaluation_request = struct_ctex_material_graph_host_evaluation_request
ctex_material_graph_host_emission_request = struct_ctex_material_graph_host_emission_request
ctex_material_graph_host_emission_result = struct_ctex_material_graph_host_emission_result
ctex_material_graph_host_node_registration_descriptor = struct_ctex_material_graph_host_node_registration_descriptor
ctex_material_graph_node_registry_info = struct_ctex_material_graph_node_registry_info
ctex_material_graph_host_contract_info = struct_ctex_material_graph_host_contract_info
ctex_smart_material_info = struct_ctex_smart_material_info
ctex_preset_application_info = struct_ctex_preset_application_info
ctex_preset_undo_info = struct_ctex_preset_undo_info
ctex_rgb_color = struct_ctex_rgb_color
ctex_channel_color_policy = struct_ctex_channel_color_policy
ctex_resolved_input_color_space = struct_ctex_resolved_input_color_space
ctex_bit_depth_warning = struct_ctex_bit_depth_warning
ctex_version = struct_ctex_version
# No inserted files

# No prefix-stripping

