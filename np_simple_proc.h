#include <iostream>
#include <string.h>
#include <vector>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <regex>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
//#include <iomanip>

using namespace std;

typedef struct cmdBlock{
	string cmd;
	vector<string> argv;
	bool has_fd_in, has_fd_out;
	int fd_in, fd_out;
	int pipeType; //0: no pipe, 1: pipe, 2: error pipe
}cmdBlock;

typedef struct Pipe{
	int count;
	int fd[2];
}Pipe;

typedef struct User{
	bool hasUser;
	bool doneServe;

	int ssock;
	string address;

	int ID;
	string name;
	string path;
}User;

//initial data in User structure of the index
void init_UserData(int index);

//create the master socket, return its fd
int create_socket(unsigned short port);

//accept a User and set its data
bool accept_newUser(int msock);

//show welcome message to the User with given ID
void show_welcomeMsg(int ID);

//show login message of the User with given ID to all User
void show_loginMsg(int ID);

//show logout message of the User with given ID to all User
void show_logoutMsg(int ID);

//tell targetID the message
//if there not exist targetID -> broadcast
void tell(int *sourceID, int *targetID, string Msg);

//read message from the client
string readFromClient();

//write message to the client
void writeToClient();

//serve User with servingID
void np_shell(int servingID);

//parse cmdLine into cmdBlocks by each pipe
void parsePipe(string cmdLine, vector<cmdBlock> &cmdBlocks);

//parse cmdBlock into command and arguments
void parseCmd(cmdBlock &cmdBlock);

//check the type of pipe and generate a new pipe
void checkPipeType(cmdBlock &cmdBlock);

//execute the command
void exec_cmd(cmdBlock &cmdBlock);

//change the vector argv to char *argv[]
void vec2arr(vector<string> &vec, char *arr[], int index);

//find the element in the vector and return its index
int findIndex(vector<string> &vec, string str);

//find the max value of User's ssock
int findMax();