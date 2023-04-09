using System.Threading;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System;
using UnityEngine;

public class Connect : MonoBehaviour
{
    Thread receiveThread;
    TcpClient client;
    IPEndPoint IP;
    int port;
    public string localIP;
    private TcpClient socketConnection;
    private Thread clientReceiveThread;


    void Awake()
    {
        port = 8080;
        IP = new IPEndPoint(IPAddress.Parse("127.0.0.1"), port);
        //ServerIP = new IPEndPoint(IPAddress.Parse("0.0.0.0"), port);
        Debug.Log("HERE");
        ConnectToTcpServer();
    }

    private void ConnectToTcpServer()
    {
        try
        {
            Debug.Log("IN TRY");
            clientReceiveThread = new Thread(new ThreadStart(ListenForData));
            clientReceiveThread.IsBackground = true;
            clientReceiveThread.Start();
            Debug.Log("STARTED");
        }
        catch (Exception e)
        {
            Debug.Log("On client connect exception " + e);
        }
    }

    private void ListenForData()
    {
        try
        {
            socketConnection = new TcpClient("127.0.0.1", 8080);
            Byte[] bytes = new Byte[1024];
            while (true)
            {
                // Get a stream object for reading 				
                using (NetworkStream stream = socketConnection.GetStream())
                {
                    int length;
                    // Read incomming stream into byte arrary. 					
                    while ((length = stream.Read(bytes, 0, bytes.Length)) != 0)
                    {
                        var incommingData = new byte[length];
                        Array.Copy(bytes, 0, incommingData, 0, length);
                        // Convert byte array to string message. 						
                        string serverMessage = Encoding.ASCII.GetString(incommingData);
                        Debug.Log("server message received as: " + serverMessage);
                    }
                }
            }
        }
        catch (SocketException socketException)
        {
            Debug.Log("Socket exception: " + socketException);
        }
    }

    public void OnApplicationQuit()
    {
        if (receiveThread != null)
        {
            receiveThread.Abort();
        }
        if (client != null)
        {
            client.Close();
        }
    }
}
