#include "BigQ.h"
#include "ComparisonEngine.h"
#include "Comparison.h"
#include <vector>
#include <algorithm>
#include <stdlib.h>

using namespace std;

void * tpmms(void *);

OrderMaker orderUs;
File file;
Schema *mySchema;

typedef struct WorkerParameters {
	OrderMaker sortorder;
	int runlen;
	Pipe *pipe;
	Pipe *outpipe;
};

/*
CompareRecords :: CompareRecords(OrderMaker *orderUs) {
	orderMaker = orderUs;
}

CompareRecords :: ~CompareRecords() {
	delete orderMaker;
}*/

/*
bool Compare(Record *left, Record *right) {
	int i;	
	ComparisonEngine comparisonEngine;
	i = comparisonEngine.Compare(left,right,&orderUs);
	if (i == -1)
		return true;
	return false; 
} */

class SortRecords {
private:
	OrderMaker* orderMaker;
	ComparisonEngine comparer;
public:
	
	SortRecords(OrderMaker* sortOrder):orderMaker(sortOrder) {}
	
	int operator() (const Record *r1, const Record *r2) {
		int result = comparer.Compare((Record *)(r1), (Record *)(r2), orderMaker);
		if (result == -1) {
			return 1;
		} else {
			return 0;
		}
	}

	int operator() (const HeapElem *r1, const HeapElem *r2) {
		int result = comparer.Compare((Record *)(r1->rec), (Record *)(r2->rec), orderMaker);
		if (result == -1) {
			return 1;
		} else {
			return 0;
		}
	}
	
};


void Sort(Page *page) {
	typedef vector<Record *> RecordList;
	RecordList sortList;
	Record rec;	
	while(page->GetFirst(&rec)!=0) {
		Record* tempRecord = new Record();
		tempRecord->Consume(&rec);
		sortList.push_back(tempRecord);
	}
	std::sort(sortList.begin(),sortList.end(),SortRecords(&orderUs));
	//printf("sorting done man !!");
	RecordList::const_iterator ci = sortList.begin();
	while (ci != sortList.end()) {
		//(*ci)->Print(mySchema);             
		page->Append((*ci));
		delete (*ci);
		ci++;
	}
}

RunPage :: ~RunPage () {
}

RunPage :: RunPage (int runlength) {
	pageSize = runlength;
	curSizeInBytes = sizeof (int);
	numRecs = 0;
}


int RunPage :: Append (Record *addMe) {
	char *b = addMe->GetBits();
	if (curSizeInBytes + ((int *) b)[0] > PAGE_SIZE) {
		curSizeInBytes = sizeof(int) + ((int *) b)[0];
		pageSize--;
		if(pageSize==0) {
			curSizeInBytes = sizeof(int);
			return 0;
		}
	}
	Record* tempRecord = new Record();
	tempRecord->Consume(addMe);
	recList.push_back(tempRecord);
	curSizeInBytes += ((int *) b)[0];
	numRecs++;
	return 1;	
}

void RunPage:: Sort() {
	std::sort(recList.begin(),recList.end(),SortRecords(&orderUs));
}

void RunPage::GeneratePageAndAddtoFile() {
	typedef vector<Record *> RecordList;
	RecordList::const_iterator ci = recList.begin();
	Page *page = new Page();	

	while (ci != recList.end()) {
		if(page->Append((*ci)) == 0) {
			if(file.GetLength() == 0) {
				page->SetPageLoc(0);
			} else {
				page->SetPageLoc(file.GetLength()-1);
			}
			
			file.AddPage(page,page->GetPageLoc());
			myPagesIndex.push_back(page->GetPageLoc());
			page->EmptyItOut();			
			page->Append((*ci));	
		}
		delete (*ci);
		ci++;
	}
	if(file.GetLength() == 0) {
		page->SetPageLoc(0);
	} else {
		page->SetPageLoc(file.GetLength()-1);
	}		
	file.AddPage(page,page->GetPageLoc());	
	myPagesIndex.push_back(page->GetPageLoc());
	//printf("size of pages Index : %d",myPagesIndex.size());
	page->EmptyItOut();
	recList.clear();
	delete page;
}

void RunPage::WriteOut() {
	Sort();
	GeneratePageAndAddtoFile();
}

int RunPage::GetFirstPageLoc() {
	if(myPagesIndex.size() > 0) {
		int pageIndex = myPagesIndex.front();
		myPagesIndex.pop_front();
		printf("updated size of pagesIndexList : %d",myPagesIndex.size());
		return pageIndex;
	} 
	return -1;
}



/*
MyHeap :: MyHeap(HeapElem arr1[],int size1) {
	arr = arr1;
	size = size1;
	Heapify();
}

void MyHeap :: Adjust(int i,int n) {
	int j=2*i;	
	ComparisonEngine comparisonEngine;
	while(j+1 < n) {
		int t=0;
		t= j+1;		
		if(j+2 < n && comparisonEngine.Compare((arr[j+1].rec), (arr[j+2].rec),&orderUs) < 0)
			t=j+2;
		
		if(comparisonEngine.Compare((arr[t].rec), (arr[j/2].rec),&orderUs) < 0 ) {
			HeapElem temp = arr[j/2];
			arr[j/2] = arr[t];
			arr[t] = temp;
		}
		j = 2*t;
	}
}

void MyHeap :: Heapify() {
	int i;	
	for (i=(size/2)-1; i>=0;i--) {
		Adjust(i,size);
	}
}

void MyHeap :: Insert(HeapElem heapElem) {
	arr[size++] = heapElem;
}

HeapElem MyHeap :: RemoveTop() {
	HeapElem elem = arr[0];
	arr[0]=arr[size-1];	
	size--;
	return elem;
}

int MyHeap :: Size() {
	return size;
}*/

BigQ :: BigQ (Pipe &in, Pipe &out, OrderMaker &sortorder, int runlen, Schema *schema) {
	// read data from in pipe sort them into runlen pages
	WorkerParameters param = {sortorder,runlen,&in,&out};
	mySchema = schema;
	pthread_t bigQWorker;
	pthread_create (&bigQWorker, NULL, tpmms, (void *)&param);	
	pthread_join (bigQWorker,NULL);
}

BigQ::~BigQ () {
}

void *tpmms(void * in) {
	
	file.Open(0,"buff.bin");
	WorkerParameters *param = (WorkerParameters *)in;
	Record *rec;
	rec = new Record();
	int cntr=0;
	RunPage *runpage;
	runpage = new RunPage(param->runlen);
	std::vector<RunPage> pageVector;
	
	while (param->pipe->Remove (rec)) {
		//rec->Print(mySchema);
		cntr++;
		if(cntr%10000 == 0) {
			printf("\ncount %d",cntr);
		}
		if(runpage->Append(rec) == 0) {
			runpage->WriteOut();
			pageVector.push_back(*runpage);
			runpage = new RunPage(param->runlen);			
			runpage->Append(rec);	
		}
	}
	runpage->WriteOut();
	pageVector.push_back(*runpage);
	printf("\nno of input records : %d",cntr);
	
	delete runpage;

	Record record;
	printf("value : %d",pageVector.size());
	std::vector<HeapElem *> recordArr; //(HeapElem *)malloc(pageVector.size()*PAGE_SIZE);
	int pageTracker[pageVector.size()];
	int index=0,heapIndex =0; 
	HeapElem *buff;
	Page *page = new Page();
	printf("Now second phase of tpmms starts");
	int count=0;
	while(index < pageVector.size()) {	
		int pageIndex = pageVector[index].GetFirstPageLoc();
		//printf("\npageIndex: %d",pageIndex);
		if(pageIndex > -1) {
			file.GetPage(page,pageIndex);
			pageTracker[index] = page->GetNumRecords();
			count=0;
			while(page->GetFirst(&record)) {
				//record.Print(mySchema);
				buff = new HeapElem(index,&record);			
				recordArr.push_back(buff);//[heapIndex++]= *buff;
				count++;
			}
			page->EmptyItOut();
			printf("No of records: %d",count);
			//record.Print(mySchema);
		}
		index++;
	}
	std::sort(recordArr.begin(),recordArr.end(),SortRecords(&orderUs));
	//printf("index : %d",index);
	//MyHeap heap (recordArr, heapIndex);
	
	while(recordArr.size()!=0) {
		//	printf("heap size : %d",heap.Size());
		HeapElem elem = *(recordArr.back());
		recordArr.pop_back();//heap.RemoveTop();
		param->outpipe->Insert(elem.rec);
		Record record;
		//printf("\nindex value : %d \n",elem.index);
		//page->EmptyItOut();
		pageTracker[elem.index]-=1;
		if(pageTracker[elem.index] == 0) {
			page->EmptyItOut();
			int pageIndex = pageVector[elem.index].GetFirstPageLoc();
			printf("\nThe PageIndex %d",pageIndex);
			printf("The Elem index %d",elem.index);
			if(pageIndex > -1) {
				file.GetPage(page,pageIndex);
				pageTracker[elem.index] = page->GetNumRecords();
				printf("No of records per page : %d",page->GetNumRecords());
				while(page->GetFirst(&record)) {
					buff = new HeapElem(elem.index,&record);			
					recordArr.push_back(buff);					
					//heap.Insert(*buff);
				}
				std::sort(recordArr.begin(),recordArr.end(),SortRecords(&orderUs));
				//heap.Heapify();
				page->EmptyItOut();			
			}
		}
	}	
			
	param->outpipe->ShutDown ();
	//	cout<<"Page List Size : "<<pageVector.size()<<"\n";
}

