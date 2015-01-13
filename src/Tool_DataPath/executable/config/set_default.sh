#!/bin/bash

PROF=$1

rm default.json default_no-opt.json

ln -s ${PROF}.json default.json
ln -s ${PROF}_no-opt.json default_no-opt.json
