#!/usr/bin/env python
import json
import sys

data = json.load(open(sys.argv[1]))

for key in sys.argv[2:]:
    data = data[key]

print(data)
