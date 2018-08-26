//---------------------------------------------------------------------------
#pragma hdrstop
#include <iostream>
//#include <cstdlib>
//#include <cstring>
//#include <utility>
//#include <time.h>


#include "pruntables.h"
#include "fifteen.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)


#define NWD 24964 //Number of wd-matrices

#define WEIGHT(piece,a,b) BNK[15+a-piece][b-piece]



//order of elements in puz array INTERN
//00 01 02 03
//07 06 05 04
//08 09 10 11
//15 14 13 12

//order of elements in puz array ZEROBASED
//00 01 02 03
//04 05 06 07
//08 09 10 11
//12 13 14 15

//order of elements in puz array ZEROTERMINATED
//01 02 03 04
//05 06 07 08
//09 10 11 12
//13 14 15 00


//wd elements
//x 0 0 0
//1 1 1 1
//2 2 2 2
//3 3 3 3


//describes the number of wd-elements per row
//for example 1 1 0 2 means 1 times 0, 2 times 1, 1 times 2 und 0 times 3 in this row
typedef unsigned char wdmatrix[4][4];

struct node{
	char pz[16];
	char pz_r[16];
};


//WD663 performance: 93 ms/random instance
//ONLY78 performance: 3 ms/random instance

__int64 nodecount=0,sumdepth=0,timeStart,timeStop;
int solutionFound=0,abortIDA=0;
char solution[85];
int lastSolution[85];//80 is enough
int CurrentSearchLength=0;//aktuelle Länge der Lösung

//++++++++++++some tables+++++++++++++++++++++++++++++++++++++++++++++++
int indextowdhash[NWD];//map 0<=index<NWD of wd-position tohashvalue 0<=x<=20*35*35*35
short int wdmoveup[NWD][4];//wd-movetable for move of blank up
short int wdmovedown[NWD][4];
unsigned char A[4][4][4][4],BCD[5][5][5][5];//for computing the wd-hash
//newidx[blank][dir] gives the new INTERNAL position index of the blank when moving in direction dir
//dir: 0 left, 1 right, 2 up, 3 down
char newidx[16][4]={{-1,1,-1,7},{0,2,-1,6},{1,3,-1,5},{2,-1,-1,4},{5,-1,3,11},{6,4,2,10},{7,5,1,9},{-1,6,0,8},
                    {-1,9,7,15},{8,10,6,14},{9,11,5,13},{10,-1,4,12},{13,-1,11,-1},{14,12,10,-1},{15,13,9,-1},{-1,14,8,-1}};
//die Richtung relativ zum puz-Array.0: benachbart, 1 nicht benachtbart, Permutation ändert sich
int mirr[16]={0,7,8,15,14,9,6,1,2,5,10,13,12,11,4,3};
int fromzerobased[16]={0,1,2,3,7,6,5,4,8,9,10,11,15,14,13,12};
char masksum[256][8];
char manhattdist[16][16];
int BNK[16][8];

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
char *prunwd;//pointers to the pruningtable
char *prun17,*prun815;
char *prun16,*prun79,*prun1015;
char *prun15,*prun610,*prun1115;
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
unsigned int Bnk(int n, int k)
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
{
	int s;
	int i;
	if (n<k)   return 0;
	for (s=1,i=n; i!=n-k; i--) s *= i;
	return s;
}

char msum(char mask, int piece){//piece from 0 to 7
	char s=0;
	for (int i=0;i<=piece;i++) if ((1<<i)&mask) s++;
	return s;
}

void buildmasksum(){
	for (int i=0;i<256;i++){
		for (int piece=0;piece<8;piece++)
			masksum[i][piece]=msum(i,piece);
	}
}

void buildBNK(){
	for (int i=0;i<16;i++)
		for (int j=0;j<8;j++)
			BNK[i][j]=Bnk(i,j);
}

int weight(char piece, int a, int b){
	return BNK[15+a-piece][b-piece];
}

int index(char*pz, int a, int b){//halb so schnell wie die andere Methode!
	char invp[16];
	char msk=0, mask[16];

	for (int i=a;i<=b;i++)mask[i]=0;
	for (int i=0;i<16;i++){
		if ((pz[i]>=a)&&pz[i]<=b){
			mask[pz[i]]|=msk;
			msk|=(1<<(pz[i]-a));
			invp[pz[i]]=i;
		}
	}
	int s=0;
	for (int i=a,j=16;i<=b;i++){
		s*=j--;
		s+=invp[i]-masksum[(unsigned char)mask[i]][i-a];

	}
	return s;
}

void inv_index(char*pz, int a, int b, int idx){
	int n=b-a;
	int offset[16];
	for (int i=16-n;i<=16;i++){
		int c=idx%i;idx/=i;
				offset[a+16-i]=c;
	}
	for (int i=0;i<16;i++) pz[i]=0;
	for (int i=a;i<=b;i++){
		int v=0,j=0;
		while (v!=offset[i]){
			if (pz[j++]==0) v++;
		}
		//jetzt nächsten freien Platz suchen
		while (pz[j]!=0) j++;
		pz[j]=i;
	}
}


///Hashes for the columns of a wd-matrix
//first column A, 2.,3.,4. dolumn BCD
void inithashABCD() {
	int hash = 0;
	for (int i = 0; i <= 3; i++)
		for (int j = 0; j <= 3 - i; j++)
			for (int k = 0; k <= 3 - i - j; k++)
				if (3 - i - j - k >= 0)
					A[i][j][k][3 - i - j - k] = hash++; // <20
	hash = 0;
	for (int i = 0; i <= 4; i++)
		for (int j = 0; j <= 4 - i; j++)
			for (int k = 0; k <= 4 - i - j; k++)
				if (4 - i - j - k >= 0)
					BCD[i][j][k][4 - i - j - k] = hash++; // <35

}

//Hash value of a wd-Matrix
//0<=hash<20*35*35*35=857500
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
int wdhash(wdmatrix m){
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	int a= A[m[0][0]][m[1][0]][m[2][0]][m[3][0]];
	int b= BCD[m[0][1]][m[1][1]][m[2][1]][m[3][1]];
	int c= BCD[m[0][2]][m[1][2]][m[2][2]][m[3][2]];
	int d= BCD[m[0][3]][m[1][3]][m[2][3]][m[3][3]];
	return a+20*b+20*35*c+20*35*35*d;
}
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
int getblankrow(wdmatrix m){
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	for (int i=0;i<4;i++) if (m[i][0]+m[i][1]+m[i][2]+m[i][3]==3) return i;
	return 0;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void copywdmatrix(wdmatrix src, wdmatrix dst){
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	for (int i=0;i<4;i++)
		for (int j=0;j<4;j++)
			dst[i][j]=src[i][j];
}

//Compute wd-matrix from permutation
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void puztowdmatrix(char *pz, wdmatrix m){
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	for (int i=0;i<16;i++)m[i/4][i%4]=0;
	for (int i=0;i<16;i++){
		if (pz[i]!=0){
			m[i/4][pz[i]/4]++;
		}
	}
}

//Compute the index of a wd-matrix
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
unsigned short wdindex(char *pz){
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	wdmatrix m;
	puztowdmatrix(pz,m);
	int n = wdhash(m);
	for (int j = 0; j < NWD; j++)
		if (indextowdhash[j] == n)
			return j;
	return 0;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void buildprunWD( char *prn){
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	 char *hashprun= new  char[20*35*35*35];
	wdmatrix  *wd = new wdmatrix[NWD+1];

	int left,right,depth,prevdepth,head,n;
 	for (int i=0;i<20*35*35*35;i++) hashprun[i]=-1;
	unsigned char v[16]={3,0,0,0,0,4,0,0,0,0,4,0,0,0,0,4};
	for (int i=0;i<16;i++) wd[0][i/4][i%4]=v[i];
    for (int i=0;i<NWD;i++)
		for (int k=0;k<4;k++){
			wdmovedown[i][k]=-1;
			wdmoveup[i][k]=-1;
		}
	//build the indextowdhash table
	// Form1->Memo->Lines->Add("create WD-Pruning Table: ");
	hashprun[wdhash(wd[0])]=0;right=0;depth=0;head=1;
	indextowdhash[0]=wdhash(wd[0]);
	prn[0]=0;
	do{
		left=right;right=head;
		prevdepth=depth++;
		for (int i=left;i<right;i++){
		Application->ProcessMessages();
			if (hashprun[wdhash(wd[i])]==prevdepth){//should always be true
				int j= getblankrow(wd[i]);
				if (j-1>=0){
					for (int k = 0; k < 4; k++) {
						if (wd[i][j - 1][k] > 0) {
							copywdmatrix(wd[i], wd[head]);
							wd[head][j - 1][k]--;
							wd[head][j][k]++;
							n = wdhash(wd[head]);
							if (hashprun[n] == -1) { // new entry
								hashprun[n] = depth;
								indextowdhash[head] = n;
								prn[head] = depth;

								wdmoveup[i][k] = head;

								head++;
							}

							else { // now search entry in table
								for (int m = 0; m <= head; m++) {
										if (indextowdhash[m] == n) {
										wdmoveup[i][k] = m;
										break;
									}

								}
							}

						}
					}
				}
				if (j + 1 < 4) {
					for (int k = 0; k < 4; k++) {
						if (wd[i][j + 1][k] > 0) {
							copywdmatrix(wd[i], wd[head]);
							wd[head][j+1][k]--;wd[head][j][k]++;
							n=wdhash(wd[head]);
							if (hashprun[n]==-1){//new entry
								hashprun[n]=depth;
								indextowdhash[head]=n;
								prn[head]=depth;

								wdmovedown[i][k] = head;

								head++;
							}

							else { // Dann den Eintrag in der Tabelle suchen
								for (int m = 0; m <= head; m++) {
										if (indextowdhash[m] == n) {
										wdmovedown[i][k] = m;
										break;
									}

								}
							}

						}
					}
				}
			}
		}
	  // Form1->Memo->Lines->Add(IntToStr(depth)+AnsiString("     ")+IntToStr(head-right));
	}while (head>right);

	delete[] hashprun;
	delete[] wd;
}

//reflection at main diagonal
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void reflect(char *src,char *dst){
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	for (int i=0;i<16;i++) dst[i]=mirr[src[mirr[i]]];
}

//solvable positions have even parity
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
int parity(char* pz){
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	int s=0;
	for (int i=0;i<15;i++){
		if (pz[i]==0) continue;
		for (int j=i+1;j<16;j++){
			if (pz[j]==0) continue;
			if (pz[i]>pz[j]) s++;
		}
	}
	return s%2;
}

//generate reandom puzzle in INTERNAL Format
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
char * randompuz(char* pz,puztype type){
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	for (int i = 0; i < 16; i++)
		pz[i] = i;
	for (int i = 16; i > 1; i--)
		swap(pz[i - 1], pz[rand() % i]);
	char p[16],p_r[16];
	char *pp;
	switch (type) {
	case ZEROBASED:
		for (int i = 0; i < 16; i++)
			p[i] = fromzerobased[pz[fromzerobased[i]]];
		pp = p;
		break;
	case ZEROTERMINATED:
		for (int i = 0; i < 16; i++)
			p_r[i] = (16 - pz[15 - i]) % 16;
		for (int i = 0; i < 16; i++)
			p[i] = fromzerobased[p_r[fromzerobased[i]]];
		pp = p;
		break;
	case INTERNAL:
		pp = pz;
		break;
	}
	if (parity(pp)!=0){//fix parity
		if (pz[0]==0 || pz[1]==0) swap(pz[2],pz[3]);
		else swap(pz[0],pz[1]);
	}
	return pz;
}

//get position of blank
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
char blankpos(char* pz){
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	for (int i=0;i<16;i++) if (pz[i]==0) return i;
	return 0;
}

//from current blank position fill all positions with -1 which can be reached
//without moving tiles not belonging to the pruning tile set
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void fill_(char *p, char blankpos){
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	if (p[blankpos]!=0) return;//occupied with pruning tile (>0) oder filled (-1)
	else{
		p[blankpos]=-1;//fill
		for (int d=0;d<4;d++){//fill neighbours if possible
			int newblankpos  = newidx[blankpos][d];
			if (newblankpos==-1)continue;//outside of puzzle
			else fill_(p,newblankpos);
		}
	}
}

//return a bitvector with all other possible blank positions for a given
//pruning tile configuration
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
short int blankcluster_(char* pz, char blankpos){//p
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	char p[16];
	for (int i=0;i<16;i++) p[i]=pz[i];
	fill_(p,blankpos);
	//it is sufficient to take only those possible blank positions
	//which have a neighbour of the pruning set
	for (int i=0;i<16;i++){
		if (p[i]!=-1) continue;//no blank position
		for (int j=0;j<4;j++){
			int n = newidx[i][j];
			if (n<0) continue;//outside
			if (p[n]>0) {p[i]=-2;break;}//has neighbour
		}
	}
	short int r=0;
	for (int i=0;i<16;i++)
		if (p[i]==-2) r|= (1<<i);
	return r;
}

//build the pruning table for pruning tiles set a,...b
//smaller memory chunks
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
int buildprun_32(char* prn, int a, int b, bool loadonly){
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	FILE *fprun;
	char filename[20],idx[20];
	filename[0]='p';filename[1]=0;
	itoa(a,idx,10);
	strcat(filename,idx);
	itoa(b,idx,10);
	strcat(filename,idx);
	unsigned int B=Bnk(16,b-a+1);
	fprun = fopen(filename,"r+b");
	if (fprun!=NULL){
		Form1->Memo->Lines->Add(AnsiString("reading from file ")+filename);
		//cout<<"\nreading from file "<<filename<<endl;
		fread(prn,1,B,fprun);
		fclose(fprun);
		Form1->Memo->Lines->Add(" done");
		return 0;
	}

	if (loadonly) return -1;
   short int *blanks1[8], *blanks1tmp[8];
	int B8=B/8;

 try{
		for (int i = 0; i < 8; i++) {
			blanks1[i] = new short int[B8];
			blanks1tmp[i] = new short int[B8];
		}
	}
	catch (...) {
		for (int i = 0; i < 8; i++) {
			delete[]blanks1[i];
			delete[]blanks1tmp[i];
		}
		Application->MessageBox
			(L"You do not have enough memory to use this heuristics.\n"
			L"You must have at least Windows 64 bit with 6 GB of RAM.", L"",
			MB_OK | MB_ICONERROR);
		return -1;
	}
	Form1->Memo->Lines->Add(AnsiString("generating table ")+filename);
	for (unsigned int i = 0; i < B; i++) {
		blanks1[i/B8][(i%B8)] = 0;
		blanks1tmp[i/B8][(i%B8)] = 0;
	}

	char pz[16];
	for (int i = 0; i < 16; i++)
		pz[i] = i; // starting configuration
	for (unsigned int i = 0; i < B; i++) {
		if (i % 100000 == 0) {
			Application->ProcessMessages();
		}

		prn[i] = 127; // set all entries to empty
	}


	int j=index(pz,a,b);//
	prn[j]=0;
	inv_index(pz,a,b,j);
		blanks1[j/B8][(j%B8)]|= blankcluster_(pz,0);//starting blankposition is 0 (INTERNAL format)

	unsigned int done=1,olddone=done;unsigned char depth=0,prevdepth;
	do{
		prevdepth=depth++;
		for (unsigned int i=0;i<B;i++){

		if (i%100000==0) {
			Application->ProcessMessages();
		}
			if (prn[i]<=prevdepth){//prn[i]==prevdepth is wrong!
				inv_index(pz,a,b,i);

				int bl = blanks1[i/B8][(i%B8)];//possible blank positions
				for (int blpos=0;blpos<16;blpos++){//<16!!!
						if ((bl & 1) == 0) {
						bl >>= 1;
						continue;
					}

					if (pz[blpos] != 0) {
						Form1->Memo->Lines->Add
							("Error while generating table. Aborted");
						return -1;
					}

					for (int dir = 0; dir < 4; dir++) {
						int newblpos = newidx[blpos][dir];
						if (newblpos < 0)
							continue; // outside puzzle
						int tmp = pz[newblpos];
						if (tmp==0) continue;//exchanging blank with non pruning tile does not count

						pz[newblpos]=0;pz[blpos]=tmp;
						j=index(pz,a,b);//beibehalten!!
						if(prn[j]==127){
							prn[j]=depth; done++;
						}
						if ((blanks1[j/B8][(j%B8)]&(1<<newblpos))==0){//new blankcluster
							char ps[16];
							for (int k=0;k<16;k++) ps[k]=pz[k];
								blanks1tmp[j/B8][(j%B8)]|= blankcluster_(ps,newblpos);
						}
						pz[blpos]=0;pz[newblpos]=tmp;//restore position
					}
					bl>>=1;
				}
			}
		}
		  Form1->Memo->Lines->Add(L"depth: "+IntToStr(depth)+L"   count:  "+IntToStr((int)(done-olddone)));
		olddone=done;
		for (unsigned int k=0;k<B;k++){
				if (k%100000==0) {
			Application->ProcessMessages();
		}
			blanks1[k/B8][(k%B8)]|= blanks1tmp[k/B8][(k%B8)] ;
		}
	} while (done<B);

	for (int i=0; i < 8; i++) {
		delete[] blanks1[i];
		delete[] blanks1tmp[i];
	}

	Form1->Memo->Lines->Add("saving file "+AnsiString(filename));
	fprun = fopen(filename,"w+b");
	fwrite(prn,1,B,fprun);
	fclose(fprun);
	Form1->Memo->Lines->Add("done ");
	return 0;
}

//build the pruning table for pruning tiles set a,...b
//if loadonly==true the tables will not be created if the files
//are not present yet
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void buildprun_(char* prn, int a, int b, bool loadonly){
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	FILE *fprun;
	char filename[20],idx[20];
	filename[0]='p';filename[1]=0;
	itoa(a,idx,10);
	strcat(filename,idx);
	itoa(b,idx,10);
	strcat(filename,idx);
	unsigned int B=Bnk(16,b-a+1);
	fprun = fopen(filename,"r+b");
	if (fprun!=NULL){
		Form1->Memo->Lines->Add(AnsiString("reading from file ")+filename);
		Application->ProcessMessages();
		fread(prn,1,B,fprun);
		fclose(fprun);
		Form1->Memo->Lines->Add(" done");
		return;
	}

	if (loadonly) return;

   Form1->Memo->Lines->Add(AnsiString("generating table ")+filename);

	short int *blanks = new short int[B];
	short int *blankstmp = new short int[B];

	for (unsigned int i=0;i<B;i++) {blanks[i]=0;blankstmp[i]=0;}
	char pz[16];
	for (int i=0;i<16;i++) pz[i]=i;//starting configuration
	for (unsigned int i=0;i<B;i++) prn[i]=127;//set all entries to empty

	int j=index(pz,a,b);//
	prn[j]=0;
	inv_index(pz,a,b,j);
	blanks[j]|= blankcluster_(pz,0);//starting blankposition is 0 (INTERNAL format)

	unsigned int done=1,olddone=done;unsigned char depth=0,prevdepth;
	do{
		prevdepth=depth++;
		for (unsigned int i=0;i<B;i++){

		if (i%100000==0) {
			Application->ProcessMessages();
		}
			if (prn[i]<=prevdepth){//prn[i]==prevdepth is wrong!
				inv_index(pz,a,b,i);

				int bl = blanks[i];//possible blank positions
				for (int blpos=0;blpos<16;blpos++){//<16!!!
						if ((bl&1)==0){bl>>=1;continue;}

					if (pz[blpos]!=0) Form1->Memo->Lines->Add("FEHLER");//darf nicht passieren!

					for (int dir=0;dir<4;dir++){
						int newblpos=newidx[blpos][dir];
						if (newblpos<0) continue;//outside puzzle
						int tmp=pz[newblpos];
						if (tmp==0) continue;//exchanging blank with non pruning tile does not count

						pz[newblpos]=0;pz[blpos]=tmp;
						j=index(pz,a,b);//beibehalten!!
						if(prn[j]==127){
							prn[j]=depth; done++;
						}
						if ((blanks[j]&(1<<newblpos))==0){//new blankcluster
							char ps[16];
							for (int k=0;k<16;k++) ps[k]=pz[k];
							blankstmp[j]|= blankcluster_(ps,newblpos);
						}
						pz[blpos]=0;pz[newblpos]=tmp;//restore position
					}
					bl>>=1;
				}
			}
		}
		  Form1->Memo->Lines->Add(L"depth: "+IntToStr(depth)+L"   count:  "+IntToStr((int)(done-olddone)));
		olddone=done;
		for (unsigned int k=0;k<B;k++){blanks[k]|=blankstmp[k];}
	} while (done<B);
	delete[] blanks;
	delete[] blankstmp;

	Form1->Memo->Lines->Add("saving file "+AnsiString(filename));
	fprun = fopen(filename,"w+b");
	fwrite(prn,1,B,fprun);
	fclose(fprun);
	Form1->Memo->Lines->Add("done ");
  //	cout<<" done"<<endl;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void treesearchMANHATTAN_(node n, char blnkpos,char lastdir,int togo){
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	if (solutionFound||abortIDA) return;

	if (togo == 0) {
		solution[togo] = blnkpos;

		if (solution[84] == 0) { // Ort für Lösungslänge
			QueryPerformanceCounter((LARGE_INTEGER *)&timeStop);
			printSolutionPieces(Form1->puz,CurrentSearchLength, (puztype)(Form1->Type->ItemIndex));
			solution[84] = CurrentSearchLength;
			if (Form1->SolNum->ItemIndex==0) { //only one solution to compute
				solutionFound=1;
				return;
			}
		}
		else {
			if (solution[84] < CurrentSearchLength) {
				QueryPerformanceCounter((LARGE_INTEGER *)&timeStop);
				solutionFound = 1;
				return;
			}
			printSolutionPieces(Form1->puz, CurrentSearchLength,
				(puztype)(Form1->Type->ItemIndex));
			Application->ProcessMessages();
		}

	}
	else {
		for (char dir = 0; dir < 4; dir++) {
			if (lastdir == -1 || (dir + lastdir) % 4 != 1) {
				int newblankpos = newidx[blnkpos][dir];
				if (newblankpos == -1)
					continue;

				n.pz[blnkpos] = n.pz[newblankpos];
				n.pz[newblankpos] = 0;

				int mandi=0;
				for (int i=0;i<16;i++) {
				   if (i != newblankpos)
						mandi += manhattdist[i][n.pz[i]];
				}
				if (mandi < togo) {
					nodecount++;
					if (nodecount % 1000000 == 0) {
						Application->ProcessMessages();
					}
					solution[togo] = blnkpos;
					treesearchMANHATTAN_(n, newblankpos, dir, togo - 1);
				}
				n.pz[newblankpos] = n.pz[blnkpos];
				n.pz[blnkpos] = 0;

			}
		}
	}
}



//only Ken'ichiro Takahashi's wd-heuristic
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void treesearchWD(node n, char blnkpos, unsigned short wdidx, unsigned short wdidx_r,char lastdir,int togo){
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	if (solutionFound||abortIDA) return;

//	char pd[16];
//	for (int i=0;i<16;i++) pd[i]=n.pz[i];

	if (togo == 0) {
		solution[togo] = blnkpos;

		if (solution[84] == 0) { // Ort für Lösungslänge
			QueryPerformanceCounter((LARGE_INTEGER *)&timeStop);
			printSolutionPieces(Form1->puz,CurrentSearchLength, (puztype)(Form1->Type->ItemIndex));
			solution[84] = CurrentSearchLength;
			if (Form1->SolNum->ItemIndex==0) { //only one solution to compute
				solutionFound=1;
				return;
			}
		}
		else {
			if (solution[84] < CurrentSearchLength) {
				QueryPerformanceCounter((LARGE_INTEGER *)&timeStop);
				solutionFound = 1;
				return;
			}
			printSolutionPieces(Form1->puz, CurrentSearchLength, (puztype)(Form1->Type->ItemIndex));
			Application->ProcessMessages();
		}

	}
	else{
		char blnkpos_r=mirr[blnkpos];
		for (char dir=0;dir<4;dir++){
		    if (lastdir!=-1 && (dir+lastdir)%4==1) continue;    //so 10% schneller
			int pos  = newidx[blnkpos][dir];
			if (pos==-1)continue;
			int pos_r  = mirr[pos];
			unsigned short wdidxnew=wdidx,wdidx_rnew=wdidx_r;

			switch (dir){
			case 0: //move blank left
				wdidx_rnew=wdmoveup[wdidx_r][n.pz_r[pos_r]/4];break;
			case 1: //move blank right
				wdidx_rnew=wdmovedown[wdidx_r][n.pz_r[pos_r]/4];break;
			case 2: //move blank up
				wdidxnew=wdmoveup[wdidx][n.pz[pos]/4];break;
			case 3: //move blank down
				wdidxnew=wdmovedown[wdidx][n.pz[pos]/4];break;
			}
			if ((prunwd[wdidxnew]+prunwd[wdidx_rnew]<=togo-1) /*&& (lastdir==-1 ||(dir+lastdir)%4!=1)*/){

				n.pz[blnkpos]=n.pz[pos];n.pz[pos]=0;
				n.pz_r[blnkpos_r]=n.pz_r[pos_r];n.pz_r[pos_r]=0;

				nodecount++;
				if (nodecount%1000000==0) {
					 Application->ProcessMessages();
				}
				solution[togo]=blnkpos;
				treesearchWD(n,pos,wdidxnew,wdidx_rnew,dir,togo-1);

				n.pz[pos]=n.pz[blnkpos];n.pz[blnkpos]=0;
				n.pz_r[pos_r]=n.pz_r[blnkpos_r];n.pz_r[blnkpos_r]=0;

			}
		}
	}
}

//additional pruning sets 1..5,6..10,11..15
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void treesearchWD555_(node n, char blnkpos, unsigned short wdidx, unsigned short wdidx_r,
				int index15, int index610, int index1115,
				int index15_r, int index610_r, int index1115_r,
			    char lastdir,int togo){
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	if (solutionFound||abortIDA) return;

 //	if (togo==0){solutionFound=1;solution[togo]=blnkpos;return;}
	if (togo == 0) {
		solution[togo] = blnkpos;

		if (solution[84] == 0) { // Ort für Lösungslänge
			QueryPerformanceCounter((LARGE_INTEGER *)&timeStop);
			printSolutionPieces(Form1->puz,CurrentSearchLength, (puztype)(Form1->Type->ItemIndex));
			solution[84] = CurrentSearchLength;
			if (Form1->SolNum->ItemIndex==0) { //only one solution to compute
				solutionFound=1;
				return;
			}
		}
		else {
			if (solution[84] < CurrentSearchLength) {
				QueryPerformanceCounter((LARGE_INTEGER *)&timeStop);
				solutionFound = 1;
				return;
			}
			printSolutionPieces(Form1->puz, CurrentSearchLength, (puztype)(Form1->Type->ItemIndex));
			Application->ProcessMessages();
		}

	}
	else{
		char blnkpos_r=mirr[blnkpos];
		for (char dir=0;dir<4;dir++){
			int newblnkpos  = newidx[blnkpos][dir];
			if (newblnkpos==-1)continue;
			int newblnkpos_r  = mirr[newblnkpos];
			unsigned short wdidxnew=wdidx,wdidx_rnew=wdidx_r;

			switch (dir){
			case 0:
				wdidx_rnew=wdmoveup[wdidx_r][n.pz_r[newblnkpos_r]/4];break;
			case 1:
				wdidx_rnew=wdmovedown[wdidx_r][n.pz_r[newblnkpos_r]/4];break;
			case 2:
				wdidxnew=wdmoveup[wdidx][n.pz[newblnkpos]/4];break;
			case 3:
				wdidxnew=wdmovedown[wdidx][n.pz[newblnkpos]/4];break;
			}
			if ((prunwd[wdidxnew]+prunwd[wdidx_rnew]<=togo-1) && (lastdir==-1 ||(dir+lastdir)%4!=1)){

				int index15new=index15; int index610new=index610; int index1115new=index1115;
				int index15new_r=index15_r; int index610new_r=index610_r; int index1115new_r=index1115_r;

				char piece=n.pz[newblnkpos];
				int deltaweight=0,deltapos=0;
				switch (blnkpos-newblnkpos){
					case 1://go ->
						if (piece<6)index15new+= WEIGHT(piece,1,5);
						else if (piece<11)index610new+= WEIGHT(piece,6,10);
						else if (piece<16)index1115new+= WEIGHT(piece,11,15);
						break;
					case -1://go <-
						if (piece<6)index15new-= WEIGHT(piece,1,5);
						else if (piece<11)index610new-= WEIGHT(piece,6,10);
						else if (piece<16)index1115new-= WEIGHT(piece,11,15);
						break;
					case 3:
					case 5:
					case 7:
						if (piece<6){
							for (int i=newblnkpos+1;i<blnkpos;i++){
								int piece1=n.pz[i];
								if (piece1<6){
									if (piece1>piece) {deltapos++;deltaweight+=WEIGHT(piece1,1,5);}//wenn piece<piece1 passiert eigentlich nicht
								}
								else deltapos++;
							}
							deltapos++;//nach newblankpos rutschen
							index15new+=deltaweight+deltapos*WEIGHT(piece,1,5);
						}
						else if (piece<11){
							for (int i=newblnkpos+1;i<blnkpos;i++){
								int piece1=n.pz[i];
								if (piece1>5&&piece1<11){
									if (piece1>piece) {deltapos++;deltaweight+=WEIGHT(piece1,6,10);}//wenn piece<piece1 passiert eigentlich nicht
								}
								else deltapos++;
							}
							deltapos++;//nach newblankpos rutschen
							index610new+=deltaweight+deltapos*WEIGHT(piece,6,10);
						}
						else if (piece<16){
							for (int i=newblnkpos+1;i<blnkpos;i++){
								int piece1=n.pz[i];
								if (piece1>10&&piece1<16){
									if (piece1>piece) {deltapos++;deltaweight+=WEIGHT(piece1,11,15);}//wenn piece<piece1 passiert eigentlich nicht
								}
								else deltapos++;
							}
							deltapos++;//nach newblankpos rutschen
							index1115new+=deltaweight+deltapos*WEIGHT(piece,11,15);
						};
						break;
					case -3:
					case -5:
					case -7:
						if (piece<6){
							for (int i=newblnkpos-1;i>blnkpos;i--){
								int piece1=n.pz[i];
								if (piece1<6){
									if (piece1>piece) {deltapos--;deltaweight-=WEIGHT(piece1,1,5);}//wenn piece<piece1 passiert eigentlich nicht
								}
								else deltapos--;
							}
							deltapos--;//nach newblankpos rutschen
							index15new+=deltaweight+deltapos*WEIGHT(piece,1,5);
						}
						else if (piece<11){
							for  (int i=newblnkpos-1;i>blnkpos;i--){
								int piece1=n.pz[i];
								if (piece1>5&&piece1<11){
									if (piece1>piece) {deltapos--;deltaweight-=WEIGHT(piece1,6,10);}//wenn piece<piece1 passiert eigentlich nicht
								}
								else deltapos--;
							}
							deltapos--;//nach newblankpos rutschen
							index610new+=deltaweight+deltapos*WEIGHT(piece,6,10);
						}
						else if (piece<16){
							for  (int i=newblnkpos-1;i>blnkpos;i--){
								int piece1=n.pz[i];
								if (piece1>10&&piece1<16){
									if (piece1>piece) {deltapos--;deltaweight-=WEIGHT(piece1,11,15);}//wenn piece<piece1 passiert eigentlich nicht
								}
								else deltapos--;
							}
							deltapos--;//nach newblankpos rutschen
							index1115new+=deltaweight+deltapos*WEIGHT(piece,11,15);
						};
						break;
				}

			    piece=n.pz_r[newblnkpos_r];
				switch (blnkpos_r-newblnkpos_r){
					case 1://go ->
						if (piece<6)index15new_r+= WEIGHT(piece,1,5);
						else if (piece<11)index610new_r+= WEIGHT(piece,6,10);
						else if (piece<16)index1115new_r+= WEIGHT(piece,11,15);
						break;
					case -1://go <-
						if (piece<6)index15new_r-= WEIGHT(piece,1,5);
						else if (piece<11)index610new_r-= WEIGHT(piece,6,10);
						else if (piece<16)index1115new_r-= WEIGHT(piece,11,15);
						break;
					case 3:
					case 5:
					case 7:
						if (piece<6){
							for (int i=newblnkpos_r+1;i<blnkpos_r;i++){
								int piece1=n.pz_r[i];
								if (piece1<6){
									if (piece1>piece) {deltapos++;deltaweight+=WEIGHT(piece1,1,5);}//wenn piece<piece1 passiert eigentlich nicht
								}
								else deltapos++;
							}
							deltapos++;//nach newblankpos rutschen
							index15new_r+=deltaweight+deltapos*WEIGHT(piece,1,5);
						}
						else if (piece<11){
							for (int i=newblnkpos_r+1;i<blnkpos_r;i++){
								int piece1=n.pz_r[i];
								if (piece1>5&&piece1<11){
									if (piece1>piece) {deltapos++;deltaweight+=WEIGHT(piece1,6,10);}//wenn piece<piece1 passiert eigentlich nicht
								}
								else deltapos++;
							}
							deltapos++;//nach newblankpos rutschen
							index610new_r+=deltaweight+deltapos*WEIGHT(piece,6,10);
						}
						else if (piece<16){
							for (int i=newblnkpos_r+1;i<blnkpos_r;i++){
								int piece1=n.pz_r[i];
								if (piece1>10&&piece1<16){
									if (piece1>piece) {deltapos++;deltaweight+=WEIGHT(piece1,11,15);}//wenn piece<piece1 passiert eigentlich nicht
								}
								else deltapos++;
							}
							deltapos++;//nach newblankpos rutschen
							index1115new_r+=deltaweight+deltapos*WEIGHT(piece,11,15);
						};
						break;
					case -3:
					case -5:
					case -7:
						if (piece<6){
							for (int i=newblnkpos_r-1;i>blnkpos_r;i--){
								int piece1=n.pz_r[i];
								if (piece1<6){
									if (piece1>piece) {deltapos--;deltaweight-=WEIGHT(piece1,1,5);}//wenn piece<piece1 passiert eigentlich nicht
								}
								else deltapos--;
							}
							deltapos--;//nach newblankpos rutschen
							index15new_r+=deltaweight+deltapos*WEIGHT(piece,1,5);
						}
						else if (piece<11){
							for  (int i=newblnkpos_r-1;i>blnkpos_r;i--){
								int piece1=n.pz_r[i];
								if (piece1>5&&piece1<11){
									if (piece1>piece) {deltapos--;deltaweight-=WEIGHT(piece1,6,10);}//wenn piece<piece1 passiert eigentlich nicht
								}
								else deltapos--;
							}
							deltapos--;//nach newblankpos rutschen
							index610new_r+=deltaweight+deltapos*WEIGHT(piece,6,10);
						}
						else if (piece<16){
							for  (int i=newblnkpos_r-1;i>blnkpos_r;i--){
								int piece1=n.pz_r[i];
								if (piece1>10&&piece1<16){
									if (piece1>piece) {deltapos--;deltaweight-=WEIGHT(piece1,11,15);}//wenn piece<piece1 passiert eigentlich nicht
								}
								else deltapos--;
							}
							deltapos--;//nach newblankpos rutschen
							index1115new_r+=deltaweight+deltapos*WEIGHT(piece,11,15);
						};
						break;
				}

				if (prun15[index15new] + prun610[index610new] +
					prun1115[index1115new] < togo && prun15[index15new_r] +
					prun610[index610new_r] + prun1115[index1115new_r] < togo) {
					nodecount++;
					if (nodecount % 1000000 == 0) {
						Application->ProcessMessages();
					}
					n.pz[blnkpos] = n.pz[newblnkpos];
					n.pz[newblnkpos] = 0;
					n.pz_r[blnkpos_r]=n.pz_r[newblnkpos_r];n.pz_r[newblnkpos_r]=0;
						solution[togo]=blnkpos;
						treesearchWD555_(n,newblnkpos,wdidxnew,wdidx_rnew, index15new,index610new,index1115new,
							index15new_r,index610new_r,index1115new_r,dir,togo-1);
						n.pz[newblnkpos]=n.pz[blnkpos];n.pz[blnkpos]=0;
						n.pz_r[newblnkpos_r]=n.pz_r[blnkpos_r];n.pz_r[blnkpos_r]=0;
				}

			}
		}
	}
}

//additional pruning sets 1..6,7..9,10..15
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void treesearchWD663_(node n, char blnkpos, unsigned short wdidx, unsigned short wdidx_r,
				int index16, int index79, int index1015,
				int index16_r, int index79_r, int index1015_r,
				char lastdir,int togo){
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	if (solutionFound||abortIDA) return;

	//if (togo==0){solutionFound=1;solution[togo]=blnkpos;return;}//wenn wd+wd_r=0, ist das Puzzle gelöst!
	if (togo == 0) {
		solution[togo] = blnkpos;

		if (solution[84] == 0) { // Ort für Lösungslänge
			QueryPerformanceCounter((LARGE_INTEGER *)&timeStop);
			printSolutionPieces(Form1->puz,CurrentSearchLength, (puztype)(Form1->Type->ItemIndex));
			solution[84] = CurrentSearchLength;
			if (Form1->SolNum->ItemIndex==0) { //only one solution to compute
				solutionFound=1;
				return;
			}
		}
		else {
			if (solution[84] < CurrentSearchLength) {
				QueryPerformanceCounter((LARGE_INTEGER *)&timeStop);
				solutionFound = 1;
				return;
			}
			printSolutionPieces(Form1->puz, CurrentSearchLength, (puztype)(Form1->Type->ItemIndex));
			Application->ProcessMessages();
		}

	}
	else{
		char blnkpos_r=mirr[blnkpos];
		for (char dir=0;dir<4;dir++){
			int newblnkpos  = newidx[blnkpos][dir];
			if (newblnkpos==-1)continue;
			int newblnkpos_r  = mirr[newblnkpos];// newidx[blnkpos_r][(dir+2)%4];
			unsigned short wdidxnew=wdidx,wdidx_rnew=wdidx_r;

			switch (dir){
			case 0:
				wdidx_rnew=wdmoveup[wdidx_r][n.pz_r[newblnkpos_r]/4];break;
			case 1:
				wdidx_rnew=wdmovedown[wdidx_r][n.pz_r[newblnkpos_r]/4];break;
			case 2:
				wdidxnew=wdmoveup[wdidx][n.pz[newblnkpos]/4];break;
			case 3:
				wdidxnew=wdmovedown[wdidx][n.pz[newblnkpos]/4];break;
			}
			if ((prunwd[wdidxnew]+prunwd[wdidx_rnew]<=togo-1) && (lastdir==-1 ||(dir+lastdir)%4!=1)){

				int index16new=index16; int index79new=index79; int index1015new=index1015;
				int index16new_r=index16_r; int index79new_r=index79_r; int index1015new_r=index1015_r;



				char piece=n.pz[newblnkpos];
				int deltaweight=0,deltapos=0;
				switch (blnkpos-newblnkpos){
					case 1://go ->
						if (piece<7)index16new+= WEIGHT(piece,1,6);
						else if (piece<10)index79new+= WEIGHT(piece,7,9);
						else if (piece<16)index1015new+= WEIGHT(piece,10,15);
						break;
					case -1://go <-
						if (piece<7)index16new-= WEIGHT(piece,1,6);
						else if (piece<10)index79new-= WEIGHT(piece,7,9);
						else if (piece<16)index1015new-= WEIGHT(piece,10,15);
						break;
					case 3:
					case 5:
					case 7:
						if (piece<7){
							for (int i=newblnkpos+1;i<blnkpos;i++){
								int piece1=n.pz[i];
								if (piece1<7){
									if (piece1>piece) {deltapos++;deltaweight+=WEIGHT(piece1,1,6);}//wenn piece<piece1 passiert eigentlich nicht
								}
								else deltapos++;
							}
							deltapos++;//nach newblankpos rutschen
							index16new+=deltaweight+deltapos*WEIGHT(piece,1,6);
						}
						else if (piece<10){
							for (int i=newblnkpos+1;i<blnkpos;i++){
								int piece1=n.pz[i];
								if (piece1>6&&piece1<10){
									if (piece1>piece) {deltapos++;deltaweight+=WEIGHT(piece1,7,9);}//wenn piece<piece1 passiert eigentlich nicht
								}
								else deltapos++;
							}
							deltapos++;//nach newblankpos rutschen
							index79new+=deltaweight+deltapos*WEIGHT(piece,7,9);
						}
						else if (piece<16){
							for (int i=newblnkpos+1;i<blnkpos;i++){
								int piece1=n.pz[i];
								if (piece1>9&&piece1<16){
									if (piece1>piece) {deltapos++;deltaweight+=WEIGHT(piece1,10,15);}//wenn piece<piece1 passiert eigentlich nicht
								}
								else deltapos++;
							}
							deltapos++;//nach newblankpos rutschen
							index1015new+=deltaweight+deltapos*WEIGHT(piece,10,15);
						};
						break;
					case -3:
					case -5:
					case -7:
						if (piece<7){
							for (int i=newblnkpos-1;i>blnkpos;i--){
								int piece1=n.pz[i];
								if (piece1<7){
									if (piece1>piece) {deltapos--;deltaweight-=WEIGHT(piece1,1,6);}//wenn piece<piece1 passiert eigentlich nicht
								}
								else deltapos--;
							}
							deltapos--;//nach newblankpos rutschen
							index16new+=deltaweight+deltapos*WEIGHT(piece,1,6);
						}
						else if (piece<10){
							for  (int i=newblnkpos-1;i>blnkpos;i--){
								int piece1=n.pz[i];
								if (piece1>6&&piece1<10){
									if (piece1>piece) {deltapos--;deltaweight-=WEIGHT(piece1,7,9);}//wenn piece<piece1 passiert eigentlich nicht
								}
								else deltapos--;
							}
							deltapos--;//nach newblankpos rutschen
							index79new+=deltaweight+deltapos*WEIGHT(piece,7,9);
						}
						else if (piece<16){
							for  (int i=newblnkpos-1;i>blnkpos;i--){
								int piece1=n.pz[i];
								if (piece1>9&&piece1<16){
									if (piece1>piece) {deltapos--;deltaweight-=WEIGHT(piece1,10,15);}//wenn piece<piece1 passiert eigentlich nicht
								}
								else deltapos--;
							}
							deltapos--;//nach newblankpos rutschen
							index1015new+=deltaweight+deltapos*WEIGHT(piece,10,15);
						};
						break;
				}

			    piece=n.pz_r[newblnkpos_r];
				switch (blnkpos_r-newblnkpos_r){
					case 1://go ->
						if (piece<7)index16new_r+= WEIGHT(piece,1,6);
						else if (piece<10)index79new_r+= WEIGHT(piece,7,9);
						else if (piece<16)index1015new_r+= WEIGHT(piece,10,15);
						break;
					case -1://go <-
						if (piece<7)index16new_r-= WEIGHT(piece,1,6);
						else if (piece<10)index79new_r-= WEIGHT(piece,7,9);
						else if (piece<16)index1015new_r-= WEIGHT(piece,10,15);
						break;
					case 3:
					case 5:
					case 7:
						if (piece<7){
							for (int i=newblnkpos_r+1;i<blnkpos_r;i++){
								int piece1=n.pz_r[i];
								if (piece1<7){
									if (piece1>piece) {deltapos++;deltaweight+=WEIGHT(piece1,1,6);}//wenn piece<piece1 passiert eigentlich nicht
								}
								else deltapos++;
							}
							deltapos++;//nach newblankpos rutschen
							index16new_r+=deltaweight+deltapos*WEIGHT(piece,1,6);
						}
						else if (piece<10){
							for (int i=newblnkpos_r+1;i<blnkpos_r;i++){
								int piece1=n.pz_r[i];
								if (piece1>6&&piece1<10){
									if (piece1>piece) {deltapos++;deltaweight+=WEIGHT(piece1,7,9);}//wenn piece<piece1 passiert eigentlich nicht
								}
								else deltapos++;
							}
							deltapos++;//nach newblankpos rutschen
							index79new_r+=deltaweight+deltapos*WEIGHT(piece,7,9);
						}
						else if (piece<16){
							for (int i=newblnkpos_r+1;i<blnkpos_r;i++){
								int piece1=n.pz_r[i];
								if (piece1>9&&piece1<16){
									if (piece1>piece) {deltapos++;deltaweight+=WEIGHT(piece1,10,15);}//wenn piece<piece1 passiert eigentlich nicht
								}
								else deltapos++;
							}
							deltapos++;//nach newblankpos rutschen
							index1015new_r+=deltaweight+deltapos*WEIGHT(piece,10,15);
						};
						break;
					case -3:
					case -5:
					case -7:
						if (piece<7){
							for (int i=newblnkpos_r-1;i>blnkpos_r;i--){
								int piece1=n.pz_r[i];
								if (piece1<7){
									if (piece1>piece) {deltapos--;deltaweight-=WEIGHT(piece1,1,6);}//wenn piece<piece1 passiert eigentlich nicht
								}
								else deltapos--;
							}
							deltapos--;//nach newblankpos rutschen
							index16new_r+=deltaweight+deltapos*WEIGHT(piece,1,6);
						}
						else if (piece<10){
							for  (int i=newblnkpos_r-1;i>blnkpos_r;i--){
								int piece1=n.pz_r[i];
								if (piece1>6&&piece1<10){
									if (piece1>piece) {deltapos--;deltaweight-=WEIGHT(piece1,7,9);}//wenn piece<piece1 passiert eigentlich nicht
								}
								else deltapos--;
							}
							deltapos--;//nach newblankpos rutschen
							index79new_r+=deltaweight+deltapos*WEIGHT(piece,7,9);
						}
						else if (piece<16){
							for  (int i=newblnkpos_r-1;i>blnkpos_r;i--){
								int piece1=n.pz_r[i];
								if (piece1>9&&piece1<16){
									if (piece1>piece) {deltapos--;deltaweight-=WEIGHT(piece1,10,15);}//wenn piece<piece1 passiert eigentlich nicht
								}
								else deltapos--;
							}
							deltapos--;//nach newblankpos rutschen
							index1015new_r+=deltaweight+deltapos*WEIGHT(piece,10,15);
						};
						break;
					}

					if (prun16[index16new] + prun79[index79new] +
						prun1015[index1015new] < togo && prun16[index16new_r] +
						prun79[index79new_r] + prun1015[index1015new_r] < togo)
					{
						nodecount++;
						if (nodecount % 1000000 == 0) {
							Application->ProcessMessages();
						}
						n.pz[blnkpos] = n.pz[newblnkpos];
						n.pz[newblnkpos] = 0;
						n.pz_r[blnkpos_r] = n.pz_r[newblnkpos_r];
						n.pz_r[newblnkpos_r] = 0;
						solution[togo] = blnkpos;
						treesearchWD663_(n, newblnkpos, wdidxnew, wdidx_rnew,
							index16new, index79new, index1015new, index16new_r,index79new_r,index1015new_r,dir,togo-1);
						n.pz[newblnkpos]=n.pz[blnkpos];n.pz[blnkpos]=0;
						n.pz_r[newblnkpos_r]=n.pz_r[blnkpos_r];n.pz_r[blnkpos_r]=0;


				}
			}
		}
	}
}
//additional pruning sets 1..7,8..15
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void treesearchWD78_(node n, char blnkpos, unsigned short wdidx, unsigned short wdidx_r,
				int index17, int index815,
				int index17_r, int index815_r,
			    char lastdir,int togo){
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	if (solutionFound||abortIDA) return;
   //	if (togo==0){solutionFound=1;solution[togo]=blnkpos;return;}
	if (togo == 0) {
		solution[togo] = blnkpos;

		if (solution[84] == 0) { // Ort für Lösungslänge
		QueryPerformanceCounter((LARGE_INTEGER *)&timeStop);
			printSolutionPieces(Form1->puz,CurrentSearchLength, (puztype)(Form1->Type->ItemIndex));
			solution[84] = CurrentSearchLength;
			if (Form1->SolNum->ItemIndex==0) { //only one solution to compute
				solutionFound=1;
				return;
			}
		}
		else {
			if (solution[84] < CurrentSearchLength) {
				QueryPerformanceCounter((LARGE_INTEGER *)&timeStop);
				solutionFound = 1;
				return;
			}
			printSolutionPieces(Form1->puz, CurrentSearchLength, (puztype)(Form1->Type->ItemIndex));
			Application->ProcessMessages();
		}

	}
	else{
		char blnkpos_r=mirr[blnkpos];
		for (char dir=0;dir<4;dir++){
			int newblnkpos  = newidx[blnkpos][dir];
			if (newblnkpos==-1)continue;
			int newblnkpos_r  = mirr[newblnkpos];
			unsigned short wdidxnew=wdidx,wdidx_rnew=wdidx_r;

			switch (dir){
			case 0:
				wdidx_rnew=wdmoveup[wdidx_r][n.pz_r[newblnkpos_r]/4];break;
			case 1:
				wdidx_rnew=wdmovedown[wdidx_r][n.pz_r[newblnkpos_r]/4];break;
			case 2:
				wdidxnew=wdmoveup[wdidx][n.pz[newblnkpos]/4];break;
			case 3:
				wdidxnew=wdmovedown[wdidx][n.pz[newblnkpos]/4];break;
			}

			if ((prunwd[wdidxnew]+prunwd[wdidx_rnew]<=togo-1) && (lastdir==-1 ||(dir+lastdir)%4!=1)){

				int index17new=index17; int index815new=index815;
				int index17new_r=index17_r; int index815new_r=index815_r;

				char piece=n.pz[newblnkpos];
				int deltaweight=0,deltapos=0;
				switch (blnkpos-newblnkpos){
					case 1://go ->
						if (piece<8)index17new+= WEIGHT(piece,1,7);
						else index815new+= WEIGHT(piece,8,15);
						break;
					case -1://go <-
						if (piece<8)index17new-= WEIGHT(piece,1,7);
						else index815new-= WEIGHT(piece,8,15);
						break;
					case 3:
					case 5:
					case 7:
						if (piece<8){
							for (int i=newblnkpos+1;i<blnkpos;i++){
								int piece1=n.pz[i];
								if (piece1<8){
									if (piece1>piece) {deltapos++;deltaweight+=WEIGHT(piece1,1,7);}//wenn piece<piece1 passiert eigentlich nicht
								}
								else deltapos++;
							}
							deltapos++;//nach newblankpos rutschen
							index17new+=deltaweight+deltapos*WEIGHT(piece,1,7);
						}
						else {
							for (int i=newblnkpos+1;i<blnkpos;i++){
								int piece1=n.pz[i];
								if (piece1>7){
									if (piece1>piece) {deltapos++;deltaweight+=WEIGHT(piece1,8,15);}//wenn piece<piece1 passiert eigentlich nicht
								}
								else deltapos++;
							}
							deltapos++;//nach newblankpos rutschen
							index815new+=deltaweight+deltapos*WEIGHT(piece,8,15);
						};
						break;
					case -3:
					case -5:
					case -7:
						if (piece<8){
							for (int i=newblnkpos-1;i>blnkpos;i--){
								int piece1=n.pz[i];
								if (piece1<8){
									if (piece1>piece) {deltapos--;deltaweight-=WEIGHT(piece1,1,7);}//wenn piece<piece1 passiert eigentlich nicht
								}
								else deltapos--;
							}
							deltapos--;//nach newblankpos rutschen
							index17new+=deltaweight+deltapos*WEIGHT(piece,1,7);
						}
						else {
							for  (int i=newblnkpos-1;i>blnkpos;i--){
								int piece1=n.pz[i];
								if (piece1>7){
									if (piece1>piece) {deltapos--;deltaweight-=WEIGHT(piece1,8,15);}//wenn piece<piece1 passiert eigentlich nicht
								}
								else deltapos--;
							}
							deltapos--;//nach newblankpos rutschen
							index815new+=deltaweight+deltapos*WEIGHT(piece,8,15);
						};
						break;
				}

			    piece=n.pz_r[newblnkpos_r];
				switch (blnkpos_r-newblnkpos_r){
					case 1://go ->
						if (piece<8)index17new_r+= WEIGHT(piece,1,7);
						else index815new_r+= WEIGHT(piece,8,15);
						break;
					case -1://go <-
						if (piece<8)index17new_r-= WEIGHT(piece,1,7);
						else index815new_r-= WEIGHT(piece,8,15);
						break;
					case 3:
					case 5:
					case 7:
						if (piece<8){
							for (int i=newblnkpos_r+1;i<blnkpos_r;i++){
								int piece1=n.pz_r[i];
								if (piece1<8){
									if (piece1>piece) {deltapos++;deltaweight+=WEIGHT(piece1,1,7);}//wenn piece<piece1 passiert eigentlich nicht
								}
								else deltapos++;
							}
							deltapos++;//nach newblankpos rutschen
							index17new_r+=deltaweight+deltapos*WEIGHT(piece,1,7);
						}
						else {
							for (int i=newblnkpos_r+1;i<blnkpos_r;i++){
								int piece1=n.pz_r[i];
								if (piece1>7){
									if (piece1>piece) {deltapos++;deltaweight+=WEIGHT(piece1,8,15);}//wenn piece<piece1 passiert eigentlich nicht
								}
								else deltapos++;
							}
							deltapos++;//nach newblankpos rutschen
							index815new_r+=deltaweight+deltapos*WEIGHT(piece,8,15);
						};
						break;
					case -3:
					case -5:
					case -7:
						if (piece<8){
							for (int i=newblnkpos_r-1;i>blnkpos_r;i--){
								int piece1=n.pz_r[i];
								if (piece1<8){
									if (piece1>piece) {deltapos--;deltaweight-=WEIGHT(piece1,1,7);}//wenn piece<piece1 passiert eigentlich nicht
								}
								else deltapos--;
							}
							deltapos--;//nach newblankpos rutschen
							index17new_r+=deltaweight+deltapos*WEIGHT(piece,1,7);
						}
						else {
							for  (int i=newblnkpos_r-1;i>blnkpos_r;i--){
								int piece1=n.pz_r[i];
								if (piece1>7){
									if (piece1>piece) {deltapos--;deltaweight-=WEIGHT(piece1,8,15);}//wenn piece<piece1 passiert eigentlich nicht
								}
								else deltapos--;
							}
							deltapos--;//nach newblankpos rutschen
							index815new_r+=deltaweight+deltapos*WEIGHT(piece,8,15);
						};
						break;
				}

				if (prun17[index17new]+prun815[index815new]<togo &&
					prun17[index17new_r]+prun815[index815new_r]<togo){
						nodecount++;
						if (nodecount % 1000000 == 0) {
							Application->ProcessMessages();
						}
						n.pz[blnkpos] = n.pz[newblnkpos];
						n.pz[newblnkpos] = 0;
						n.pz_r[blnkpos_r] = n.pz_r[newblnkpos_r];
						n.pz_r[newblnkpos_r] = 0;
						solution[togo]=blnkpos;
						treesearchWD78_(n,newblnkpos,wdidxnew,wdidx_rnew, index17new,index815new,
							index17new_r,index815new_r,dir,togo-1);
						n.pz[newblnkpos]=n.pz[blnkpos];n.pz[blnkpos]=0;
						n.pz_r[newblnkpos_r]=n.pz_r[blnkpos_r];n.pz_r[blnkpos_r]=0;
				}
			}
		}
	}
}

//Only 7-8 pattern database is faster usually as together with WD
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void treesearch78_(node n, char blnkpos,int index17, int index815,
				int index17_r, int index815_r,char lastdir,int togo){
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	if (solutionFound||abortIDA)
		return;
	// if (togo==0){solutionFound=1;solution[togo]=blnkpos;return;}

	if (togo == 0) {
		solution[togo] = blnkpos;

		if (solution[84] == 0) { // Ort für Lösungslänge
			QueryPerformanceCounter((LARGE_INTEGER *)&timeStop);
			printSolutionPieces(Form1->puz,CurrentSearchLength, (puztype)(Form1->Type->ItemIndex));
			solution[84] = CurrentSearchLength;
			if (Form1->SolNum->ItemIndex==0) { //only one solution to compute
				solutionFound=1;
				return;
			}
		}
		else {
			if (solution[84] < CurrentSearchLength) {
				QueryPerformanceCounter((LARGE_INTEGER *)&timeStop);
				solutionFound = 1;
				return;
			}
			printSolutionPieces(Form1->puz, CurrentSearchLength, (puztype)(Form1->Type->ItemIndex));
			Application->ProcessMessages();
		}

	}

	else{
		char blnkpos_r=mirr[blnkpos];
		for (char dir=0;dir<4;dir++){


			if (lastdir==-1 || (dir + lastdir) % 4 != 1) {

				int newblnkpos = newidx[blnkpos][dir];
				if (newblnkpos == -1)
					continue;
				int newblnkpos_r = mirr[newblnkpos];

				int index17new = index17;
				int index815new = index815;
				int index17new_r = index17_r;
				int index815new_r = index815_r;

				char piece = n.pz[newblnkpos];
				int deltaweight=0,deltapos=0;
				switch (blnkpos-newblnkpos){
					case 1://go ->
						if (piece<8)index17new+= WEIGHT(piece,1,7);
						else index815new+= WEIGHT(piece,8,15);
						break;
					case -1://go <-
						if (piece<8)index17new-= WEIGHT(piece,1,7);
						else index815new-= WEIGHT(piece,8,15);
						break;
					case 3:
					case 5:
					case 7:
						if (piece<8){
							for (int i=newblnkpos+1;i<blnkpos;i++){
								int piece1=n.pz[i];
								if (piece1<8){
									if (piece1>piece) {deltapos++;deltaweight+=WEIGHT(piece1,1,7);}//wenn piece<piece1 passiert eigentlich nicht
								}
								else deltapos++;
							}
							deltapos++;//nach newblankpos rutschen
							index17new+=deltaweight+deltapos*WEIGHT(piece,1,7);
						}
						else {
							for (int i=newblnkpos+1;i<blnkpos;i++){
								int piece1=n.pz[i];
								if (piece1>7){
									if (piece1>piece) {deltapos++;deltaweight+=WEIGHT(piece1,8,15);}//wenn piece<piece1 passiert eigentlich nicht
								}
								else deltapos++;
							}
							deltapos++;//nach newblankpos rutschen
							index815new+=deltaweight+deltapos*WEIGHT(piece,8,15);
						};
						break;
					case -3:
					case -5:
					case -7:
						if (piece<8){
							for (int i=newblnkpos-1;i>blnkpos;i--){
								int piece1=n.pz[i];
								if (piece1<8){
									if (piece1>piece) {deltapos--;deltaweight-=WEIGHT(piece1,1,7);}//wenn piece<piece1 passiert eigentlich nicht
								}
								else deltapos--;
							}
							deltapos--;//nach newblankpos rutschen
							index17new+=deltaweight+deltapos*WEIGHT(piece,1,7);
						}
						else {
							for  (int i=newblnkpos-1;i>blnkpos;i--){
								int piece1=n.pz[i];
								if (piece1>7){
									if (piece1>piece) {deltapos--;deltaweight-=WEIGHT(piece1,8,15);}//wenn piece<piece1 passiert eigentlich nicht
								}
								else deltapos--;
							}
							deltapos--;//nach newblankpos rutschen
							index815new+=deltaweight+deltapos*WEIGHT(piece,8,15);
						};
						break;
				}

			    piece=n.pz_r[newblnkpos_r];
				switch (blnkpos_r-newblnkpos_r){
					case 1://go ->
						if (piece<8)index17new_r+= WEIGHT(piece,1,7);
						else index815new_r+= WEIGHT(piece,8,15);
						break;
					case -1://go <-
						if (piece<8)index17new_r-= WEIGHT(piece,1,7);
						else index815new_r-= WEIGHT(piece,8,15);
						break;
					case 3:
					case 5:
					case 7:
						if (piece<8){
							for (int i=newblnkpos_r+1;i<blnkpos_r;i++){
								int piece1=n.pz_r[i];
								if (piece1<8){
									if (piece1>piece) {deltapos++;deltaweight+=WEIGHT(piece1,1,7);}//wenn piece<piece1 passiert eigentlich nicht
								}
								else deltapos++;
							}
							deltapos++;//nach newblankpos rutschen
							index17new_r+=deltaweight+deltapos*WEIGHT(piece,1,7);
						}
						else {
							for (int i=newblnkpos_r+1;i<blnkpos_r;i++){
								int piece1=n.pz_r[i];
								if (piece1>7){
									if (piece1>piece) {deltapos++;deltaweight+=WEIGHT(piece1,8,15);}//wenn piece<piece1 passiert eigentlich nicht
								}
								else deltapos++;
							}
							deltapos++;//nach newblankpos rutschen
							index815new_r+=deltaweight+deltapos*WEIGHT(piece,8,15);
						};
						break;
					case -3:
					case -5:
					case -7:
						if (piece<8){
							for (int i=newblnkpos_r-1;i>blnkpos_r;i--){
								int piece1=n.pz_r[i];
								if (piece1<8){
									if (piece1>piece) {deltapos--;deltaweight-=WEIGHT(piece1,1,7);}//wenn piece<piece1 passiert eigentlich nicht
								}
								else deltapos--;
							}
							deltapos--;//nach newblankpos rutschen
							index17new_r+=deltaweight+deltapos*WEIGHT(piece,1,7);
						}
						else {
							for  (int i=newblnkpos_r-1;i>blnkpos_r;i--){
								int piece1=n.pz_r[i];
								if (piece1>7){
									if (piece1>piece) {deltapos--;deltaweight-=WEIGHT(piece1,8,15);}//wenn piece<piece1 passiert eigentlich nicht
								}
								else deltapos--;
							}
							deltapos--;//nach newblankpos rutschen
							index815new_r+=deltaweight+deltapos*WEIGHT(piece,8,15);
						};
						break;
				}

				if (prun17[index17new] + prun815[index815new] <
					togo && prun17[index17new_r] +
					prun815[index815new_r] < togo) {
					nodecount++;
					if (nodecount % 1000000 == 0) {
						Application->ProcessMessages();
					}
					n.pz[blnkpos] = n.pz[newblnkpos];
					n.pz[newblnkpos] = 0;
					n.pz_r[blnkpos_r] = n.pz_r[newblnkpos_r];
					n.pz_r[newblnkpos_r] = 0;
					solution[togo] =blnkpos;
						treesearch78_(n,newblnkpos,index17new,index815new,index17new_r,index815new_r,dir,togo-1);
						n.pz[newblnkpos]=n.pz[blnkpos];n.pz[blnkpos]=0;
						n.pz_r[newblnkpos_r]=n.pz_r[blnkpos_r];n.pz_r[blnkpos_r]=0;
				}
			}
		}
	}
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
int IDASearch_(char* p, puztype t, heuristic h){
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  solutionFound=0; abortIDA=0;
   for (int i=0; i < 85; i++) solution[i]=0;
  nodecount=0;
  char p_r[16];
  node nd;
  switch (t){
	  case INTERNAL:
		  for (int i=0;i<16;i++) nd.pz[i]=p[i];
		  p=nd.pz;
		  break;
	  case ZEROBASED:
		  for (int i=0;i<16;i++) nd.pz[i]=fromzerobased[p[fromzerobased[i]]];
		  p=nd.pz;
		  break;
	  case ZEROTERMINATED:
		  for (int i=0;i<16;i++) p_r[i]= (16-p[15-i])%16;
		  for (int i=0;i<16;i++) nd.pz[i]=fromzerobased[p_r[fromzerobased[i]]];
		  p=nd.pz;
		  break;
  }


  if (parity(p)!=0) return -1;//unsolvable
  reflect(p,p_r);
  for (int i=0;i<16;i++) nd.pz_r[i]=p_r[i];
  CurrentSearchLength = 2-blankpos(p)%2;
  QueryPerformanceCounter((LARGE_INTEGER *)&timeStart);
  do{
	  // Form1->Memo->Lines->Add(IntToStr(togo)+AnsiString(":"));
	  //if (togo>60) cout<<togo<<":"<<endl;
	  switch(h){
		  case ONLY78:{
			  treesearch78_(nd,blankpos(p),index(p,1,7),index(p,8,15),
				  index(p_r,1,7),index(p_r,8,15),-1,CurrentSearchLength);
			  break;
					  }
		  case WD78:{
			  treesearchWD78_(nd,blankpos(p),
				  wdindex(p),wdindex(p_r),index(p,1,7),index(p,8,15),
				  index(p_r,1,7),index(p_r,8,15),-1,CurrentSearchLength);
			  break;
					}
		  case WD:{
			  treesearchWD(nd,blankpos(p),wdindex(p),wdindex(p_r),-1,CurrentSearchLength);
			  break;
				  }
		  case WD663:{
			  treesearchWD663_(nd,blankpos(p),
				  wdindex(p),wdindex(p_r),index(p,1,6),index(p,7,9),index(p,10,15),
				index(p_r,1,6),index(p_r,7,9),index(p_r,10,15),-1,CurrentSearchLength);
			  break;
					 }
		  case WD555:{
			  treesearchWD555_(nd,blankpos(p),
				  wdindex(p),wdindex(p_r), index(p, 1, 5), index(p, 6, 10),
					index(p, 11, 15), index(p_r, 1, 5), index(p_r, 6, 10),
					index(p_r, 11, 15), -1, CurrentSearchLength);
				break;
			}
		case MANHATTAN: {
				treesearchMANHATTAN_(nd, blankpos(p), -1,CurrentSearchLength);
				break;

			}

		}
		Form1->Searchdepth->Text=IntToStr(CurrentSearchLength);
		//  Application->ProcessMessages();
	  CurrentSearchLength+=2;
	   Form1->Searchdepth->Text=IntToStr(CurrentSearchLength);

  }while (solutionFound==0&&abortIDA==0);
  return CurrentSearchLength-2;
}


//Print the abolute positions of the solving moves
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void printSolutionPositions(int l,puztype t){
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	 //Form1->Memo->Lines->Add(IntToStr(l)+AnsiString(":"));
	//cout<<l<<": ";
	String s ="";
	switch(t){
		case INTERNAL:
			for (int i=l-1;i>=0;i--) //cout<<(int)solution[i]<<" ";cout<<endl;
			s+=IntToStr((int)solution[i])+AnsiString(" ");
			break;
		case ZEROBASED:
			for (int i=l-1;i>=0;i--) //cout<<fromzerobased[solution[i]]<<" ";cout<<endl;
			s+=IntToStr(fromzerobased[solution[i]])+AnsiString(" ");
			break;
		case ZEROTERMINATED:
			for (int i=l-1;i>=0;i--)// cout<<15-fromzerobased[solution[i]]<<" ";cout<<endl;
				s+=IntToStr(15-fromzerobased[solution[i]])+AnsiString(" ");
			break;
	}
	Form1->Memo->Lines->Add(IntToStr(l)+AnsiString(":")+s);
	lastSolution[84]=l; //Länge der Lösung
}

//Print the tile numbers to be moved
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void printSolutionPieces(char* pz,int l,puztype t){
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	//cout<<l<<": ";
   //	Form1->Memo->Lines->Add(IntToStr(l)+AnsiString(": "));
	char pztmp[16];
	String sol="";
	switch(t){
		case INTERNAL:
			for (int i=0;i<16;i++) pztmp[i]=pz[i];
			for (int i=l-1;i>=0;i--){
				int s= solution[i];
				lastSolution[i]= pztmp[s];
				sol+= IntToStr((int)pztmp[s])+AnsiString(" | ");
			   //	cout<<(int)pztmp[s]<<" ";
				pztmp[blankpos(pztmp)]=pztmp[s];
				pztmp[s]=0;
			}
			//cout<<endl;
		case ZEROBASED:
			for (int i=0;i<16;i++) pztmp[fromzerobased[i]]=pz[i];
			for (int i=l-1;i>=0;i--){
				int s= solution[i];
				lastSolution[i]= pztmp[s];
				sol+= IntToStr((int)pztmp[s])+AnsiString(" | ");
				//cout<<(int)pztmp[s]<<" ";
				pztmp[blankpos(pztmp)]=pztmp[s];
				pztmp[s]=0;
			}
		   //	cout<<endl;
			break;
		case ZEROTERMINATED:
			for (int i=0;i<16;i++) pztmp[fromzerobased[i]]=(16-pz[15-i])%16;
			for (int i=l-1;i>=0;i--){
				int s= solution[i];
				lastSolution[i]= (16-pztmp[s])%16;
				sol+= IntToStr((16-pztmp[s])%16)+AnsiString(" | ");
			   //	cout<<(16-pztmp[s])%16<<" ";
				pztmp[blankpos(pztmp)]=pztmp[s];
				pztmp[s]=0;
			}
		   //	cout<<endl;
			break;
	}
   Form1->Memo->Lines->Add(DupeString("-",170));
   if (l>-1) {
	if (l==1)
			Form1->Memo->Lines->Add("1 move:");
		else
			Form1->Memo->Lines->Add(IntToStr(l) + L" moves:");
		Form1->Memo->Lines->Add(L"| "+sol);
		lastSolution[84] = l; // Länge der Lösung
	}
	else
		Form1->Memo->Lines->Add(L"No solution");
}

void buildManhattDist(){// im internal Format
	for (int i = 0; i < 16; i++) {
		int it = fromzerobased[i];
		for (int j = 0; j < 16; j++) {
			int jt = fromzerobased[j];
			manhattdist[i][j] = abs(it / 4 - jt / 4) + abs(it % 4 - jt % 4);
		}
	}
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void init(){
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


	prunwd = new char[NWD];

	prun15 = new char[Bnk(16,5)];
	prun610 = new char[Bnk(16,5)];
	prun1115 = new char[Bnk(16,5)];

	prun16 = new char[Bnk(16,6)];
	prun79 = new char[Bnk(16,3)];
	prun1015 = new char[Bnk(16,6)];

	try {
		prun17 = new char[Bnk(16, 7)];
		prun815 = new char[Bnk(16, 8)];
	}
	catch (...) {
		delete[]prun17;
		delete[]prun815;
		prun815 = NULL;
		prun17 = NULL;
	}


	srand ( time(NULL) );
	buildBNK();
	inithashABCD();
	buildprunWD(prunwd);//wd+wd_r Mittelwert 39.28
	buildManhattDist();
	buildmasksum();
	buildprun_(prun15,1,5,true);buildprun_(prun610,6,10,true);buildprun_(prun1115,11,15,true);
	buildprun_(prun16,1,6,true);buildprun_(prun79,7,9,true);buildprun_(prun1015,10,15,true);
    if (prun815!=NULL) {buildprun_32(prun17,1,7,true);buildprun_32(prun815,8,15,true);}
	nodecount=0;sumdepth=0;
	Form1->showPuzzle();
}
