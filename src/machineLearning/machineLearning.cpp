#include "dataDAO.h"
#include <stdio.h>
#include "include/limit.h"
#include "include/KCentroid.h"
#include "include/Node.h"
#include <unordered_map>

using namespace std;


unordered_map<string,Node*> Nodes;

int testData(Data* data){
	int resLimit = testLimit(data);
	string nodeName = string(data->fromNode);

	if(Nodes[nodeName] == NULL)
		Nodes[nodeName] = new Node();

	Node* node = Nodes[nodeName];
	int resKCentroid = node->kcentroidControl.testData(data,data->fromNode);
	int resKrls = node->krlsControl.testData(data,data->fromNode);
	int resRvm = node->rvmControl.testData(data,data->fromNode);
	
	return resLimit | resKCentroid | resKrls | resRvm;
}
