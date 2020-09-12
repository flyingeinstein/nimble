
#include "GelfLogger.h"


GelfLogger::GelfLogger(short id)
  : Module(id, 2, 2500), host("kuba.lan"), port(5000)
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
    Udp.beginPacket(destip, 12201);
    Udp.write(
        content.c_str(), 
        content.length()
    );
    Udp.endPacket();
}

void GelfLogger::begin()
{
}

Rest::Endpoint GelfLogger::delegate(Rest::Endpoint &p) 
{
    // todo: IMPLEMENT log api call
    return {};

#if 0
  std::function<int(RestRequest&)> log_src_cat = [this](RestRequest& request) {
    auto src = request.query("src");
    auto cat = request["cat"];

    if (!request.hasJson || request.body.isNull()) {
        return 400;
    }
    if(!destip.isSet()) {
        Serial.println("log destination is not valid or not set");
        return 500;
    }

    JsonObject root = request.body.as<JsonObject>();
 
    // set some optional client fields, but are required in the message
    if(!root.containsKey("version"))
        root["version"] = "1.1";
    if(!root.containsKey("host"))
        root["host"] = WiFi.hostname();
    if(!root.containsKey("_product"))
        root["_product"] = "nimbl";

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

  on("/config")
    .withContentType("text/plain", true)    // todo: need to turn these into utility helpers
    .PUT("host", [this](RestRequest& rr) { host = rr.server.arg("plain"); WiFi.hostByName(host.c_str(), destip, 5000); return 200; } )
    .GET("host", [this](RestRequest& rr) { rr.response["host"] = host; rr.response["ip"] = destip.toString(); return 200; } )
    .PUT("port", [this](RestRequest& rr) { port = atoi(rr.server.arg("plain").c_str()); return 200; } )
    .GET("port", [this](RestRequest& rr) { rr.response["port"] = port; return 200; } );
    //.GET("port", port );

  //dest.fromString("192.168.44.5");
#endif
}

void GelfLogger::handleUpdate()
{

}
