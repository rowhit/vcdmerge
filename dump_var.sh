#!/bin/bash

set -e

grep \\\(\ $1$\\\)\\\|\\\(^\#\\\) $2 |less
