// BuiltInSort.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <cstring>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <fstream>
#include <Windows.h>

using namespace std;



const int algNum = 3;
int sizeNum = 14;
int threadNum = 2;
int sizeInit = 1000000;
int sizeGrowth = 500000;

int** incidence;	// incidence of each initial character in each threads array part
int** offset;	// offset to be used when copying array

typedef struct data{
	string* l;	//array to be sorted
	string* t;	//temporary array
	int n;		//size of array
	int index;	//index of thread
	int portion;//portion to work on
	HANDLE ready;	//ready event
	HANDLE proceed;	//event on which to prceed
} DATA, *PDATA;


int presortmerge(string* strings, int n, int T);
DWORD WINAPI presortmergeThread(LPVOID lpParam);
int sortportionsmerge(string* strings, int n, int );
DWORD WINAPI sortportionsmergeThread(LPVOID lpParam);
bool createListFromFile(char* fileName, int lines, string* dest);
bool isSorted(string* list, int size);
int compare (const void * a, const void * b);


int _tmain(int argc, _TCHAR* argv[])
{
	LARGE_INTEGER freq;
	LARGE_INTEGER time1;
	LARGE_INTEGER time2;
	__int64 dt;

	QueryPerformanceFrequency(&freq);

	//time measurement cost calculation
	int linexp=1;
	int readgen=1;

	cout << "Please input the following data:\n";
	cout << "Maximum number of threads (default: 2): ";
	cin >> threadNum;
	cout << "\nInitial size of array (default: 1 000 000): ";
	cin >> sizeInit;
	cout << "\nLinear(1) or exponential growth(2): ";
	cin >> linexp;
	if(linexp==1){
		cout << "\nGrowth of array per iteration (default: 500 000): ";
		cin >> sizeGrowth;
		cout << "\nNumber of iterations (default: 14): ";
		cin >> sizeNum;
	}
	else{
		sizeNum=4;
		cout << "\nEach iteration multiplicates the array with (iteration-1)th power of 2.";
		cout << "\nNumber of iterations (default: 5): ";
		cin >> sizeNum;
	}
	cout << "\nUse partially ordered filenames(1), randomized filenames(2) or generated data(3): ";
	cin >> readgen;

	__int64** algsT[algNum];
	for(int i=0;i<algNum;i++){
		algsT[i] = new __int64*[sizeNum];
		for(int j=0;j<sizeNum;j++){
			algsT[i][j] = new __int64[threadNum];
		}
	}
	
	int n;
	if(linexp==1)//linear growth
		n = sizeInit+sizeNum*sizeGrowth;
	else//exponential growth
		n = sizeInit*pow((double)2,(double)sizeNum);

	string* unsorted;
	string* list;
	int size;

	unsorted = new string [n];
	char* fileName;
	bool readSuccess;

	if(readgen==1){
		fileName = "C:\\allfiles1000x.txt";
		cout << "Reading file...\n";
		readSuccess = createListFromFile(fileName, n, unsorted);
		if(!readSuccess){
			cout << "Freeing resources...\n\n";
			delete [] unsorted;
			unsorted = NULL;
			cout << "Press a key to exit!";
			cin.get();
			cin.get();
			return -1;
		}
		cout << n << "lines read.\n\n";
	}
	else{
		cout << "Not implemented!";
		cout << "Freeing resources...\n\n";
		delete [] unsorted;
		unsorted = NULL;
		cout << "Press a key to exit!";
		cin.get();
		cin.get();
		return -2;
	}
	
	// *************************************
	// ********** testing section **********
	// *************************************
	cout << "Testing started...";
	bool OK = true;
	int j;

	cout << "\nName\tSize";
	for(int k=0; k<threadNum; k++)
		if(k==0)cout << "\t" << k+1 << " thread";
		else cout << "\t" << k+1 << " threads";

	for (int i=0;i<sizeNum;i++)
	{
		size=1;
		if(linexp==1)//linear case
			size = sizeInit+i*sizeGrowth;
		else//exponential case
			size= sizeInit*pow((double)2,i);

		j=0;	// ********** qsort **********
		cout << "\nqsort\t" << size;
		cout << "\t create list";
			list = new string[size];	// create list to sort
			for(int l=0; l<size; l++) list[l] = unsorted[l];
			cout << "\b\b\b\b\b\b\b\b\b\b\b           \b\b\b\b\b\b\b\b\b\b\b";
		QueryPerformanceCounter(&time1);	// start time
			qsort(list,size,sizeof(std::string),compare);
		QueryPerformanceCounter(&time2);	// ready time
		dt = time2.QuadPart-time1.QuadPart;	// calculate delta time
		OK = isSorted(list,size); // verify if list is sorted

		cout << "\b\b";
		for(int k=0;k<threadNum;k++){
			if(OK)algsT[j][i][k]=dt*1000000/freq.QuadPart;	// if list is sorted put time in array
			else algsT[j][i][k]=0;							// if not, put zero
			cout << "\t " << algsT[j][i][k];
		}
		cout << "\t delete list";
		delete [] list;	// delete list
		list = NULL;
		cout << "\b\b\b\b\b\b\b\b\b\b\b           \b\b\b\b\b\b\b\b\b\b\b";

		j++;	// ********** pre-sort-merge **********
		cout << "\nPSM\t" << size << "\t ";
		for (int k=0; k<threadNum; k++){

			cout << "create list";
			list = new string[size];	// create list to sort
			for(int l=0; l<size; l++) list[l] = unsorted[l];
			cout << "\b\b\b\b\b\b\b\b\b\b\b           \b\b\b\b\b\b\b\b\b\b\b";
			QueryPerformanceCounter(&time1);	// start time
				presortmerge(list,size,k+1);
			QueryPerformanceCounter(&time2);	// ready time
			dt = time2.QuadPart-time1.QuadPart;	// calculate delta time
			OK = isSorted(list,size); // verify if list is sorted
			if(OK)algsT[j][i][k]=dt*1000000/freq.QuadPart;	// if list is sorted put time in array
			else algsT[j][i][k]=0;							// if not, put zero
			cout << algsT[j][i][k];

			cout << "\t delete list";
			delete [] list;	// delete list
			list = NULL;
			cout << "\b\b\b\b\b\b\b\b\b\b\b           \b\b\b\b\b\b\b\b\b\b\b";
		}

		j++;	// ********** sort portions-merge **********
		cout << "\nSPM\t" << size << "\t ";
		for (int k=0; k<threadNum; k++){

			cout << "create list";
			list = new string[size];	// create list to sort
			for(int l=0; l<size; l++) list[l] = unsorted[l];
			cout << "\b\b\b\b\b\b\b\b\b\b\b           \b\b\b\b\b\b\b\b\b\b\b";
			QueryPerformanceCounter(&time1);	// start time
				sortportionsmerge(list,size,k+1);
			QueryPerformanceCounter(&time2);	// ready time
			dt = time2.QuadPart-time1.QuadPart;	// calculate delta time
			OK = isSorted(list,size); // verify if list is sorted

			if(OK)algsT[j][i][k]=dt*1000000/freq.QuadPart;	// if list is sorted put time in array
			else algsT[j][i][k]=0;							// if not, put zero
			cout << algsT[j][i][k];
				
			cout << "\t delete list";
			delete [] list;	// delete list
			list = NULL;
			cout << "\b\b\b\b\b\b\b\b\b\b\b           \b\b\b\b\b\b\b\b\b\b\b";
		}
		cout << "\n";
	}
	delete [] unsorted;
	unsorted = NULL;

	cout << "\n\nTesting ready";

		

	/*cout << "Frequency: " << freq.QuadPart << "\n";
	cout << "Time1: " << time1.QuadPart << "\n";
	cout << "Time2: " << time2.QuadPart << "\n";
	dt = time2.QuadPart-time1.QuadPart;
	cout << "deltaT: " << dt << " cycles\n";
	cout << dt*1000/freq.QuadPart << " milliseconds\n";
	cout << dt*1000000/freq.QuadPart << " microseconds";*/

	cin.get();
	cin.get();
	
	return 0;
}

// Alg 1. preprocess + sort + merge
int presortmerge(string* strings, int n, int T){

	incidence = new int*[T];
	for(int i=0;i<T;i++){
		incidence[i]=new int[256];
	}
	offset = new int*[T];
	for(int i=0;i<T;i++){
		offset[i]=new int[256];
	}

	string* temp = new string[n];

	HANDLE* waitForThreads = new HANDLE[T];
	HANDLE* unPauseThreads = new HANDLE[T];
	HANDLE* threads = new HANDLE[T];

	int portion = ceil((float)n/(float)T);

	for(int i=0;i<T;i++)
		for(int j=0;j<256;j++){
			incidence[i][j]=0;
			offset[i][j]=0;
		}

	PDATA* dataArray = new PDATA[T];

	for(int i=0;i<T-1;i++){
		waitForThreads[i]=CreateEvent(NULL,TRUE,FALSE,NULL);
		unPauseThreads[i]=CreateEvent(NULL,TRUE,FALSE,NULL);

		dataArray[i] = (PDATA) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DATA));
		if(dataArray[i] == NULL ) return -4;

		dataArray[i]->l=strings;
		dataArray[i]->t=temp;
		dataArray[i]->n=n;
		dataArray[i]->index=i;
		dataArray[i]->portion=portion;
		dataArray[i]->ready=waitForThreads[i];
		dataArray[i]->proceed=unPauseThreads[i];

		LPDWORD lpThreadId = 0;
		threads[i]=CreateThread(NULL,0,presortmergeThread,dataArray[i],0,lpThreadId);
		if (threads[i]==NULL) return -2;
	}

	//part 1 counting incidences
	int c;
	for(int i=(T-1)*portion; i<n; i++){
		c=(strings[i])[0];
		if(c<0)c=256+c;
		incidence[T-1][c]++;
	}

	if (T>1){
		//wait for other threads to finish part 1
		WaitForMultipleObjects(T-1,waitForThreads,TRUE,INFINITE);
		//unset ready events
		for(int i=0;i<T-1;i++){
			ResetEvent(waitForThreads[i]);
		}
	}

	//calculate offsets
	for(int j=0; j<256;j++){
		for(int i=1; i<T; i++){
			offset[i][j]=offset[i-1][j]+incidence[i-1][j];
		}
		if(j<255)offset[0][j+1]+=offset[T-1][j]+incidence[T-1][j];
	}

	if (T>1){
		//unpause threads to start part 2
		for(int i=0;i<T-1;i++){
			SetEvent(unPauseThreads[i]);
		}
	}

	//part 2 copy array
	int* count = new int[256];
	for(int i=0; i<256; i++)count[i]=0;

	for(int i=(T-1)*portion;i<n;i++){
		c=(strings[i])[0];
		if(c<0)c=256+c;
		if(i==20730){
			int k=0;
		}
		temp[offset[T-1][c]+count[c]]=strings[i];
		count[c]++;
	}
	delete [] count;
	count = NULL;

	if (T>1){
		//wait for other threads to finish part 2
		WaitForMultipleObjects(T-1,waitForThreads,TRUE,INFINITE);
		//unset ready events
		for(int i=0;i<T-1;i++){
			ResetEvent(waitForThreads[i]);
		}

		//unpause threads to start part 3
		for(int i=0;i<T-1;i++){
			SetEvent(unPauseThreads[i]);
		}
	}

	//part 3 sorting using qsort
	qsort(&temp[(T-1)*portion],n-((T-1)*portion),sizeof(std::string),compare);

	if (T>1){
		//wait for other threads to finish part 3
		//WaitForMultipleObjects(treads-1,waitForThreads,TRUE,INFINITE);
	}
	
	if (T>1) WaitForMultipleObjects(T-1,threads,TRUE,INFINITE);

	//part 4 merge

	int* iterator = new int[T];
	for(int i=0; i<T-1; i++){
		iterator[i]=portion-1;
	}
	iterator[T-1]=n-((T-1)*portion)-1;
	string s;
	int maxh;
	int b;
	for(int i=n-1; i>-1; i--){
		maxh = 0;
		s="";
		for(int j=0; j<T; j++){
			if(iterator[j]>-1){
				if(s==""){
					s=temp[iterator[j]];
					maxh=j;
				}
				else
				{
					b=s.compare(temp[portion*(T-1)+iterator[j]]);
					if(b<0){
						s=temp[portion*(T-1)+iterator[j]];
						maxh=j;
					}
				}
			}
		}
		iterator[maxh]--;
		strings[i]=s;
	}
	delete [] temp;
	temp = NULL;

	return 0;
}

// Thread for Alg 1.
DWORD WINAPI presortmergeThread(LPVOID lpParam){
	PDATA pDataArray = (PDATA)lpParam;
	int portion = pDataArray->portion;

	//part 1 counting incidences
	int c;
	for(int i=(pDataArray->index)*portion;i<((pDataArray->index)+1)*portion;i++){
		c=((pDataArray->l)[i])[0];
		if(c<0)c=256+c;
		incidence[pDataArray->index][c]++;
	}
	if(SetEvent(pDataArray->ready)==FALSE)return -3;	//part 1 ready

	//waiting for part 2 to start
	WaitForSingleObject(pDataArray->proceed,INFINITE);
	ResetEvent(pDataArray->proceed);

	//part 2 copy array
	int* count = new int[256];
	for(int i=0; i<256; i++)count[i]=0;
	for(int i=(pDataArray->index)*portion;i<((pDataArray->index)+1)*portion;i++){
		c=((pDataArray->l)[i])[0];
		if(c<0)c=256+c;
		(pDataArray->t)[offset[pDataArray->index][c]+count[c]]=(pDataArray->l)[i];
		count[c]++;
	}
	delete [] count;
	count = NULL;
	if(SetEvent(pDataArray->ready)==FALSE)return -3;	//part 2 ready

	//waiting for part 3 to start
	WaitForSingleObject(pDataArray->proceed,INFINITE);
	ResetEvent(pDataArray->proceed);

	//part 3 sorting using qsort
	qsort(&(pDataArray->t[(pDataArray->index)*portion]),portion,sizeof(std::string),compare);
	//if(SetEvent(ready)==FALSE)return -3;

	//waiting for part 4 to start
	//WaitForSingleObject(pDataArray->proceed,INFINITE);
	//ResetEvent(pDataArray->proceed);

	//part 4 merge arrays
}

// Alg 2. sort portions + merge
int sortportionsmerge(string* strings, int n, int T){

	string* temp;

	HANDLE* waitForThreads;
	HANDLE* unPauseThreads;
	HANDLE* threads;
	PDATA* dataArray;

	int portion = ceil((float)n/(float)T);

	if(T>1){
		waitForThreads = new HANDLE[T];
		unPauseThreads = new HANDLE[T];
		threads = new HANDLE[T];
		dataArray = new PDATA[T];
		temp = new string[n];
	}
	for(int i=0;i<T-1;i++){
		waitForThreads[i]=CreateEvent(NULL,TRUE,FALSE,NULL);
		unPauseThreads[i]=CreateEvent(NULL,TRUE,FALSE,NULL);

		dataArray[i] = (PDATA) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DATA));
		if(dataArray[i] == NULL ) return -4;

		dataArray[i]->l=strings;
		dataArray[i]->t=temp;
		dataArray[i]->n=n;
		dataArray[i]->index=i;
		dataArray[i]->portion=portion;
		dataArray[i]->ready=waitForThreads[i];
		dataArray[i]->proceed=unPauseThreads[i];

		LPDWORD lpThreadId = 0;
		threads[i]=CreateThread(NULL,0,sortportionsmergeThread,dataArray[i],0,lpThreadId);
		if (threads[i]==NULL) return -2;
	}

	//part 1 sorting portion using qsort
	qsort(&strings[(T-1)*portion],n-((T-1)*portion),sizeof(std::string),compare);
	if (T>1){
		for(int i=(T-1)*portion;i<n;i++){
			temp[i]=strings[i];
		}
	}

	//part 2 merge
	
	if (T>1){
		WaitForMultipleObjects(T-1,threads,TRUE,INFINITE);

		int* iterator = new int[T];
		for(int i=0; i<T-1; i++){
			iterator[i]=portion-1;
		}
		iterator[T-1]=n-((T-1)*portion)-1;
		string s;
		int maxh;
		int b;
		for(int i=n-1; i>-1; i--){
			maxh = 0;
			s="";
			for(int j=0; j<T; j++){
				if(iterator[j]>-1){
					if(s==""){
						s=temp[iterator[j]];
						maxh=j;
					}
					else
					{
						b=s.compare(temp[portion*(T-1)+iterator[j]]);
						if(b<0){
							s=temp[portion*(T-1)+iterator[j]];
							maxh=j;
						}
					}
				}
			}
			iterator[maxh]--;
			strings[i]=s;
		}
		delete [] temp;
		temp=NULL;
	}
	
	return 0;
}

// Thread for Alg 2.
DWORD WINAPI sortportionsmergeThread(LPVOID lpParam){
	PDATA pDataArray = (PDATA)lpParam;

	int portion = pDataArray->portion;

	//part 1 sorting using qsort
	qsort(&(pDataArray->l[(pDataArray->index)*portion]),portion,sizeof(std::string),compare);

	for(int i=(pDataArray->index)*portion;i<portion;i++){
		pDataArray->t[i]=pDataArray->l[i];
	}

	//waiting for part 2 to start
	//WaitForSingleObject(pDataArray->proceed,INFINITE);
	//ResetEvent(pDataArray->proceed);

	//part 2 merge arrays

	return 0;
}

//function to populate string array, from file
bool createListFromFile(char* fileName, int lines, string* dest){

	int lnum=0;
	ifstream fileToSort;

	fileToSort.open(fileName);
	if(fileToSort == NULL){
		cout << "Error opening file.\n";
		return false;
	}
	else
	{	
		for	(int i=0; i<lines; i++){
			if(!fileToSort.eof()){
				getline(fileToSort, dest[i]);
				lnum++;
			}
			else{
				cout << "File doesen't contain enough lines, only " << lnum << ".\n";
				fileToSort.close();
				return false;
			}
		}
		fileToSort.close();
	}
	return true;
}

//checks if given list is in ascending order
bool isSorted(string* list, int size){
	for (int i=0; i<size-1; i++) if(list[i].compare(list[i+1])>0) return false;
	return true;
}

//compares two strings (to feed qsort)
int compare (const void * a, const void * b)
{
	return (*(string*)a).compare(*(string*)b);
}
