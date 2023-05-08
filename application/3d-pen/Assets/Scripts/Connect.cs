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
    public Vector3 pos;
    public GameObject cursor;


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
                        //Debug.Log("received: " + serverMessage);
                        string[] datas = serverMessage.Split(',');
                        if(datas.Length >= 6)
                        {
                            pos.x = (datas[0] == "1" ? -1 : 1) * float.Parse(datas[1]) * 2;
                            pos.y = (datas[2] == "1" ? -1 : 1) * float.Parse(datas[3]) * 2;
                            pos.z = (datas[4] == "1" ? -1 : 1) * float.Parse(datas[5]) * 2;
                            //Debug.Log(pos.x);
                            //Debug.Log(pos.y);
                            //Debug.Log(pos.z);
                        }
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

    void Update()
    {
        cursor.transform.position = pos;
    }
}
