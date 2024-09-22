import os

# Get the project's environment variables
env = DefaultEnvironment()

# Run a shell command before compilation

print       (     "!!!!!!!!!!!!!!!!!!!!!!!!!! PRE1 !!!!!!!!!!!!!!!!!!!!!!!!!")    
os.system   ("git add .")
os.system   ("git commit -m auto-build")
os.system   ("echo !!!!!!!!!!!!!!!!!!!!!!!!!! PRE2 !!!!!!!!!!!!!!!!!!!!!!!!!")

