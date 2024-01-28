#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <libgen.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <pwd.h>

//const char gDBDir[32] = "/usr/local/boomiViz/";
const char gDBDir[32] = "./";
const char gDBName[32] = "boomiProc.db";

void pSyntax(char *pDir) {
	printf("Syntax: %s -o [Process|Component] Objectname\n", pDir);
	exit(0);
}

int fCrMapNodes(char *pObjName,char *pNodeFile) {
	char lMapOut[512] = "tmp/nodeMap_";

	strcat(lMapOut,pObjName);
	strcat(lMapOut,".html");

	FILE *fMapOut;
	fMapOut = fopen(lMapOut,"w");
	fprintf(fMapOut,"<!doctype html>");
	fprintf(fMapOut,"<html>\n<center>\n");
	fprintf(fMapOut,"<title>Network::%s",pObjName);
	fprintf(fMapOut,"</title>\n");
	fprintf(fMapOut,"</title><head>\n");
	fprintf(fMapOut,"<style type=\"text/css\">\n");
	fprintf(fMapOut,"#mynetwork {\n");
	fprintf(fMapOut,"width: 1680px;\n");
	fprintf(fMapOut,"height: 900px;\n");
	fprintf(fMapOut,"border: 0px solid lightgray;}\n");
	fprintf(fMapOut,"body {\n");
	fprintf(fMapOut,"background: #bdc3c7; background: -webkit-linear-gradient(to right, #2c3e50, #bdc3c7); background: linear-gradient(to right, #2c3e50, #bdc3c7);</style>\n");
	fprintf(fMapOut,"<script type=\"text/javascript \"src=\"https://cdnjs.cloudflare.com/ajax/libs/vis/4.21.0/vis-network.min.js\"></script>\n");
	fprintf(fMapOut,"<script type=\"text/javascript\">\n");
	fprintf(fMapOut,"function draw() {\n");
	fprintf(fMapOut,"var nodes = [\n");

	FILE *fNodeMap;
	fNodeMap = fopen(pNodeFile,"r");
	if (fNodeMap == NULL)
	{
		fprintf(stderr, "Unable to read '%s'\n",pNodeFile);
		exit(1);
	}

	int intIdx=0;
	int intNodeIdx=0;
	int intSz=0;
	int intLen=0;
	int intProc=0;
	int intFieldIDIn;
	char strIn[255];
	char strDelim[2]="|";
	int intFieldID[255];
	char *strField;
	char *strFieldID;
	char *strFieldSrc;
	char *strFieldLbl;
	char *strFieldType;

	while(fgets(strIn, 255, (FILE*) fNodeMap))
	{
		intIdx+=1;
		intSz=20;
		intProc=1;

		strField = strtok(strIn, strDelim);
		strFieldLbl = strField;
		while(strField != NULL)
		{
			strField = strtok(NULL, strDelim);
			strFieldLbl = strField;
			strField = strtok(NULL, strDelim);
			strFieldID = strField;
			intFieldIDIn=atoi(strFieldID);
			strField = strtok(NULL, strDelim);
			strFieldType = strField;
			if(strcmp(strFieldLbl,pObjName)==0) 
			{
				intLen = strlen(strFieldType);
				if (strFieldType[intLen-1]='\n')
				{
					strFieldType[intLen-1]='\0';
				}

				if((strcmp(strFieldType,"process")==0) || strcmp(strFieldLbl,pObjName)==0)
				{
					intSz=35;
				}
			}

			for (intNodeIdx=1; intNodeIdx <= intIdx; intNodeIdx++)
			{
				if (intFieldIDIn == intFieldID[intNodeIdx])
				{
					intProc=0;
					break;
				}
			}

			if(intProc==1)
			{
				fprintf(fMapOut,"\n{id: %s,\"label\":\"%s\",\"group\":%d, \"size\": %d, font: { size: %d, face: 'Segoe UI', color: 'orange'}},",strFieldID,strFieldLbl,intIdx,intSz,intSz);
			}

			strField = strtok(NULL, strDelim);
			intFieldID[intIdx]=atoi(strFieldID);
		}
	}

	fprintf(fMapOut,"];\n");
	fclose(fMapOut);

	char _exec[255]="xdg-open ";
	strcat(_exec,lMapOut);
	system(_exec);
	return 0;
}

int fCrMapEdges(char *pObjName,char *pNodeFile) {
	char lMapOut[512] = "tmp/nodeMap_";

	strcat(lMapOut,pObjName);
	strcat(lMapOut,".html");

	FILE *fNodeMap;
	fNodeMap = fopen(pNodeFile,"r");
	if (fNodeMap == NULL)
	{
		fprintf(stderr, "Unable to read '%s'\n",pNodeFile);
		exit(1);
	}

	FILE *fMapOut;
	fMapOut = fopen(lMapOut,"a");
	fprintf(fMapOut,"var edges = [\n");

	char strIn[255];
	char strDelim[2]="|";
	char *strField;
	char *strFieldSrc;
	char *strFieldTarget;
	while(fgets(strIn, 255, (FILE*) fNodeMap))
	{
		strField = strtok(strIn, strDelim);
		while(strField != NULL)
		{
			strField = strtok(NULL, strDelim);
			strField = strtok(NULL, strDelim);
			strFieldSrc = strField;
			strField = strtok(NULL, strDelim);
			strField = strtok(NULL, strDelim);
			strFieldTarget = strField;
			fprintf(fMapOut,"\n{from: %s,\"to\":%s,arrows:'to'},",strFieldSrc,strFieldTarget);
			strField = strtok(NULL, strDelim);
		}
	}
	fprintf(fMapOut,"];\n");

	fprintf(fMapOut,"var container = document.getElementById('mynetwork');\n");
	fprintf(fMapOut,"var data = {nodes: nodes,edges: edges};\n");
	fprintf(fMapOut,"var options = { nodes: {shape: 'dot',size: 16 },\n");
	fprintf(fMapOut,"physics: { forceAtlas2Based: { gravitationalConstant: -70, centralGravity: 0.005, springLength: 230, springConstant: 0.18 },\n");
	fprintf(fMapOut,"maxVelocity: 150,solver: 'forceAtlas2Based', timestep: 0.05, stabilization: {iterations: 150}}};\n");
	fprintf(fMapOut,"var network = new vis.Network(container, data, options); } </script>\n");
	fprintf(fMapOut,"</head>\n");
	fprintf(fMapOut,"<body onload=\"draw()\">\n");
	fprintf(fMapOut,"<div id=\"mynetwork\"></div></center></body></html>");

	fclose(fMapOut);
}

int fChkDir(char *pDir) {
	printf("Checking directory '%s'...\n",pDir);

	DIR *lDir = opendir(pDir);
	DIR *lDirP;

	if (lDir) {
    		closedir(lDir);
		strcat(pDir,"/process");
		lDirP = opendir(pDir);
		if (lDirP) {
			return 0;
		} else if (ENOENT == errno) {
			return 1;
		}
		else
		{
			return 2;
		}
	} else if (ENOENT == errno) {
		return 10;
	} else {
	return 20;
	}
}

int main(int argc,char* argv[]) {
	int lRetVal = 0;

	char *strBName = basename(argv[0]);
	char *strObjType = argv[2];
	char *strObjName = argv[3];

	//struct passwd *pw = getpwuid(getuid());
	//const char *strHomeDir = pw->pw_dir;

	if(argc != 4)
	{
		pSyntax(strBName);
	}

	char strSQLSrc[1024];
	char *strSQLTarget;

	if (strcmp(strObjType,"Process")==0)
	{
		strcpy(strSQLSrc,"select main_process,source_node,source,source_type from vw_get_process_component_map ");
		strSQLTarget="select main_process,target_node,target,target_type from vw_get_process_component_map ";
		strcat(strSQLSrc,"where Main_Process = '");
		strcat(strSQLSrc,strObjName);
		strcat(strSQLSrc,"' union ");
		strcat(strSQLSrc,strSQLTarget);
		strcat(strSQLSrc,"where Main_Process = '");
		strcat(strSQLSrc,strObjName);
		strcat(strSQLSrc,"' order by 1");
	}
	else if (strcmp(strObjType,"Component")==0)
	{
		strcpy(strSQLSrc,"select main_process,source_node,source,source_type from vw_get_source_component_map ");
		strSQLTarget="select main_process,target_node,target,target_type from vw_get_source_component_map ";
		strcat(strSQLSrc,"where source_node = '");
		strcat(strSQLSrc,strObjName);
		strcat(strSQLSrc,"' union ");
		strcat(strSQLSrc,strSQLTarget);
		strcat(strSQLSrc,"where source_node = '");
		strcat(strSQLSrc,strObjName);
		strcat(strSQLSrc,"' order by 1");
	}

	char lExecSQLProc[2048] = "";
	char lExecSQLOut[512] = "tmp/";

	char *dirname = "tmp";
        if(mkdir(dirname,0700) && errno != EEXIST ) {
                printf("error while trying to create '%s' (%m)\n",dirname);
                exit(1);
        }

	int lCount = 0;
	int lIdx;
	char strObjNameTmp[512];

	//strObjNameTmp = strObjName;
	strcpy(strObjNameTmp,strObjName);
  
    	for (int lIdx = 0; strObjNameTmp[lIdx]; lIdx++) 
	{
        	if (strObjNameTmp[lIdx] != ' ') 
		{
            		strObjNameTmp[lCount++] = strObjNameTmp[lIdx];
   		}
 	}

    	strObjNameTmp[lCount] = '\0';

	//strcat(lExecSQLOut,strObjName);
	strcat(lExecSQLOut,strObjNameTmp);
	strcat(lExecSQLOut,".out");

        strcpy(lExecSQLProc,"sqlite3 ");
        strcat(lExecSQLProc,gDBDir);
        strcat(lExecSQLProc,gDBName);
        strcat(lExecSQLProc," \"");

	//strcat(lExecSQLProc,"sqlite3 ./boomiProc.db \"");
	strcat(lExecSQLProc,strSQLSrc);
	strcat(lExecSQLProc,"\" > ");
	strcat(lExecSQLProc,lExecSQLOut);
	lRetVal = system(lExecSQLProc);

	FILE *fNodeOut;
	int fSz;

	fNodeOut = fopen(lExecSQLOut,"r");
	fseek(fNodeOut, 0L, SEEK_END);
	fSz = ftell(fNodeOut);
	if (fSz==0)
	{
		printf("No data returned for %s '%s'\n",strObjType,strObjName);
		lRetVal = remove(lExecSQLOut);
		if(lRetVal!=0)
		{
			puts("\nFailed to remove tmp file");
		}
		exit(0);
	}

        //char cwd[255];
        //getcwd(cwd, sizeof(cwd));

	lRetVal = fCrMapNodes(strObjName,lExecSQLOut);

	if (strcmp(strObjType,"Process")==0)
	{
		strcpy(strSQLSrc,"select main_process,source_node,source,target_node,target from vw_get_process_component_map ");
		strcat(strSQLSrc,"where Main_Process = '");
		strcat(strSQLSrc,strObjName);
		strcat(strSQLSrc,"' order by 1,2,3,4");
	}
	else if (strcmp(strObjType,"Component")==0)
	{
		strcpy(strSQLSrc,"select main_process,source_node,source,target_node,target from vw_get_source_component_map ");
		strcat(strSQLSrc,"where source_node = '");
		strcat(strSQLSrc,strObjName);
		strcat(strSQLSrc,"' order by 1,2,3,4");
	}

	strcpy(lExecSQLProc,"");
        strcpy(lExecSQLProc,"sqlite3 ");
        strcat(lExecSQLProc,gDBDir);
        strcat(lExecSQLProc,gDBName);
        strcat(lExecSQLProc," \"");

	//strcat(lExecSQLProc,"sqlite3 ./boomiProc.db \"");
	strcat(lExecSQLProc,strSQLSrc);
	strcat(lExecSQLProc,"\" > ");
	strcat(lExecSQLProc,lExecSQLOut);
	lRetVal = system(lExecSQLProc);
	puts(lExecSQLProc);
	exit;

	fNodeOut = fopen(lExecSQLOut,"r");
	fseek(fNodeOut, 0L, SEEK_END);
	fSz = ftell(fNodeOut);
	if (fSz==0)
	{
		printf("\nNo data returned for %s '%s'",strObjType,strObjName);
		lRetVal = remove(lExecSQLOut);
		if(lRetVal!=0)
		{
			puts("\nFailed to remove tmp file");
		}
		exit(0);
	}

	lRetVal = fCrMapEdges(strObjName,lExecSQLOut);
	lRetVal = remove(lExecSQLOut);

	printf("Processing completed\n");
	return 0;
}
