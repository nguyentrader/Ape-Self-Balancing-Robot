#include "web_server.h"

#include "config.h"
#include "debug_logger.h"

#include <WebServer.h>
#include <SPIFFS.h>

static WebServer server(WEB_SERVER_PORT);

static String getContentType(const String& path)
{
    if (path.endsWith(".html"))
    {
        return "text/html";
    }

    if (path.endsWith(".css"))
    {
        return "text/css";
    }

    if (path.endsWith(".js"))
    {
        return "application/javascript";
    }

    if (path.endsWith(".png"))
    {
        return "image/png";
    }

    if (path.endsWith(".jpg") ||
        path.endsWith(".jpeg"))
    {
        return "image/jpeg";
    }

    if (path.endsWith(".gif"))
    {
        return "image/gif";
    }

    if (path.endsWith(".ico"))
    {
        return "image/x-icon";
    }

    return "text/plain";
}

static bool handleFileRead(String path)
{
    if (path.endsWith("/"))
    {
        path += "index.html";
    }

    if (!SPIFFS.exists(path))
    {
        logWarn(
            "File not found: " +
            path
        );

        return false;
    }

    File file = SPIFFS.open(path, "r");

    if (!file)
    {
        logError(
            "Failed to open file: " +
            path
        );

        return false;
    }

    String contentType = getContentType(path);

    server.streamFile(
        file,
        contentType
    );

    file.close();

    return true;
}

void webServerInit()
{
    if (!SPIFFS.begin(true))
    {
        logError("SPIFFS initialization failed");
        return;
    }

    logInfo("SPIFFS initialized");

    server.on(
        "/",
        HTTP_GET,
        []()
        {
            if (!handleFileRead("/index.html"))
            {
                server.send(
                    404,
                    "text/plain",
                    "index.html not found"
                );
            }
        }
    );

    server.onNotFound(
        []()
        {
            if (!handleFileRead(server.uri()))
            {
                server.send(
                    404,
                    "text/plain",
                    "File not found"
                );
            }
        }
    );

    server.begin();

    logInfo(
        "HTTP Web Server started on port " +
        String(WEB_SERVER_PORT)
    );
}

void webServerLoop()
{
    server.handleClient();
}