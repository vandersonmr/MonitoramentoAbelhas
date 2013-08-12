#ifndef DATADAO_H
#define DATADAO_H
#include "mysql/mysql.h"
#define PATH_CONF "config/db.conf"
#define LINE_SIZE 255

typedef struct Data{
	char* fromNode;
	int temperature;
	int status;
	char* time;
}Data;

int loadLastsDatasByMinutes(Data**,int);
void getConfData(char*, char*, char*, char*);
extern int connectDatabase();
extern void saveData(char*,char*,char*,int);
int loadLastsDatas(Data**,int,char*);
void closeConnection();

#endif
