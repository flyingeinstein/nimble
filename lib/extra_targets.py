try:
    import configparser
except ImportError:
    import ConfigParser as configparser

Import("env")

config = configparser.ConfigParser()
config.read("platformio.ini")

def generate_docs(*args, **kwargs):
    browser = None
    doxygen_bin = "doxygen"
    
    doxygen_config = config.get("documentation", "doxygen_config")
    if config.has_option("documentation", "browser"):
        browser = config.get("documentation", "browser")
    if config.has_option("documentation", "doxygen"):
        doxygen_bin = config.get("documentation", "doxygen")
    
    print("generating docs using "+doxygen_config+" config...")
    cmdline = "mkdir -p ./docs && "+doxygen_bin+" " + doxygen_config
    if browser:
        cmdline += " && "+browser
    env.Execute(cmdline)

env.AlwaysBuild(env.Alias("docs",
    None,
    generate_docs))



def generate_sensor_drivers():
    device_defines = []
    device_names = []
    for devgroup, items in config.items("devices"):
        devices = items.split('\n')
        #print(devgroup+" => " + ':'.join(devices))
        for dev in devices:
            if dev:
                devdef = devgroup.upper() + '_' + dev.upper().replace(' ','_')
                #print("   "+devdef)
                device_defines.append(devdef)
                device_names.append(dev.lower())
    env.Append(CPPDEFINES=device_defines)
    print("CONFIGURED NIMBLE DRIVERS: "+", ".join(device_names))


# build sensor factory source file
generate_sensor_drivers()
