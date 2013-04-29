#!/usr/bin/python
from subprocess import call
import sys

if len(sys.argv) < 2:
    print "Please provide file with usernames"
    exit()
else:
    file = open(sys.argv[1])

for line in file:
    for uname in line.split(' ', 1):
        print("Deleting user: " + uname.strip())
        if call("sudo userdel -r " + uname.strip(), shell=True) == 0:
            print("Success!")
        else:
            print("Failed!")
        