#include "np_simple_proc.h"

static regex reg_numberPipe("[|!][0-9]+");
static regex reg_userPipe_send("[>][0-9]+");
static regex reg_userPipe_receive("[<][0-9]+");

fd_set activefds, readfds, writefds;
User Users[30];
int servingID;
vector<UserPipe> UserPipes;
int FD_NULL;

int main(int argc, char *argv[]){
	if (argc != 2){
		return 0;
	}

	//initial server
	for (int i=0; i<30; i++){
		init_UserData(i);
	}
	FD_ZERO(&activefds);
	FD_ZERO(&readfds);
	FD_ZERO(&writefds);
	servingID = 0;
	UserPipes.clear();
	FD_NULL = open("/dev/null", O_RDWR);

	//create socket and start listen
	unsigned short port = (unsigned short)atoi(argv[1]);

	int msock = create_socket(port);
	listen(msock, 30);

	struct timeval timeval = {0, 10};
	while(true){
		bcopy(&activefds, &readfds, sizeof(readfds));
		if (select(findMax(msock), &readfds, NULL, NULL, &timeval) < 0){
			cerr << "select fail" << endl;
		}

		//accept
		if (FD_ISSET(msock, &readfds)){
			accept_newUser(msock);
		}

		//serve User
		for (int i=0; i<30; i++){
			np_shell(i);
		}
	}
	return 0;
}

void init_UserData(int index){
	Users[index].hasUser = false;
	Users[index].ssock = 0;
	Users[index].address = "";
	Users[index].doneServe = true;
	Users[index].ID = 0;
	Users[index].name = "";
	Users[index].path = "bin:.";
	Users[index].pipes.clear();
}

int create_socket(unsigned short port){
	int msock;
	if ((msock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		cerr << "Socket create fail.\n";
		exit(0);
	}
	struct sockaddr_in sin;

	bzero((char *)&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(port);

	if (bind(msock, (struct sockaddr *)&sin, sizeof(sin)) < 0){
		cerr << "Socket bind fail.\n";
		exit(0);
	}
	FD_SET(msock, &activefds);
	return msock;
}

bool accept_newUser(int msock){
	struct sockaddr_in sin;
	unsigned int alen = sizeof(sin);
	bool success = false;
	struct timeval timeval = {0, 10};

	for (int i=0; i<30; i++){
		if (Users[i].hasUser){
			continue;
		} else {
			int ssock;
			if ((ssock = accept(msock, (struct sockaddr *)&sin, &alen)) < 0){
				return success;
			}
			cout << "accept success." << endl;
			Users[i].hasUser = true;
			Users[i].ssock = ssock;
			Users[i].address = inet_ntoa(sin.sin_addr);
			Users[i].address += ":" + to_string(htons(sin.sin_port));
			Users[i].ID = i;

			success = true;
			FD_SET(Users[i].ssock, &activefds);

			show_welcomeMsg(Users[i].ID);
			show_loginMsg(Users[i].ID);
			break;
		}
	}

	return success;
}

void show_welcomeMsg(int ID){
	string Msg = "";
	Msg = Msg + "***************************************\n"
		+  "** Welcome to the information server **\n"
		+  "***************************************\n";
	broadcast(NULL, &ID, Msg);
}

void show_loginMsg(int ID){
	string UserName = Users[ID].name;
	if (UserName == ""){
		UserName = "(no name)";
	}
	string Msg = "*** User '" + UserName + "' entered from " + Users[ID].address + ". ***\n";
	cout << Msg << endl;
	broadcast(NULL, NULL, Msg);
}

void show_logoutMsg(int ID){
	string UserName = Users[ID].name;
	if (UserName == ""){
		UserName = "(no name)";
	}
	string Msg = "*** User '" + UserName + "' left. ***\n";
	cout << Msg << endl;
	broadcast(NULL, NULL, Msg);
}

void who(){
	string Msg = "<ID>\t<nickname>\t<IP:port>\t<indicate me>\n";
	for (int i=0; i<30; i++){
		if (Users[i].hasUser){
			string UserName = Users[i].name;
			if (UserName == ""){
				UserName = "(no name)";
			}
			Msg += to_string(Users[i].ID + 1) + "\t" + UserName + "\t" + Users[i].address;
			if (Users[i].ID == servingID){
				Msg += "\t<-me";
			}
			Msg += "\n";
		}
	}
	broadcast(NULL, &servingID, Msg);
}

void tell(int targetID, string Msg){
	if (Users[targetID].hasUser){
		string UserName = Users[servingID].name;
		if (UserName == ""){
			UserName = "(no name)";
		}
		Msg = "*** " + UserName + " told you ***: " + Msg + "\n";
		broadcast(NULL, &targetID, Msg);
	} else {
		Msg = "*** Error: user #" + to_string(targetID + 1) + " does not exist yet. ***\n";
		broadcast(NULL, &servingID, Msg);
	}
}

void yell(string Msg){
	string UserName = Users[servingID].name;
	if (UserName == ""){
		UserName = "(no name)";
	}
	Msg = "*** " + UserName + " yelled ***: " + Msg + "\n";
	broadcast(NULL, NULL, Msg);
}

void name(string newName){
	for (int i=0; i<30; i++){
		if (i == servingID){
			continue;
		}
		if (Users[i].hasUser && Users[i].name == newName){
			string Msg = "*** User '" + newName + "' already exists. ***\n";
			broadcast(NULL, &servingID, Msg);
			return;
		}
	}
	Users[servingID].name = newName;
	string Msg = "*** User from " + Users[servingID].address + " is named '" + Users[servingID].name + "'. ***\n";
	broadcast(NULL, NULL, Msg);
}

void broadcast(int *sourceID, int *targetID, string origin_Msg){
	const char *Msg = origin_Msg.c_str();
	char unit;
	if (targetID == NULL){
		for (int i=0; i<30; i++){
			if (Users[i].hasUser){
				write(Users[i].ssock, Msg, sizeof(unit)*origin_Msg.length());
			}
		}
	} else {
		if (Users[*targetID].hasUser){
			write(Users[*targetID].ssock, Msg, sizeof(unit)*origin_Msg.length());
		} else {
			write(Users[*sourceID].ssock, Msg, sizeof(unit)*origin_Msg.length());
		}
	}
}

void np_shell(int ID){
	servingID = ID;
	if (!Users[servingID].hasUser){
		return;
	}
	setenv("PATH", Users[servingID].path.data(), 1);

	string cmdLine;
	char readbuf[15000];
	vector<cmdBlock> cmdBlocks;
	bool is_first_cmd = true;

	if (Users[servingID].doneServe){
		string Msg = "% ";
		broadcast(NULL, &servingID, Msg);
		Users[servingID].doneServe = false;
	}

	//read cmdLine from serving User
	if (FD_ISSET(Users[servingID].ssock, &readfds)){
		bzero(readbuf, sizeof(readbuf));
		int readCount = read(Users[servingID].ssock, readbuf, sizeof(readbuf));
		if (readCount < 0){
			cerr << servingID << ": read error." << endl;
			return;
		} else {
			cmdLine = readbuf;
			if (cmdLine[cmdLine.length()-1] == '\n'){
				cmdLine = cmdLine.substr(0, cmdLine.length()-1);
				if (cmdLine[cmdLine.length()-1] == '\r'){
					cmdLine = cmdLine.substr(0, cmdLine.length()-1);
				}
			}
		}
	} else {
		return;
	}

	parsePipe(cmdLine, cmdBlocks);

	while (!cmdBlocks.empty()){
		parseCmd(cmdBlocks[0]);
		checkPipeType(cmdBlocks[0]);
		if (!is_first_cmd){
			for (int i=0; i<Users[servingID].pipes.size(); i++){
				if (Users[servingID].pipes[i].count == -1) {
					cmdBlocks[0].fd_in = Users[servingID].pipes[i].fd[0];
					break;
				}
			}
			cmdBlocks[0].has_fd_in = true;
		} else {
			for (int i=0; i<Users[servingID].pipes.size(); i++){
				if (Users[servingID].pipes[i].count == 0){
					cmdBlocks[0].fd_in = Users[servingID].pipes[i].fd[0];
					cmdBlocks[0].has_fd_in = true;
					break;
				}
			}
		}

		if (!exec_cmd(cmdBlocks[0])){
			return;
		}
		cmdBlocks.erase(cmdBlocks.begin());
		is_first_cmd = false;
	}

	for (int i=0; i<Users[servingID].pipes.size(); i++){
		Users[servingID].pipes[i].count--;
	}

	Users[servingID].doneServe = true;
}

void parsePipe(string cmdLine, vector<cmdBlock> &cmdBlocks){
	int front = 0;
	int end;
	cmdBlock cmdBlock;
	cmdLine += "| ";

	while ((end = cmdLine.find(' ', cmdLine.find_first_of("|!", front))) != -1){
		//initial cmdBlock
		cmdBlock.cmdLine = cmdLine.substr(0, cmdLine.length()-2);
		cmdBlock.has_fd_in = false;
		cmdBlock.has_fd_out = false;
		cmdBlock.fd_in = 0;
		cmdBlock.fd_out = 0;
		cmdBlock.pipeType = 0;
		cmdBlock.has_userpipe_send = false;
		cmdBlock.has_userpipe_receive = false;
		cmdBlock.sendID = 0;
		cmdBlock.receiveID = 0;

		//parse cmdLine
		cmdBlock.cmd = cmdLine.substr(front, end-front);
		while(cmdBlock.cmd[0] == ' ') cmdBlock.cmd = (cmdBlock.cmd).substr(1);
		if (end == cmdLine.length()-1) cmdBlock.cmd = (cmdBlock.cmd).substr(0, (cmdBlock.cmd).length()-1);
		while((cmdBlock.cmd)[(cmdBlock.cmd).length()-1] == ' ') cmdBlock.cmd = (cmdBlock.cmd).substr(0, (cmdBlock.cmd).length()-1);
		cmdBlocks.push_back(cmdBlock);

		front = end + 1;
	}
}

void parseCmd(cmdBlock &cmdBlock){
	int front = 0;
	int end;
	cmdBlock.cmd += " ";

	//read arguments
	while ((end = (cmdBlock.cmd).find(" ", front)) != -1){
		if (end == front) {
			front = end + 1;
			continue;
		}
		(cmdBlock.argv).push_back((cmdBlock.cmd).substr(front, end-front));
		front = end + 1;
	}
}

void checkPipeType(cmdBlock &cmdBlock){
	string last_argv = cmdBlock.argv.back();
	Pipe newPipe;
	newPipe.count = 0;
	if (regex_match(last_argv, reg_numberPipe)){    //number pipe case
		if (last_argv[0] == '|'){
			cmdBlock.pipeType = 1;
		} else if (last_argv[0] == '!'){
			cmdBlock.pipeType = 2;
		}
		newPipe.count = stoi(last_argv.substr(1));
		bool merge_numberpipe = false;
		for (int i=0; i<Users[servingID].pipes.size(); i++){
			if (newPipe.count == Users[servingID].pipes[i].count){
				cmdBlock.fd_out = Users[servingID].pipes[i].fd[1];
				merge_numberpipe = true;
				break;
			}
		}
		if (!merge_numberpipe){
			pipe(newPipe.fd);
			cmdBlock.fd_out = newPipe.fd[1];
			Users[servingID].pipes.push_back(newPipe);
		}
	}else {
		if (last_argv == "|"){		     //normal pipe case
			cmdBlock.pipeType = 1;
		} else if (last_argv == "!"){    //error pipe case
			cmdBlock.pipeType = 2;
		} else {						 //no pipe case
			cmdBlock.pipeType = 0;
		}
		if (cmdBlock.pipeType != 0){
			newPipe.count = -1;
			pipe(newPipe.fd);
			cmdBlock.fd_out = newPipe.fd[1];
			Users[servingID].pipes.push_back(newPipe);
		}
	}
	if (cmdBlock.pipeType != 0){
		cmdBlock.has_fd_out = true;
	}

	// handle userpipe
	if (regex_match(last_argv, reg_userPipe_send)){	//User pipe ">" case
		cmdBlock.has_userpipe_send = true;
		cmdBlock.sendID = stoi(last_argv.substr(1));
	} else if (regex_match(cmdBlock.argv[cmdBlock.argv.size()-2], reg_userPipe_send)){
		cmdBlock.has_userpipe_send = true;
		cmdBlock.sendID = stoi(cmdBlock.argv[cmdBlock.argv.size()-2].substr(1));
	}
	if (regex_match(last_argv, reg_userPipe_receive)){	//User pipe "<" case
		cmdBlock.has_userpipe_receive = true;
		cmdBlock.receiveID = stoi(last_argv.substr(1));
	} else if (regex_match(cmdBlock.argv[cmdBlock.argv.size()-2], reg_userPipe_receive)){
		cmdBlock.has_userpipe_receive = true;
		cmdBlock.receiveID = stoi(cmdBlock.argv[cmdBlock.argv.size()-2].substr(1));
	}
	if (cmdBlock.has_userpipe_send){
		cmdBock.argv.erase(cmdBock.argv.end());
	}
}

bool exec_cmd(cmdBlock &cmdBlock){
	char *argv[1000];
	string cmd = cmdBlock.argv[0];
	int child_pid;

	if (cmd == "printenv"){
		if (getenv(cmdBlock.argv[1].data()) != NULL)
			cout << getenv(cmdBlock.argv[1].data()) << endl;
	} else if (cmd == "setenv"){
		Users[servingID].path = cmdBlock.argv[2];
		setenv(cmdBlock.argv[1].data(), cmdBlock.argv[2].data(), 1);
	} else if (cmd == "who"){
		who();
	} else if (cmd == "tell"){
		string Msg = cmdBlock.cmd.substr(cmdBlock.cmd.find(cmdBlock.argv[2]));
		tell(stoi(cmdBlock.argv[1])-1, Msg);
	} else if (cmd == "yell"){
		string Msg = cmdBlock.cmd.substr(cmdBlock.cmd.find(cmdBlock.argv[1]));
		yell(Msg);
	} else if (cmd == "name"){
		name(cmdBlock.argv[1]);
	} else if (cmd == "exit" || cmd == "EOF"){
		show_logoutMsg(servingID);
		FD_CLR(Users[servingID].ssock, &activefds);
		close(Users[servingID].ssock);
		init_UserData(servingID);
		return false;
	} else {
		int status;
		while((child_pid = fork()) < 0){
			while(waitpid(-1, &status, WNOHANG) > 0){

			};
		}
		switch (child_pid){
			case 0 :
				if (cmdBlock.has_fd_in){
					dup2(cmdBlock.fd_in, STDIN_FILENO);
				}
				if (cmdBlock.has_fd_out){
					if (cmdBlock.pipeType == 1){
						dup2(cmdBlock.fd_out, STDOUT_FILENO);
					} else if (cmdBlock.pipeType == 2){
						dup2(cmdBlock.fd_out, STDOUT_FILENO);
						dup2(cmdBlock.fd_out, STDERR_FILENO);
					}
				} else {
					dup2(Users[servingID].ssock, STDOUT_FILENO);
					dup2(Users[servingID].ssock, STDERR_FILENO);
				}

				//child closes all pipe fds
				for (int i=0; i<Users[servingID].pipes.size(); i++){
					close(Users[servingID].pipes[i].fd[0]);
					close(Users[servingID].pipes[i].fd[1]);
				}
				for (int i=0; i<UserPipes.size(); i++){
					close(UserPipes[i].fd[0]);
					close(UserPipes[i].fd[1]);
				}

				// file redirection
				int fd_redirection, index;
				if ((index = findIndex(cmdBlock.argv, ">")) == -1){
					if (cmdBlock.pipeType == 0)
						vec2arr(cmdBlock.argv, argv, (cmdBlock.argv).size());
					else
						vec2arr(cmdBlock.argv, argv, (cmdBlock.argv).size()-1);
				} else {
					fd_redirection = open((cmdBlock.argv).back().data(), O_RDWR | O_CREAT | O_TRUNC | O_CLOEXEC, 0600);
					dup2(fd_redirection, STDOUT_FILENO);
					dup2(fd_redirection, STDERR_FILENO);
					close(fd_redirection);
					vec2arr(cmdBlock.argv, argv, index);
				}
				if (execvp(cmd.data(), argv) == -1)
					cerr << "Unknown command: [" << cmd << "]." << endl;
				exit(0);
			default :
				//parent close useless pipe fd
				if (cmdBlock.has_fd_in){
					for (int i=0; i<Users[servingID].pipes.size(); i++){
						if (Users[servingID].pipes[i].fd[0] == cmdBlock.fd_in){
							close(Users[servingID].pipes[i].fd[0]);
							close(Users[servingID].pipes[i].fd[1]);
							Users[servingID].pipes.erase(Users[servingID].pipes.begin()+i);
							break;
						}
					}
				}
				if (cmdBlock.pipeType == 0){
					waitpid(child_pid, &status, 0);
				} else {
					waitpid(-1, &status, WNOHANG);
				}
		}
	}
	return true;
}

void vec2arr(vector<string> &vec, char *arr[], int index){
	for (int i=0; i<index; i++){
		arr[i] = (char*)vec[i].data();
	}
	arr[vec.size()] = NULL;
}

int findIndex(vector<string> &vec, string str){
	int index;
	for (index=0; index<vec.size(); index++){
		if (vec[index] == str){
			break;
		}
	}
	if (index == vec.size()){
		index = -1;
	}
	return index;
}

int findMax(int msock){
	int maxValue = msock;
	for (int i=0; i<30; i++){
		if (Users[i].ssock > maxValue){
			maxValue = Users[i].ssock;
		}
	}
	return maxValue + 1;
}