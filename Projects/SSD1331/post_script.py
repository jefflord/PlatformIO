import os
Import("env")

def after_build(source, target, env):
    # Custom command or action
    print("Build completed!")
    # Example: Run a shell command
    os.system("echo Custom command after build")

# Hook the after_build function to the 'post' build event
env.AddPostAction("buildprog", after_build)