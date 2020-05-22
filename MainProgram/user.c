#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <netdb.h>
#include <stdbool.h>
#include <stdlib.h>
#include <dirent.h>
#define BUFFER_SIZE 8192


bool getOut = false;
int fdUDP;
int fdTCP;

ssize_t n;
socklen_t addrlen;
struct sockaddr_in addr;
struct addrinfo hints, *res, hintsTCP, *resTCP;
char recvBuffer[BUFFER_SIZE], requestCode[BUFFER_SIZE];
char availableTopics[BUFFER_SIZE], availableTopicQuestions[BUFFER_SIZE];
char activeTopic[20] = "\0";
char activeQuestion[20] = "\0";
char formattedTopic[17];
char userID[6] = "-1";
int qIMG = false,port ;
long qsize, qisize;
char *question, *qdata, *qidata, *ip; // malloc e free
char *qiext;
char *imgPath, *filePath;

void printToScreenTopics(char *availableTopics) {
    int topicCount = 1, i = 6;

    if (availableTopics[i] == ' ') i++;
    printf("available topics:\n");
    while (availableTopics[i] != '\0') {
         printf("%d - ",topicCount);
         while (availableTopics[i] != ':') {
             printf("%c", availableTopics[i]);
             i++;
         }
         printf(" (proposed by ");
         i++;
         while (availableTopics[i] != ' ' && availableTopics[i] != '\n' && availableTopics[i] != '\0' ) {
             printf("%c", availableTopics[i]);
             i++;
         }
         printf(")\n");
         i++;
         topicCount++;
     }

}

void printQuestionsToScreen(char *recvBuffer, char *topic) {
    int i = 6, questionCount = 1, endOfLine = 0;

    printf("available questions about %s:\n", formattedTopic);
    if (recvBuffer[4] == '0' ) printf("There are no questions for this topic yet\n");
    else {
        if (recvBuffer[i] == ' ') i++;
        while (recvBuffer[i] != '\n') {
            printf("%d - ", questionCount);
            while (recvBuffer[i] != ':') {
                printf("%c", recvBuffer[i]);
                i++;
            }
            while (recvBuffer[i] != ':')
                i++;
            printf(" ( proposed by ");
            i++;
            while (recvBuffer[i] != ' ') {
                if (recvBuffer[i] == '\n') {
                    endOfLine = 1;
                    break;
                }
                printf("%c", recvBuffer[i]);
                i++;
            }
            printf(" answer(s))");
            printf("\n");
            if (endOfLine) break;
            i++;
            questionCount++;
        }
    }
}

void formatTopic() {
    int i = 0;
    memset(formattedTopic,0,sizeof(formattedTopic));
    while (activeTopic[i] != ' ') {
        formattedTopic[i] = activeTopic[i];
        i++;
    }
}

void getDesiredQuestionByName(char *desiredQuestion, char *destQuestion) {
    int j = 0, i = 6, questionFound = 0;

    if (availableTopicQuestions[i] == ' ') i++;
    int l = i;
    while( l<strlen(availableTopicQuestions)) {
        
        while( availableTopicQuestions[l] == desiredQuestion[j]) {
            j++;
            l++;

        }

        if (desiredQuestion[j] == '\0' && availableTopicQuestions[l] == ':') {
            questionFound = 1;
            break;
        }
        else if (desiredQuestion[j] == '\0') {
            break;
        }

        else
            j=0;
        while(availableTopicQuestions[l] != ' ' && availableTopicQuestions[l] != '\0') {
            l++;
        }  
        l++;

    }

    if (questionFound) {
        printf("%s",desiredQuestion);
        strcpy(destQuestion, desiredQuestion);
    }
    else strcpy(destQuestion, "\0");


}

void getDesiredQuestionByNumber( int questionNumber, char *destQuestion) {
    int currentQuestion = 1, questionFound = 0, i = 6;

    if (availableTopicQuestions[i] == ' ') i++;

    while (availableTopicQuestions[i] != '\0') {
        if (availableTopicQuestions[i] == ' ')  currentQuestion++;
        if (currentQuestion == questionNumber) {
            questionFound = 1;
            if (questionNumber != 1) i++;
            break;
        }
        i++;
    }
    if (questionFound){

        while (availableTopicQuestions[i] != ':') {
            sprintf(destQuestion, "%s%c" ,destQuestion,availableTopicQuestions[i]);
            i++;
        }
    }
    else strcpy(destQuestion, "\0");
}

void getDesiredTopicByNumber(int desiredTopic, char *destTopic ) {
    int currentTopic = 1, topicFound = 0, i = 6;

    if (availableTopics[i] == ' ') i++;

    while (availableTopics[i] != '\0') {
        if (availableTopics[i] == ' ')  currentTopic++;
        if (currentTopic == desiredTopic) {
            topicFound = 1;
            if (desiredTopic != 1) i++;
            break;
        }
        i++;
    }
    if (topicFound) {

        while (availableTopics[i] != ' ' && availableTopics[i] != '\n' && availableTopics[i] != '\0') {
            sprintf(destTopic, "%s%c" ,destTopic,availableTopics[i]);
            i++;
            if (availableTopics[i] == ':') {
                i++;
                strcat(destTopic, " (");
            }
        }

        strcat(destTopic, ")\n");
    }
    else strcpy(destTopic, "\0");

}

void getDesiredTopicByName(char *desiredTopic, char* destTopic ) {
    int j = 0, i = 6, k, topicFound = 0;
    if (availableTopics[i] == ' ') i++;
    int l = i;
    while( l < strlen(availableTopics)) {

        while( availableTopics[l] == desiredTopic[j]) {
            j++;
            l++;

        }

        if (desiredTopic[j] == '\0' && availableTopics[l] == ':') {
            topicFound = 1;
            k = l +1;
            break;
        }
        else if (desiredTopic[j] == '\0') {
            break;
        }

        else
            j=0;

        while(availableTopics[l] != ' ' && availableTopics[l] != '\0') {
            l++;
        }  
        l++;
    }

    if (topicFound) {

        strcpy(destTopic, desiredTopic);
        strcat(destTopic, " (");
        while (availableTopics[k] != ' ' && availableTopics[k] != '\n' && availableTopics[k] != '\0') {
            sprintf(destTopic, "%s%c" ,destTopic,availableTopics[k]);
            k++;
        }
        strcat(destTopic, ")\n");
    }
    else strcpy(destTopic, "\0");

}

void connectToUDP() {

    struct timeval timeout;
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;
    
    n = sendto(fdUDP, requestCode ,strlen(requestCode),0, res->ai_addr, res->ai_addrlen);
    if(n==-1)/*error*/exit(1);
    
    if (setsockopt(fdUDP, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
        perror("Error");
    
    n = recvfrom(fdUDP, recvBuffer, sizeof(recvBuffer), 0, (struct sockaddr*) &addr, &addrlen);
    if(n==-1)/*error*/{
        printf("Timeout exceeded\n");
        exit(1);
    }
}

void readToBuffer(char *buffer, int nBytes) {
	int i = nBytes;
	while (i > 0) {
		memset(recvBuffer, 0, sizeof(recvBuffer));
		n = read(fdTCP, recvBuffer,i);
		if (n == -1) exit(1);
		i -=n;
		if (n > 0)strcat(buffer, recvBuffer);
	}
}

void writeToBuffer(char *buffer, int nBytes) {
	int i = nBytes;
	while (i > 0) {
		n = write(fdTCP, buffer,i);
		if (n == -1) exit(1);
		i -=n;
	}
}


void readInputFiles(char *filePath, char *imgPath,char *currentQuestion, int qIMG, int flag) {
    int i;
    char delim[] = " ";
    char *currentTopic;

    memset(requestCode, 0, sizeof(requestCode));
    memset(recvBuffer, 0, sizeof(recvBuffer));


    FILE *f = fopen(filePath, "rb");
    fseek(f, 0L, SEEK_END);
    qsize = ftell(f);
    fseek(f, 0L, SEEK_SET);
    qdata = (char*) malloc((qsize+1)*sizeof(char)); //free
    memset(qdata,0, (qsize+1)*sizeof(char));
    long left = qsize;


    while (left > 0) {

        long readBytes = fread(recvBuffer,1,sizeof(recvBuffer),f);

        sprintf(qdata,recvBuffer,strlen(recvBuffer));

        left -= readBytes;
        memset(recvBuffer,0,strlen(recvBuffer));
    }
    qdata[strlen(qdata)] = '\0';
    fclose(f);

            //Create question submit message to send to server
    memset(requestCode,0,sizeof(requestCode));
    if (flag == 1) strcat(requestCode, "QUS ");
    else strcat(requestCode, "ANS ");
    strcat(requestCode, userID);
    sprintf(requestCode, "%s%c", requestCode, ' ');
    currentTopic = strtok(activeTopic, delim);
    strcat(requestCode, currentTopic);
    sprintf(requestCode, "%s%c", requestCode, ' ');
    strcat(requestCode, currentQuestion);
    sprintf(requestCode, "%s %ld %s ",requestCode, qsize, qdata);
    requestCode[strlen(requestCode)] = '\0';
    writeToBuffer(requestCode, strlen(requestCode));

    memset(requestCode, 0 ,sizeof(requestCode));


    if (!qIMG) {
        writeToBuffer( "0", 1);

    }

    //Handle img if one is provided
    if (qIMG) {

        char *aux = (char*) malloc((strlen(imgPath)+1) * sizeof(char));
        memset(aux,0, (strlen(imgPath)+1)*sizeof(char));
        strcpy(aux, imgPath);

        qiext = strtok(aux, ".");
        qiext = strtok(NULL, ".");


        f = fopen(imgPath, "rb");


        fseek(f, 0L, SEEK_END);
        qisize = ftell(f);
        fseek(f, 0L, SEEK_SET);

        sprintf(requestCode, "1 %s %ld ", qiext, qisize);
        requestCode[strlen(requestCode)] = '\0';
        writeToBuffer(requestCode, strlen(requestCode));

        memset(requestCode, 0 ,sizeof(requestCode));

        char bufferFile[qisize];
        memset(bufferFile,0,qisize);
        free(aux);

        while(!feof(f)) {
           int readIt = fread(bufferFile,1,sizeof(bufferFile),f);

           long toSend = readIt;
           long written;
           while (toSend) {
                written = write(fdTCP,bufferFile,toSend);
                toSend -= written;
            }
           //memset(bufferFile,0,sizeof(bufferFile));

       }

        fclose(f);

        memset(requestCode, 0 ,sizeof(requestCode));

    }

    i = 1;
    while (i > 0) {
        n = write(fdTCP,"\n", i);
        if (n == -1) exit(1);
        i -=n;
    }

    free(qdata);

}

void connectToTCP_qg(char * requestCode) {
    char delim[] = " ";
    char *message,*imgOutput, *subDir, *txtOutput, *questionUser, *sizeQuestion, *sizeImage;
    char *space = (char*) malloc(2*sizeof(char));
    int i;
    writeToBuffer(requestCode, strlen(requestCode));



    if (n == -1) perror("Write in socket error\n");

    memset(recvBuffer, 0, sizeof(recvBuffer));

    message = (char*) malloc(4*sizeof(char));
    memset(message, 0, 4*sizeof(char));
    readToBuffer(message, 3);

    message[strlen(message)] = '\0';

    if (strcmp(message, "ERR") == 0) {printf("Error on protocol\n"); free(message);}

    else {
        free(message);
        questionUser = (char*) malloc(7*sizeof(char));
        memset(questionUser,0, 7*sizeof(char));

        memset(space, 0, 2*sizeof(char));

        readToBuffer(space, 1);

        message = (char*) malloc(7*sizeof(char));
        memset(message,0, 7*sizeof(char));
        i= 3;
        while (i > 0) {
            n = read(fdTCP, recvBuffer,i );
            if (n == -1) exit(1);
            i -= n;
            if ( n > 0 ) strcat(message, recvBuffer);
        }
        if (strcmp(message, "EOF") == 0) {
            
            printf("Question / Topic couldn't be Found\n"); 
            free(message);
            
        } 
        else if (strcmp(message, "ERR") == 0) {
            printf("Protocol message badly formatted\n"); 
            free(message);
        }
        else {
            i=3;
            while (i > 0) {
                n = read(fdTCP, recvBuffer,i );
                if (n == -1) exit(1);
                i -= n;
                if ( n > 0 ) strcat(message, recvBuffer);
            }
            message[strlen(message)] = '\0';

        

            strcpy(questionUser, message);
            strtok(questionUser, delim);
            free(message);


            memset(recvBuffer, 0, strlen(recvBuffer));
            sizeQuestion = (char*) malloc(12*sizeof(char));
            memset(sizeQuestion,0, 12*sizeof(char));
            sizeImage = (char*) malloc(12*sizeof(char));
            memset(sizeImage,0, 12*sizeof(char));
            readToBuffer(sizeQuestion, 1);

            strcpy(sizeQuestion, recvBuffer);

            while (strcmp(recvBuffer, " ") != 0) {

                n = read(fdTCP, recvBuffer,1 );
                if (n == -1) exit(1);
                if ( n > 0 ) strcat(sizeQuestion, recvBuffer);

            }
            sizeQuestion[strlen(sizeQuestion)] = '\0';
            strtok(sizeQuestion, delim);


            memset(recvBuffer, 0, strlen(recvBuffer));

            long qSize = atoi(sizeQuestion);

            char qData[qSize+1];
            long alreadyRead = 0;
            memset(qData, 0, sizeof(qData));

            printf("Retriving question data file...\n");


            while (alreadyRead < qSize) {
                alreadyRead += read(fdTCP, recvBuffer,qSize);
                strcat(qData, recvBuffer);
                memset(recvBuffer, 0, strlen(recvBuffer));

            }
            qData[strlen(qData)] = '\0';

            printf("Question data file retrieved\n");


            subDir = (char*) malloc((30+ strlen(activeQuestion))*sizeof(char)); //TODO verify this math

            memset(subDir, 0,(30+ strlen(activeQuestion))*sizeof(char)) ;
            strcpy(subDir, "TopicsReceived/");
            strcat(subDir, formattedTopic);
            mkdir(subDir,0700);
            strcat(subDir, "/");
            strcat(subDir, activeQuestion);
            mkdir(subDir,0700);


            txtOutput = (char*) malloc((strlen(subDir)+6+strlen(activeQuestion))*sizeof(char));  //TODO verify this math
            memset(txtOutput,0,(strlen(subDir)+6+strlen(activeQuestion))*sizeof(char));
            strcat(txtOutput,subDir);
            strcat(txtOutput, "/");
            strcat(txtOutput, activeQuestion);
            strcat(txtOutput,".txt");

            FILE * fp = fopen(txtOutput, "w");
            fwrite(qData, 1, qSize, fp);
            fclose(fp);


            int    qIMG;
            char * qiExt = (char*) malloc(5*sizeof(char));
            memset(qiExt, 0, 5*sizeof(char));
            long    qiSize;
            message = (char*) malloc(3*sizeof(char));
            memset(message,0, 3*sizeof(char));
            memset(recvBuffer,0,sizeof(recvBuffer));

            readToBuffer(message, 2);
            message[strlen(message)] = '\0';
            free(txtOutput);

            qIMG = atoi(message);
            free(message);
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

                    n = read(fdTCP, recvBuffer,1 );
                    if (n == -1) exit(1);

                    if ( n > 0 ) strcat(sizeImage, recvBuffer);

                }

                sizeImage[strlen(sizeImage)] = '\0';
                strtok(sizeImage, delim);
                qiSize = atoi(sizeImage);

                //Create img file
                imgOutput =(char*) malloc((strlen(subDir)+6+strlen(activeQuestion))*sizeof(char)); //TODO verify this math
                memset(imgOutput,0,(strlen(subDir)+5+strlen(activeQuestion))*sizeof(char));
                strcat(imgOutput, subDir);
                strcat(imgOutput, "/");
                strcat(imgOutput, activeQuestion);
                strcat(imgOutput,qiExt);

                FILE *fp = fopen(imgOutput, "wb");

                char bufferFile[qiSize];


                memset(bufferFile,0,sizeof(bufferFile));

                long toRead = qiSize, readbytes;

                printf("Retrieving question image file...\n");


                while (toRead > 0) {

                    readbytes = read(fdTCP, bufferFile,toRead);
                    if (readbytes > 0 ) fwrite(bufferFile, 1, readbytes, fp);

                    memset(bufferFile,0,sizeof(bufferFile));
                    toRead -=readbytes;

                }
                //fwrite("\0",1,1,fp);
                free(imgOutput);
                fclose(fp);

                printf("Question image retrieved\n");

            }


            memset(recvBuffer,0,sizeof(recvBuffer));
            memset(space, 0, 2*sizeof(char));

                // space between image and answers
                readToBuffer(space, 1);

            memset(recvBuffer,0,sizeof(recvBuffer));
            message = (char*) malloc(3*sizeof(char));
            memset(message,0, 3*sizeof(char));

            memset(recvBuffer,0,sizeof(recvBuffer));

            readToBuffer(message, 2);
            message[strlen(message)] = '\0';


            int nAnswers = atoi(message);

            free(message);
            int notLastOne = 1, nDownloaded = 0;
            char *current = (char*) malloc(4*sizeof(char));

            if (nAnswers == 0) notLastOne = 0;

            printf("%d answers found for this question:\n", nAnswers);

            while (notLastOne) {
                nDownloaded++;
                memset(recvBuffer,0,sizeof(recvBuffer));
                i= 1; //space
                while (i > 0) {
                    n = read(fdTCP, recvBuffer,i );
                    if (n == -1) exit(1);
                    i -= n;

                }

                memset(recvBuffer,0,sizeof(recvBuffer));
                memset(current,0, 4*sizeof(char));
                readToBuffer(current, 3);
                current[strlen(current)] ='\0';

                strtok(current, delim);

                if (nAnswers == nDownloaded ) notLastOne = 0;

                memset(recvBuffer,0,sizeof(recvBuffer));
                memset(questionUser,0,7*sizeof(char) );
                readToBuffer(questionUser, 6);
                questionUser[strlen(questionUser)] = 6;


                strtok(questionUser, delim);


                memset(recvBuffer, 0, BUFFER_SIZE);
                memset(sizeQuestion,0, 12*sizeof(char));
                memset(sizeImage,0, 12*sizeof(char));

                readToBuffer(sizeQuestion, 1);


                while (strcmp(recvBuffer, " ") != 0) {

                    n = read(fdTCP, recvBuffer,1 );
                    if (n == -1) exit(1);
                    if ( n > 0 ) strcat(sizeQuestion, recvBuffer);

                }
                sizeQuestion[strlen(sizeQuestion)] = '\0';
                strtok(sizeQuestion, delim);

                memset(recvBuffer, 0, strlen(recvBuffer));


                qSize = atoi(sizeQuestion);
                char qData[qSize+1];

                alreadyRead = 0;
                memset(qData, 0, sizeof(qData));

                printf("Retriving answer %s data file...\n", current);

                while (alreadyRead < qSize) {
                    alreadyRead += read(fdTCP, recvBuffer,qSize);
                    strcat(qData, recvBuffer);
                    memset(recvBuffer, 0, strlen(recvBuffer));

                }
                qData[strlen(qData)] = '\0';
                printf("Answer %s retrieved\n", current);
                free(subDir);
                subDir = (char*) malloc((22+strlen(current)+strlen(formattedTopic)+2*strlen(activeQuestion))*sizeof(char));

                memset(subDir, 0, (22+strlen(current)+strlen(formattedTopic)+2*strlen(activeQuestion))*sizeof(char));
                strcpy(subDir, "TopicsReceived/");
                strcat(subDir, formattedTopic);
                strcat(subDir, "/");
                strcat(subDir, activeQuestion);
                sprintf(subDir, "%s/%s",subDir, activeQuestion );
                strcat(subDir, "_");
                strcat(subDir, current);
                mkdir(subDir, 0700);

                txtOutput = (char*) malloc((7+strlen(subDir)+strlen(activeQuestion)+strlen(current))*sizeof(char));

                memset(txtOutput,0,(7+strlen(subDir)+strlen(activeQuestion)+strlen(current))*sizeof(char));
                strcat(txtOutput,subDir);
                strcat(txtOutput, "/");
                strcat(txtOutput, activeQuestion);
                strcat(txtOutput, "_");
                strcat(txtOutput, current);
                strcat(txtOutput,".txt");
                fp = fopen(txtOutput, "w");
                fwrite(qData, 1, qSize, fp);
                fclose(fp);



                memset(qiExt,0, 5*sizeof(char));


                message = (char*) malloc(3*sizeof(char));
                memset(message,0, 3*sizeof(char));
                memset(recvBuffer,0,sizeof(recvBuffer));
                readToBuffer(message,2);
                message[strlen(message)] = '\0';


                qIMG = atoi(message);
                free(message);

                if (qIMG) {
                    memset(space, 0, 2*sizeof(char));

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

                        n = read(fdTCP, recvBuffer,1 );
                        if (n == -1) exit(1);

                        if ( n > 0 ) strcat(sizeImage, recvBuffer);

                    }
                    sizeImage[strlen(sizeImage)] = '\0';

                    strtok(sizeImage, delim);
                    qiSize = atoi(sizeImage);

                    //Create img file

                    imgOutput = (char*) malloc((strlen(subDir)+7+strlen(activeQuestion)+strlen(current))*sizeof(char)); //TODO verify this math
                    bzero(imgOutput,(strlen(subDir)+7+strlen(activeQuestion)+strlen(current))*sizeof(char));

                    strcat(imgOutput, subDir);
                    strcat(imgOutput, "/");
                    strcat(imgOutput, activeQuestion);
                    strcat(imgOutput, "_");
                    strcat(imgOutput, current);
                    strcat(imgOutput,qiExt);


                    fp = fopen(imgOutput, "wb");

                    char bufferFile[qiSize];


                    memset(bufferFile,0,sizeof(bufferFile));

                    long toRead = qiSize, readbytes;

                    printf("Retriving answer %s image file...\n", current);


                    while (toRead > 0) {

                        readbytes = read(fdTCP, bufferFile,toRead);
                        if (readbytes > 0 ) fwrite(bufferFile, 1, readbytes, fp);

                        memset(bufferFile,0,sizeof(bufferFile));
                        toRead -=readbytes;

                    }

                    memset(bufferFile,0,sizeof(bufferFile));
                    fclose(fp);

                    printf("Answer %s image retrived\n", current);


                    free(imgOutput);
                    memset(recvBuffer,0,sizeof(recvBuffer));
                }

                free(txtOutput);

            }

            i= 1; //sppace
            while (i > 0) {  // /n final
                n = read(fdTCP, recvBuffer,i );
                if (n == -1) exit(1);
                i -= n;

            }


            printf("Files downloaded to: TopicsReceived/%s/%s\n", formattedTopic, activeQuestion);
            free(qiExt);
            free(subDir);
            free(current);
        }
        free(questionUser);
        free(sizeQuestion);
        free(sizeImage);
    }
    free(space);
    
}

int checkInputSpaces(char *input) {
    int spacesCount=1, i = 0;

    while (input[i] ) {
        if (input[i] == ' ') spacesCount++;
        i++;
    }
    return spacesCount;
}

void readAndProcess(char *ip, char *port) {
    int topicNumber, questionNumber;
    char delim[] = " ", endOfLine[] = "\n";
    memset(&hints,0 , sizeof(hints));
    memset(&hintsTCP,0 , sizeof(hintsTCP));
    memset(&addr,0 , sizeof(addr));

    char *tcpResponse = (char*) malloc(11*sizeof(char));
    memset(tcpResponse,0, 11*sizeof(char));
    char *originalInput = (char*) malloc(BUFFER_SIZE*sizeof(char));
    memset(originalInput, 0, BUFFER_SIZE*sizeof(char));
    char termBuffer[BUFFER_SIZE] = {'\0'};
    read(1,termBuffer, sizeof(termBuffer));
    strcpy(originalInput, termBuffer);
    char *readableReq = strtok(termBuffer, endOfLine) ;
    char *inputToken = strtok(termBuffer, delim);

        if (!readableReq) printf("Error:Input missing\n");
        else if (strcmp(termBuffer, "register") == 0 || strcmp(termBuffer, "reg") == 0) {
            inputToken = strtok(NULL, delim);

            if (!inputToken) printf("Error: ID missing\n");
            else if (strlen(inputToken) != 5) printf("Error: ID must be 5 numbers long\n");
            else {
                if (strcmp(userID ,"-1" ) != 0) printf("User already registered\n");

                else {
                    strcat(requestCode, "REG ");
                    strcat(requestCode,inputToken );
                    strcpy(userID,inputToken);
                    sprintf(requestCode,"%s%c", requestCode, '\n');

                    hints.ai_family = AF_INET;
                	hints.ai_socktype = SOCK_DGRAM;
                	hints.ai_flags = AI_NUMERICSERV;


                    n = getaddrinfo(ip, port, &hints, &res);
                	if (n != 0) exit(1);

                    fdUDP = socket(res->ai_family,res->ai_socktype,res->ai_protocol);
                    if (fdUDP == -1) exit(1);

                    addrlen = sizeof(addr);
                    connectToUDP();
                    recvBuffer[strlen(recvBuffer)] = '\0';
                    if (strncmp(recvBuffer, "RGR OK",6) == 0) {
                        char *registeredID = strtok(inputToken, endOfLine);
                        printf("User '%s' registered\n", registeredID);
                    }
                    else if (strncmp(recvBuffer, "ERR",3) == 0){
                        printf("Error on protocol\n");
                        memset(userID, 0, 6);
                        strcpy(userID, "-1");
                    }
                    else {
                        strcpy(userID, "-1");
                        printf("Authentication failed. Try again\n");
                    }
                    freeaddrinfo(res);
                    close(fdUDP );
                    }
                }
		} else if (strcmp(readableReq, "topic_list") == 0 || strcmp(readableReq, "tl") == 0) {

            memset(&availableTopics,0, sizeof(availableTopics));

            strcpy(requestCode, "LTP\n");

            hints.ai_family = AF_INET;
        	hints.ai_socktype = SOCK_DGRAM;
        	hints.ai_flags = AI_NUMERICSERV;



            n = getaddrinfo(ip, port, &hints, &res);
        	if (n != 0) exit(1);

            fdUDP = socket(res->ai_family,res->ai_socktype,res->ai_protocol);
            if (fdUDP == -1) exit(1);

            addrlen = sizeof(addr);
            connectToUDP();
            recvBuffer[strlen(recvBuffer)] = '\0';

            if (strcmp(recvBuffer, "LTR 0\n") == 0) printf("There are no topics yet\n");
            else if (strncmp(recvBuffer, "ERR",3) == 0) printf("Error on protocol\n");
            else printToScreenTopics(recvBuffer);
            strcpy(availableTopics, recvBuffer);
            freeaddrinfo(res);
            close(fdUDP );

        } else if (strcmp(readableReq, "topic_select") == 0) {
            char *destTopic = (char*) malloc( 20*sizeof(char));
            memset(destTopic,0, 20*sizeof(char));
            inputToken = strtok(NULL, delim);

            if (!inputToken) printf("Error: topic name missing\n");
            else {
                getDesiredTopicByName(inputToken, destTopic);
                if (strcmp(destTopic, "\0")!= 0) {
                    memset(availableTopicQuestions,0,sizeof(availableTopicQuestions));
                    strcpy(availableTopicQuestions, "\0");
                    memset(activeTopic,0, sizeof(activeTopic));
                    strcpy(activeTopic, destTopic);
                    printf( "Selected topic: %s", activeTopic);

                }
                else printf("Non existing topic.\n");
        }
        free(destTopic);
        } else if (strcmp(readableReq, "ts") == 0) {
            char *destTopic = (char*) malloc( 20*sizeof(char));
            memset(destTopic,0, 20*sizeof(char));;

            inputToken =strtok(NULL, delim);
            if (!inputToken) printf("Error: topic number missing\n");
            else {
                topicNumber = atoi(inputToken);
                getDesiredTopicByNumber(topicNumber, destTopic);
                if (strcmp(destTopic, "\0")!= 0) {
                    memset(availableTopicQuestions,0,sizeof(availableTopicQuestions));
                    strcpy(availableTopicQuestions, "\0");
                    memset(activeTopic,0, sizeof(activeTopic));
                    strcpy(activeTopic, destTopic);
                    printf( "Selected topic: %s", activeTopic);

                }
            else printf("Non existing topic.\n");

        }
        free(destTopic);
        } else if (strcmp(readableReq, "topic_propose") == 0 || strcmp(readableReq, "tp") == 0) {
            if (strncmp(userID, "-1",2) == 0) printf("Error: must be signed in to create a topic\n");
            else {
                strcpy(requestCode, "PTP ");
                strcat(requestCode, userID);
                sprintf(requestCode, "%s%c", requestCode, ' ');
                inputToken = strtok(NULL, delim);
                if (!inputToken) printf("Error: topic name missing\n");
                else if (strlen(inputToken) > 10) {
                    printf("Error: topic name must have 10 or less characters\n");
                }
                else {
                    strcat(requestCode, inputToken);
                    sprintf(requestCode, "%s%c", requestCode, '\n');

                    hints.ai_family = AF_INET;
                	hints.ai_socktype = SOCK_DGRAM;
                	hints.ai_flags = AI_NUMERICSERV;



                    n = getaddrinfo(ip, port, &hints, &res);
                	if (n != 0) exit(1);

                    fdUDP = socket(res->ai_family,res->ai_socktype,res->ai_protocol);
                    if (fdUDP == -1) exit(1);

                    addrlen = sizeof(addr);
                    connectToUDP();
                    recvBuffer[strlen(recvBuffer)] = '\0';
                    if (strncmp(recvBuffer, "PTR OK",6) == 0) {
                        memset(activeTopic,0, sizeof(activeTopic));
                        strcpy(activeTopic, inputToken);
                        printf("Topic accepted\n");}
                    else if (strncmp(recvBuffer, "PTR DUP",7) == 0) printf("Error: Duplicate topic reported by server\n");
                    else if (strncmp(recvBuffer, "PTR FUL",7) == 0) printf("Error: topic list full reported by server\n");
                    else printf("Error: topic not created\n");
                    freeaddrinfo(res);
                    close(fdUDP );
                }
            }

        } else if (strcmp(termBuffer, "question_list") == 0 || strcmp(termBuffer, "ql") == 0) {
            if (strncmp(activeTopic, "\0",2) == 0) printf("Error: No topic selected\n");
            else {

                memset(availableTopicQuestions,0, sizeof(availableTopicQuestions));
                formatTopic();

                strcpy(requestCode, "LQU ");
                strcat(requestCode, formattedTopic);

                sprintf(requestCode, "%s%c", requestCode,'\n');

                hints.ai_family = AF_INET;
            	hints.ai_socktype = SOCK_DGRAM;
            	hints.ai_flags = AI_NUMERICSERV;



                n = getaddrinfo(ip, port, &hints, &res);
            	if (n != 0) exit(1);

                fdUDP = socket(res->ai_family,res->ai_socktype,res->ai_protocol);

                if (fdUDP == -1) exit(1);

                addrlen = sizeof(addr);

                connectToUDP();
                recvBuffer[strlen(recvBuffer)] = '\0';

                strcpy(availableTopicQuestions ,recvBuffer);

                printQuestionsToScreen(recvBuffer,formattedTopic);
                freeaddrinfo(res);
                close(fdUDP );

            }

        } else if (strcmp(readableReq, "question_get") == 0) {
            char recvDir[18] = "TopicsReceived/";

	        if(opendir(recvDir) == NULL) mkdir(recvDir, 0700);
            char *destQuestion = (char*) malloc( 20*sizeof(char));
            memset(destQuestion,0, 20*sizeof(char));

            inputToken = strtok(NULL, delim);
            if (!inputToken) printf("Error: topic number missing\n");

            else {
                strtok(inputToken,endOfLine);
                getDesiredQuestionByName(inputToken, destQuestion);
                if (strcmp(destQuestion, "\0")!= 0) {
                    memset(activeQuestion,0, sizeof(activeQuestion));
                    strcpy(activeQuestion, destQuestion);

                    hintsTCP.ai_family = AF_INET;
                    hintsTCP.ai_socktype = SOCK_STREAM;
                    hintsTCP.ai_flags = AI_NUMERICSERV;
                    n = getaddrinfo(ip, port, &hintsTCP, &resTCP);
                    fdTCP = socket(resTCP->ai_family,resTCP->ai_socktype,resTCP->ai_protocol);
                    if (fdTCP == -1) exit(1);

                    addrlen = sizeof(addr);


                    n = connect(fdTCP, resTCP->ai_addr, resTCP->ai_addrlen);
                    if (n==-1) {
                        printf("connect error");
                        exit(1);
                    }
                    strcpy(requestCode, "GQU");
                    strcat(requestCode, formattedTopic);
                    strcat(requestCode, " ");
                    strcat(requestCode, activeQuestion);
                    strcat(requestCode, "\n");
                    connectToTCP_qg(requestCode);
                    
                    freeaddrinfo(resTCP);
                    close(fdTCP);
                }
                else printf("Error: Non existing question\n");

            }

            free(destQuestion);

        } else if (strcmp(readableReq, "qg") == 0) {

            char recvDir[18] = "TopicsReceived/";

	        if(opendir(recvDir) == NULL) mkdir(recvDir, 0700);

            char *destQuestion = (char*) malloc( 20*sizeof(char));
            memset(destQuestion,0, 20*sizeof(char));
            inputToken = strtok(NULL, delim);
            if (!inputToken) printf("Error: question number missing\n");

            else if(strcmp(activeTopic, "\0") == 0) printf("Error: no topic selected\n");

            else {

                questionNumber = atoi(inputToken);

                if (questionNumber == 0) printf("Error: Invalid Number\n");
                getDesiredQuestionByNumber(questionNumber, destQuestion);
                if (strcmp(destQuestion, "\0") != 0) {
                    memset(activeQuestion,0, sizeof(activeQuestion));
                    strcpy(activeQuestion, destQuestion);
                    printf( "Selected question: %s\n", activeQuestion);

                    hintsTCP.ai_family = AF_INET;
                    hintsTCP.ai_socktype = SOCK_STREAM;
                    hintsTCP.ai_flags = AI_NUMERICSERV;
                    n = getaddrinfo(ip, port, &hintsTCP, &resTCP);
                    fdTCP = socket(resTCP->ai_family,resTCP->ai_socktype,resTCP->ai_protocol);
                    if (fdTCP == -1) exit(1);

                    addrlen = sizeof(addr);


                    n = connect(fdTCP, resTCP->ai_addr, resTCP->ai_addrlen);
                    if (n==-1) {
                        printf("connect error");
                        exit(1);
                    }


                    strcpy(requestCode, "GQU ");

                    strcat(requestCode, formattedTopic);
                    strcat(requestCode, " ");
                    strcat(requestCode, activeQuestion);
                    strcat(requestCode, "\n");

                    connectToTCP_qg(requestCode);

                    freeaddrinfo(resTCP);
                    close(fdTCP);
                }
                else printf("Non existing question.\n");

            }
            free(destQuestion);
        } else if (strcmp(readableReq, "question_submit") == 0 || strcmp(readableReq, "qs") == 0) {
            qIMG = false;
            int checkOK = 1, freeImage = 0;
            readableReq = strtok(NULL, delim);

            if (strcmp(userID ,"-1" ) == 0) printf("Error: must be signed in to create a question\n");

            else if (!readableReq) printf("Error: parameters missing\n");

            else if(strcmp(activeTopic, "\0")==0) printf("Error: no topic selected\n");

            else {
                hintsTCP.ai_family = AF_INET;
                hintsTCP.ai_socktype = SOCK_STREAM;
                hintsTCP.ai_flags = AI_NUMERICSERV;
                n = getaddrinfo(ip, port, &hintsTCP, &resTCP);
                fdTCP = socket(resTCP->ai_family,resTCP->ai_socktype,resTCP->ai_protocol);
                if (fdTCP == -1) exit(1);

                addrlen = sizeof(addr);


                n = connect(fdTCP, resTCP->ai_addr, resTCP->ai_addrlen);
                if (n==-1) {
                    printf("connect error");
                    exit(1);
                }

                int argNumber = checkInputSpaces(originalInput);

                if (strlen(readableReq) > 10) printf("Error: question must be 10 characters long or less\n");

                else if ( argNumber != 3 && argNumber != 4) printf("Error: some parameters missing or invalid\n");

                else {
                    question = (char*) malloc((strlen(readableReq)+1)*sizeof(char)); //free
                    memset(question,0, (strlen(readableReq)+1)*sizeof(char));
                    strcpy(question, readableReq);
                    //Handles filepath portion of input
                    readableReq = strtok(NULL, delim);
                    int ptrSize = (int)strlen(readableReq);
                    char *auxPath = (char*) malloc((ptrSize+5)*sizeof(char));
                    memset(auxPath,0, (ptrSize+5)*sizeof(char));


                    if(argNumber == 3) {

                        //no image next
                        filePath = (char*) malloc((ptrSize+6)*sizeof(char)); //free
                        memset(filePath,0, (ptrSize+6)*sizeof(char));
                        strcpy(auxPath, readableReq);
                        strcat(auxPath, ".txt");
                        FILE *txt = fopen(auxPath, "r");
                        if (txt == NULL) checkOK = 0;
                        else {
                            strcpy(filePath, auxPath);
                            filePath[ptrSize+5] = '\0';
                            fclose(txt);
                        }
                    } else {

                        filePath = (char*) malloc((ptrSize+5)*sizeof(char)); //free
                        memset(filePath,0, (ptrSize+5)*sizeof(char));
                        strcpy(auxPath, readableReq);
                        strcat(auxPath, ".txt");
                        FILE *txt = fopen(auxPath, "r");
                        if (txt == NULL)
                            checkOK = 0;


                        else {
                            strcpy(filePath, auxPath);
                            filePath[strlen(filePath)] = '\0';
                            fclose(txt);
                        //Handles img portion of input (if any)

                            readableReq = strtok(NULL, delim);
                            ptrSize = (int)strlen(readableReq);
                            if (readableReq != NULL) {

                                if(readableReq[ptrSize] == '\0') {
                                    freeImage = 1;
                                    imgPath = (char*) malloc((ptrSize+1)*sizeof(char)); //free
                                    memset(imgPath,0, (ptrSize+1)*sizeof(char));
                                    FILE *img = fopen(readableReq, "r");
                                    if (img == NULL) checkOK = 0;

                                    else {

                                        strcpy(imgPath, readableReq);
                                        fclose(img);
                                        imgPath[strlen(imgPath)] = '\0';
                                        qIMG = true;
                                    }
                                }
                            }
                        }
                    }

                    if (!checkOK) printf("Error: One or more files unavailable\n");

                    else {
                        //Handle txt file
                        if(!qIMG)
                        readInputFiles(filePath, NULL,question, 0, 1 );
                        else readInputFiles(filePath,imgPath,question,1,1);

                        memset(recvBuffer, 0, sizeof(recvBuffer));
                        memset(tcpResponse,0,11*sizeof(char));

                        readToBuffer(tcpResponse,1);


                        while (strcmp(recvBuffer, "\n") != 0) {
					        memset(recvBuffer, 0, strlen(recvBuffer));
                            n = read(fdTCP, recvBuffer, 1);
                            if ( n == - 1) exit(1);
                            if ( n > 0) strcat(tcpResponse, recvBuffer);


                        }
                        tcpResponse[strlen(tcpResponse)] = '\0';

                        if (strcmp(tcpResponse, "QUR OK\n") == 0) {
                            memset(activeQuestion,0, sizeof(activeQuestion));
                            strcpy(activeQuestion, question);
                            printf("Question accepted\n");
                        }
                        else if (strcmp(tcpResponse, "QUR DUP\n") == 0) printf("Error: Question already exists\n");
                        else if (strcmp(tcpResponse,"QUR FUL\n" ) == 0) printf("Error: Question list is full\n");
                        else printf("Error: Question not submitted\n");

                    }
                    if ( freeImage) free(imgPath);
                    free(filePath);
                    free(auxPath);
                    free(question);
                }
                freeaddrinfo(resTCP);
                close(fdTCP);

            }
        } else if (strcmp(readableReq, "answer_submit") == 0 || strcmp(readableReq, "as") == 0)  {
            qIMG = false;
            int checkOK = 1, freeImage = 0;
            readableReq = strtok(NULL, delim);
            if (strcmp(userID ,"-1" ) == 0) printf("Error: must be signed in to create a question\n");

            else if (!readableReq) printf("Error: parameters missing\n");

            else if(strcmp(activeTopic, "\0") == 0)  printf("Error: no topic selected\n");


            else if(strcmp(activeQuestion, "\0")==0) printf("Error: no question selected\n");

            else {
                hintsTCP.ai_family = AF_INET;
                hintsTCP.ai_socktype = SOCK_STREAM;
                hintsTCP.ai_flags = AI_NUMERICSERV;
                n = getaddrinfo(ip, port, &hintsTCP, &resTCP);
                fdTCP = socket(resTCP->ai_family,resTCP->ai_socktype,resTCP->ai_protocol);
                if (fdTCP == -1) exit(1);

                addrlen = sizeof(addr);


                n = connect(fdTCP, resTCP->ai_addr, resTCP->ai_addrlen);
                if (n==-1) {
                    printf("connect error");
                    exit(1);
                }

                int argNumber = checkInputSpaces(originalInput);


                if ( argNumber != 2 && argNumber != 3) printf("Error: some parameters missing or invalid\n");

                else {

                    //Handles filepath portion of input

                    int ptrSize = (int)strlen(readableReq);
                    char *auxPath = (char*) malloc( (ptrSize+5)*sizeof(char)); //free
                    memset(auxPath,0,  (ptrSize+5)*sizeof(char));


                    if(argNumber == 2) {

                        //no image next
                        filePath = (char*) malloc((ptrSize+5)*sizeof(char)); //free
                        memset(filePath, 0, (ptrSize+5)*sizeof(char));
                        strcpy(auxPath, readableReq);
                        strcat(auxPath, ".txt");
                        FILE *txt = fopen(auxPath, "r");
                        if (txt == NULL) checkOK = 0;
                        else {
                            strcpy(filePath, auxPath);
                            filePath[strlen(filePath)] = '\0';
                            fclose(txt);
                        }
                    } else {

                        filePath = (char*) malloc((ptrSize+5)*sizeof(char)); //free
                        memset(filePath, 0, (ptrSize+5)*sizeof(char));
                        strcpy(auxPath, readableReq);
                        strcat(auxPath, ".txt");
                        FILE *txt = fopen(auxPath, "r");
                        if (txt == NULL)
                            checkOK = 0;


                        else {
                            strcpy(filePath, auxPath);
                            filePath[strlen(filePath)] = '\0';

                            fclose(txt);
                        //Handles img portion of input (if any)

                            readableReq = strtok(NULL, delim);
                            ptrSize = (int)strlen(readableReq);
                            if (readableReq != NULL) {

                                if(readableReq[ptrSize] == '\0') {
                                    freeImage = 1;
                                    imgPath = (char*) malloc((ptrSize+5)*sizeof(char)); //free
                                    memset(imgPath, 0, (ptrSize+5)*sizeof(char));
                                    FILE *img = fopen(readableReq, "r");
                                    if (img == NULL) checkOK = 0;

                                    else {

                                        strcpy(imgPath, readableReq);
                                        fclose(img);
                                        imgPath[strlen(imgPath)] = '\0';
                                        qIMG = true;
                                    }
                                }
                            }
                        }
                    }

                    if (!checkOK) printf("Error: One or more files unavailable\n");

                    else {
                        //Handle txt file
                        if(!qIMG)
                        readInputFiles(filePath, NULL, activeQuestion, 0,2 );
                        else readInputFiles(filePath,imgPath,activeQuestion,1,2);

                        memset(recvBuffer, 0, sizeof(recvBuffer));
                        memset(tcpResponse,0,11*sizeof(char));

                        readToBuffer(tcpResponse,1);

                        while (strcmp(recvBuffer, "\n") != 0) {
					        memset(recvBuffer, 0, strlen(recvBuffer));
                            n = read(fdTCP, recvBuffer, 1);
                            if ( n == - 1) exit(1);
                            if ( n > 0 ) strcat(tcpResponse, recvBuffer);


                        }

                        tcpResponse[strlen(tcpResponse)] = '\0';
                        if (strcmp(tcpResponse, "ANR OK\n") == 0) printf("Answer accepted\n");
                        else if (strcmp(tcpResponse, "ANR DUP\n") == 0) printf("Error: Answer already exists\n");
                        else if (strcmp(tcpResponse,"ANR FUL\n" ) == 0) printf("Error: Answer list is full\n");
                        else printf("Error: Answer not submitted\n");

                    }
                    if ( freeImage) free(imgPath);
                    free(filePath);
                    free(auxPath);
                }
                freeaddrinfo(resTCP);
                close(fdTCP);


            }


        } else if (strcmp(readableReq, "exit") == 0) {
				printf("Program exiting...\n");

				exit(0);
                getOut = true;
		} else if (strcmp(readableReq, "?") == 0) {
            write(1,"- register/reg <userID>\n",25);
            write(1,"- topic_list/tl\n",17);
            write(1,"- topic_select <topic name>\n",29);
            write(1,"- ts <topic_number>\n",21);
            write(1,"- topic_propose/tp <topic>\n", 28);
            write(1,"- question_list/ql\n",18);
            write(1,"- question_get <question name>\n",32);
            write(1,"- qg <question_number>\n",24);
            write(1,"- answer_submit/as <text_file [image_file]>\n", 45);
            write(1,"- question_submit/qs <text_file [image_file]>\n",47);
            write(1,"- exit\n",8);
        }
        else
            printf( "Bad command\n");

        memset(&termBuffer, 0, sizeof(termBuffer));
        memset(&requestCode,0, sizeof(requestCode));
        memset(&recvBuffer,0, sizeof(recvBuffer));
        memset(&inputToken,0, sizeof(inputToken));
        memset(&readableReq,0, sizeof(readableReq));
        free(originalInput);
        free(tcpResponse);

		write(1, "Enter command (? for help) : ", 29);

}

int main(int argc, char **argv) {
    int invalid_flag = 1;
	long option_index;
	char *IP = NULL;
	char *PORT = NULL;
	opterr = 0;


    	while ((option_index = getopt(argc,argv, "n:p:")) !=-1){
    		switch(option_index) {
    			case 'n': {
    				IP = optarg;
                    invalid_flag = 0;
                    break;

                } case 'p': {
    				PORT = optarg;
                    invalid_flag = 0;
                    break;
                }
    		}
    	}


    if (invalid_flag && argc > 1) {
		fprintf(stderr,"Invalid argument.\n");
		exit(EXIT_FAILURE);
	}

	if ( !IP) {
		IP = "127.0.0.1";
	}
	if (!PORT) {
		//58000 + Group Number
		PORT = "58058"; //58
	}



    write(1, "Enter command (? for help) : ", 29);
    while(!getOut) {
        readAndProcess(IP, PORT);
    }


}
