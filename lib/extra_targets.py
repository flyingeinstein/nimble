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