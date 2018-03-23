"""
Cython wrapper around the C functionality.

Author: G.J.J. van den Burg

"""

import cython

from cpython.version cimport PY_MAJOR_VERSION


cdef extern void c_fast_file_count(char *root, long l_recursive, 
        long l_hidden, long l_quiet, long *result_files, long *result_dirs)


def fast_file_count(root, recursive, hidden, quiet):
    """
    """

    cdef long i_recursive = recursive
    cdef long i_hidden = hidden
    cdef long i_quiet = quiet

    cdef long res_files
    cdef long res_dirs

    c_fast_file_count(root, i_recursive, i_hidden, i_quiet, &res_files, 
            &res_dirs)

    return res_files, res_dirs
