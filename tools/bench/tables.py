#!/usr/bin/env python3
###############################################################################
#                                                                             #
#   tables.py: the Markdown tables of the README from the CSV of campaign.sh  #
#                                                                             #
#   Usage:  tables.py <out of campaign.sh>                                    #
#                                                                             #
#   For each table it sums, over the instances, the median of the             #
#   repetitions of a run, and prints the time of every search on every        #
#   number of cores, the speedup over sequential A* and the A/A noise, i.e.,  #
#   the median and the largest relative gap between two repetitions of the    #
#   same run. A cell is empty when a search did not solve all the instances.  #
#                                                                             #
#                                Donato Meoli                                 #
#                         Dipartimento di Informatica                         #
#                             Universita' di Pisa                             #
#                                                                             #
###############################################################################

import collections
import csv
import os
import statistics
import sys


def load(path):
    """the rows of a CSV, or [] if it is not there"""
    if not os.path.exists(path):
        return []
    with open(path) as f:
        return list(csv.DictReader(f))


def name(r):
    """the name of a search in the tables"""
    a = r['algo']
    if a == 'hda':
        return 'HDA*[%s] %s' % (r['dist'], r['runtime'])
    return {'astar': 'A* ' + r['open'], 'spa': 'SPA*', 'pbnf': 'Safe PBNF',
            'farm': 'farm (FastFlow)', 'pase': 'PA*SE',
            'burns': 'A* of Burns et al.'}.get(a, a)


def summarize(rows):
    """{ ( search , cores ) : ( total seconds , instances ) } and the gaps"""
    runs = collections.defaultdict(list)
    for r in rows:
        if r['status'] != 'solved':
            continue
        runs[(name(r), int(r['cores']), r['instance'])].append(
            float(r['seconds']))
    tot = collections.defaultdict(float)
    cnt = collections.Counter()
    gaps = []
    for (s, c, i), v in runs.items():
        tot[(s, c)] += statistics.median(v)
        cnt[(s, c)] += 1
        if len(v) >= 2 and max(v) > 0:
            gaps.append((max(v) - min(v)) / max(v))
    return tot, cnt, gaps


def scaling(title, rows, seq):
    """the table of the times of the searches on every number of cores"""
    tot, cnt, gaps = summarize(rows)
    inst = max(cnt.values()) if cnt else 0
    cores = sorted({c for (_, c) in tot})
    searches = sorted({s for (s, _) in tot}, key=lambda s: (s != seq, s))
    base = tot.get((seq, 1))
    print('**%s** (%d instances; seconds, and speedup over %s on 1 core)\n'
          % (title, inst, seq))
    print('| search | ' + ' | '.join(str(c) for c in cores) + ' |')
    print('|---|' + '---|' * len(cores))
    for s in searches:
        cells = []
        for c in cores:
            if cnt.get((s, c)) != inst:
                cells.append('')
            elif base:
                cells.append('%.2f (%.1fx)' % (tot[(s, c)],
                                               base / tot[(s, c)]))
            else:
                cells.append('%.2f' % tot[(s, c)])
        print('| %s | %s |' % (s, ' | '.join(cells)))
    if gaps:
        print('\nA/A noise: median %.1f%%, largest %.1f%% over %d pairs.\n'
              % (100 * statistics.median(gaps), 100 * max(gaps), len(gaps)))


def table_a(rows):
    """sequential A* on the 100 instances of Korf"""
    by = collections.defaultdict(list)
    for r in rows:
        by[name(r)].append(r)
    print('| search | solved | seconds | expanded | expansions/s |')
    print('|---|---|---|---|---|')
    for s, v in sorted(by.items()):
        ok = [r for r in v if r['status'] == 'solved']
        sec = sum(float(r['seconds']) for r in ok)
        exp = sum(int(r['expanded']) for r in ok)
        print('| %s | %d | %.1f | %.3g | %.2f M |'
              % (s, len(ok), sec, exp, exp / sec / 1e6 if sec else 0))
    print()


def table_e(rows):
    """the examples, the median of their runs"""
    by = collections.defaultdict(list)
    order = []
    for r in rows:
        k = (r['example'], r['arguments'])
        if k not in by:
            order.append(k)
        by[k].append(r)
    print('| example | arguments | cost | expanded | milliseconds |')
    print('|---|---|---|---|---|')
    for k in order:
        v = [r for r in by[k] if r['seconds']]
        if not v:
            continue
        ms = statistics.median(float(r['seconds']) for r in v) * 1000
        print('| %s | %s | %s | %s | %s |' % (k[0], k[1] or '-',
              v[0]['cost'] or '-', v[0]['expanded'],
              '%.0f' % ms if ms >= 100 else '%.3g' % ms))
    print()


def main(out):
    a = load(os.path.join(out, 'A.csv'))
    if a:
        print('### Sequential A* on the 100 instances of Korf\n')
        table_a(a)
    b = load(os.path.join(out, 'B.csv'))
    if b:
        print('### Parallel searches on the 15-puzzle\n')
        scaling('15-puzzle', b, 'A* buckets')
    c = load(os.path.join(out, 'C.csv'))
    if c:
        print('### Parallel searches on grids\n')
        for d in ('grid4', 'grid8'):
            scaling(d, [r for r in c if r['domain'] == d], 'A* buckets')
    d = load(os.path.join(out, 'D.csv'))
    if d:
        print('### Slow expansions\n')
        for w in sorted({r['slow'] for r in d}, key=int):
            scaling('work %s per expansion' % w,
                    [r for r in d if r['slow'] == w], 'A* buckets')
    e = load(os.path.join(out, 'E.csv'))
    if e:
        print('### The examples\n')
        table_e(e)


if __name__ == '__main__':
    main(sys.argv[1])

###############################################################################
############################## End of tables.py ###############################
###############################################################################
