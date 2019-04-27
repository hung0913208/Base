#!/bin/bash

ROOT="$(git rev-parse --show-toplevel)"

SCRIPT="$(basename "$0")"
PROJECT="$(basename `git config  --get remote.origin.url`)"

