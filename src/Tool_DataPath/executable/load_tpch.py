#!/usr/bin/env python2.7
#
#  Copyright 2012 Christopher Dudley
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#

import os
from os.path import realpath
import argparse
import shutil
import sys
import subprocess
import glob
import tempfile

"""A script to load the TPC-H dataset into the database."""

# Set up the argument parser
parser = argparse.ArgumentParser(
    description='Load the TPC-H dataset into the database.')

parser.add_argument('-d',
                    '--data',
                    action='store',
                    default='./data',
                    metavar='path',
                    help='the directory in which to store the table data. [./data]')

parser.add_argument('-g', '--gen-version',
                    action='store',
                    default='2.14.3',
                    metavar='x.x.x',
                    help='the version of the TPC-H generator to use. [2.14.3]')

parser.add_argument('-f', '--scale-factor',
        action='store',
        default=1,
        type=int,
        metavar='scale',
        help='the scale factor to use for the TPC-H data. [1]')

parser.add_argument('-k', '--keep-files',
        action='store_true',
        default=False,
        help='do not delete temporary files.')

parser.add_argument('-r', '--reload',
        action='store_true',
        default=False,
        help='redownload the tpch data generator and regenerate the data, even if it already exists.')

parser.add_argument('-s', '--schema',
        action='store',
        default='./LOAD_TPCH/schema.pgy',
        metavar='path',
        help='the query file that creates the schema. [./LOAD_TPCH/schema.pgy]')

parser.add_argument('-n', '--num-stripes',
        action='store',
        default=1,
        type=int,
        metavar='stripes',
        help='the number of stripes in which to generate the TPC-H data. [1]')

parser.add_argument('-x', '--script',
        action='store',
        default=None,
        metavar='file',
        help='a script to execute inbetween TPCH data generation and data loading. [None]')

parser.add_argument('-P', '--postscript',
        action='store',
        default='',
        help='postscript to add to the end of table names.')

init_group = parser.add_argument_group('Initialization Options', 'Options used to initialize Grokit on first run.')

init_group.add_argument('-i', '--initialize',
        action='store_true',
        default=False,
        help='initialize Grokit (i.e. answer the questions it asked upon first run).')

init_group.add_argument('-p', '--disk-pattern',
        action='store',
        default='./disks/disks%d',
        help='the pattern to use for the disks when initializing Grokit. [./disks/disk%%d]')

init_group.add_argument('-e', '--exponent',
        action='store',
        default=0,
        type=int,
        help='the page multiplier exponent to use when initializing Grokit. [0]')

init_group.add_argument('-m', '--num-disks',
        action='store',
        type=int,
        default=1,
        help='the number of disks to specify when initializing Grokit. [1]')

parser.add_argument('--ignore-dp',
        action='store_true',
        default=False,
        help='ignore errors caused by non-zero return codes from the Grokit executable.')

# Parse the arguments
args = parser.parse_args()

# Set up constants we'll use
dataDir = realpath(args.data)
dpExec = "grokit"

tpchVersion = args.gen_version.replace('.', '_')
tpchDir = os.path.join( dataDir, 'tpch_' + tpchVersion)
tpchArchiveName = 'tpch_' + tpchVersion + '.tgz'
tpchFile = os.path.join( dataDir, tpchArchiveName )
tpchURL = 'http://www.tpc.org/tpch/spec/'
tpchArchiveWeb = tpchURL + tpchArchiveName

dbgenDir = os.path.join( tpchDir, 'dbgen' )

scaleFactor = str(args.scale_factor)
numStripes = str(args.num_stripes)

sedCommand = "s/CC[[:space:]]*=.*$/CC = gcc/ ; " \
        "s/DATABASE[[:space:]]*=.*$/DATABASE = SQLSERVER/ ; " \
        "s/MACHINE[[:space:]]*=.*$/MACHINE = LINUX/ ; " \
        "s/WORKLOAD[[:space:]]*=.*$/WORKLOAD = TPCH/"

queryFiles = { 'tpch.pgy', 'bulkload_lineitem.pgy', 'bulkload_customer.pgy', 'bulkload_region.pgy', 'bulkload_nation.pgy',
        'bulkload_orders.pgy', 'bulkload_part.pgy', 'bulkload_partsupp.pgy', 'bulkload_supplier.pgy'}

tables = { 'region', 'nation', 'lineitem', 'orders', 'customer', 'part', 'partsupp', 'supplier' }

# Tables that have only a single .tbl file, regardless of striping
tablesSingle = { 'region', 'nation' }

# Flags for the -T option for dbgen
tableFlag = { 'region' : 'r', 'nation' : 'n', 'customer' : 'c', 'supplier' : 's',
             'lineitem' : 'L', 'orders' : 'O', 'part' : 'P', 'partsupp' : 'S'}

# Used when asking the user a yes or no question
yes = {'Y', 'y', 'yes', 'YES', 'Yes'}
no = {'N', 'n', 'no', 'NO', 'No'}

# Template for generated query files
bulkload_template = """
USING base;
data = READ FILE "{file}" {striping}
    USING GI: base\CSVReader<"sep"="|">
    ATTRIBUTES FROM {relation}{postfix};

STORE data INTO {relation}{postfix};
"""

def cleanup(force = False):
    """Clean up directories used."""

    if not force and args.keep_files:
        return

    # Test for existence of tpch directory
    if os.path.exists( tpchDir ):
        shutil.rmtree( tpchDir )

    # Test for existence of tpch archive
    if os.path.exists( tpchFile ):
        os.remove( tpchFile )

    return

def getArchive():
    """Download the TPC-H data generation tools archive from the TPC website."""

    try:
        subprocess.check_call(['wget', tpchArchiveWeb], cwd=dataDir)
    except:
        print "Error: Failed to download TPC-H generation tools archive."
        sys.exit(2)

    return

def buildGenerator():
    """Build the TPC-H generator, first downloading and extracting the generator if needed."""

    if not os.path.exists(tpchDir):
        print 'No existing TPC-H generator directory found, extracting from archive.'

        # Need to extract the archive
        os.mkdir( tpchDir )

        # Check to see if we already have a local copy of the archive
        if not os.path.exists(tpchFile):
            print 'No existing TPC-H generator archive found, downloading from tpc.org'
            # Download the archive
            getArchive()

        try:
            subprocess.check_call(['tar', '-xf', tpchFile, '-C', tpchDir], cwd=dataDir)
        except:
            print "Error: unable to extract TPC-H generator archive."
            shutil.rmtree( tpchDir )
            sys.exit(3)

    # We should have the generator extracted into tpchDir at this point
    # Let's set the configuration file properly
    print 'Configuring TPC-H generator makefile.'

    configFile = open( os.path.join( dbgenDir, 'makefile'), 'w')
    sourceConfigFile = os.path.join( dbgenDir, 'makefile.suite')

    try:
        subprocess.check_call(['sed', '-e', sedCommand, sourceConfigFile], stdout=configFile)
    except:
        print "Error: Failed to configure TPC-H generator makefile"
        sys.exit(4)

    # At this point, we should be ready to make the generator.
    print 'Compiling TPC-H generator.'


    try:
        subprocess.check_call('make', cwd=dbgenDir)
    except:
        print "Error: Failed to make TPC-H generator."
        sys.exit(5)

    # Build successful!
    return

def generatePipes( relation ):
    if relation in tablesSingle or args.num_stripes == 1:
        os.mkfifo( os.path.join(tpchDir, "{0}.tbl".format(relation)) )
        if args.script != None:
            os.mkfifo( os.path.join(tpchDir, "{0}.tbl.script".format(relation)) )
    else:
        for i in range(1, args.num_stripes + 1):
            os.mkfifo( os.path.join(tpchDir, "{0}.tbl.{1}".format(relation, i)) )
            if args.script != None:
                os.mkfifo( os.path.join(tpchDir, "{0}.tbl.{1}.script".format(relation, i)) )

    return

def generateQuery( relation ):
    tableFile = "{0}.tbl".format(relation)
    striping = ""
    if args.num_stripes > 1 and not relation in tablesSingle:
        striping = ": {0}".format(args.num_stripes)
        tableFile += ".%d"

    if args.script != None:
        tableFile += ".script"

    tableFile = os.path.join(tpchDir, tableFile)

    queryFileName = os.path.join(dataDir, 'bulkload_{0}.pgy'.format(relation))
    queryFile = open(queryFileName, 'w')

    queryFile.write( bulkload_template.format(relation=relation, file=tableFile, striping=striping, postfix=args.postscript))
    queryFile.close()

    return queryFileName

def handleError( ):
    if not args.ignore_dp:
        print 'Warning: an error occurred while loading the data. It\'s possible that'
        print 'Grokit exited with a non-zero exit code because it does not yet quit cleanly.'
        print 'If this is the case, it is safe to continue.'
        print

        s = raw_input('Continue? (Y/N) --> ')

        while s not in yes and s not in no:
            print "Please answer yes or no."
            s = raw_input('Continue? (Y/N) --> ')

        if s in no:
            print "Exiting."
            sys.exit(9)

    return

def loadTable( relation ):
    dbgenExec = os.path.join( dbgenDir, 'dbgen' )
    distsFile = os.path.join( dbgenDir, 'dists.dss')

    if not os.path.exists( dbgenExec ):
        print 'No existing dbgen exectuable found, building it now.'
        buildGenerator()

    # Remove any previous tables.
    for f in glob.glob( os.path.join( tpchDir, '{0}.tbl*'.format(relation) ) ):
        os.remove(f)

    generatePipes( relation )

    tpchProcs = list()

    # Start the TPCH DBGEN processes
    if relation in tablesSingle or args.num_stripes == 1:
        proc = subprocess.Popen([dbgenExec, '-qf', '-s', scaleFactor, '-b', distsFile, '-T', tableFlag[relation]], cwd=tpchDir)
        tpchProcs.append(proc)
    else:
        for i in xrange(1, args.num_stripes + 1):
            proc = subprocess.Popen([dbgenExec, '-qf', '-s', scaleFactor, '-b', distsFile,
                '-C', numStripes, '-S', str(i), '-T', tableFlag[relation]], cwd=tpchDir)
            tpchProcs.append(proc)

    # Start up scripts if necessary
    scriptProcs = list()
    if args.script != None:
        scriptFileName = os.path.realpath(args.script)

        if relation in tablesSingle or args.num_stripes == 1:
            pipeInName = os.path.join(tpchDir, "{0}.tbl".format(relation))

            pipeOutName = os.path.join(tpchDir, "{0}.tbl.script".format(relation))

            proc = subprocess.Popen("{0} < {1} > {2}".format(scriptFileName, pipeInName, pipeOutName), shell=True)
            scriptProcs.append(proc)
        else:
            for i in xrange(1, args.num_stripes + 1):
                pipeInName = os.path.join(tpchDir, "{0}.tbl.{1}".format(relation, i))

                pipeOutName = os.path.join(tpchDir, "{0}.tbl.{1}.script".format(relation, i))

                proc = subprocess.Popen("{0} < {1} > {2}".format(scriptFileName, pipeInName, pipeOutName), shell=True)
                scriptProcs.append(proc)

    # Start up DP
    queryFileName = generateQuery( relation )

    try:
        subprocess.check_call([dpExec, '-w', 'run', queryFileName])
    except:
        # Kill any running processes
        for p in tpchProcs:
            if p.poll() == None:
                p.terminate()
        for p in scriptProcs:
            if p.poll() == None:
                p.terminate()
        handleError()

    # Cleanup the pipes
    for f in glob.glob( os.path.join( tpchDir, '{0}.tbl*'.format(relation) ) ):
        os.remove(f)

    return

def getDPInit():
    """If necessary, create a temporary file containing initialization information for Grokit."""

    if args.initialize:
        tmpFile = tempfile.TemporaryFile();
        tmpFile.write(str(args.exponent) + '\n')
        tmpFile.write(str(args.num_disks) + '\n')
        tmpFile.write(args.disk_pattern + '\n')

        tmpFile.flush()
        tmpFile.seek(0)

        return tmpFile
    else:
        return sys.stdin

def loadData():
    """Load the generated TPC-H data into the database."""

    if not os.path.exists( dataDir ):
        os.mkdir( dataDir )
    elif not os.path.isdir( dataDir ):
        print 'Error: specified data directory is not actually a directory!'
        sys.exit(7)

    schemaFile = os.path.realpath(args.schema)
    if not os.path.exists( schemaFile ):
        print 'Error: specified schema query file does not exist!'
        sys.exit(8)

    with open(schemaFile, 'r') as schemaText:
        schemaFileContents = schemaText.read()

    formattedSchemaFile = os.path.join( dataDir, 'schema.pgy' )
    with open(formattedSchemaFile, 'w') as outFile:
        outFile.write(schemaFileContents.format(postfix = args.postscript))

    print "NOTICE: Grokit currently does not exit cleanly when running a single script."
    print "In order to work around this, Grokit will simply sit idle and do nothing"
    print "once it has finished executing its query. At this point, you will need to"
    print "close Grokit by sending an interrupt (CTRL+C). If everything has gone"
    print "well, it will have already saved all the information that it needs to."
    print

    s = raw_input('Continue? (Y/N) --> ')

    while s not in yes and s not in no:
        print "Please answer yes or no."
        s = raw_input('Continue? (Y/N) --> ')

    if s in no:
        print "Exiting."
        sys.exit(8)

    print "Loading schema into database."
    inFile = getDPInit()

    try:
        subprocess.check_call([dpExec, '-w', 'run', formattedSchemaFile], stdin=inFile)
    except subprocess.CalledProcessError:
        if not args.ignore_dp:
            print 'Warning: an error occurred while loading the schema. It\'s possible that'
            print 'Grokit exited with a non-zero exit code because it does not yet quit cleanly.'
            print 'If this is the case, it is safe to continue.'
            print

            s = raw_input('Continue? (Y/N) --> ')

            while s not in yes and s not in no:
                print "Please answer yes or no."
                s = raw_input('Continue? (Y/N) --> ')

            if s in no:
                print "Exiting."
                sys.exit(8)

    if inFile != None:
        inFile.close()

    print "Loading data into database."

    for relation in tables:
        loadTable( relation )

    return

if args.reload:
    cleanup()

loadData()

cleanup()
