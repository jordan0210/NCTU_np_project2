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
	string cmdLine;
	string cmd;
	vector<string> argv;
	bool has_fd_in, has_fd_out;
	int fd_in, fd_out;
	int pipeType; //0: no pipe, 1: normal pipe, 2: error pipe
	bool has_userpipe_send, has_userpipe_receive;
	int send_to_ID, receive_from_ID;
}cmdBlock;

typedef struct Pipe{
	int count;
	int fd[2];
}Pipe;

typedef struct UserPipe{
	int sourceID;
	int targetID;
	int fd[2];
}UserPipe;

typedef struct User{
	bool hasUser;

	int ssock;
	string address;

	bool doneServe; //check is the user need to be send "% "
	int ID;
	string name;
	string path;
	vector<Pipe> pipes;
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

//show who's on the server
void who();

//tell targetID the message
void tell(int targetID, string Msg);

//yell the message
void yell(string Msg);

//change the User's name
void name(string newName);

//send the message to targetID
//if there not exist targetID -> broadcast
void broadcast(int *sourceID, int *targetID, string Msg);

//check and create user pipe, then set the fds in cmdBlock
void handleUserPipe(cmdBlock &cmdBlock);

//check if the User pipe is exist
bool checkUserPipeExist(int &index, int sourceID, int targetID);

//return UserName (handle username == "")
string getUserName(string UserName);

//serve User with servingID
void np_shell(int ID);

//parse cmdLine into cmdBlocks by each pipe
void parsePipe(string cmdLine, vector<cmdBlock> &cmdBlocks);

//parse cmdBlock into command and arguments
void parseCmd(cmdBlock &cmdBlock);

//check the type of pipe and generate a new pipe
void checkPipeType(cmdBlock &cmdBlock);

//execute the command
bool exec_cmd(cmdBlock &cmdBlock);

//change the vector argv to char *argv[]
void vec2arr(vector<string> &vec, char *arr[], int index);

//find the element in the vector and return its index
int findIndex(vector<string> &vec, string str);

//find the max value of User's ssock
int findMax(int msock);