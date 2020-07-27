#include "civetweb.h"
#include <cstdint>
#include <memory>
#include <filesystem>

#define LOCALHOST "127.0.0.1"

class Server
{
private:
	mg_context *_server;
	const char *_port;
public:
	/*  Is called when the client intends to establish a websocket connection,
		before websocket handshake.
		Return value :
			0 : civetweb proceeds with websocket handshake.
			1 : connection is closed immediately. */
	static int WebsocketConnect(const mg_connection *conn, void*)
	{
		// Perhaps get the player PIN from a GET/POST to authenticate?
		const mg_request_info *RequestInfo = mg_get_request_info(conn);
		printf("New connection from IP %s.\n", RequestInfo->remote_addr);
		return 0;
	}

	/*  Is called when websocket handshake is successfully completed, and
        connection is ready for data exchange. */
	static void WebsocketReady(mg_connection *conn, void*)
	{
		printf("Websocket ready.\n");
	}

	/*  Is called when a data frame has been received from the client.
		Parameters:
          bits: first byte of the websocket frame, see websocket RFC at http://tools.ietf.org/html/rfc6455, section 5.2
	      data, data_len : payload, with mask(if any) already applied.
		  Return value :
	        1 : keep this websocket connection open.
		    0 : close this websocket connection. */
	static int WebsocketData(mg_connection *conn, int bits, char *data, size_t data_len, void *)
	{
		printf("Received %lu bytes of data from client.\n Client data: ", (unsigned long)data_len);
		fwrite(data, 1, data_len, stdout);
		printf("\n");
		return 1;
	}

	/* Is called when the connection is closed */
	static void WebsocketClose(const mg_connection *conn, void*)
	{
		
	}

	/*  Called when a new request comes in.  This callback is URI based
        and configured with mg_set_request_handler().
        Parameters:
          conn: current connection information.
          cbdata: the callback data configured with mg_set_request_handler().
        Returns:
          0: the handler could not handle the request, so fall through.
          1 - 999: the handler processed the request. The return code is
	      stored as a HTTP status code for the access log. */
	static int RequestHandlerRoot(mg_connection *conn, void *cbdata)
	{
		const mg_request_info *RequestInfo = mg_get_request_info(conn);
		printf("[ROOT] Connection '%s' requested uri '%s'.\n", RequestInfo->remote_addr, RequestInfo->local_uri);

		mg_send_file(conn, "static/HTML/Index.html");

		return 200;
	}

	static int RequestHandlerInterface(mg_connection *conn, void *cbdata)
	{
		const mg_request_info *RequestInfo = mg_get_request_info(conn);
		printf("[INTERFACE] Connection '%s' requested uri '%s'.\n", RequestInfo->remote_addr, RequestInfo->local_uri);

		//mg_printf(conn, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n");
		mg_send_file(conn, "static/HTML/Interface.html");

		return 200;
	}

	static int RequestHandlerStatic(mg_connection *conn, void *cbdata)
	{
		const mg_request_info *RequestInfo = mg_get_request_info(conn);
		printf("[STATIC] Connection requested uri '%s'.\n", RequestInfo->local_uri);

		namespace fs = std::experimental::filesystem;
		fs::path ReqFile = ".";
		ReqFile += RequestInfo->local_uri;

		ReqFile = fs::absolute(ReqFile);
		if (fs::exists(ReqFile) && fs::is_regular_file(ReqFile))
		{
			mg_send_mime_file(conn, ReqFile.string().c_str(), nullptr);
		}
		else
		{
			printf("\tERROR! '%s' is not a valid file. Ensure it exists and is not read-protected.\n", ReqFile.string().c_str());
		}

		return 200;
	}

	Server(const char *Port) :
		_port(Port),
		_server(nullptr)
	{

	}

	~Server()
	{
		mg_stop(_server);
	}

	bool Initialize()
	{
		const char *Options[] = { "document_root", ".", "listening_ports", _port, 0 };
		
		mg_callbacks Callbacks;
		memset(&Callbacks, 0, sizeof(Callbacks));

		_server = mg_start(&Callbacks, nullptr, Options);

		mg_set_request_handler(_server, "/", Server::RequestHandlerRoot, nullptr);
		mg_set_request_handler(_server, "/static", Server::RequestHandlerStatic, nullptr);
		mg_set_request_handler(_server, "/interface", Server::RequestHandlerInterface, nullptr);

		mg_set_websocket_handler(_server, "/websocket", Server::WebsocketConnect, Server::WebsocketReady, Server::WebsocketData, Server::WebsocketClose, nullptr);

		printf("Server initialized!\n");
		return true;
	}
};

int main()
{
	Server server("8081");
	server.Initialize();

	while (true)
	{

	}

	return 0;
}