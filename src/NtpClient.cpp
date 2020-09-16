#include "NtpClient.h"

NtpClient* defaultNtpClient = nullptr;

unsigned long long unix_timestamp() {
    return defaultNtpClient
        ? defaultNtpClient->ntp().getEpochTime()
        : millis() / 1000;
}


NtpClient::NtpClient(short id)
  : Module(id, 0, 5000), _ntp(_udp)
{
    alias = "ntp";

    // first ntp client becomes default
    if(!defaultNtpClient)
        defaultNtpClient = this;

    _ntp.setUpdateInterval(900);
}

const char* NtpClient::getDriverName() const
{
    return "NtpClient";
}

void NtpClient::begin()
{
  // begin the ntp client
  _ntp.begin();
}

void NtpClient::handleUpdate()
{
    state = _ntp.update()
        ? Nimble::Nominal
        : Nimble::Offline;
}

Rest::Endpoint NtpClient::delegate(Rest::Endpoint &p) 
{
    Module::delegate(p);

    return p / "update" / Rest::POST([this](RestRequest& req) {
        if(_ntp.forceUpdate()) {
            req.response["timetamp"] = _ntp.getEpochTime();
            req.response["time"] = _ntp.getFormattedTime();
            return Rest::OK;
        } else
            return Rest::InternalServerError;
    });
}
