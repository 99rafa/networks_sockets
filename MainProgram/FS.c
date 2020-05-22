#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>

#define BUFFER_SIZE 8192


int fdUDP, fdTCP, newfd;
fd_set rset;
ssize_t n;
socklen_t addrlen;
struct addrinfo hints, *res, hintsTCP, *resTCP;
struct sockaddr_in addr;
char recvBuffer[BUFFER_SIZE], sendBuffer[BUFFER_SIZE];
char activeTopic[11];

void validate_user(char* userID, char *response){
	if(strlen(userID) == 5)
		strcpy(response, "OK");
	else
		strcpy(response, "NOK");
}

int createTopicDir(char *topic) {
	char subDir[20] = "Topics/";

	if(opendir(subDir) == NULL) mkdir(subDir, 0700);

	strcat(subDir, topic);

	if(mkdir(subDir,0700) == -1) {
		return 1;
	}

	return 0;
}

int getQuestionsByTopic(char *topic, char *questions) {
	int firstDir = 1, nAnswers = 0;
	struct dirent *de, *qd;
	struct stat s;
	char buffer[6];
	char *subDir = (char*) malloc (21*sizeof(char));
	memset(subDir, 0, 21*sizeof(char));
	char *subDirID = (char*) malloc (56*sizeof(char));
	memset(subDirID, 0, 56*sizeof(char));
	char *questionSubDir = (char*) malloc (50*sizeof(char));
	memset(questionSubDir, 0, 50*sizeof(char));
	char *answerDir = (char*) malloc (68*sizeof(char));
	memset(answerDir, 0, 68*sizeof(char));

	bzero(questions, BUFFER_SIZE);

   	strcpy(subDir,"Topics/");
   	strcat(subDir,topic );
   	DIR *dr = opendir(subDir), *questionDir;
	if (dr == NULL) {  // opendir returns NULL if couldn't open directory
		free(subDir);
		free(subDirID);
		free(questionSubDir);
		free(answerDir);
		return 0;
   }

	while ((de = readdir(dr)) != NULL) {
	   if (strcmp(de->d_name, "..") != 0 && strcmp(de->d_name, ".") != 0  ) {

			strcpy(questionSubDir, subDir);
			sprintf(questionSubDir, "%s%c", questionSubDir, '/');
			strcat(questionSubDir,de->d_name);
			questionDir = opendir(questionSubDir);
			if (questionDir != NULL) {

				while ((qd = readdir(questionDir)) != NULL) {
					strcpy(answerDir, questionSubDir);
					strcat(answerDir, "/");
					strcat(answerDir, qd->d_name);
					if (strcmp(qd->d_name, "..") != 0 && strcmp(qd->d_name, ".") != 0  ) {
						if ( stat(answerDir,&s) == 0 ) {
							if( s.st_mode & S_IFDIR ) {
								nAnswers++;
							}
						}
					}
				}
			if (firstDir) {

	 		   firstDir = 0;
	 		   strcpy(questions, de->d_name);

	 		}
	 		else {

	 			sprintf(questions, "%s%c", questions, ' ');
	 			strcat(questions, de->d_name);
	 		}

			strcpy(subDirID,subDir);
			strcat(subDirID, "/");
			strcat(subDirID, de->d_name);
			strcat(subDirID, "/");
			strcat(subDirID, de->d_name);
			strcat(subDirID,"_UID.txt");
			FILE *userFile = fopen(subDirID, "r");
			if (userFile == NULL) printf("erro\n");
			strcat(questions, ":");
			fgets(buffer, 6,userFile);
			fclose(userFile);
			strcat(questions,buffer);


			sprintf(questions, "%s%c%d", questions, ':', nAnswers);

	   	}
	   	closedir(questionDir);
	   	nAnswers = 0;
   		}
	}

   	closedir(dr);
	free(subDir);
	free(questionSubDir);
	free(subDirID);
	free(answerDir);
	return 1;

}

int checkForExistingTopic(char *topic, char *userID) {
	FILE *topicList, *fileExists;
	char *buffer = (char*) malloc (BUFFER_SIZE*sizeof(char));
	memset(buffer, 0, BUFFER_SIZE*sizeof(char));
	char *topicInfo = (char*) malloc (17*sizeof(char));
	memset(topicInfo, 0, 17*sizeof(char));
	char *singleTopic,  space[] = " " ;
	int i = 0, topicCount = 0;
	fileExists = fopen("topicList.txt", "r");
	topicList = fopen("topicList.txt", "a+");

	strcpy(topicInfo,topic);
	sprintf(topicInfo, "%s%c", topicInfo, ':');
	strcat(topicInfo, userID);

	if (fileExists) {

		fgets(buffer,BUFFER_SIZE, topicList);
		singleTopic = strtok(buffer,space );
		topicCount++;
		while (singleTopic !=NULL) {
			while (singleTopic[i] == topic[i]) {
				i++;
			}
			if (topic[i] =='\0') {
				free(buffer);
				free(topicInfo);
				fclose(fileExists);
				fclose(topicList);
				return 2;
			}
			singleTopic = strtok(NULL,space );
			topicCount++;
			i = 0;
		}
		if (topicCount < 99) {
			if (createTopicDir(topic) == 1) return 0;
			fprintf(topicList, " %s", topicInfo );
			free(buffer);
			free(topicInfo);
			fclose(fileExists);
			fclose(topicList);
			return 1;
		}
		else {
			free(buffer);
			free(topicInfo);
			fclose(fileExists);
			fclose(topicList);
			return 3;
		}
	}
	else {
		if (createTopicDir(topic) == 1) return 0;
		fprintf(topicList, "%s", topicInfo );
		free(buffer);
		free(topicInfo);
		fclose(topicList);

		return 1;
	}

}

int getOpenedTopics(char * availableTopics) {
	FILE *topicList;
	char *buffer = (char*) (char*) malloc (BUFFER_SIZE*sizeof(char));
	memset(buffer,0, sizeof(char)*BUFFER_SIZE);
	topicList = fopen("topicList.txt", "r");
	if (topicList) {
		while (fgets(buffer, BUFFER_SIZE,topicList)) {
			strcpy(availableTopics,buffer);
		}
		free(buffer);
		fclose(topicList);
		return 1;
	}
	else {
		free(buffer);
		
		return 0;
	}
}


int topicCounter( char *availableTopics) {
	int counter = 1,i=0;
	while (availableTopics[i] != '\0') {
		if (availableTopics[i] == ' ')
		counter++;
		i++;
	}
	return counter;
}



int isKnownUser(int userID) {

	FILE *usersList;
	char *buff = (char*) malloc(BUFFER_SIZE*sizeof(char));
	memset(buff,0, BUFFER_SIZE*sizeof(char));
	int possible_match, res = 0;


	usersList = fopen("authenticated_users/authenticated_list.txt", "r");
	if (usersList) {
		while (fgets(buff,BUFFER_SIZE, usersList)) {
			possible_match = atoi(buff);
			if (possible_match == userID){ res = 1; break; }
		}
		fclose(usersList);
	}
	else {
		perror("Error on opening file.");
	}
	free(buff);
	return res;
}

int getAnswerNumber(char *path) {

	int nAnswers = 0;
	struct dirent *de;
	DIR *nd;
	char destination[50];


	DIR *dr = opendir(path);

	if (dr == NULL) {  // opendir returns NULL if couldn't open directory
		return 0;
 	}



	while ((de = readdir(dr)) != NULL) {

		if (strcmp(de->d_name, "..") != 0 && strcmp(de->d_name, ".") != 0  ) {

			strcpy(destination,path);

			strcat(destination, "/");
			strcat(destination, de->d_name);

			nd = opendir(destination);
			if (nd) nAnswers++;
			closedir(nd);

		}
		memset(&destination, 0, sizeof(destination));

	 }
	 closedir(dr);

	return nAnswers;
}

//flag :1-> create question dir
//		2-> create answer dir
int createSubDir(char *path, char *subName, char* id, int flag)  {
	int dirFound = 0, dirFull = 0, subDirCount = 0, nAnswers;
	char *newPath = (char*) malloc(50*sizeof(char));
	memset(newPath, 0, 50*sizeof(char));
	char *userFile;
	struct dirent *de;
	DIR *dr = opendir(path);
	if (dr == NULL) {  // opendir returns NULL if couldn't open directory
		return 0;
 	}

	if (flag == 1) {


		while ((de = readdir(dr)) != NULL) {
		if (strcmp(de->d_name, "..") != 0 && strcmp(de->d_name, ".") != 0  ) {

			if (strcmp(de->d_name, subName) == 0) {
				dirFound = 1;

				break;
			}
			subDirCount++;
			if (subDirCount == 99) {

				dirFull = 1;
				break;
			}
			}
		}
		if (dirFound) {
			free(newPath);
			return 2;
		}
		else if (dirFull){
			free(newPath);
			 return 3;
		 }
	 }
	if (flag == 2) {
		nAnswers = getAnswerNumber(path);

		if (nAnswers == 99) {
			free(newPath);
			return 3;
		}
	}


	strcpy(newPath,path);
	strcat(newPath, "/");
	strcat(newPath, subName);
	if (flag == 2) {
		strcat(newPath, "_");
		if (nAnswers+1 < 10) strcat(newPath, "0");
		sprintf(newPath, "%s%d", newPath,nAnswers+1);
	}

	if(mkdir(newPath,0700) == -1) {
		free(newPath);
		return 0;
	}
	if (flag == 2) {
			userFile = (char*) malloc((strlen(newPath)+strlen(subName)+13)*sizeof(char));
			memset(userFile, 0, (strlen(newPath)+strlen(subName)+13)*sizeof(char));
	}
	else {
		userFile = (char*) malloc((strlen(newPath)+strlen(subName)+10)*sizeof(char));
		memset(userFile, 0, (strlen(newPath)+strlen(subName)+10)*sizeof(char));
	}
	strcpy(userFile, newPath);
	strcat(userFile, "/");
	strcat(userFile, subName);
	if (flag == 2) {
		strcat(userFile, "_");
		if (nAnswers+1 < 10) strcat(userFile, "0");
		sprintf(userFile, "%s%d", userFile,nAnswers+1);
	}
	strcat(userFile, "_UID.txt");


	FILE *user = fopen(userFile, "w");

	int error = flock(fileno(user), LOCK_EX);
	if(error == -1){ perror("Error on flock in locking at createSubDir()."); exit(-1); }

	if (user != NULL) {

		fprintf(user, "%s", id);
		error = flock(fileno(user), LOCK_UN);
		if(error == -1){ perror("Error on flock in unlocking at createSubDir()."); exit(-1); }
		fclose(user);
	}
	free(newPath);
	free(userFile);
	return 1;
}

int max(int x, int y)
{
    if (x > y)
        return x;
    else
        return y;
}

void getImgFile(char *path, char *imgFile) {
	struct stat s;
	struct dirent *de;  // Pointer for directory entry
	char *ext, name[11], newPath[70], point[] = ".", end[] = "\0";
    // opendir() returns a pointer of DIR type.
    DIR *dr = opendir(path);

    if (dr == NULL)  // opendir returns NULL if couldn't open directory
    {
        printf("Could not open current directory" );
		memset(imgFile, 0, 70*sizeof(char));
        strcpy(imgFile, "-1");
		return;
    }

    while ((de = readdir(dr)) != NULL) {
		if (strcmp(de->d_name, "..") != 0 && strcmp(de->d_name, ".") != 0  ) {
			strcpy(newPath, path);
			strcat(newPath, "/");
			strcat(newPath, de->d_name);

			if ( stat(newPath,&s) == 0 ) {
				if( s.st_mode & S_IFREG ) {

					strcpy(name, de->d_name);
					strtok(name, point);
					ext = strtok(NULL, end);

					if (strcmp(ext, "txt") != 0) {
						//if it is not one of the 2 other ones and it is not a directory, it is the img file

						strcat(imgFile,de->d_name);
						closedir(dr);
						return;
					}

				}
			}
			memset(newPath, 0, strlen(newPath));
		}
	}


    closedir(dr);
	memset(imgFile, 0, 70*sizeof(char));
	strcpy(imgFile, "-1");
}

void readToBuffer(char *buffer, int nBytes) {
	int i = nBytes;
	while (i > 0) {
		memset(recvBuffer, 0, sizeof(recvBuffer));
		n = read(newfd, recvBuffer,i);
		if (n == -1) exit(1);
		i -=n;
		if (n > 0)strcat(buffer, recvBuffer);
	}
}

void writeToBuffer(char *buffer, int nBytes) {
	int i = nBytes;
	while (i > 0) {
		n = write(newfd, buffer,i);
		if (n == -1) exit(1);
		i -=n;
	}
}

//Check Arguments
int checkUserIDArg(char* userID){
	//CUIDADO COM O \n
	return strlen(userID) == 5 ? 1 : 0;
}

int checkTopicArg(char* topic){
	//max 10 letters
	return strlen(topic) >= 10 ? 1 : 0;

}

int checkQuestionTitleArg(char* qTitle){
	//max 10 letters
	return strlen(qTitle) >= 10 ? 1 : 0;
}

int checkQuestionSizeArg(long qSize){
	//max 10 bytes
	return qSize <= 10 ? 1 : 0;

}

int checkQuestionDataArg(char* qData){
	//max 10 lettrs
	return strlen(qData) >= 10 ? 1 : 0;
}

int main(int argc, char **argv) {

	int invalid_flag = 1,i, userID, maxfdp1, n_topics, topicStatus, pid, topicCount = 0;
	long option_index;
	char *sq;
	char *IP = NULL;
	char *PORT = NULL;
	char *availableTopics = (char*) malloc (BUFFER_SIZE*sizeof(char));
	memset(availableTopics,0,BUFFER_SIZE*sizeof(char));
	char *proposedTopic = (char*) malloc (11*sizeof(char));
	memset(proposedTopic,0, 11*sizeof(char));
	char *id = (char*) malloc (6*sizeof(char)); //estava a 5, pode dar erro
	memset(id,0,6*sizeof(char));
	char *activeTopicQuestions =  (char*) malloc (BUFFER_SIZE*sizeof(char));
	memset(activeTopicQuestions,0, BUFFER_SIZE*sizeof(char));
	memset(recvBuffer,0, sizeof(recvBuffer));
	memset(sendBuffer,0, sizeof(sendBuffer));
	memset(activeTopic,0, sizeof(activeTopic));
	char delim[] = " ",endOfLine[] = "\n", topicsAlg[2];
	opterr = 0;

	while ((option_index = getopt(argc,argv, "p:")) !=-1){
		switch(option_index) {
			case 'p': {
				PORT = optarg;
				invalid_flag = 0;
			}
		}
	}

	if (invalid_flag && argc > 1) {
		fprintf(stderr,"Invalid argument.\n");
		exit(EXIT_FAILURE);
	}
	if (!PORT) {
		//58000 + Group Number
		PORT = "58058"; //58
	}

	memset(&hints, 0, sizeof(hints));

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE|AI_NUMERICSERV;

	hintsTCP.ai_family = AF_INET;
	hintsTCP.ai_socktype = SOCK_STREAM;
	hintsTCP.ai_flags = AI_PASSIVE|AI_NUMERICSERV;

	n = getaddrinfo(NULL, PORT, &hintsTCP, &resTCP);
	if (n != 0) exit(1);

	fdTCP = socket(resTCP->ai_family, resTCP->ai_socktype, resTCP->ai_protocol);

	if (fdTCP == -1) exit(1);

	if (setsockopt(fdTCP, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) == -1)
    perror("setsockopt(SO_REUSEADDR) failed");

	n = bind(fdTCP, resTCP->ai_addr, resTCP->ai_addrlen);
    if( n == -1) {
        printf("bind error");
        exit(1);
    }

	if ( listen(fdTCP,5) == -1) {
        printf("lis ten error");
        exit(1);
    }

	n = getaddrinfo(NULL, PORT, &hints, &res);
	if (n != 0) exit(1);

	fdUDP = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (fdUDP==-1) exit(1);


	n = bind(fdUDP, res->ai_addr, res->ai_addrlen);
	if (n==-1) exit(1);

	FD_ZERO(&rset);

	maxfdp1 = max(fdTCP, fdUDP) + 1;

	while(1) {

		FD_SET(fdTCP, &rset);
        FD_SET(fdUDP, &rset);

		n = select(maxfdp1, &rset, NULL, NULL, NULL);
		if ( n == -1) perror("Select error\n");
		memset(recvBuffer, 0, sizeof(recvBuffer));

	 	if (FD_ISSET(fdTCP, &rset)) {

			addrlen = sizeof(addr);

			if((newfd = accept(fdTCP, (struct sockaddr*)&addr, &addrlen))==-1){
		        printf("accept error");
		        exit(1);
		    }

			pid = fork();

			if (pid < 0) {
				perror("Failed to create new process.");
				exit(EXIT_FAILURE);
			}

			if (pid > 0) {
				//parent process
				continue;
			} 
			else {
				//child process
				FILE *fp;
				char delim[] = " ";
				char * requestCode  = (char*) malloc(5*sizeof(char));
				memset(requestCode,0, sizeof(char)*5);
				char *space = (char*) malloc(2*sizeof(char));

				readToBuffer(requestCode,4);
				requestCode[strlen(requestCode)] = '\0';

				printf("Received request: ");

				IP = inet_ntoa(addr.sin_addr);

				if (strncmp(requestCode, "GQU ",3) == 0) {

					char* clientArg = strtok(requestCode, "GQU");

					if(strcmp(clientArg, " ") == 0){
						char *questionPath = (char*) malloc(33*sizeof(char));
						memset(questionPath, 0, 33*sizeof(char));
						memset(recvBuffer, 0, sizeof(recvBuffer));
						char * topic = (char*) malloc(12*sizeof(char));
						memset(topic, 0, 12*sizeof(char));
						char *qiext;
						char *qTitle = (char*) malloc(12*sizeof(char));
						memset(qTitle, 0, 12*sizeof(char));

						readToBuffer(topic, 1);
						if (strcmp(topic, " ") == 0) {
							//In case there are more spaces than desired
							memset(sendBuffer,0, sizeof(sendBuffer));
							strcpy(sendBuffer, "QGR ERR\n");
							//read(newfd,recvBuffer,1);
							writeToBuffer(sendBuffer, strlen(sendBuffer));

							//printf("err\n");
						
						} 
						else {
							while (strcmp(recvBuffer, " ") != 0 && strcmp(recvBuffer, "\n") != 0 ) {
								memset(recvBuffer, 0, sizeof(recvBuffer));

								n = read(newfd, recvBuffer,1 );
								if (n == -1) exit(1);
								if( n > 0) strcat(topic, recvBuffer);
							}
							topic[strlen(topic)] = '\0';
							strtok(topic, delim);

							memset(recvBuffer, 0, sizeof(recvBuffer));
							

							readToBuffer(qTitle, 1);
							if (strcmp(qTitle, " ") == 0) {
								//In case there are more spaces than desired
								memset(sendBuffer,0, sizeof(sendBuffer));
								strcpy(sendBuffer, "QGR ERR\n");
								//read(newfd,recvBuffer,1);
								writeToBuffer(sendBuffer, strlen(sendBuffer));

								printf("err\n");
							
							} else {
								while (strcmp(recvBuffer, "\n") != 0) {
									memset(recvBuffer, 0, sizeof(recvBuffer));
									n = read(newfd, recvBuffer,1 );
									if (n == -1) exit(1);
									if ( n > 0) strcat(qTitle, recvBuffer);
								}
								qTitle[strlen(qTitle)] = '\0';
								strtok(qTitle, endOfLine);
								printf("Send question files: %s/%s\n", topic,qTitle);

								memset(questionPath,0, strlen(questionPath));

								strcpy(questionPath, "Topics/");
								strcat(questionPath, topic);
								strcat(questionPath, "/");
								strcat(questionPath, qTitle);

								memset(sendBuffer,0, sizeof(sendBuffer));

								char userFile[30];
								memset(userFile, 0, sizeof(userFile));

								strcpy(userFile, questionPath);
								strcat(userFile, "/");
								strcat(userFile, qTitle);
								strcat(userFile, "_UID.txt");
								
								//userFile[32] = 'X';
								if (access(userFile, F_OK) == -1) {
									printf("Err: No such topic/question\n");
									memset(sendBuffer,0, sizeof(sendBuffer));
									strcpy(sendBuffer, "QGR EOF\n");
									//read(newfd,recvBuffer,1);
									writeToBuffer(sendBuffer, strlen(sendBuffer));
								} else {
									FILE *idFile = fopen(userFile, "r");
									int error = flock(fileno(idFile), LOCK_EX);
									if(error == -1){ perror("Error on flock in unlocking at createSubDir()."); exit(-1); }

									fgets(id, 6,idFile);
									error = flock(fileno(idFile), LOCK_UN);
									if(error == -1){ perror("Error on flock in unlocking at createSubDir()."); exit(-1); }
									fclose(idFile);

									char infoFile[70];
									memset(infoFile, 0, sizeof(infoFile));

									strcpy(infoFile, questionPath);
									strcat(infoFile, "/");
									strcat(infoFile, qTitle);
									strcat(infoFile, ".txt");
									FILE *fileData = fopen(infoFile, "r");
									error = flock(fileno(fileData), LOCK_EX);
									if(error == -1){ perror("Error on flock in unlocking at createSubDir()."); exit(-1); }


									fseek(fileData, 0L, SEEK_END);
									long qsize = ftell(fileData);
									fseek(fileData, 0L, SEEK_SET);

									char *qdata = (char*) malloc((qsize+1)*sizeof(char));
									memset(qdata,0, (qsize+1)*sizeof(char)); //free
									long left = qsize;

									memset(recvBuffer,0,sizeof(recvBuffer));

									while (left > 0) {

										long readBytes = fread(recvBuffer,1,sizeof(recvBuffer),fileData);

										sprintf(qdata,recvBuffer,sizeof(recvBuffer));

										left -= readBytes;
										memset(recvBuffer,0,sizeof(recvBuffer));
									}
									qdata[strlen(qdata)] = '\0';
									error = flock(fileno(fileData), LOCK_UN);
									if(error == -1){ perror("Error on flock in unlocking at createSubDir()."); exit(-1); }
									fclose(fileData);

									char imgFile[70];
									int qIMG = 0;
									memset(imgFile, 0, sizeof(imgFile));

									strcpy(imgFile, questionPath);
									strcat(imgFile, "/");
									getImgFile(questionPath, imgFile);
									if (strcmp(imgFile, "-1") != 0) {
										qIMG = 1;
									}

									strcpy(sendBuffer, "QGR ");
									strcat(sendBuffer,id);
									sprintf(sendBuffer, "%s%c", sendBuffer, ' ');
									sprintf(sendBuffer, "%s%ld", sendBuffer, qsize);
									sprintf(sendBuffer, "%s%c", sendBuffer, ' ');
									strcat(sendBuffer, qdata);
									sprintf(sendBuffer, "%s%c", sendBuffer, ' ');
									sprintf(sendBuffer, "%s%d", sendBuffer, qIMG);

									printf("Sending question data...\n");

									writeToBuffer(sendBuffer, strlen(sendBuffer));

									printf("Question data sent\n");

									memset(sendBuffer,0, sizeof(sendBuffer));

									if (qIMG) {
										char *aux = (char*) malloc((strlen(imgFile)+1) * sizeof(char));
										memset(aux, 0, (strlen(imgFile)+1) * sizeof(char));
										strcpy(aux, imgFile);

										qiext = (char*) malloc(4*sizeof(char));
										memset(qiext,0, sizeof(char));

										qiext = strtok(aux, ".");
										qiext = strtok(NULL, ".");


										FILE *f = fopen(imgFile, "rb");
										error = flock(fileno(f), LOCK_EX);
										if(error == -1){ perror("Error on flock in unlocking at createSubDir()."); exit(-1); }
										fseek(f, 0L, SEEK_END);
										long qisize = ftell(f);
										fseek(f, 0L, SEEK_SET);


										sprintf(sendBuffer, " %s %ld ", qiext, qisize);

										writeToBuffer(sendBuffer, strlen(sendBuffer));

										memset(sendBuffer, 0 ,sizeof(sendBuffer));

										char bufferFile[qisize+1];

										printf("Sending question image file...\n");

										while(!feof(f)) {
											int readIt = fread(bufferFile,1,sizeof(bufferFile),f);

											long toSend = readIt;
											long written;
											while (toSend) {
													written = write(newfd,bufferFile,toSend);
													toSend -= written;
												}
											//memset(bufferFile,0,sizeof(bufferFile));

										}
										error = flock(fileno(f), LOCK_UN);
										if(error == -1){ perror("Error on flock in unlocking at createSubDir()."); exit(-1); }
										fclose(f);

										printf("Question image sent\n");

										free(aux);
										//free(qiext);
									}


									int nAnswers = getAnswerNumber(questionPath);
									int realAnswers = nAnswers;
									if (nAnswers > 10) nAnswers = 10;
									strcpy(sendBuffer, " ");
									if (nAnswers < 10) sprintf(sendBuffer,"%s0%d", sendBuffer, nAnswers);
									else sprintf(sendBuffer,"%s%d", sendBuffer, nAnswers);
									i = strlen(sendBuffer);

									writeToBuffer(sendBuffer, strlen(sendBuffer));

									free(questionPath);
									//Handle Answers to buffer from now on
									int answersSent = 0;
									printf("%d answers found for this question:\n", nAnswers);
									while (answersSent < nAnswers) {

										answersSent++;

										questionPath = (char*) malloc((strlen(topic)+2*strlen(qTitle)+16)*sizeof(char));
										memset(questionPath,0,(strlen(topic)+2*strlen(qTitle)+16)*sizeof(char));

										strcpy(questionPath, "Topics/");
										strcat(questionPath, topic);
										strcat(questionPath, "/");
										strcat(questionPath, qTitle);
										strcat(questionPath, "/");
										strcat(questionPath, qTitle);
										strcat(questionPath, "_");
										int diff = realAnswers+answersSent-10;
										if (diff < 10) strcat(questionPath, "0");
										if (realAnswers >= 10) sprintf(questionPath, "%s%d", questionPath, diff);
										else sprintf(questionPath, "%s%d", questionPath, answersSent);


										memset(sendBuffer, 0 ,sizeof(sendBuffer));
										if ((diff) >= 10) strcat(sendBuffer," ");
										else strcat(sendBuffer, " 0");
										if (realAnswers >= 10)sprintf(sendBuffer, "%s%d", sendBuffer, diff);
										else sprintf(sendBuffer, "%s%d", sendBuffer, answersSent);



										writeToBuffer(sendBuffer, strlen(sendBuffer));



										memset(userFile, 0, sizeof(userFile));

										strcpy(userFile, questionPath);
										strcat(userFile, "/");
										strcat(userFile, qTitle);
										strcat(userFile, "_");
										if (diff < 10) strcat(userFile, "0");

										if (realAnswers > 10) sprintf(userFile, "%s%d", userFile, diff);
										else sprintf(userFile, "%s%d", userFile, answersSent);
										strcat(userFile, "_UID.txt");


										memset(id, 0, strlen(id));

										idFile = fopen(userFile, "r");


										fgets(id, 6,idFile);

										fclose(idFile);

										memset(infoFile, 0, sizeof(infoFile));


										strcpy(infoFile, questionPath);
										strcat(infoFile, "/");
										strcat(infoFile, qTitle);
										strcat(infoFile, "_");
										if (diff < 10) strcat(infoFile, "0");

										if (realAnswers > 10) sprintf(infoFile, "%s%d", infoFile, diff);
										else sprintf(infoFile, "%s%d", infoFile, answersSent);


										strcat(infoFile, ".txt");

										fileData = fopen(infoFile, "r");
										error = flock(fileno(fileData), LOCK_EX);
										if(error == -1){ perror("Error on flock in unlocking at createSubDir()."); exit(-1); }


										fseek(fileData, 0L, SEEK_END);
										long qsize = ftell(fileData);
										fseek(fileData, 0L, SEEK_SET);
										char *qdata = (char*) malloc((qsize+1)*sizeof(char));
										memset(qdata,0, (qsize+1)*sizeof(char));
										long left = qsize;

										memset(recvBuffer,0,sizeof(recvBuffer));

										while (left > 0) {

											long readBytes = fread(recvBuffer,1,sizeof(recvBuffer),fileData);

											sprintf(qdata,recvBuffer,sizeof(recvBuffer));

											left -= readBytes;
											memset(recvBuffer,0,sizeof(recvBuffer));
										}
										error = flock(fileno(fileData), LOCK_UN);
										if(error == -1){ perror("Error on flock in unlocking at createSubDir()."); exit(-1); }
										fclose(fileData);

										qIMG = 0;
										memset(imgFile, 0, sizeof(imgFile));

										strcpy(imgFile, questionPath);
										strcat(imgFile, "/");

										getImgFile(questionPath, imgFile);

										if (strcmp(imgFile, "-1") != 0) {
											qIMG = 1;
										}



										memset(sendBuffer, 0 ,sizeof(sendBuffer));

										strcpy(sendBuffer, " ");
										strcat(sendBuffer,id);
										sprintf(sendBuffer, "%s%c", sendBuffer, ' ');
										sprintf(sendBuffer, "%s%ld", sendBuffer, qsize);
										sprintf(sendBuffer, "%s%c", sendBuffer, ' ');
										strcat(sendBuffer, qdata);
										sprintf(sendBuffer, "%s%c", sendBuffer, ' ');
										sprintf(sendBuffer, "%s%d", sendBuffer, qIMG);

										if (diff< 10) printf("Sending answer 0%d data file...\n", answersSent );
										else printf("Sending answer %d data file...\n", diff);

										writeToBuffer(sendBuffer, strlen(sendBuffer));

										if (diff< 10) printf("Answer 0%d data sent\n", answersSent );
										else printf("Answer %d data sent\n", diff);


										memset(sendBuffer,0, sizeof(sendBuffer));

										if (qIMG) {
											char *aux = (char*) malloc((strlen(imgFile)+1) * sizeof(char));
											memset(aux, 0, (strlen(imgFile)+1)*sizeof(char));
											strcpy(aux, imgFile);

											qiext = (char*) malloc(4*sizeof(char));
											memset(qiext, 0, 4*sizeof(char));

											qiext = strtok(aux, ".");
											qiext = strtok(NULL, ".");


											FILE *f = fopen(imgFile, "rb");
											error = flock(fileno(f), LOCK_EX);
											if(error == -1){ perror("Error on flock in unlocking at createSubDir()."); exit(-1); }
											fseek(f, 0L, SEEK_END);
											long qisize = ftell(f);
											fseek(f, 0L, SEEK_SET);

											sprintf(sendBuffer, " %s %ld ", qiext, qisize);

											writeToBuffer(sendBuffer, strlen(sendBuffer));

											memset(sendBuffer, 0 ,sizeof(sendBuffer));

											char *bufferFile =(char*) malloc((qisize+1)*sizeof(char));
											memset(bufferFile,0, (qisize+1)*sizeof(char) );

											if (diff< 10) printf("Sending answer 0%d image file...\n", answersSent );
											else printf("Sending answer %d image file...\n", diff);

											while(!feof(f)) {
												int readIt = fread(bufferFile,1,sizeof(bufferFile),f);

												long toSend = readIt;
												long written;
												while (toSend) {
													written = write(newfd,bufferFile,toSend);
													toSend -= written;
												}
												//memset(bufferFile,0,sizeof(bufferFile));

											}
											error = flock(fileno(f), LOCK_UN);
											if(error == -1){ perror("Error on flock in unlocking at createSubDir()."); exit(-1); }

											fclose(f);
											if (diff< 10) printf("Answer 0%d image sent\n", answersSent );
											else printf("Answer %d image sent\n", diff);
											memset(sendBuffer, 0 ,sizeof(sendBuffer));
											free(aux);
											free(bufferFile);
											//free(qiext);
										}
										free(questionPath);
										free(qdata);
									}
									i = 1;
									while (i > 0) {
										n = write(newfd,"\n", i);
										i -=n;
									}

									printf("Operation sucessful. Check files\n");
									free(topic);
									free(qTitle);
								}
							}
						}
					}
					else {
						memset(sendBuffer,0, sizeof(sendBuffer));
						strcpy(sendBuffer, "ERR\n");
						writeToBuffer(sendBuffer, strlen(sendBuffer));
					}
				}
				else if (strncmp(requestCode, "QUS",3) == 0) {

					char* clientArg = strtok(requestCode, "QUS");

					if(strcmp(clientArg, " ") == 0){
						int bad_protocol = 0;
						memset(recvBuffer, 0, strlen(recvBuffer));
						char * userID = (char*) malloc(7*sizeof(char));
						memset(userID,0, 7*sizeof(char));
						sq = (char*) malloc( 7*sizeof(char));
						memset(sq,0, 7*sizeof(char));


						//reading first argument - qUserID
						readToBuffer(sq, 6);
						sq[strlen(sq)] = '\0';

						strcpy(userID,sq);
						free(sq);
						strtok(userID, delim);


//						printf("UserID no QUS depois do Strtok: %s tamanho = %d\n", userID, (int)strlen(userID));
						bad_protocol ? bad_protocol : checkUserIDArg(userID);

						//reading second argument - topic	
						memset(recvBuffer, 0, strlen(recvBuffer));
						char * topic = (char*) malloc(12*sizeof(char));
						memset(topic,0, 12*sizeof(char));

						readToBuffer(topic, 1);
						while (strcmp(recvBuffer, " ") != 0 && strcmp(recvBuffer, "\n") != 0 ) {
							memset(recvBuffer, 0, sizeof(recvBuffer));

							n = read(newfd, recvBuffer,1 );
							if (n == -1) exit(1);
							if ( n > 0 ) strcat(topic, recvBuffer);

						}
						topic[strlen(topic)] = '\0';
						strtok(topic, delim);

//						printf("Topic no QUS depois do Strtok: %s tamanho = %d\n", topic, (int)strlen(topic));
						bad_protocol ? bad_protocol : checkTopicArg(topic);

						//reading third argument - qTitle
						memset(recvBuffer, 0, strlen(recvBuffer));
						char * qTitle = (char*) malloc(12*sizeof(char));
						memset(qTitle,0, 12*sizeof(char));

						readToBuffer(qTitle, 1);
						while (strcmp(recvBuffer, " ") != 0 && strcmp(recvBuffer, "\n") != 0 ) {
							memset(recvBuffer, 0, sizeof(recvBuffer));

							n = read(newfd, recvBuffer,1 );
							if (n == -1) exit(1);
							if ( n > 0 ) strcat(qTitle, recvBuffer);

						}
						qTitle[strlen(qTitle)] = '\0';
						strtok(qTitle, delim);

						bad_protocol ? bad_protocol : checkQuestionTitleArg(qTitle);

						printf("New question received: %s/%s\n", topic,qTitle);

						//reading fourth argument - qsize
						memset(recvBuffer, 0, strlen(recvBuffer));
						char *sizeQuestion = (char*) malloc(12*sizeof(char));
						memset(sizeQuestion,0, 12*sizeof(char));
						char *sizeImage = (char*) malloc(12*sizeof(char));
						memset(sizeImage,0, 12*sizeof(char));

						readToBuffer(sizeQuestion, 1);

						while (strcmp(recvBuffer, " ") != 0 && strcmp(recvBuffer, "\n") != 0 ) {
							memset(recvBuffer, 0, sizeof(recvBuffer));
							n = read(newfd, recvBuffer,1 );
							if (n == -1) exit(1);
							if ( n > 0 ) strcat(sizeQuestion, recvBuffer);

						}
						sizeQuestion[strlen(sizeQuestion)] = '\0';
						strtok(sizeQuestion,delim);
						memset(recvBuffer, 0, sizeof(recvBuffer));
						long qSize = atoi(sizeQuestion);
						bad_protocol ? bad_protocol : checkQuestionSizeArg(qSize);

						//reading fifth argument - qdata
						char qData[qSize+1];
						long alreadyRead = 0;
						memset(qData, 0, sizeof(qData));

						while (alreadyRead < qSize) {
							alreadyRead += read(newfd, recvBuffer,qSize);
							strcat(qData, recvBuffer);
							memset(recvBuffer, 0, strlen(recvBuffer));

						}

						bad_protocol ? bad_protocol : checkQuestionDataArg(qTitle);
						//Processing request
						if(!bad_protocol){
							

							// processing txt
							char *subDir = (char*) malloc((12+strlen(qTitle)+strlen(topic))*sizeof(char)); //TODO verify this math
							memset(subDir,0, (12+strlen(topic)+strlen(qTitle))*sizeof(char));

							strcpy(subDir, "Topics/");
							strcat(subDir, topic);

							int questionStatus = createSubDir(subDir,qTitle,userID,1);


							strcat(subDir, "/");
							strcat(subDir, qTitle);

							//Create text file
							char *txtOutput = (char*) malloc((strlen(subDir)+strlen(qTitle)+6)*sizeof(char));  //TODO verify this math
							memset(txtOutput,0, (strlen(subDir)+strlen(qTitle)+6)*sizeof(char));

							strcpy(txtOutput,subDir);
							strcat(txtOutput, "/");
							strcat(txtOutput, qTitle);
							strcat(txtOutput,".txt");
							if (questionStatus == 1) {
								fp = fopen(txtOutput, "w");
								int error = flock(fileno(fp), LOCK_EX);
								if(error == -1){ perror("Error on flock in unlocking at createSubDir()."); exit(-1); }
								fwrite(qData, 1, qSize, fp);
								error = flock(fileno(fp), LOCK_UN);
								if(error == -1){ perror("Error on flock in unlocking at createSubDir()."); exit(-1); }
								fclose(fp);
							}
							//processing image
							int   qIMG;
							char *qiExt = (char*) malloc(5*sizeof(char));
							memset(qiExt,0, 5*sizeof(char));
							long  qiSize;

							memset(recvBuffer,0,sizeof(recvBuffer));
							sq = (char*) malloc(3*sizeof(char));
							memset(sq,0, 3*sizeof(char));

							//reading qIMG and space after it
							readToBuffer(sq, 2);
							sq[strlen(sq)] = '\0';
							qIMG = atoi(sq);
							free(sq);


							if (qIMG) {
								memset(space, 0, 2*sizeof(char));
								//reading the space
								readToBuffer(space, 1);

								//In case of an img
								strcpy(qiExt,".");
								memset(recvBuffer,0,sizeof(recvBuffer));

								readToBuffer(qiExt, 3);
								qiExt[strlen(qiExt)] = '\0';


								//reading the space
								memset(recvBuffer,0,sizeof(recvBuffer));

								memset(space, 0, 2*sizeof(char));
								readToBuffer(space, 1);

								//reading image size
								memset(recvBuffer,0,sizeof(recvBuffer));
								memset(sizeImage, 0, 12*sizeof(char));
								while (strcmp(recvBuffer, " ") != 0 && strcmp(recvBuffer, "\n") != 0 ) {
									memset(recvBuffer,0,sizeof(recvBuffer));

									n = read(newfd, recvBuffer,1 );
									if (n == -1) exit(1);

									if ( n > 0 ) strcat(sizeImage, recvBuffer);

								}
								sizeImage[strlen(sizeImage)] = '\0';
								strtok(sizeImage, delim);
								qiSize = atoi(sizeImage);

								//Create img file
								char * imgOutput =(char*) malloc((strlen(subDir)+strlen(qTitle)+6)*sizeof(char));
								memset(imgOutput,0,(strlen(subDir)+strlen(qTitle)+6)*sizeof(char));
								strcat(imgOutput, subDir);
								strcat(imgOutput, "/");
								strcat(imgOutput, qTitle);
								strcat(imgOutput,qiExt);

								if(questionStatus == 1){
									fp = fopen(imgOutput, "wb");
									int error = flock(fileno(fp), LOCK_EX);
									if(error == -1){ perror("Error on flock in unlocking at createSubDir()."); exit(-1); }
								}

								//reading image data

								char bufferFile[qiSize+1];
								memset(bufferFile,0,sizeof(bufferFile));

								long toRead = qiSize, readbytes;

								while (toRead > 0) {
									readbytes = read(newfd, bufferFile,toRead);
									if(questionStatus == 1) {
										if (readbytes > 0 ) fwrite(bufferFile, 1, readbytes, fp);
									}
									memset(bufferFile,0,sizeof(bufferFile));
									toRead -=readbytes;
								}

								memset(bufferFile,0,sizeof(bufferFile));
								if (questionStatus == 1){
									int error = flock(fileno(fp), LOCK_UN);
									if(error == -1){ perror("Error on flock in unlocking at createSubDir()."); exit(-1); }
									fclose(fp);
								}

								free(imgOutput);

							} //if(qIMG)

							//reading \n				
							memset(space, 0, 2*sizeof(char));
							readToBuffer(space, 1);

							memset(sendBuffer,0, sizeof(sendBuffer));

							if (questionStatus == 1) strcpy(sendBuffer, "QUR OK\n");
							else if (questionStatus == 2) strcpy(sendBuffer, "QUR DUP\n");
							else if (questionStatus == 3) strcpy(sendBuffer, "QUR FUL\n");
							else strcpy(sendBuffer, "QUR NOK\n");

							
							free(txtOutput);
							free(qiExt);
							free(subDir);
						} //if(!bad_protocol)
								
						//send response to User and free memory
						writeToBuffer(sendBuffer, strlen(sendBuffer));
						free(userID);
						free(topic);
						free(qTitle);
						free(sizeQuestion);
						free(sizeImage);
					}
					else{
						memset(sendBuffer,0, sizeof(sendBuffer));
						strcpy(sendBuffer, "ERR\n");
						writeToBuffer(sendBuffer, strlen(sendBuffer));

					}
				} 
				else if (strcmp(requestCode, "ANS ")== 0) {
					memset(recvBuffer, 0, strlen(recvBuffer));
					char * userID = (char*) malloc(7*sizeof(char));
					memset(userID, 0, 7*sizeof(char));

					i = 6;
					sq = (char*) malloc( 7*sizeof(char));
					memset(sq,0, 7*sizeof(char));
					readToBuffer(sq, 6);
					sq[strlen(sq)] = '\0';

					strcpy(userID,sq);
					free(sq);
					strtok(userID, delim);


					memset(recvBuffer, 0, strlen(recvBuffer));
					char * topic = (char*) malloc(12*sizeof(char));
					memset(topic, 0, 12*sizeof(char));

					readToBuffer(topic, 1);

					while (strcmp(recvBuffer, " ") != 0 && strcmp(recvBuffer, "\n") != 0) {
						memset(recvBuffer, 0, sizeof(recvBuffer));

						n = read(newfd, recvBuffer,1 );
						if (n == -1) exit(1);
						if ( n > 0 )strcat(topic, recvBuffer);

					}
					topic[strlen(topic)] = '\0';
					strtok(topic, delim);


					memset(recvBuffer, 0, strlen(recvBuffer));
					char * qTitle = (char*) malloc(12*sizeof(char));
					memset(qTitle,0,12*sizeof(char));

					readToBuffer(qTitle, 1);

					while (strcmp(recvBuffer, " ") != 0 && strcmp(recvBuffer, "\n") != 0 ) {
						memset(recvBuffer, 0, sizeof(recvBuffer));

						n = read(newfd,recvBuffer,1 );
						if (n == -1) exit(1);
						if ( n > 0 ) strcat(qTitle, recvBuffer);

					}
					qTitle[strlen(qTitle)] = '\0';
					strtok(qTitle, delim);


					printf("New answer received: %s/%s\n", topic,qTitle);


					memset(recvBuffer, 0, strlen(recvBuffer));
					char *sizeQuestion = (char*) malloc(12*sizeof(char));
					memset(sizeQuestion,0, 12*sizeof(char));
					char *sizeImage = (char*) malloc(12*sizeof(char));
					memset(sizeImage,0, 12*sizeof(char));

					readToBuffer(sizeQuestion, 1);


					while (strcmp(recvBuffer, " ") != 0 && strcmp(recvBuffer, "\n") != 0 ) {
						memset(recvBuffer, 0, sizeof(recvBuffer));

						n = read(newfd, recvBuffer,1 );
						if (n == -1) exit(1);
						if ( n > 0 ) strcat(sizeQuestion, recvBuffer);

					}
					sizeQuestion[strlen(sizeQuestion)] = '\0';
					strtok(sizeQuestion, delim);

					memset(recvBuffer, 0, strlen(recvBuffer));

					long qSize = atoi(strtok(sizeQuestion,delim));

					char qData[qSize+1];
					long alreadyRead = 0;
					memset(qData, 0, sizeof(qData));

					while (alreadyRead < qSize) {
						alreadyRead += read(newfd, recvBuffer,qSize);
						strcat(qData, recvBuffer);
						memset(recvBuffer, 0, strlen(recvBuffer));

					}

					char *subDir = (char*) malloc((13+2*strlen(qTitle)+strlen(topic))*sizeof(char)); //TODO verify this math
					memset(subDir,0, (13+strlen(topic)+2*strlen(qTitle))*sizeof(char));

					strcpy(subDir, "Topics/");
					strcat(subDir, topic);
					strcat(subDir, "/");
					strcat(subDir, qTitle);

					int answerStatus = createSubDir(subDir,qTitle,userID,2);

					//Create text file

					int nAnswers = getAnswerNumber(subDir);
					strcat(subDir, "/");
					strcat(subDir, qTitle);
					char *txtOutput = (char*) malloc((strlen(subDir)+strlen(qTitle)+12)*sizeof(char));  //TODO verify this math
					memset(txtOutput,0, (strlen(subDir)+strlen(qTitle)+12)*sizeof(char));
					strcat(txtOutput,subDir);
					strcat(txtOutput, "_");
					if (nAnswers < 10) strcat(txtOutput,"0" );
					sprintf(txtOutput, "%s%d", txtOutput, nAnswers);
					strcat(txtOutput, "/");
					strcat(txtOutput, qTitle);
					strcat(txtOutput, "_");
					if (nAnswers < 10) strcat(txtOutput,"0" );
					sprintf(txtOutput, "%s%d", txtOutput, nAnswers);
					strcat(txtOutput,".txt");


					if (answerStatus == 1) {

						fp = fopen(txtOutput, "w");
						int error = flock(fileno(fp), LOCK_EX);
						if(error == -1){ perror("Error on flock in unlocking at createSubDir()."); exit(-1); }
						fwrite(qData, 1, qSize, fp);
						error = flock(fileno(fp), LOCK_UN);
						if(error == -1){ perror("Error on flock in unlocking at createSubDir()."); exit(-1); }
						fclose(fp);
					}

					int    qIMG;
					sq = (char*) malloc(3*sizeof(char));
					memset(sq,0, 3*sizeof(char));
					char * qiExt = (char*) malloc(5*sizeof(char));
					bzero(qiExt, 5*sizeof(char));
					long    qiSize;

					memset(recvBuffer,0,sizeof(recvBuffer));

					readToBuffer(sq, 2);
					sq[strlen(sq)] = '\0';

					qIMG = atoi(sq);
					free(sq);
					if (qIMG) {
						memset(space, 0, 2*sizeof(char));
						//reading the space
						readToBuffer(space, 1);


						//In case of an img
						strcpy(qiExt,".");
						memset(recvBuffer,0,sizeof(recvBuffer));
						readToBuffer(qiExt, 3);
						qiExt[strlen(qiExt)] = '\0';

						memset(space, 0, 2*sizeof(char));
						//reading the space
						readToBuffer(space, 1);


						memset(recvBuffer,0,sizeof(recvBuffer));
						memset(sizeImage, 0, 12*sizeof(char));
						while (strcmp(recvBuffer, " ") != 0) {
							memset(recvBuffer,0,sizeof(recvBuffer));

							n = read(newfd, recvBuffer,1 );
							if (n == -1) exit(1);

							if ( n > 0 )strcat(sizeImage, recvBuffer);

						}
						sizeImage[strlen(sizeImage)] = '\0';
						strtok(sizeImage, delim);
						qiSize = atoi(sizeImage);

						//Create img file
						char * imgOutput =(char*) malloc((strlen(subDir)+strlen(qiExt)+strlen(qTitle)+8)*sizeof(char)); //TODO verify this math
						memset(imgOutput,0,(strlen(subDir)+strlen(qiExt)+strlen(qTitle)+8)*sizeof(char));
						strcat(imgOutput, subDir);
						strcat(imgOutput, "_");

						if (nAnswers < 10) strcat(imgOutput,"0" );
						sprintf(imgOutput, "%s%d", imgOutput, nAnswers);
						strcat(imgOutput, "/");
						strcat(imgOutput, qTitle);
						strcat(imgOutput, "_");
						if (nAnswers < 10) strcat(imgOutput,"0" );
						sprintf(imgOutput, "%s%d", imgOutput, nAnswers);
						strcat(imgOutput,qiExt);


						if(answerStatus == 1){
							fp = fopen(imgOutput, "wb");
							int error = flock(fileno(fp), LOCK_EX);
							if(error == -1){ perror("Error on flock in unlocking at createSubDir()."); exit(-1); }
						}

						char bufferFile[qiSize+1];


						memset(bufferFile,0,sizeof(bufferFile));

						long toRead = qiSize, readbytes;

						while (toRead > 0) {

							readbytes = read(newfd, bufferFile,toRead);
							if(answerStatus == 1) {
								if (readbytes > 0 ) fwrite(bufferFile, 1, readbytes, fp);
							}
							memset(bufferFile,0,sizeof(bufferFile));
							toRead -=readbytes;

						}

						memset(bufferFile,0,sizeof(bufferFile));
						if (answerStatus == 1) {
							int error = flock(fileno(fp), LOCK_UN);
							if(error == -1){ perror("Error on flock in unlocking at createSubDir()."); exit(-1); }
							fclose(fp);
						}
						free(imgOutput);

					}
					memset(space, 0, 2*sizeof(char));
					readToBuffer(space, 1);


					memset(sendBuffer,0, sizeof(sendBuffer));

					if (answerStatus == 1) {strcpy(sendBuffer, "ANR OK\n");}
					else if (answerStatus == 2) {strcpy(sendBuffer, "ANR DUP\n");}
					else if (answerStatus == 3) {strcpy(sendBuffer, "ANR FUL\n");}
					else{ strcpy(sendBuffer, "ANR NOK\n");}


					writeToBuffer(sendBuffer, strlen(sendBuffer));
					free(userID);
					free(topic);
					free(qTitle);
					free(sizeQuestion);
					free(sizeImage);
					free(subDir);
					free(txtOutput);
					free(qiExt);


				}
				else {
					memset(sendBuffer,0, sizeof(sendBuffer));
					strcpy(sendBuffer, "ERR\n");
					//read(newfd,recvBuffer,1);
					writeToBuffer(sendBuffer, strlen(sendBuffer));

					printf("err\n");
				}
				printf("Request originated on IP: %s and port: %s\n", IP, PORT );
				free(requestCode);
				free(space);
			}
		 close(newfd);
			// Wait for Childs
			int status;
			while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
		        if (pid < 0) {
					if (errno == EINTR) {
						//Este codigo de erro significa que chegou signal que interrompeu a espera
						//pela terminacao de filho; logo voltamos a esperar
						continue;
					} else {
						perror("Unexpected error while waiting for child.");
						exit (-1);
					}
				}
			}

		    //freeaddrinfo(resTCP);


		}
		if (FD_ISSET(fdUDP, &rset)) {
			addrlen = sizeof(addr);
			memset(recvBuffer,0, sizeof(recvBuffer));

			n = recvfrom(fdUDP, recvBuffer, sizeof(recvBuffer), 0, (struct sockaddr*) &addr, &addrlen);
			if (n==-1) exit(1);

			recvBuffer[strlen(recvBuffer)] = '\0';

			IP = inet_ntoa(addr.sin_addr);
			char *clientArg = strtok(recvBuffer, delim);


			printf("Received request: ");

			memset(sendBuffer,0, sizeof(sendBuffer));

			if (strncmp(recvBuffer, "REG", 3) == 0) {
				clientArg = strtok(NULL, delim);
				userID = atoi(clientArg);

				if(userID == 0 || checkUserIDArg(clientArg)){
					memset(sendBuffer,0, sizeof(sendBuffer));
					strcpy(sendBuffer, "ERR\n");
				}
				else{
					printf("Register user %d\n", userID);
					strcpy(sendBuffer, "RGR ");
					if (isKnownUser(userID)){
						printf("User sucessfully registered\n");
							strcat(sendBuffer, "OK\n");
					}
					else  strcat(sendBuffer, "NOK\n");
				}
			
			}
			else if (strncmp(recvBuffer, "LTP",3) == 0) {
				char* possible_trash = strtok(recvBuffer, "LTP");

				if(strcmp(possible_trash, "\n") == 0){
					printf("List of topics available\n");
					memset(availableTopics, 0, BUFFER_SIZE*sizeof(char));
					int status = getOpenedTopics(availableTopics);
					if (status != 0 ) {
						n_topics = topicCounter(availableTopics);
						strcpy(sendBuffer, "LTR ");
						sprintf(topicsAlg, "%d", n_topics);
						strcat(sendBuffer,topicsAlg);
						strcat(sendBuffer, " ");
						strcat(sendBuffer, availableTopics);
						sprintf(sendBuffer, "%s%c", sendBuffer, '\n');
						printf("Data sent\n");

					}
					else strcpy(sendBuffer, "LTR 0\n");
				}
				else strcpy(sendBuffer, "ERR\n");
			} 
			else if (strncmp(recvBuffer, "PTP",3) == 0) {

				clientArg = strtok(NULL, delim);

				strcpy(id, clientArg);

				//NULL pointer
				if(clientArg){
					clientArg = strtok(NULL, delim);
					if(clientArg){
						proposedTopic = strtok(clientArg, endOfLine);
						printf("Propose topic %s\n", proposedTopic);
						topicStatus = checkForExistingTopic(proposedTopic, id);
						//memset(&buffer, 0, sizeof(buffer));
						if (topicStatus == 1) {strcpy(sendBuffer, "PTR OK\n");}
						else if (topicStatus == 2) {strcpy(sendBuffer, "PTR DUP\n");}
						else if (topicStatus == 3) {strcpy(sendBuffer, "PTR FUL\n");}
						else{ strcpy(sendBuffer, "PTR NOK\n");}	
					}
					else strcpy(sendBuffer, "ERR\n");
				}
				else strcpy(sendBuffer, "ERR\n");
			}
			else if (strncmp(recvBuffer, "LQU",3) == 0) {
				topicCount = 0;
				i = 0;
				clientArg = strtok(NULL, delim);


				if(clientArg){
					proposedTopic = strtok(clientArg, endOfLine);
					if(proposedTopic){
						strcpy(activeTopic, proposedTopic);
						printf("List questions for topic: %s\n", proposedTopic);
						memset(activeTopicQuestions,0, strlen(activeTopicQuestions));

						int status = getQuestionsByTopic(activeTopic, activeTopicQuestions);

						if (status == 0) continue;
						else {
							if (activeTopicQuestions[i] != '\0') topicCount++;
							while (activeTopicQuestions[i] != '\0') {
								if (activeTopicQuestions[i] == ' ') topicCount++;
								i++;
							}
						}

						strcpy(sendBuffer, "LQR ");
						sprintf(sendBuffer, "%s%d", sendBuffer, topicCount);
						if (topicCount != 0) sprintf(sendBuffer, "%s%c",sendBuffer, ' ');
						strcat(sendBuffer, activeTopicQuestions);
						sprintf(sendBuffer, "%s%c", sendBuffer, '\n');
						printf("Data sent\n");
					}
					else strcpy(sendBuffer, "ERR\n");
				}
				else strcpy(sendBuffer, "ERR\n");
			}
			else strcpy(sendBuffer,"ERR\n");
			
			printf("Request originated on IP: %s and port: %s\n", IP, PORT );

			n = sendto(fdUDP, sendBuffer, strlen(sendBuffer), 0, (struct sockaddr*)&addr, addrlen);
			if(n==-1)/*error*/exit(1);

			memset(&clientArg, 0, sizeof(clientArg));
			memset(&activeTopic, 0, sizeof(activeTopic));
			memset(&proposedTopic, 0, sizeof(proposedTopic));
			memset(&recvBuffer,0, sizeof(recvBuffer));
			memset(&sendBuffer,0, sizeof(sendBuffer));

		}
	}
	memset(recvBuffer,0, sizeof(recvBuffer));
	free(availableTopics);
	free(proposedTopic);
	free(id);
	free(activeTopicQuestions);
	freeaddrinfo(res);
	close(fdUDP);
	return 0;
}
