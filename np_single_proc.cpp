#include "np_single_proc.h"

static regex reg_numberPipe("[|!][0-9]+");
static regex reg_userPipe_send("[>][0-9]+");
static regex reg_userPipe_receive("[<][0-9]+");

fd_set activefds, readfds;
User Users[30];
int servingID;
vector<UserPipe> UserPipes;
const int FD_NULL = open("/dev/null", O_RDWR);

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
	servingID = 0;
	UserPipes.clear();
	clearenv();

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
	Users[index].envVals.clear();
	envVal new_envVal;
	new_envVal.name = "PATH";
	new_envVal.value = "bin:.";
	Users[index].envVals.push_back(new_envVal);
	for (int i=0; i<Users[index].pipes.size(); i++){
		close(Users[index].pipes[i].fd[0]);
		close(Users[index].pipes[i].fd[1]);
	}
	Users[index].pipes.clear();
	for (int i=0; i<UserPipes.size(); i++){
		if (UserPipes[i].sourceID == index || UserPipes[i].targetID == index){
			close(UserPipes[i].fd[0]);
			close(UserPipes[i].fd[1]);
			UserPipes.erase(UserPipes.begin()+i);
		}
	}
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
			// cout << "accept success." << endl;
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
	Msg = Msg + "****************************************\n"
			  + "** Welcome to the information server. **\n"
			  + "****************************************\n";
	broadcast(NULL, &ID, Msg);
}

void show_loginMsg(int ID){
	string Msg = "*** User '" + getUserName(Users[ID].name) + "' entered from " + Users[ID].address + ". ***\n";
	// cout << Msg << endl;
	broadcast(NULL, NULL, Msg);
}

void show_logoutMsg(int ID){
	string Msg = "*** User '" + getUserName(Users[ID].name) + "' left. ***\n";
	// cout << Msg << endl;
	broadcast(NULL, NULL, Msg);
}

void who(){
	string Msg = "<ID>\t<nickname>\t<IP:port>\t<indicate me>\n";
	for (int i=0; i<30; i++){
		if (Users[i].hasUser){
			Msg += to_string(Users[i].ID + 1) + "\t" + getUserName(Users[i].name) + "\t" + Users[i].address;
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
		Msg = "*** " + getUserName(Users[servingID].name) + " told you ***: " + Msg + "\n";
		broadcast(NULL, &targetID, Msg);
	} else {
		Msg = "*** Error: user #" + to_string(targetID + 1) + " does not exist yet. ***\n";
		broadcast(NULL, &servingID, Msg);
	}
}

void yell(string Msg){
	Msg = "*** " + getUserName(Users[servingID].name) + " yelled ***: " + Msg + "\n";
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
		write(Users[*targetID].ssock, Msg, sizeof(unit)*origin_Msg.length());
	}
}

//check and create user pipe, then set the fds in cmdBlock
void handleUserPipe(cmdBlock &cmdBlock){
	int index;

	if (cmdBlock.has_userpipe_receive){
		if (cmdBlock.receive_from_ID < 0 || cmdBlock.receive_from_ID > 29 || !Users[cmdBlock.receive_from_ID].hasUser){
			string Msg = "*** Error: user #" + to_string(cmdBlock.receive_from_ID + 1) + " does not exist yet. ***\n";
			broadcast(NULL, &servingID, Msg);
			cmdBlock.has_fd_in = true;
			cmdBlock.fd_in = FD_NULL;
		} else {
			if (checkUserPipeExist(index, cmdBlock.receive_from_ID, servingID)){
				string Msg = "*** " + getUserName(Users[servingID].name) + " (#" + to_string(servingID + 1) + ") just received from "
							+ getUserName(Users[cmdBlock.receive_from_ID].name) + " (#" + to_string(cmdBlock.receive_from_ID + 1) + ") by '" + cmdBlock.cmdLine + "' ***\n";
				broadcast(NULL, NULL, Msg);
				cmdBlock.has_fd_in = true;
				cmdBlock.fd_in = UserPipes[index].fd[0];
			} else {
				string Msg = "*** Error: the pipe #" + to_string(cmdBlock.receive_from_ID + 1) + "->#" + to_string(servingID + 1) + " does not exist yet. ***\n";
				broadcast(NULL, &servingID, Msg);
				cmdBlock.has_fd_in = true;
				cmdBlock.fd_in = FD_NULL;
			}
		}
	}
	if (cmdBlock.has_userpipe_send){
		if (cmdBlock.send_to_ID < 0 || cmdBlock.send_to_ID > 29 || !Users[cmdBlock.send_to_ID].hasUser){
			string Msg = "*** Error: user #" + to_string(cmdBlock.send_to_ID + 1) + " does not exist yet. ***\n";
			broadcast(NULL, &servingID, Msg);
			cmdBlock.has_fd_out = true;
			cmdBlock.fd_out = FD_NULL;
		} else {
			if (checkUserPipeExist(index, servingID, cmdBlock.send_to_ID)){
				string Msg = "*** Error: the pipe #" + to_string(servingID + 1) + "->#" + to_string(cmdBlock.send_to_ID + 1) + " already exists. ***\n";
				broadcast(NULL, &servingID, Msg);
				cmdBlock.has_fd_out = true;
				cmdBlock.fd_out = FD_NULL;
			} else {
				string Msg = "*** " + getUserName(Users[servingID].name) + " (#" + to_string(servingID + 1) + ") just piped '" + cmdBlock.cmdLine + "' to "
							+ getUserName(Users[cmdBlock.send_to_ID].name) + " (#" + to_string(cmdBlock.send_to_ID + 1) + ") ***\n";
				broadcast(NULL, NULL, Msg);
				UserPipe newUserPipe;
				newUserPipe.sourceID = servingID;
				newUserPipe.targetID = cmdBlock.send_to_ID;
				pipe(newUserPipe.fd);
				UserPipes.push_back(newUserPipe);
				cmdBlock.has_fd_out = true;
				cmdBlock.fd_out = newUserPipe.fd[1];
			}
		}
	}
}

bool checkUserPipeExist(int &index, int sourceID, int targetID){
	bool IsExist = false;
	for (int i=0; i<UserPipes.size(); i++){
		if (UserPipes[i].sourceID == sourceID && UserPipes[i].targetID == targetID){
			index = i;
			IsExist = true;
			break;
		}
	}
	return IsExist;
}

string getUserName(string UserName){
	if (UserName == "") return "(no name)";
	else return UserName;
}

void np_shell(int ID){
	servingID = ID;
	if (!Users[servingID].hasUser){
		return;
	}
	clearenv();
	for (int i=0; i<Users[servingID].envVals.size(); i++){
		setenv(Users[servingID].envVals[i].name.data(), Users[servingID].envVals[i].value.data(), 1);
	}

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
	if (cmdLine.length() == 0){
		Users[servingID].doneServe = true;
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

		//check and create user pipe, then set the fds in cmdBlock
		handleUserPipe(cmdBlocks[0]);

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
		cmdBlock.send_to_ID = 0;
		cmdBlock.receive_from_ID = 0;

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

	if (cmdBlock.argv.size() == 1){
		return;
	}

	// handle userpipe
	// User pipe ">" case (parse only)
	if (regex_match(last_argv, reg_userPipe_send)){
		cmdBlock.has_userpipe_send = true;
		cmdBlock.send_to_ID = stoi(last_argv.substr(1)) - 1;
	} else if (regex_match(cmdBlock.argv[cmdBlock.argv.size()-2], reg_userPipe_send)){
		cmdBlock.has_userpipe_send = true;
		cmdBlock.send_to_ID = stoi(cmdBlock.argv[cmdBlock.argv.size()-2].substr(1)) - 1;
	}

	//User pipe "<" case (parse only)
	if (regex_match(last_argv, reg_userPipe_receive)){
		cmdBlock.has_userpipe_receive = true;
		cmdBlock.receive_from_ID = stoi(last_argv.substr(1)) - 1;
	} else if (regex_match(cmdBlock.argv[cmdBlock.argv.size()-2], reg_userPipe_receive)){
		cmdBlock.has_userpipe_receive = true;
		cmdBlock.receive_from_ID = stoi(cmdBlock.argv[cmdBlock.argv.size()-2].substr(1)) - 1;
	}
	if (cmdBlock.has_userpipe_send){
		cmdBlock.argv.erase(cmdBlock.argv.end());
	}
	if (cmdBlock.has_userpipe_receive){
		cmdBlock.argv.erase(cmdBlock.argv.end());
	}
}

bool exec_cmd(cmdBlock &cmdBlock){
	char *argv[1000];
	string cmd = cmdBlock.argv[0];
	int child_pid, status;

	if (cmd == "printenv"){
		if (getenv(cmdBlock.argv[1].data()) != NULL){
			string Msg = getenv(cmdBlock.argv[1].data());
			broadcast(NULL, &servingID, Msg + "\n");
		}
	} else if (cmd == "setenv"){
		envVal new_envVal;
		new_envVal.name = cmdBlock.argv[1];
		new_envVal.value = cmdBlock.argv[2];
		Users[servingID].envVals.push_back(new_envVal);
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
		while(waitpid(-1, &status, WNOHANG) > 0){}
		return false;
	} else {
		while((child_pid = fork()) < 0){
			while(waitpid(-1, &status, WNOHANG) > 0){}
		}
		switch (child_pid){
			case 0 :
				if (cmdBlock.has_fd_in){
					dup2(cmdBlock.fd_in, STDIN_FILENO);
				}
				if (cmdBlock.has_fd_out){
					if (cmdBlock.pipeType == 1){
						dup2(cmdBlock.fd_out, STDOUT_FILENO);
						dup2(Users[servingID].ssock, STDERR_FILENO);
					} else if (cmdBlock.pipeType == 2){
						dup2(cmdBlock.fd_out, STDOUT_FILENO);
						dup2(cmdBlock.fd_out, STDERR_FILENO);
					} else {
						dup2(cmdBlock.fd_out, STDOUT_FILENO);
						dup2(Users[servingID].ssock, STDERR_FILENO);
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
				if (((index = findIndex(cmdBlock.argv, ">")) == -1) || cmdBlock.has_userpipe_send){
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
					for (int i=0; i<UserPipes.size(); i++){
						if (UserPipes[i].fd[0] == cmdBlock.fd_in){
							close(UserPipes[i].fd[0]);
							close(UserPipes[i].fd[1]);
							UserPipes.erase(UserPipes.begin()+i);
							break;
						}
					}
				}
				if (cmdBlock.has_fd_out){
					waitpid(-1, &status, WNOHANG);
				} else {
					waitpid(child_pid, &status, 0);
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