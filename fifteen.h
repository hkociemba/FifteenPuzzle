//---------------------------------------------------------------------------

#ifndef fifteenH
#define fifteenH

#define SQSIZE 64
extern int lastSolution[85];
extern __int64 nodecount;

extern char *prun17,*prun815, *prun16, *prun79, *prun1015, *prun15, *prun610,*prun1115;
extern __int64 timeStart,timeStop;
extern int fromzerobased[16];
extern char *prunwd;
extern int CurrentSearchLength;
extern int solutionFound,abortIDA;

//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.StdCtrls.hpp>
#include <Vcl.Forms.hpp>
#include <Vcl.ExtCtrls.hpp>
#include <Vcl.Imaging.GIFImg.hpp>
#include <Vcl.Imaging.jpeg.hpp>
//---------------------------------------------------------------------------
class TForm1 : public TForm
{
__published:	// Von der IDE verwaltete Komponenten
	TTimer *Timer1;
	TPanel *Panel1;
	TImage *Image1;
	TImage *Image2;
	TImage *Image3;
	TImage *Image4;
	TImage *Image5;
	TImage *Image6;
	TImage *Image7;
	TImage *Image8;
	TImage *Image9;
	TImage *Image10;
	TImage *Image11;
	TImage *Image12;
	TImage *Image13;
	TImage *Image14;
	TImage *Image15;
	TRadioGroup *Heuristic;
	TButton *ButtonSolve;
	TRadioGroup *Type;
	TMemo *Memo;
	TButton *BRandom;
	TButton *ApplySolution;
	TEdit *Time;
	TLabel *Label1;
	TLabel *Label2;
	TLabel *Label3;
	TEdit *Nodes;
	TRadioGroup *SolNum;
	TEdit *ManhattanDist;
	TEdit *WalkingDist;
	TEdit *_78Dist;
	TEdit *WD663Dist;
	TEdit *WD555Dist;
	TLabel *Label5;
	TEdit *Searchdepth;
	TMemo *Memo1;
	TButton *Button2;
	void __fastcall Timer1Timer(TObject *Sender);
	void __fastcall FormCreate(TObject *Sender);
	void __fastcall Image1MouseDown(TObject *Sender, TMouseButton Button, TShiftState Shift,
          int X, int Y);
	void __fastcall TypeClick(TObject *Sender);
	void __fastcall FormActivate(TObject *Sender);
	void __fastcall ButtonSolveClick(TObject *Sender);
	void __fastcall BRandomClick(TObject *Sender);
	void __fastcall Image1DragOver(TObject *Sender, TObject *Source, int X, int Y, TDragState State,
          bool &Accept);
	void __fastcall Panel1DragOver(TObject *Sender, TObject *Source, int X, int Y, TDragState State,
          bool &Accept);
	void __fastcall Panel1DragDrop(TObject *Sender, TObject *Source, int X, int Y);
	void __fastcall Image1DragDrop(TObject *Sender, TObject *Source, int X, int Y);
	void __fastcall ApplySolutionClick(TObject *Sender);
	void __fastcall HeuristicClick(TObject *Sender);
	void __fastcall SolNumClick(TObject *Sender);
	void __fastcall FormCloseQuery(TObject *Sender, bool &CanClose);
	void __fastcall Button2Click(TObject *Sender);




private:	// Anwender-Deklarationen
public:		// Anwender-Deklarationen
	 TImage *img[16];
	 char puz[16];
	__fastcall TForm1(TComponent* Owner);
	void moveImage(TImage *Image, int dir);
	void showPuzzle();
	int manhattanDistance();
	int walkingDistance();
	int _78Distance();
	int _663Distance();
	int _555Distance();
};
//---------------------------------------------------------------------------
extern PACKAGE TForm1 *Form1;
//---------------------------------------------------------------------------
#endif



