/**
    socket_programming - server : Multi threaded web server accept incoming connection requests.Then handle GET and Post
    requests, in case of GET it returns HTTP/1.1 200 OK and the data to the client, and in case of POST it get the data
    from the request body and store it in a file on the server machine.
    @file main.cpp
    @author Bahaa Khalf
    @version 1.0 12-11-2020
*/
#include <iostream>
#include <vector>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <string>
#include <regex>
#include<bits/stdc++.h>

#define MAX_LEN 4096

using namespace std;

int socketID;                                 //The socket number
struct sockaddr_in sock_add;
struct timeval tv;                           //struct for the time val in seconds and milliseconds
vector<int> clients;
pthread_t workers[100];
int workersCount;

/**
    Get the data from an external file.
    @param string file name.
    @return string with the file content.
*/
string getFileContent(string fileName){
    string s = "";
    ifstream file (fileName.c_str());

    string temp = "";
    while (getline (file, temp)) {
      s += temp+"\n";
    }
    file.close();

    return s;

}

/**
    Save the data to an external file.
    @param string file name, string the data to be saved in the file.
    @return .
*/
void saveToFile(string fileName,string c){
    ofstream file (fileName.c_str());
    for (int i=0;i<c.size();i++) {
      file << c[i];
    }
    file.close();

}

/**
    Check if a new connections arrived to the server.
    @param fd_set store the clients tha made the requests.
    @return .
*/
void processServerRequest(fd_set rfds){
    //ready to read something new
    if(FD_ISSET(socketID, &rfds)){
        int client = accept(socketID, NULL, NULL);
        if(client >0){
            cout <<"\nConnected to new client : "<<client<<endl;
            clients.push_back(client);
            send(client,"Connected Successfully to the server",100,0);
        }

    }
}

/**
    Parse the request message and see if the request is GET or POST and take the action
    based on each one of them.
    @param char array the request message.
    @return string the message that will be send to the client.
*/
string getDataToSend(char ch[]){
        std::istringstream f(ch);
        std::string line;
        vector<string> parts;
        string request = "", data = "", send = "";

        int i = 0;
        bool dataStart = false;
        while (std::getline(f, line)) {
            if(i == 0){
                string q = line;
                string s = q.substr(0, q.size()-1);
                regex str_expr ("(GET|POST)(\\s)(\\/)(\\s*)(\\S+)(\\.)(jpg|JPG|png|PNG|gif|GIF|jpeg|JPEG|html|HTML|txt|TXT)(\\s*)(\\/?)(\\s*)(HTTP\\/1\\.1)");
                //regex get_home ("(GET)(\\s)(\\/)(\\s*)(HTTP\\/1\\.1");
                smatch sm;
                if(regex_match (s,sm,str_expr)){
                    i++;
                    parts.push_back(sm[1]);
                    string q []= {sm[5],sm[6],sm[7]};
                    parts.push_back(q[0]+q[1]+q[2]);
                }
                else{
                    cout<<"Received wrong syntax request ==> IGNORE"<<endl;
                    cout<<line<<"\n\n";
                    return "HTTP/1.1 404 Not Found\r\n";
                }

            }
            if( (int)line[0] == 13 ){
                dataStart = true;
                continue;
            }
            if(dataStart){
                data += line+"\n";
            }else{
                request += line+"\n";
            }
        }
        cout<<request<<"\n******************************************************\n";
        if(parts[0] == "POST"){
            saveToFile(parts[1], data);
            send = "HTTP/1.1 200 OK\r\n";
        }else if(parts[0] == "GET"){
            data = getFileContent(parts[1]);
            if(data == ""){
                send = "HTTP/1.1 404 Not Found\r\n";
            }
            else{
                send = "HTTP/1.1 200 OK\r\n";
                send += data;
            }

        }
        return send;
}

/**
    The thread function to handle multi requests at the same time.
    @param void* the thread parameter.
    @return .
*/
void* procesMessage(void* param){
    int client = *((int *)param);
    char ch[MAX_LEN];
    int r = recv(client, ch, MAX_LEN, 0);
    if(r <= 0){          //There is error
        cout<<"close the connection for the client : "<<client<<endl<<endl;

        vector<int>::iterator it = clients.begin();
        for(it;it!=clients.end();it++){
            if(client == *it){
                clients.erase(it);
                break;
            }
        }
        close(client);
    }else{
        //cout<<"\n\nThe server received the message from client: "<<client<<endl;
        string s = getDataToSend(ch);
        char toSend [s.size()+1];
        strncpy(toSend, s.c_str(), sizeof(toSend));

        send(client, toSend, strlen(toSend),0);
        pthread_exit(0);
    }

}

/**
    Main function that intialize the server and looping while listening to new requests.
    @param int argc, char *argv[].
    @return int 0if it run successfully.
*/
int main(int argc,char *argv[])
{
    int port = 9600;
    if(argc == 2){
        port = atoi(argv[1]);
    }
    workersCount=0;
    //creating socket and make it socket stream that use TCP not socket diagram(UDP)
    socketID = socket(AF_INET , SOCK_STREAM ,0);

    // initialize sock_add
    bzero(&sock_add, sizeof(sock_add));
    sock_add.sin_addr.s_addr = inet_addr("127.0.0.1");
    sock_add.sin_family = AF_INET;
    sock_add.sin_port = htons(port);
    memset(&(sock_add.sin_zero) , 0 ,8);

    //Make more than one socket can use the same IP and port
    int optVal = 0;
    int optLen = sizeof(optVal);
    int r = setsockopt(socketID, SOL_SOCKET, SO_REUSEADDR, (const char*) &optVal, optLen);

    //Binding the socket with the sockaddr_in
    bind(socketID , (sockaddr*) &sock_add , sizeof(sockaddr_in));


    //listen to the wanted socket and 10 as the number of requests that can be at the active queue
    listen(socketID, 10);

    //initialize the time value variables
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    int sockLen = sizeof(sock_add);
    cout << "Ready to listen"<<endl;
    while(true){

        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(socketID, &rfds);
        int mx = socketID;
        for(int i = 0; i<clients.size();i++){
            FD_SET(clients[i], &rfds);
            if(clients[i]>mx)mx=clients[i];
        }
        //call the select functions and see if there is any requests to the socket or non or there is a failure
        int nSelect = select(mx+1 , &rfds, NULL, NULL, &tv);
        if(nSelect > 0){
                processServerRequest(rfds);
                for(int i=0;i<clients.size();i++){
                    //ready to read something new
                    if(workersCount>=95){
                        for(int j=0;j<workersCount;j++){
                            pthread_join(workers[j],NULL);
                        }
                        workersCount = 0;
                    }
                    if(FD_ISSET(clients[i], &rfds)){
                        pthread_create(&workers[workersCount++],0,procesMessage, &clients[i]);
                    }
                }

        }else if(nSelect == 0){                           //It work but no requests
        }else{                                            //There is a big failure
        }
        cout<<"clients.size :"<<clients.size()<<endl;
        sleep(1);
    }
    return 0;
}
