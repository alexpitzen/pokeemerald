#!/bin/bash

python3 parsediffs.py <(git diff master HEAD -- src/data/pokemon/evolution.h) <(git diff master HEAD -- src/data/pokemon/level_up_learnsets.h) | vim
