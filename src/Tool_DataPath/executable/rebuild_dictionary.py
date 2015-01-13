#!/usr/bin/python

import sqlite3
import argparse
import sys

parser = argparse.ArgumentParser(
	description='Rebuilds the dictionary for a FACTOR'
)

parser.add_argument(
	'dictionary',
	help='The name of the dictionary'
)

parser.add_argument(
	'--database', '-d',
	help='The file containing the SQLite3 metadata database',
	default='./datapath.sqlite'
)

args = parser.parse_args()

conn = sqlite3.connect(args.database)
cursor = conn.cursor()

table_name = 'Dictionary_' + args.dictionary

# Test if table exists
cursor.execute('''
SELECT name FROM sqlite_master WHERE type='table' AND name=?
''', (table_name,))

ret = cursor.fetchall()

if len(ret) == 0:
	sys.stderr.write('Table for dictionary {0} does not exist\n'.format(args.database))
	sys.exit(1)

cursor.execute('SELECT "str", "id" FROM {0}'.format(table_name))

values = set()

for row in cursor:
	values.add(row)

v_list = list(values)
v_list.sort()

to_insert = [(fid, order, value) for order, (value, fid) in enumerate(v_list)]

cursor.execute('DELETE FROM {0}'.format(table_name))

cursor.executemany('INSERT INTO {0}("id", "order", "str") VALUES (?,?,?)'.format(table_name), to_insert)

conn.commit()