#ifndef CM_PRODUCTION_MUTATION_INTERLOCK_H
#define CM_PRODUCTION_MUTATION_INTERLOCK_H

#include <Arduino.h>
#include <WebServer.h>

namespace CM
{
class ProductionMutationInterlock
{
public:
    static void acquire()
    {
        state() = true;
    }

    static void release()
    {
        state() = false;
    }

    static bool active()
    {
        return state();
    }

private:
    static bool& state()
    {
        static bool locked = false;
        return locked;
    }
};

class ProductionMutationInterlockHandler : public RequestHandler
{
public:
    bool canHandle(HTTPMethod requestMethod, String requestUri) override
    {
        return ProductionMutationInterlock::active() &&
               requestMethod != HTTP_GET &&
               requestUri.startsWith("/api/");
    }

    bool handle(WebServer& server,
                HTTPMethod requestMethod,
                String requestUri) override
    {
        if (!canHandle(requestMethod, requestUri)) return false;
        server.send(409, "application/json; charset=utf-8",
                    "{\"error\":\"restore_mutation_active\",\"read_only_allowed\":true}");
        return true;
    }
};

class ProductionMutationInterlockRegistration
{
public:
    explicit ProductionMutationInterlockRegistration(WebServer& server)
    {
        // RemoteBackupWeb is constructed before configureWebServer() registers
        // application routes, so this catch-all becomes the first handler.
        // It is inert unless restore apply/rollback owns the mutation lock.
        server.addHandler(new ProductionMutationInterlockHandler());
    }
};
}

#endif // CM_PRODUCTION_MUTATION_INTERLOCK_H
