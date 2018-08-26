//---------------------------------------------------------------------------

#ifndef pruntablesH
#define pruntablesH
//---------------------------------------------------------------------------
#endif
#include <StrUtils.hpp>



enum puztype {ZEROBASED,ZEROTERMINATED,INTERNAL};
enum heuristic {MANHATTAN,WD,WD555,WD663,ONLY78,WD78};
void init();
void printSolutionPieces(char* pz,int l,puztype t);
void printSolutionPositions(int l,puztype t);
int IDASearch_(char* p, puztype t, heuristic h);
void buildprun_(char* prn, int a, int b, bool loadonly);
int buildprun_32(char* prn, int a, int b, bool loadonly);
char * randompuz(char* pz,puztype type);
unsigned int Bnk(int n, int k);
void reflect(char *src,char *dst);
unsigned short wdindex(char *pz);
int index(char*pz, int a, int b);

template <class T> void swap ( T& a, T& b )
{
  T c(a); a=b; b=c;
}

