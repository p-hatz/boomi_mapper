#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <string.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <mxml.h>

const char *gFInsProcess = "tmp/insProcess.sql";
const char *gFInsComp = "tmp/insComp.sql";
//const char gDBDir[32] = "/usr/local/boomiViz/";
const char gDBDir[32] = "./";
const char gDBName[32] = "boomiProc.db";

const char *gSQLProcessImpPre = "insert into imp_process (uid,name) values (";
const char *gSQLProcessPre = "insert into process (uid,name)";
const char *gSQLCompImpPre = "insert into imp_component (uid,name,_type,p_uid,r_uid) values (";
const char *gSQLCompPre = "insert into component (uid,name) ";
const char *gSQLInsSrcNodePre = "insert into source_node (uid,r_uid)";
const char *gSQLInsNodeCompPre = "insert into source_node_component (node_id,component_id)";

char gProc[100][50];
char gProcName[250];
char gRootProcFile[250];

xmlDoc *gdocChk = NULL;
xmlXPathContext *gxCtx;
xmlNodeSet *gxRes;
xmlXPathObject *gres;

void pSyntax(char *pDir) {
	printf("Syntax: %s -d AtomInstallationDir\n", pDir);
	exit(0);
}

xmlXPathObjectPtr getnodeset (xmlDocPtr doc, xmlChar *xpath) {
	xmlXPathContextPtr context;
	xmlXPathObjectPtr result;

	context = xmlXPathNewContext(doc);
	if (context == NULL) {
		printf("Error in xmlXPathNewContext\n");
		return NULL;
	}
	result = xmlXPathEvalExpression(xpath, context);
	xmlXPathFreeContext(context);
	if (result == NULL) {
		printf("Error in xmlXPathEvalExpression\n");
		return NULL;
	}
	if(xmlXPathNodeSetIsEmpty(result->nodesetval)){
		xmlXPathFreeObject(result);
                printf("No result\n");
		return NULL;
	}
	return result;
}

int fProcessProcDir(char *pDir) {
	char **lFileProc = NULL;
	char **s = NULL;
	char l[50][200];
	int lIdx = 0;
	int lCount = 0;

	DIR *lDir;
	char *lExt;
	struct dirent *lDirStruct;
	lDir = opendir(pDir);
	if (lDir) {
		while ((lDirStruct = readdir(lDir)) != NULL)
        	{
			if (strcmp(lDirStruct->d_name,".") !=0 && strcmp(lDirStruct->d_name,"..") !=0) {
				lExt = strrchr(lDirStruct->d_name,'.');
				if(!lExt)
				{
					strcpy(gProc[lCount], lDirStruct->d_name);
					lCount++;
				}
			}
        	}
	        closedir(lDir);
	}

	return lCount;
}

int fProcRefs(char *pSrc, char *pDir) {
	xmlDoc *docChk = NULL;
	xmlDoc *docChkType = NULL;
	xmlXPathContext *xCtx;
	xmlXPathContext *xpathCtx;
	xmlNodeSet *xRes;
	xmlXPathObject *res;
	xmlNode *lVal;

	int i;
	int lInsSQL = 0;
	char *lRefId;
	char *lCompId;
	char *lCompName;
	xmlXPathObject *val;
	xmlXPathObject *xpathObj;
	xmlNodeSet *xRes2;
	char lProcFileChk[500];
	char *strDName;
	char lSQL[2048];

	xmlXPathObjectPtr result;
	xmlNodeSetPtr nodeset;
	xmlChar *lNode;

	xmlXPathContext *xCtx2;
	xmlXPathObjectPtr result2;
	xmlNodeSetPtr nodeset2;

	FILE *fSQLComp;
	fSQLComp = fopen(gFInsComp,"a");

	puts("Phase 1: Processing references...");
	docChk=xmlReadFile(pSrc, NULL, 0);
	xCtx = xmlXPathNewContext(docChk);
	res = xmlXPathEval("//Component/References/Ref", xCtx);
	xRes = res->nodesetval;
	strDName = dirname(pSrc);

	if(!xmlXPathNodeSetIsEmpty(xRes))
	{
		for(i = 0; i < xmlXPathNodeSetGetLength(xRes); i++)
        	{
			lVal = xmlXPathNodeSetItem(xRes, i);
			val = xmlXPathNodeEval(lVal, "@refId", xCtx);
			lRefId = xmlXPathCastNodeSetToString(val->nodesetval);

			//lVal = xmlXPathNodeSetItem(xRes, i);
			val = xmlXPathNodeEval(lVal, "@compId", xCtx);
			lCompId = xmlXPathCastNodeSetToString(val->nodesetval);

			//lVal = xmlXPathNodeSetItem(xRes, i);
			val = xmlXPathNodeEval(lVal, "@name", xCtx);
			lCompName = xmlXPathCastNodeSetToString(val->nodesetval);

			strcpy(lProcFileChk,"");
			strcpy(lProcFileChk,strDName);
			strcat(lProcFileChk,"/");
			strcat(lProcFileChk,lCompId);
			strcat(lProcFileChk,".xml");

			FILE *fp;
			mxml_node_t *tree;
			fp = fopen(lProcFileChk, "r");
			tree = mxmlLoadFile(NULL, fp, MXML_OPAQUE_CALLBACK);
			mxml_node_t *node = mxmlFindPath(tree, "Component/Type");
			/*mxml_node_t *node = mxmlFindElement(tree, tree, "Type",
                                        NULL,NULL,
                                        MXML_DESCEND);*/
			const char *lCompType = mxmlGetOpaque(node);
			fclose(fp);

			char lParentProcFile[100];
			strcpy(lParentProcFile,gRootProcFile);
			strcpy(lParentProcFile,basename(lParentProcFile));
			//lParentProcFile[strlen(lParentProcFile) - 4] = '\0';

			if (lCompType=="")
			{
				lCompType = lRefId;
			}

			if (strcmp(lCompType,"process")==0)
			{
				strcpy(lSQL,"");
				strcat(lSQL,gSQLCompImpPre);
				strcat(lSQL,"'");
				strcat(lSQL,lParentProcFile);
				strcat(lSQL,"','");
				strcat(lSQL,lCompName);
				strcat(lSQL,"','");

				if (lCompId != basename(pSrc))
				{
					lCompType = "subprocess";
				}

				strcat(lSQL,lCompType);
				strcat(lSQL,"','");
				strcat(lSQL,lCompId);
				strcat(lSQL,"','");
				strcat(lSQL,basename(pSrc));
				strcat(lSQL,"');");
       		             	fprintf(fSQLComp,"%s\n",lSQL);

				lInsSQL = 1;

			}

			//if(lCompType != "note" && lCompType != NULL)
                        //if (strcmp(lCompType,"note")!=0)
			if(lCompType != NULL && lInsSQL == 0)
			{
				strcpy(lSQL,"");
                            	strcat(lSQL,gSQLCompImpPre);
                            	strcat(lSQL,"'");
                            	strcat(lSQL,lCompId);
                            	strcat(lSQL,"','");
                            	strcat(lSQL,lCompName);
                            	strcat(lSQL,"','");
                            	strcat(lSQL,lCompType);
                            	strcat(lSQL,"','");
                            	strcat(lSQL,lParentProcFile);
                            	strcat(lSQL,"','");
                            	strcat(lSQL,gRootProcFile);
                            	strcat(lSQL,"');");
       		             	fprintf(fSQLComp,"%s\n",lSQL);
			}

			lInsSQL = 0;
		}
	}

	xmlXPathFreeObject(res);
	xmlXPathFreeContext(xCtx);
	xmlFreeDoc(docChk);
	xmlFreeDoc(docChkType);
        xmlCleanupParser();
	fclose(fSQLComp);

	return 0;
}

char fProcShapes(char *pSrc, char *pDir) {
	xmlDoc *docChk = NULL;
	xmlDoc *docChkType = NULL;
	xmlXPathContext *xCtx;
	xmlXPathContext *xpathCtx;
	xmlNodeSet *xRes;
	xmlXPathObject *res;
	xmlNode *lVal;

	int i;
	char *lRefId;
	char *lCompId;
	char *lCompName;
	char *lCompType;
	char *lCompLbl;
    	//char *lCompType;
	xmlXPathObject *val;
	xmlXPathObject *xpathObj;
	xmlNodeSet *xRes2;
	char lProcFileChk[500];
	char *strDName;
	char lSQL[2048];

	xmlXPathObjectPtr result;
	xmlNodeSetPtr nodeset;
	xmlChar *lNode;

	xmlXPathContext *xCtx2;
	xmlXPathObjectPtr result2;
	xmlNodeSetPtr nodeset2;

	FILE *fSQLComp;
	fSQLComp = fopen(gFInsComp,"a");

	docChk=xmlReadFile(pSrc, NULL, 0);
	xCtx = xmlXPathNewContext(docChk);
	res = xmlXPathEval("//Component/Object/process/shapes/shape", xCtx);
	xRes = res->nodesetval;
	strDName = dirname(pSrc);

	puts("Phase 2: Processing shapes...");
	if(!xmlXPathNodeSetIsEmpty(xRes))
	{
		for(i = 0; i < xmlXPathNodeSetGetLength(xRes); i++)
        	{
			lVal = xmlXPathNodeSetItem(xRes, i);
			val = xmlXPathNodeEval(lVal, "@name", xCtx);

			lRefId = xmlXPathCastNodeSetToString(val->nodesetval);

			val = xmlXPathNodeEval(lVal, "@userlabel", xCtx);
			lCompLbl = xmlXPathCastNodeSetToString(val->nodesetval);

			val = xmlXPathNodeEval(lVal, "@shapetype", xCtx);
			lCompType = xmlXPathCastNodeSetToString(val->nodesetval);

			if (strcmp(gProcName,lCompLbl)!=0 && strcmp(lCompType,"start")!=0)
			{
				if (strcmp(lCompLbl,"") == 0)
				{
       	                 		strcpy(lCompLbl,lCompType);
				}

                                if (strcmp(lCompType,"processcall")!=0 && strcmp(lCompType,"note")!=0)
				{
					if (lCompType == "")
					{
						lCompType = "null";
					}
				}
			}

			char lParentProcFile[100];
			strcpy(lParentProcFile,pSrc);
			strcpy(lParentProcFile,basename(lParentProcFile));
			//lParentProcFile[strlen(lParentProcFile) - 4] = '\0';

                        strcpy(lSQL,"insert into imp_component (name,_type,p_uid,r_uid) values ('");
			strcat(lSQL,lCompType);
			strcat(lSQL,"','");
			strcat(lSQL,lCompType);
			strcat(lSQL,"','");
                        strcat(lSQL,lParentProcFile);
                        strcat(lSQL,"','");
                        strcat(lSQL,gRootProcFile);
			strcat(lSQL,"');");
       		        fprintf(fSQLComp,"%s\n",lSQL);
		}
	}

	xmlXPathFreeObject(res);
	xmlXPathFreeContext(xCtx);
	xmlFreeDoc(docChk);
	xmlFreeDoc(docChkType);
        xmlCleanupParser();
	fclose(fSQLComp);

	return 0;
}

int fProcProcessDir(char *pDir, char *pSrc) {
	xmlDoc *doc = NULL;
	xmlNode *root_element = NULL;

	char lProcFile[500];
	char lProcDir[500];
	strcpy(lProcFile,"");
	strcat(lProcFile,pDir);
	strcat(lProcFile,pSrc);
	strcpy(lProcDir,lProcFile);

	char lSQL[1000];

	int lIdx;
	int lXIdx;
	int lRetVal;
	xmlChar *lNode;
	xmlChar strNode[3][100];
	xmlChar lNodeVal[3][100];
	xmlChar *xPathId = (xmlChar*) "//Component/Id";
	xmlChar *xPathName = (xmlChar*) "//Component/Name";
	xmlNodeSetPtr nodeset;
	xmlXPathObjectPtr result;
	char *lCompName;
	char lProcFilePre[100];

	xmlChar xPath[5][100];
	strcpy(xPath[0],"//Component/Id");
	strcpy(xPath[1],"//Component/Name");
	strcpy(xPath[2],"//Component/Type");

	FILE *fSQLProc;
	fSQLProc = fopen(gFInsProcess,"a");

	FILE *fSQLComp;
	fSQLComp = fopen(gFInsComp,"a");

	DIR *lDir;
	struct dirent *lDirStruct;
	lDir = opendir(lProcDir);

	char *lExt;

	char lProcFileShapes[250];
	if (lDir) {
		while ((lDirStruct = readdir(lDir)) != NULL)
		{
			lExt = strrchr(lDirStruct->d_name,'.');
			if(lExt && strcmp(lExt,".xml")==0)
			{
				strcpy(lProcFile,"");
				strcat(lProcFile,pDir);
				strcat(lProcFile,pSrc);
				strcat(lProcFile,"/");
				strcat(lProcFile,lDirStruct->d_name);
				strcpy(lProcFilePre,lProcFile);
				strcpy(gRootProcFile,basename(lProcFilePre));
				gRootProcFile[strlen(gRootProcFile) - 4] = '\0';
				printf("Processing: %s\n",lProcFilePre);
				doc=xmlReadFile(lProcFilePre, NULL, 0);
				if (doc==NULL) {
					printf("Could not parse the XML file");
				}

				FILE *fp;
				mxml_node_t *tree;
				fp = fopen(lProcFile, "r");
				tree = mxmlLoadFile(NULL, fp, MXML_OPAQUE_CALLBACK);

				mxml_node_t *node = mxmlFindPath(tree, "Component/Name");
				const char *lCompName = mxmlGetOpaque(node);
				strcpy(gProcName,lCompName);
				fclose(fp);

				for (lXIdx=0; lXIdx <= 2; lXIdx++)
				{
					result = getnodeset (doc, xPath[lXIdx]);
					if (result) {
						nodeset = result->nodesetval;
						for (lIdx=0; lIdx < nodeset->nodeNr; lIdx++) {
							lNode = xmlNodeListGetString(doc, nodeset->nodeTab[lIdx]->xmlChildrenNode, 1);
							if (lNode)
							{
								strcpy(lNodeVal[lXIdx],lNode);
								if (strcmp(lNode,"process") == 0)
								{
									if (strcmp(lNodeVal[0],pSrc) != 0)
									{
										strcpy(lNodeVal[2],"subprocess");
									}

									char lFileName[50];
									strcpy(lSQL,"");
									strcat(lSQL,gSQLProcessImpPre);
									strcat(lSQL,"'");
									strcpy(lFileName,lDirStruct->d_name);
									lFileName[strlen(lFileName) - 4] = '\0';
									strcat(lSQL,lFileName);
									strcat(lSQL,"','");
									strcat(lSQL,lNodeVal[1]);
									strcat(lSQL,"');");
       		             						fprintf(fSQLProc,"%s\n",lSQL);

									if (lXIdx == 2)
									{
										strcpy(lSQL,"");
										strcat(lSQL,gSQLCompImpPre);
										strcat(lSQL,"'");
										strcat(lSQL,lFileName);
										strcat(lSQL,"','");
										strcat(lSQL,lNodeVal[1]);
										strcat(lSQL,"','");
										strcat(lSQL,lNodeVal[2]);
										strcat(lSQL,"','");
										strcat(lSQL,lNodeVal[0]);
										strcat(lSQL,"','");
										strcat(lSQL,pSrc);
										strcat(lSQL,"');");
       		             							fprintf(fSQLComp,"%s\n",lSQL);
									}
								}
							}
							xmlFree(lNode);
						}
						xmlXPathFreeObject (result);
					}
				}

				strcpy(lProcFileShapes,"");
				strcpy(lProcFileShapes,lProcFile);
				lRetVal = fProcRefs(lProcFile, pDir);
				lRetVal = fProcShapes(lProcFileShapes, pDir);
			}
		}

		xmlFreeDoc(doc);
		xmlCleanupParser();
	}

	fclose(fSQLProc);
	fclose(fSQLComp);

	return 0;
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
	const char *strNodeID = "Component/Id";
	const char *strNodeName = "Component/Name";
	const char *strNodeType = "Component/Type";
	const char *strNodeRefs = "Component/References";
	const char *strNodeShapes = "Component/Object/process/shapes";
	const char *strNodeConnAction = "Component/Object/process/shape/configuration/connectoraction";
	int lRet = 0;

	char *strBName = basename(argv[0]);

	if(argc != 3)
	{
		pSyntax(strBName);
	}

	char *strAtomDir = argv[2];
	char strAtomDirPre[256];

	strcpy(strAtomDirPre,strAtomDir);

	int lRetVal;
	lRetVal = fChkDir(strAtomDir);
	if (lRetVal != 0)
	{
		if (lRetVal == 1) {
			printf("Directory '%s' doesn't look like a Boomi directory!\n",strAtomDirPre);
		} else {
			printf("An error occurred reading '%s'!\n",strAtomDir);
		}
		exit(lRetVal);
	} else {
		puts("Looks good. Continuing...");
	}

	lRetVal = 0;
	char ret[5];
	int lRetCount = 0;
	
	lRetCount = fProcessProcDir(strAtomDir);
	if (lRetCount > 0) {
		printf("Detected %d processes\n",lRetCount);
	} else {
		puts("Didn't detect any processes!");
		exit(0);
	}

	char *dirname = "tmp"; 
	if(mkdir(dirname,0700) && errno != EEXIST ) {
		printf("error while trying to create '%s' (%m)\n",dirname);
		exit(1);
	}

	const char *strSQLProc = "delete from imp_process;";
	const char *strSQLComp = "delete from imp_component;";

	FILE *fSQLProc;
	fSQLProc = fopen(gFInsProcess,"w");
	fprintf(fSQLProc,"%s\n",strSQLProc);
	fclose(fSQLProc);

	FILE *fSQLComp;
	fSQLComp = fopen(gFInsComp,"w");
	fprintf(fSQLComp,"%s\n",strSQLComp);
	fclose(fSQLComp);

	char *lProcessesDirPre = "/processes/";
	char lProcDir[500];

	char lExecSQLProc[500];
	strcpy(lExecSQLProc,"sqlite3 ");
	strcat(lExecSQLProc,gDBDir);
	strcat(lExecSQLProc,gDBName);
	strcat(lExecSQLProc," < tmp/insProcess.sql");

	char lExecSQLComp[500];
	strcpy(lExecSQLComp,"sqlite3 ");
	strcat(lExecSQLComp,gDBDir);
	strcat(lExecSQLComp,gDBName);
	strcat(lExecSQLComp," < tmp/insComp.sql");

	int lIdx = 0;
	strcat(strAtomDirPre,lProcessesDirPre);

	int lRetValSys;
	char lSQL[2048];

	printf("Working directory set to '%s'\n",strAtomDirPre);
        for(lIdx = 0; lIdx < lRetCount; lIdx++) {
		strcpy(lProcDir,gProc[lIdx]);
		if (strcmp(lProcDir, "") != 0)
		{
			printf("[Processing directory %s...]\n",lProcDir);
			lRetVal = fProcProcessDir(strAtomDirPre,lProcDir);
			if (lRetVal == 0)
			{
				fSQLComp = fopen(gFInsComp,"a");

				strcpy(lSQL,"");
                            	strcpy(lSQL,gSQLCompPre);
                            	strcat(lSQL,"select i.uid,i.name from imp_component i where not exists (select 1 from component c_x where c_x.uid=i.uid) and i.uid is not null");
                            	strcat(lSQL," and not exists (select 1 from imp_component i_x where i_x.id = i.id and i_x._type = 'subprocess' and i_x.uid = i_x.r_uid)");
                            	strcat(lSQL," union select i.uid,i.name from imp_component i where not exists (select 1 from component c_x where c_x.name=i.name) and i.uid is null;");
       		             	fprintf(fSQLComp,"%s\n",lSQL);

				strcpy(lSQL,"");
                            	strcpy(lSQL,gSQLInsSrcNodePre);
                            	strcat(lSQL," select distinct c.uid,c.r_uid from imp_component c where exists (select 1");
                            	strcat(lSQL," from imp_component c_x where c_x.p_uid=c.uid)");
                            	strcat(lSQL," and not exists (select 1 from source_node sn_x where sn_x.uid=c.p_uid);");
       		             	fprintf(fSQLComp,"%s\n",lSQL);

				strcpy(lSQL,"");
                            	strcpy(lSQL,gSQLInsNodeCompPre);
				strcat(lSQL," select distinct s_n.id,c.id from source_node s_n inner join imp_component i_c on s_n.uid=i_c.p_uid");
				strcat(lSQL," inner join component c on i_c.uid=c.uid");
				strcat(lSQL, " where not exists (select 1 from source_node_component snc_x where snc_x.node_id=s_n.id)");
				strcat(lSQL, " union");
				strcat(lSQL, " select s_n.id,c.id from source_node s_n inner join imp_component i_c on s_n.uid=i_c.p_uid");
				strcat(lSQL, " inner join component c on c.name=i_c.name");
				strcat(lSQL, " where i_c.uid is null and c.uid is null");
				strcat(lSQL, " and not exists (select 1 from source_node_component snc_x where snc_x.component_id=c.id);");
       		             	fprintf(fSQLComp,"%s\n",lSQL);

				fclose(fSQLComp);
				lRetVal = system(lExecSQLComp);

				fSQLProc = fopen(gFInsProcess,"a");
				strcpy(lSQL,"");
                            	strcpy(lSQL,gSQLProcessPre);
				strcat(lSQL," select distinct i.uid,i.name from imp_component i where i._type='process' and not exists (select 1 from process p_x where p_x.uid=i.uid);");
       		             	fprintf(fSQLProc,"%s\n",lSQL);
				fclose(fSQLProc);
				lRetVal = system(lExecSQLProc);
			}
		}
	}

        char cwd[256];
        getcwd(cwd, sizeof(cwd));

	printf("\nProcessing completed\nRun '%s/mapper -o [Process|Component] Objectname' to create a Node Map\n",cwd);
	return 0;
}
