
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
    dest->write(*this);
}

void LogEntry::print(Stream& s) {
  switch(_severity) {
    case Debug: s.print("DEBUG"); break;
    case Info: s.print("INFO"); break;
    case Warning: s.print("WARN"); break;
    case Error: s.print("ERROR"); break;
    case Fatal: s.print("FATAL"); break;
    case Alert: s.print("ALERT"); break;
  }
  if(_module.length()) {
    s.print(" [");
    s.print(_module);
    s.print("]");
  }
  if(_category.length()) {
    s.print(" ");
    s.print(_category);
  }
  if(_sensor) {
    s.print(" $");
    s.print(_sensor.toString());
  }
  if(_error > 0) {
    s.print(" error ");
    s.print(_error);
  }

  s.print(": ");
  s.print(_message);

  if(_detail.length()) {
    s.print(" => ");
    s.print(_detail);
  }
  s.println();
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

  return 0;
}

void Logger::write(JsonObject& msg)
{
  if(!remote)
    return;

  if(!logEndpoint.handler.handler) {
    // find the logger by scanning each root module
    ModuleManager::Default.modules().forEach([this](Module& mod, void* pData) -> bool {
      if(mod && mod.hasEndpoints()) {
        Endpoints* ep = mod.endpoints();
        Endpoints::Request request = ep->resolve(Rest::HttpPost, "/log");
        if(request.status == Rest::UriMatched && request.handler.handler != nullptr) {
          logEndpoint = request;
          Serial.print("Info: using logger ");
          Serial.println(mod.getDriverName());
          return false;
        }
      }
      return true;
    });
  }

  if(logEndpoint.handler.handler) {
    RestRequest rr(*(WebServer*)nullptr, logEndpoint);
    rr.hasJson = true;
    rr.method = Rest::HttpPost;
    rr.body = msg;
    rr.timestamp = millis();
    rr.contentType = Rest::ApplicationJsonMimeType;
    logEndpoint.handler.handler(rr);
  } else {
    Serial.println("Warning: unable to find a logger module");
    remote = false;
  }
}

void Logger::write(LogEntry& entry)
{
  Severity sev = entry.severity();
  if(sev <= consoleLevel)
    entry.print(Serial);

  if(sev <= remoteLevel) {
    DynamicJsonDocument doc(256);
    JsonObject root = doc.to<JsonObject>();
    if(entry.toJson(root) >=0) {
      // write entry
      write(root);
    }
  }
}


}  // ns:Nimble
