#include "TwoWayList.h"
#include "Record.h"
#include "Schema.h"
#include "File.h"
#include "Comparison.h"
#include "ComparisonEngine.h"
#include "DBFile.h"
#include "Defs.h"
#include <iostream>
#include <stdio.h>
#include <string>

extern struct AndList *final;

// stub file .. replace it with your own DBFile.cc

DBFile::DBFile () {
	metaDataFile = ".header";
	
}

int DBFile::Create (char *f_path, fType f_type, void *startup) {
	metaFilePntr = fopen (metaDataFile, "a+");
	char space[200];
	sprintf(space,"%s %s\n",f_path,enum_arr[f_type]);
	fseek (metaFilePntr, 0, SEEK_END);
	fprintf(metaFilePntr,"%s",space);
	file.Open (0, f_path);	
	fclose(metaFilePntr);
}

void DBFile::Load (Schema &f_schema, char *loadpath) {
	Record temp;
	int counter = 0;
	ComparisonEngine comp;
	FILE *tableFile = fopen (loadpath, "r");
	page.EmptyItOut();
    while (temp.SuckNextRecord (&f_schema, tableFile) == 1) {
		counter++;
		if (counter % 10000 == 0) {
			cerr << counter << "\n";
		}
		temp.Print(&f_schema);
		// page is appended to the end of file always in this method.
		if(page.Append(&temp)==0) {	
			if(file.GetLength() == 0) {
				page.SetPageLoc(0);
			} else {
				page.SetPageLoc(file.GetLength()-1);
			}
			
			file.AddPage(&page,page.GetPageLoc());
			page.EmptyItOut ();
			page.Append(&temp);
			
		} 
	}
	cout << "loaded " << counter << "recs\n";

	if(file.GetLength() == 0) {
		page.SetPageLoc(0);
	} else {
		page.SetPageLoc(file.GetLength()-1);
	}
	file.AddPage(&page,page.GetPageLoc());
	page.EmptyItOut ();
}

int DBFile::Open (char *f_path) {	
	metaFilePntr = fopen (metaDataFile, "r");
	fseek (metaFilePntr, 0, SEEK_SET);
	char space[200];
	while(1) {	
		if(fscanf (metaFilePntr, "%s", space) != EOF) {
			cout << space << "\n";
			string cstring = space; 
			if(cstring.find(f_path)!=-1) {
				cout << "File found !!";
				file.Open (1, f_path);
				fclose(metaFilePntr);
				return 1;
			}	
			fscanf (metaFilePntr, "%s", space);
		} else {
			break;
		}
	}
	cout << "File not found !!";
	fclose(metaFilePntr);
	return 0;
}

void DBFile::MoveFirst () {
	
	//if(page.GetPageLoc() != 0) {
	//	if(page.IsDirty()) {
	//		file.AddPage(&page,page.GetPageLoc());
	//	}
		page.EmptyItOut ();
		file.GetPage(&page,0);
	//}
}

int DBFile::Close () {
	//if(page.IsDirty()) {
	//	file.AddPage(&page,page.GetPageLoc());
	//}
	page.EmptyItOut ();
	file.Close();
}

void DBFile::Add (Record &rec) {
	//if(page.GetPageLoc() != file.GetLength()-1) {
	//	if(page.IsDirty()) {
	//		file.AddPage(&page,page.GetPageLoc());
	//	}
	//	page.EmptyItOut ();
	//	file.GetPage(&page,file.GetLength()-1);
	//}
	page.Append(&rec);
}

int DBFile::GetNext (Record &fetchme ) {
	if(page.GetFirst(&fetchme)==0) {
		off_t pageLoc = page.GetPageLoc();
		page.EmptyItOut();
		if(pageLoc+2 >= file.GetLength()) {
			return 0;
		}
		
		file.GetPage(&page,pageLoc+1);
		page.GetFirst(&fetchme);
	}
	return 1;
}

int DBFile::GetNext (Record &fetchme, CNF &cnf, Record &literal) {
	ComparisonEngine comp;
	while(1) {
		if(page.GetFirst(&fetchme)==0) {
			off_t pageLoc = page.GetPageLoc();
			page.EmptyItOut();
			if(pageLoc+2 >= file.GetLength()) {
				return 0;
			}
			file.GetPage(&page,pageLoc+1);
			page.GetFirst(&fetchme);
		}
		if (comp.Compare (&fetchme, &literal, &cnf)) {
			return 1;
		}
	}
}
