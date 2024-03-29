#ifndef BIGQ_H
#define BIGQ_H
#include <pthread.h>
#include <iostream>
#include "Pipe.h"
#include "File.h"
#include "Record.h"
#include "Schema.h"
#include "ComparisonEngine.h"
#include "Comparison.h"
#include <stdlib.h>
#include <list>
#include <vector>

using namespace std;

class Record;

class HeapElem {
	public:	
	int index;
	Record *rec;	
	HeapElem() {
	};
	HeapElem (int i, Record *r) {
		index = i;
		rec = new Record();
		rec->Consume(r);	
	};
	
	void Print(Schema *mySchema) {
		cout <<" index value : "<<index;
		rec->Print(mySchema);
	};
};


class RunPage {
private:
	std::vector<Record *> recList;
	std::list<int> myPagesIndex;
	int pageSize;
	int numRecs;
	int curSizeInBytes;
public:
	
	RunPage (int runlength);
	virtual ~RunPage ();

	// this appends the record to the end of a page.  The return value
	// is a one on success and a aero if there is no more space
	// note that the record is consumed so it will have no value after
	int Append (Record *addMe);

	void Sort();
	void GeneratePageAndAddtoFile();
	void WriteOut();
	int GetFirstPageLoc();

};

/*class CompareRecords {
private:
	OrderMaker *orderMaker;

public:
	CompareRecords(OrderMaker *orderUs);
	~CompareRecords();
	bool Compare(Record *left,Record *right);

};*/

class BigQ {

public:

	BigQ (Pipe &in, Pipe &out, OrderMaker &sortorder, int runlen, Schema *mySchema);
	~BigQ ();
};

/*
class MyHeap {
	private:
		HeapElem* arr;
		int size;
		
	public:
		MyHeap(HeapElem elemArray[],int size);		
		
		void Insert(HeapElem heapElem);
		
		HeapElem RemoveTop();
		
		int Size();
		
		void Adjust(int i, int n);
		
		void Heapify();
};*/

#endif
