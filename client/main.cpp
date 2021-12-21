/**
    socket_programming - client : Web client must read and parse a series of commands from the input file, parse
    only two requests GET & POST.
    @file main.cpp
    @author Bahaa Khalf
    @version 1.0 12-11-2020
*/
#include <iostream>
#include "stdio.h"
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <string>
#include <fstream>
#include <vector>
#include <regex>
#include <sstream>

#define MAX_LEN 4096

using namespace std;

struct sockaddr_in sock_add;
struct timeval tv;                           //struct for the time val in seconds and milliseconds
int reqIdx;
vector<string> requests;

/**
    Parse the request message and see if the request is GET or POST and take the action
    based on each one of them.
    @param string the request message, vector to store the command type and the file.
    @return string the message that will be send to the client.
*/
string  readInput(string req,vector<string>& words){
    string toSend="";
    char ch[req.size()+2];
    strcpy(ch, req.c_str());
    std::istringstream f(ch);

    int i=0;
    while(true){
        string temp;
        getline (f, temp);
        if(i==0){
            string s = temp;
            regex str_expr ("(GET|POST)(\\s)(\\/)(\\s*)(\\S+)(\\.)(jpg|JPG|png|PNG|gif|GIF|jpeg|JPEG|html|HTML|txt|TXT)(\\s*)(\\/?)(\\s*)(HTTP\\/1\\.1)");
            smatch sm;
            if(regex_match (s,sm,str_expr)){

                i++;
                words.push_back(sm[1]);
                string q []= {sm[5],sm[6],sm[7]};
                words.push_back(q[0]+q[1]+q[2]);
            }else{
                cout<<"There is a request with wrong syntax ==> IGNORE"<<endl;
                cout<<temp<<"\n\n";
            }

        }
        if(i == 1){
            if(temp == ""){
                toSend+="\r\n";
                break;
            }
            toSend+=temp+"\r\n";
        }

    }
    return toSend;
}

/**
    Organize the requsts and send one at time to the server.
    @param vector to store the command type and the file.
    @return string the message that will be send to the client.
*/
string  organizeInput(vector<string>& words){
    string ans="";
    if(reqIdx < requests.size()){
        ans = readInput(requests[reqIdx++],words);
    }
    else{
        words.push_back(" ");
        words.push_back(" ");
    }
    return ans;
}

/**
    Get the data from an external file if the file exist.
    @param string file name, string to store the answer.
    @return boolean to tell if the file exist or not.
*/
bool getFileContent(string fileName, string &ans){
    string s = "";
    ifstream file (fileName.c_str());
    if(file.fail()){
        return false;
    }else {
        string temp = "";
        while (getline (file, temp)) {
          s += temp+"\n";
        }
        ans = s;
        file.close();
    }


    return true;

}

/**
    Save the data to an external file.
    @param string file name, char array the data to be saved in the file.
    @return .
*/
void saveToFile(string fileName,char c[]){
    ofstream file (fileName.c_str());
    for (int i=0;i<strlen(c);i++) {
      file << c[i];
    }
    file.close();

}

/**
    Remove the header request that received from the server.
    @param char arry contain the server response.
    @return vector contain the header and the data if exist.
*/
vector<string> removeHeader(char ch[]){
    std::istringstream f(ch);
    std::string line;
    vector<string> c;
    string response = "", data = "";

    int i = 0;
    bool dataStart = false;
    while (std::getline(f, line)) {
        if(i == 0){
            response = line;i++;
            c.push_back(response);
        }else{
            data += line+"\n";
        }
    }
    c.push_back(data);
    return c;
}

/**
    Read the external file input that can contain more than one request.
    @param string file name.
    @return vector contains all requsets that was in the file.
*/
vector<string> readInputFromFile(string fileName){
    string s = "";
    vector<string> v;
    ifstream file (fileName.c_str());
    if(file.fail()){
    }else {
        string temp = "";
        while (getline (file, temp)) {
          s += temp+"\n";
          if(temp == ""){
            v.push_back(s);
            s="";
          }
        }
        file.close();
    }
    return v;
}

/**
    Main function that intialize the server socket and establish the connection with
    the server then send the requests one by one.
    @param int argc, char *argv[].
    @return int 0if it run successfully.
*/
int main(int argc,char *argv[])
{
    reqIdx = 0;                      //variable to keep track of the requests vector
    int port = 9600;                   //defult server port
    char *ip = "127.0.0.1";         //defult server ip
    if(argc == 3){
        ip = (char *)argv[1];
        port = atoi(argv[2]);
    }

    requests = readInputFromFile("input.txt");
    //creating socket and make it socket stream that use TCP not socket diagram(UDP)
    int socketID = socket(AF_INET , SOCK_STREAM ,0);

    // initialize sock_add
    bzero(&sock_add, sizeof(sock_add));
    sock_add.sin_addr.s_addr = inet_addr(ip);
    sock_add.sin_family = AF_INET;
    sock_add.sin_port = htons(port);
    memset(&(sock_add.sin_zero) , 0 ,8);

    int connected = connect(socketID , (sockaddr*) &sock_add , sizeof(sockaddr_in));
    if(connected < 0){
        cout << "Big fail"<<endl;
    }else{
        cout << "Connected successfully"<<endl;
        char ch[MAX_LEN];
        recv(socketID, ch, MAX_LEN,0);
        cout<<ch<<endl;
        while(true){
            memset(&ch , 0 ,MAX_LEN);
            vector<string> v;
            string content = organizeInput(v);
            if(v[0]=="POST"){
                string s="";
                if(getFileContent(v[1],s) == 0){
                    cout<<"This file don't exist\n\n";
                    continue;
                }
                content += s;
                strncpy(ch, content.c_str(), sizeof(ch));
                send(socketID, ch, strlen(ch), 0);
                memset(&ch , 0 ,MAX_LEN);
                recv(socketID, ch, MAX_LEN, 0);
                cout<<"*********************"<<endl;
                cout<<ch<<endl;
            }else if(v[0]=="GET"){
                strncpy(ch, content.c_str(), sizeof(ch));
                send(socketID, ch, strlen(ch), 0);
                memset(&ch , 0 ,MAX_LEN);
                recv(socketID, ch, MAX_LEN, 0);
                vector<string> temp = removeHeader(ch);
                cout<<"*********************"<<endl;
                cout<<temp[0]<<endl;
                if (temp[0].find("200") != std::string::npos) {
                    char c [temp[1].size()+1];
                    strcpy(c, temp[1].c_str());
                    saveToFile(v[1],c);
                }
            }

        }
    }

    return 0;
}
