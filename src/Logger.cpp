
#include "Logger.h"
#include "ModuleManager.h"

namespace Nimble {

LogEntry::LogEntry(LogEntry&& copy) {
  dest = copy.dest;
  copy.dest = nullptr;
  _module = copy._module;
  _category = copy._category;
  _message = copy._message;
  _detail = copy._detail;
  _error = copy._error;
  _severity = copy._severity;
  _sensor = copy._sensor;
}

LogEntry::~LogEntry() {
  if(dest != nullptr)
    dest->write(this);
}

short LogEntry::toJson(JsonObject& msg) {
  msg["level"] = (short)_severity;
  if(_message.length())
    msg["short_message"] = _message;
  if(_module.length())
    msg["_module"] = _module;
  if(_category.length())
    msg["category"] = _category;
  if(_detail.length())
    msg["full_message"] = _detail;
  if(_error !=0)
    msg["_errorcode"] = _error;

  String content;
  serializeJson(msg, content);
  Serial.print("log => ");
  Serial.println(content);

  return 0;
}

void Logger::write(JsonObject& msg)
{
  if(!logEndpoint.handler || !logEndpoint.handler.handler) {
    Endpoints::Request request(Rest::HttpPost, "/log");

    // find the logger
    #if 0
    ModuleManager::Default.modules().forEach([this](Module& mod, void* pData) -> bool {
      if()
    })
    #endif
    
    Module& modLogger = ModuleManager::Default.modules()[ 3 ];

    if(modLogger && modLogger.hasEndpoints()) {
      Endpoints* ep = modLogger.endpoints();
      if( ep->resolve(request) && request.isSuccessful()) {
        logEndpoint = request;
        Serial.print("found logger at ");
        Serial.print(modLogger.getDriverName());
        Serial.print(" id:");
        Serial.println(modLogger.id);
        if(!request.handler || !request.handler.handler)
          Serial.println("yet still no handler");
      }
    }
  }

  if(logEndpoint.handler && logEndpoint.handler.handler) {
    RestRequest rr(*(WebServer*)nullptr, logEndpoint);
    rr.hasJson = true;
    rr.method = Rest::HttpPost;
    rr.body = msg;
    rr.timestamp = millis();
    rr.contentType = Rest::ApplicationJsonMimeType;
        Serial.println("calling logger");

    logEndpoint.handler.handler(rr);
  } else {
    Serial.println("Warning: unable to find a logger module");
  }
}

void Logger::write(LogEntry* entry)
{
  DynamicJsonDocument doc(256);
  JsonObject root = doc.to<JsonObject>();
  if(entry->toJson(root) >=0) {
    // write entry
    write(root);
  }
}


}  // ns:Nimble
