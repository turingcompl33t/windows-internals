// server.cs
// Simple websocket server application.

using System;
using System.Net;
using System.Text;
using System.Threading;
using System.Net.WebSockets;
using System.Threading.Tasks;

namespace WebsocketServer
{
    class Program
    {
        static void Main(string[] args)
        {
            Run().Wait();
        }

        static async Task Run()
        {
            var server = new HttpListener();
            server.Prefixes.Add("http://localhost:8000/ws/");
            server.Start();

            var http = await server.GetContextAsync();
            Console.WriteLine("Connected");

            if (!http.Request.IsWebSocketRequest)
            {
                http.Response.StatusCode = 400;
                http.Response.Close();
                Console.WriteLine("Not a websocket request");
            }

            // upgrade to a websocket connection
            var ws = await http.AcceptWebSocketAsync(null);

            for (var i = 0; i < 10; ++i)
            {
                await Task.Delay(1000);

                var time = DateTime.Now.ToLongTimeString();
                var buffer = Encoding.UTF8.GetBytes(time);
                var segment = new ArraySegment<Byte>(buffer);

                Console.WriteLine(time);

                await ws.WebSocket.SendAsync(
                    segment,
                    WebSocketMessageType.Text,
                    true,
                    CancellationToken.None
                    );
            }

            // graceful exit
            await ws.WebSocket.CloseAsync(
                WebSocketCloseStatus.NormalClosure,
                null,
                CancellationToken.None
                );
        }
    }
}
