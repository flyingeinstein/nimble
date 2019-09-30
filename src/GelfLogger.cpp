
#include "GelfLogger.h"


GelfLogger::GelfLogger(short id)
  : Module(id, 2, 2500)
{

}

const char* GelfLogger::getDriverName() const
{
    return "GelfLogger";
}

void GelfLogger::log(JsonObject& node)
{
    // serialize the Json
    String content;
    serializeJson(node, content);

    // send the log message
    Udp.beginPacket(dest, 12201);
    Udp.write(
        content.c_str(), 
        content.length()
    );
    Udp.endPacket();
}

void GelfLogger::begin()
{
  std::function<int(RestRequest&)> log_src_cat = [this](RestRequest& request) {
    auto src = request["src"];
    auto cat = request["cat"];

    if (!request.hasJson || request.body.isNull()) {
        return 400;
    }
    if(!dest.isSet()) {
        Serial.println("log destination is not valid or not set");
        return 500;
    }

    JsonObject root = request.body.as<JsonObject>();
    
    // todo: validate that the required fields are present
    if(!root.containsKey("short_message")) {
        request.response["error"] = "requires short_message field";
        return 400;
    }

    // set some optional client fields, but are required in the message
    if(!root.containsKey("version"))
        root["version"] = "1.1";
    if(!root.containsKey("host"))
        root["host"] = WiFi.hostname();

    if(src.isString())
        root["_module"] = (String)src;
    if(cat.isString())
        root["_category"] = (String)cat;

    // upload using the GELF protocol
    log(root);

    request.response["reply"] = root;
    return 200;
  };

  on("/log")
    .POST( log_src_cat )
      .on(":src(string)")
      .POST( log_src_cat )
      .POST(":cat(string)", log_src_cat );

  WiFi.hostByName("kuba.lan", dest, 5000);
  //dest.fromString("192.168.44.5");
}

void GelfLogger::handleUpdate()
{

}
