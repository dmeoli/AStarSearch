#!/usr/bin/env python3
# --------------------------------------------------------------------------- #
#    pages.py                                                                 #
#                                                                             #
#    Writes the pages of the site of the documentation into                   #
#    build-docs/pages, from the README, which stays the one source of the     #
#    text: each section of the README becomes a page, the citations [n] of    #
#    the README point to the page of the references, the links to the files   #
#    of the repository point to GitHub, the design note of the                #
#    branch-and-bound becomes a page of the user guide, and the CHANGELOG     #
#    becomes the release notes. The workflow .github/workflows/docs.yml runs  #
#    it before mkdocs.                                                        #
#                                                                             #
#        python3 docs/pages.py && mkdocs build                                #
#                                                                             #
#                                Donato Meoli                                 #
#                         Dipartimento di Informatica                         #
#                             Universita' di Pisa                             #
# --------------------------------------------------------------------------- #

import os
import re

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, 'build-docs', 'pages')
BLOB = 'https://github.com/dmeoli/AStarSearch/blob/main/'
TREE = 'https://github.com/dmeoli/AStarSearch/tree/main/'

# the sections of the README and the page each one goes to; the sections
# not listed here go to the home page
PAGES = {
    'The search problem': 'user-guide/search-problem.md',
    'The algorithms': 'user-guide/algorithms.md',
    'Branch-and-bound': 'user-guide/branch-and-bound.md',
    'The data structures': 'user-guide/data-structures.md',
    'Building and using the library': 'getting-started/installation.md',
    'The examples and the benchmark': 'user-guide/examples.md',
    'Testing': 'user-guide/testing.md',
    'References': 'user-guide/references.md',
}
REFERENCES = 'user-guide/references.md'


def links(text, page):
    """the links of the README for the page: the citations to the page of
    the references, the files of the repository on GitHub"""
    up = '../' * page.count('/')

    def fix(m):
        target = m.group(2)
        if target.startswith('#ref-'):
            if page == REFERENCES:
                return m.group(0)
            return '%s(%s%s%s)' % (m.group(1), up, REFERENCES, target)
        if re.match(r'[a-z]+:', target) or target.startswith('#'):
            return m.group(0)
        if target == 'docs/branch-and-bound.md':
            return '%s(%suser-guide/branch-and-bound-design.md)' % (
                m.group(1), up)
        base = TREE if target.endswith('/') else BLOB
        return '%s(%s%s)' % (m.group(1), base, target)
    return re.sub(r'(\]|\]\s)\(([^)]+)\)', fix, text)


def write(path, text):
    full = os.path.join(OUT, path)
    os.makedirs(os.path.dirname(full), exist_ok=True)
    with open(full, 'w') as f:
        f.write(text)


def main():
    readme = open(os.path.join(ROOT, 'README.md')).read()
    parts = re.split(r'^## ', readme, flags=re.M)
    home, pages = [parts[0]], {}
    for part in parts[1:]:
        title, _, body = part.partition('\n')
        page = PAGES.get(title.strip())
        text = '# %s\n%s' % (title.strip(), body)
        if page:
            pages[page] = pages.get(page, '') + text
        else:
            home.append('## ' + part)
    write('index.md', links(''.join(home), 'index.md'))
    for page, text in pages.items():
        write(page, links(text, page))
    note = 'user-guide/branch-and-bound-design.md'
    write(note, links(open(os.path.join(ROOT, 'docs',
                                        'branch-and-bound.md')).read(), note))
    log = open(os.path.join(ROOT, 'CHANGELOG.md')).read()
    write('release_notes.md', links(log.replace('# Changelog',
                                                '# Release Notes', 1),
                                    'release_notes.md'))


if __name__ == '__main__':
    main()

# --------------------------------------------------------------------------- #
#                              End of pages.py                                #
# --------------------------------------------------------------------------- #
