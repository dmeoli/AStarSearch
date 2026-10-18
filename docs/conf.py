# --------------------------------------------------------------------------- #
#    conf.py                                                                  #
#                                                                             #
#    The configuration of Sphinx for the documentation of AStarSearch: the    #
#    pages that docs/pages.py writes from the README, the design note and     #
#    the CHANGELOG (Markdown, by MyST), and the API reference, which Exhale   #
#    builds from the XML that Doxygen extracts from the headers and Breathe   #
#    renders; the theme is that of Read the Docs. The workflow                #
#    .github/workflows/docs.yml builds it with                                #
#                                                                             #
#        python3 docs/pages.py && sphinx-build -b html docs build-docs/html   #
#                                                                             #
#                                Donato Meoli                                 #
#                         Dipartimento di Informatica                         #
#                             Universita' di Pisa                             #
# --------------------------------------------------------------------------- #

import os
import subprocess

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

project = 'AStarSearch'
author = 'Donato Meoli'
copyright = '2026, Donato Meoli'
# the version is the last git tag, as for CMake [see cmake/DeriveVersion.cmake]
try:
    release = subprocess.run(['git', 'describe', '--tags', '--abbrev=0'],
                             cwd=ROOT, capture_output=True, text=True,
                             check=True).stdout.strip().lstrip('v')
except (OSError, subprocess.CalledProcessError):
    release = 'unreleased'
version = release

extensions = [
    'breathe',              # the Doxygen XML in Sphinx
    'exhale',               # a page per class, function and file
    'myst_parser',          # the pages in Markdown
    'sphinx.ext.mathjax',   # the formulae of the comments
]

source_suffix = {'.rst': 'restructuredtext', '.md': 'markdown'}
myst_enable_extensions = ['dollarmath', 'colon_fence']
myst_heading_anchors = 3
# the design note is read through its copy in generated/
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store', 'branch-and-bound.md']

breathe_projects = {'AStarSearch': os.path.join(ROOT, 'build-docs', 'xml')}
breathe_default_project = 'AStarSearch'
primary_domain = 'cpp'
highlight_language = 'cpp'

exhale_args = {
    'containmentFolder': './api',
    'rootFileName': 'library_root.rst',
    'doxygenStripFromPath': os.path.join(ROOT, 'include'),
    'rootFileTitle': 'API Reference',
    'createTreeView': True,
    'exhaleExecutesDoxygen': True,
    'exhaleDoxygenStdin': '\n'.join([
        'INPUT = "' + os.path.join(ROOT, 'include', 'AStarSearch') + '"',
        'FILE_PATTERNS = *.h',
        'RECURSIVE = YES',
        'EXCLUDE_SYMBOLS = detail::* AStar_di_unipi_it::detail::*',
        'EXTRACT_ALL = YES',
        'JAVADOC_AUTOBRIEF = YES',
        'BUILTIN_STL_SUPPORT = YES',
        'GENERATE_HTML = NO',
        'GENERATE_LATEX = NO',
        'QUIET = YES',
        'WARNINGS = NO',
    ]),
}

html_theme = 'sphinx_rtd_theme'
html_title = 'Sequential and Parallel A* Search'
html_theme_options = {'navigation_depth': 3, 'collapse_navigation': False}
html_context = {
    'display_github': True,
    'github_user': 'dmeoli',
    'github_repo': 'AStarSearch',
    'github_version': 'main',
    'conf_py_path': '/docs/',
}

# --------------------------------------------------------------------------- #
#                              End of conf.py                                 #
# --------------------------------------------------------------------------- #
