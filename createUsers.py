#!/usr/bin/python
from subprocess import call
from crypt import crypt
import sys

# password to set
defaultpass = "blahblah"

if len(sys.argv) < 2:
    print "Please provide file with usernames"
    exit()
else:
    file = open(sys.argv[1])

for line in file:
    for uname in line.split(' ', 1):
        pw = crypt(defaultpass, "password")
        print("Creating user: " + uname.strip())
        if call("sudo useradd -m -p " + pw + " " + uname.strip(), shell=True) == 0:
            print("Success!")
        else:
            print("Failed!")
        print("Adding " + uname.strip() + " to i2c group")
        if call("sudo usermod -aG i2c " + uname.strip(), shell=True) == 0:
            print("Success!")
        else:
            print("Failed!")
        print("Adding " + uname.strip() + " to video group")
        if call("sudo usermod -aG video " + uname.strip(), shell=True) == 0:
            print("Success!")
        else:
            print("Failed!")