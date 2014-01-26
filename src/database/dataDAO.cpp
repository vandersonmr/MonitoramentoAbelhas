#include <stdio.h>
#include <cmath>
#include <vector>
#include <string>
#include "dataDAO.hpp" 
#include <time.h>
#include <cstdlib>
#include <cstring>
#include <stdlib.h>

MYSQL* connection;

void getConfData(char* server, char* user, char* pass, char* db){
	FILE *fp = fopen(PATH_CONF,"r");
	if(fp != NULL){
		char *data, *type, *temp;
		data = (char*)malloc(LINE_SIZE*sizeof(char));
		type = (char*)malloc(LINE_SIZE*sizeof(char));
		temp = (char*)malloc(LINE_SIZE*sizeof(char));

		while (fgets(data,LINE_SIZE,fp) != NULL){
			sscanf(data,"%[^=]=\"%[^\"]",type,temp);
			if (strcmp(type,"SERVER") == 0)
				memcpy(server,temp,LINE_SIZE);
			else if (strcmp(type,"USER") == 0)
				memcpy(user,temp,LINE_SIZE);
			else if (strcmp(type,"PASSWORD") == 0)
				memcpy(pass,temp,LINE_SIZE);
			else if (strcmp(type,"DATABASE") == 0)
				memcpy(db,temp,LINE_SIZE);
		}
		fclose(fp);
		free(data);
		free(type);
		free(temp);
	}else{
		printf("Arquivo não encontrado");
		exit(1);
	}
}

int connectDatabase(){
	connection=mysql_init(NULL);

	if(connection == NULL){
		fprintf(stderr,"Inicialização no Mysql falhou.\n");
		return 0;
	}

	char *server = (char*)malloc(LINE_SIZE*sizeof(char));
	char *user = (char*)malloc(LINE_SIZE*sizeof(char));
	char *pass = (char*)malloc(LINE_SIZE*sizeof(char));
	char *db = (char*)malloc(LINE_SIZE*sizeof(char));

	getConfData(server,user,pass,db);
	
	connection = mysql_real_connect(connection,server,user,pass,db,0,NULL,0);

	if (strlen(server) == 0 && strlen(user) == 0 &&
			strlen(pass) == 0 && strlen(db) == 0){
		fprintf(stderr,"Dado vazio.\n");
		connection = NULL;
	}

	free(server);
	free(user);
	free(pass);
	free(db);

	if(connection == NULL){
		fprintf(stderr,"Impossível se conectar.\n");
		return 0;
	}

	return 1;
}


void saveData(Data data, int status){
	char query[LINE_SIZE];
  char *date;
  
  time_t t;
  time(&t);
  date = ctime(&t);
  sscanf(date,"%[^\n]",date);
	
  snprintf(query,LINE_SIZE,"INSERT INTO temperatures VALUES ('%s','%s','%d','%d','%s')",
          data.nickname.c_str(),date,(int)ceil(data.value),status,data.node.c_str());
	
  if (mysql_query(connection,query)) //return true if get an error.
		printf("%s\n",mysql_error(connection));
	
}

vector<Data> load(char* query){
	MYSQL_RES *res_set;
	MYSQL_ROW row;

	mysql_query(connection,query);

	res_set = mysql_store_result(connection);
  
  vector<Data> result; 

	while ((row = mysql_fetch_row(res_set)) != NULL){
    Data data;
		data.value = atoi((char*)row[2]);
		data.status = atoi((char*)row[3]);
		data.nickname = (char*) row[0];
		data.time = atof((char*) row[1]);
    data.node = (char*) row[4];
		result.push_back(data);
	}

	return result;	
}

vector<Data> loadLastsDatas(int q, string prefix) {
	char* queryWithOutQ = (char *) "select * from temperatures where Prefix like '%s' order by Date desc limit 0,%d;";
	char query[100];
  snprintf(query,100,queryWithOutQ,prefix.c_str(),q-1);
	return load(query);
}

vector<Data> loadLastsDatasByMinutes(int minutes) {
  char* queryWithOutQ = (char *) "select * from (select *,str_to_date(Date,'%%a %%b %%e %%H:%%i:%%s %%Y') as Time from temperatures) as t INNER JOIN nodesOnline ON t.nodeIP=nodesOnline.nodeID where t.Time > NOW() - INTERVAL %d MINUTE ORDER BY Time DESC;";
	char query[300];
  snprintf(query,300,queryWithOutQ,minutes-1);
	return load(query);
}

int clearNodesOnline() {
  char* query = (char*) "delete from nodesOnline;";
  return mysql_query(connection, query);
}

int insertNodeOnline(string prefix) {
  char* queryWithoutPrefix = (char*) "insert into nodesOnline values ('%s')";
  char query[200];
  snprintf(query,200, queryWithoutPrefix, prefix.c_str());
  return mysql_query(connection, query);
}

void closeConnection() {
	mysql_close(connection);
}