Import("env")

def before_upload(source, target, env):
    print("Uploading LittleFS first...")
    env.Execute("pio run -t uploadfs")

env.AddPreAction("upload", before_upload)
