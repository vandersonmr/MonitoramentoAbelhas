#include <stdio.h>
#include "dataDAO.h"
#include <stdlib.h>

void printTemperatures(Data* temperature,int numrows){
    int i;
	for(i=0; i < numrows; i++){
        printf("%s & %s & %d & %d<br>\n",
	    	temperature[i].fromNode,
            temperature[i].time,
            temperature[i].temperature,
			temperature[i].status); 
    }
 
}

int main()
{
    printf("Content-Type: text/html;charset=us-ascii\n\n");
 
    if (!connectDatabase())
		return EXIT_FAILURE;

    Data* data = malloc(sizeof(Data)*75);
    int numrows;
    
	numrows = loadLastsDatas(data,75);

    printTemperatures(data,numrows);
    
    closeConnection();  
    return 0;
}
