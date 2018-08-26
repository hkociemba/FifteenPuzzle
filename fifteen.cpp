#include <vcl.h>
#include <StrUtils.hpp>

#pragma hdrstop

#include "fifteen.h"
#include "pruntables.h"
// ---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TForm1 *Form1;

TImage *ti; // for the move routine
int dir;

// ---------------------------------------------------------------------------
__fastcall TForm1::TForm1(TComponent* Owner) : TForm(Owner) {
}

void TForm1::moveImage(TImage *Image, int dir) {

	switch (dir) {
	case 0:
		Image->Left -= SQSIZE / 8;
		break;
	case 1:
		Image->Left += SQSIZE / 8;
		break;
	case 2:
		Image->Top -= SQSIZE / 8;
		break;
	case 3:
		Image->Top += SQSIZE / 8;
		break;
	default: ;
	}
}

void __fastcall TForm1::Timer1Timer(TObject *Sender) {
	moveImage(ti, dir);
	((TTimer*)Sender)->Tag--;
	if (((TTimer*)Sender)->Tag == 0) {
		((TTimer*)Sender)->Enabled = False;
		((TTimer*)Sender)->Tag = 8;
		showPuzzle();
	}
}

void __fastcall TForm1::FormCreate(TObject *Sender) {

	img[1] = Image1;
	img[2] = Image2;
	img[3] = Image3;
	img[4] = Image4;
	img[5] = Image5;
	img[6] = Image6;
	img[7] = Image7;
	img[8] = Image8;
	img[9] = Image9;
	img[10] = Image10;
	img[11] = Image11;
	img[12] = Image12;
	img[13] = Image13;
	img[14] = Image14;
	img[15] = Image15;
	for (int i = 0; i < 15; i++) { // start zero terminated
		puz[i] = i + 1;
	}
	puz[15] = 0;
}

// ---------------------------------------------------------------------------
void __fastcall TForm1::Image1MouseDown(TObject *Sender, TMouseButton Button,
	TShiftState Shift, int X, int Y) {
	TImage* t = (TImage*)Sender;
	ti = t;
	int idx;
	switch (Button) {
	case mbLeft: //
		idx = (t->Top - 2) / SQSIZE * 4 + (t->Left - 2) / SQSIZE;
		if (idx < 15 && puz[idx + 1] == 0 && (idx + 1) % 4 != 0) {
			ti = img[puz[idx]];
			dir = 1;
			puz[idx + 1] = puz[idx];
			puz[idx] = 0;
			Timer1->Enabled = True;
		}
		else if (idx > 0 && puz[idx - 1] == 0 && idx % 4 != 0) {
			ti = img[puz[idx]];
			dir = 0;
			puz[idx - 1] = puz[idx];
			puz[idx] = 0;
			Timer1->Enabled = True;
		}
		else if (idx - 4 >= 0 && puz[idx - 4] == 0) {
			ti = img[puz[idx]];
			dir = 2;
			puz[idx - 4] = puz[idx];
			puz[idx] = 0;
			Timer1->Enabled = True;
		}
		else if (idx + 4 < 16 && puz[idx + 4] == 0) {
			ti = img[puz[idx]];
			dir = 3;
			puz[idx + 4] = puz[idx];
			puz[idx] = 0;
			Timer1->Enabled = True;
		}
		else
			t->BeginDrag(true, -1);
		break;
	case mbRight:
		t->BeginDrag(true, -1);
		break;
	case mbMiddle:
		t->BeginDrag(true, -1);
		break;
	}
}

// ---------------------------------------------------------------------------
void __fastcall TForm1::TypeClick(TObject *Sender) {
	if (((TRadioGroup*)Sender)->ItemIndex == 1) {
		for (int i = 0; i < 15; i++) // zero terminated
				puz[i] = i + 1;
		puz[15] = 0;
	}
	else {
		for (int i = 0; i < 16; i++) // zero based
				puz[i] = i;
	}
	showPuzzle();
}
// ---------------------------------------------------------------------------

void TForm1::showPuzzle() {
	for (int i = 0; i < 16; i++) {
		if (puz[i] != 0) {
			img[puz[i]]->Left = 2 + SQSIZE * (i % 4);
			img[puz[i]]->Top = 2 + SQSIZE * (i / 4);
		}
	}
	ManhattanDist->Text = IntToStr(manhattanDistance());
	WalkingDist->Text = IntToStr(walkingDistance());
	_78Dist->Text = IntToStr(_78Distance());
	int d1 = _663Distance();
	int d2 = walkingDistance();
	if (d1 != -1 && d2 > d1)
		d1 = d2;
	WD663Dist->Text = IntToStr(d1);
	d1 = _555Distance();
	d2 = walkingDistance();
	if (d1 != -1 && d2 > d1)
		d1 = d2;
	WD555Dist->Text = IntToStr(d1);
}

// ---------------------------------------------------------------------------

void __fastcall TForm1::FormActivate(TObject *Sender) {
	init();
}

// ---------------------------------------------------------------------------
void __fastcall TForm1::ButtonSolveClick(TObject *Sender) {
	__int64 f, t0, t1;
	// ButtonSolve->Enabled = false;
	if (ButtonSolve->Caption == "Abort") {
		abortIDA = 1;
		ButtonSolve->Caption = "Solve!";
		Memo->Lines->Add(L"aborted");
		return;
	}
	ButtonSolve->Caption = "Abort";
	BRandom->Enabled = false;
	ApplySolution->Enabled = false;
	Type->Enabled = false;
	Heuristic->Enabled = false;
	Time->Text = "";
	Nodes->Text = "";
	QueryPerformanceFrequency((LARGE_INTEGER *)&f);
	// Frequenz in ticks per second
	// if (SolNum->ItemIndex==1){
	Memo->Lines->Add(L"");
	Memo->Lines->Add(DupeString("-", 170));
	// bei mehreren potentiellen Lösungen
	// }
	int l = IDASearch_(puz, (puztype)Type->ItemIndex,
		(heuristic)Heuristic->ItemIndex);

	if (l == -1) {
		Time->Text = "-----";
		Nodes->Text = "-----";
		Searchdepth->Text = "unsolvable";
	}
	else if (abortIDA == 0) {
		Time->Text = IntToStr((timeStop - timeStart) * 1000 / f);
		Nodes->Text = IntToStr(nodecount);
		Searchdepth->Text = IntToStr(l - 2 * SolNum->ItemIndex);
		Memo->Lines->Add(DupeString("-", 170));
	}
	// ButtonSolve->Enabled = true;
	ButtonSolve->Caption = "Solve!";
	BRandom->Enabled = true;
	if (SolNum->ItemIndex == 0)
		ApplySolution->Enabled = true;
	Type->Enabled = true;
	Heuristic->Enabled = true;
}

// ---------------------------------------------------------------------------
void __fastcall TForm1::BRandomClick(TObject *Sender) {
	randompuz(puz, (puztype)Type->ItemIndex);
	ApplySolution->Enabled = false;
	showPuzzle();
}

// ---------------------------------------------------------------------------
void __fastcall TForm1::Image1DragOver(TObject *Sender, TObject *Source, int X,
	int Y, TDragState State, bool &Accept) {
	if (Source->InheritsFrom(__classid(TImage)))
		Accept = true;
}

// ---------------------------------------------------------------------------
void __fastcall TForm1::Panel1DragOver(TObject *Sender, TObject *Source, int X,
	int Y, TDragState State, bool &Accept) {
	if (Source->InheritsFrom(__classid(TImage)))
		Accept = true;
}
// ---------------------------------------------------------------------------

void __fastcall TForm1::Panel1DragDrop(TObject *Sender, TObject *Source,
	int X, int Y)

{
	if (Source->InheritsFrom(__classid(TImage))) {
		TImage* t = (TImage*)Source;
		int sourceidx = (t->Top - 2) / SQSIZE * 4 + (t->Left - 2) / SQSIZE;
		int destidx = (Y - 2) / SQSIZE * 4 + (X - 2) / SQSIZE;
		puz[destidx] = puz[sourceidx];
		puz[sourceidx] = 0;
		showPuzzle();
	}
}
// ---------------------------------------------------------------------------

void __fastcall TForm1::Image1DragDrop(TObject *Sender, TObject *Source,
	int X, int Y)

{
	if (Source->InheritsFrom(__classid(TImage))) {
		TImage* tsrc = (TImage*)Source;
		TImage* tdst = (TImage*)Sender;
		if (tsrc == tdst)
			return;
		int sourceidx = (tsrc->Top - 2) / SQSIZE * 4 + (tsrc->Left - 2)
			/ SQSIZE;
		int destidx = (tdst->Top - 2) / SQSIZE * 4 + (tdst->Left - 2) / SQSIZE;
		swap(puz[destidx], puz[sourceidx]);
		showPuzzle();
	}
}
// ---------------------------------------------------------------------------

void __fastcall TForm1::ApplySolutionClick(TObject *Sender) {
	int idx;
	for (int i = lastSolution[84] - 1; i >= 0; i--) {

		while (Timer1->Enabled == true) {
			Sleep(20);
			Application->ProcessMessages();
		};
		for (int j = 0; j < 16; j++) {
			if (puz[j] == lastSolution[i]) {
				idx = j;
				break;
			}
		}
		if (idx < 15 && puz[idx + 1] == 0 && (idx + 1) % 4 != 0) {
			ti = img[puz[idx]];
			dir = 1;
			puz[idx + 1] = puz[idx];
			puz[idx] = 0;
			Timer1->Enabled = True;
		}
		else if (idx > 0 && puz[idx - 1] == 0 && idx % 4 != 0) {
			ti = img[puz[idx]];
			dir = 0;
			puz[idx - 1] = puz[idx];
			puz[idx] = 0;
			Timer1->Enabled = True;
		}
		else if (idx - 4 >= 0 && puz[idx - 4] == 0) {
			ti = img[puz[idx]];
			dir = 2;
			puz[idx - 4] = puz[idx];
			puz[idx] = 0;
			Timer1->Enabled = True;
		}
		else if (idx + 4 < 16 && puz[idx + 4] == 0) {
			ti = img[puz[idx]];
			dir = 3;
			puz[idx + 4] = puz[idx];
			puz[idx] = 0;
			Timer1->Enabled = True;
		}

	}
}
// ---------------------------------------------------------------------------}

void __fastcall TForm1::HeuristicClick(TObject *Sender) {
	TRadioGroup* s = (TRadioGroup*)Sender;
	switch (s->ItemIndex) {
	case WD555:
		if (prun15[0] != 0)
			break; // tables loaded
		if (Application->MessageBox
			(L"The first time you use this heuristcs "
			L"some tables of about 1.5 MB will be created and saved " L"to your hard disk. This will take about 1 minute.\n"
			L"Continue?", L"", MB_YESNO | MB_ICONQUESTION) == idYes) {
			buildprun_(prun15, 1, 5, false);
			buildprun_(prun610, 6, 10, false);
			buildprun_(prun1115, 11, 15, false);
		}
		else
			s->ItemIndex = 1;
		dynamic_cast<TRadioButton*>(s->Controls[1])->SetFocus();
		break;
	case WD663:
		if (prun16[0] != 0)
			break; // tables loaded
		if (Application->MessageBox
			(L"The first time you use this heuristcs "
			L"some tables of about 12 MB will be created and saved " L"to your hard disk. This will take a few minutes.\n"
			L"Continue?", L"", MB_YESNO | MB_ICONQUESTION) == idYes) {
			buildprun_(prun16, 1, 6, false);
			buildprun_(prun79, 7, 9, false);
			buildprun_(prun1015, 10, 15, false);
		}
		else
			s->ItemIndex = 1;
		dynamic_cast<TRadioButton*>(s->Controls[1])->SetFocus();
		break;
	case ONLY78:
		if (prun815 == NULL) {
			Application->MessageBox
				(L"You do not have enough memory to use this heuristics.\n"
				L"You must have at least Windows 64 bit with 6 GB of RAM.", L"",
				MB_OK | MB_ICONERROR);
			s->ItemIndex = 1;
			dynamic_cast<TRadioButton*>(s->Controls[1])->SetFocus();
			break;
		}
		else if (prun815[0] == 0 || prun17[0] == 0) {
			if (Application->MessageBox
				(L"The first time you use this heuristcs "
				L"some tables of about 570 MB will be created and saved " L"to your hard disk. This will take several hours!\n"
				L"With this heuristics, random puzzles will be solved " L"within less than 0.005s on average.\n"
				L"Continue?", L"", MB_YESNO | MB_ICONQUESTION) == idYes) {
				int ret1, ret2;
				if (prun815[0] == 0)
					ret2 = buildprun_32(prun815, 8, 15, false);
				if (prun17[0] == 0)
					ret1 = buildprun_32(prun17, 1, 7, false);
				if (ret1 == -1 || ret2 == -1) {
					delete[]prun17;
					delete[]prun815;
					prun815 = NULL;
					prun17 = NULL;
					s->ItemIndex = 1;
					dynamic_cast<TRadioButton*>(s->Controls[1])->SetFocus();
					break;
				}
			}
			else {
				s->ItemIndex = 1;
				dynamic_cast<TRadioButton*>(s->Controls[1])->SetFocus();
				break;
			}
		}
	default: ;
	}
}

// ---------------------------------------------------------------------------
void __fastcall TForm1::SolNumClick(TObject *Sender) {
	TRadioGroup* s = (TRadioGroup*)Sender;
	if (s->ItemIndex == 0)
		ApplySolution->Enabled = true;
	else
		ApplySolution->Enabled = false;
}
// ---------------------------------------------------------------------------

int TForm1::manhattanDistance() {

	int s = 0;
	switch (Type->ItemIndex) {

	case 0: // Zero-based
		for (int i = 0; i < 16; i++)
			if (puz[i] != 0)
				s += abs(i / 4 - puz[i] / 4) + abs(i % 4 - puz[i] % 4);
		break;
	case 1: // Zero -terminated
		for (int i = 0; i < 16; i++)
			if (puz[i] != 0)
				s += abs(i / 4 - (puz[i] - 1) / 4) +
					abs(i % 4 - (puz[i] - 1) % 4);
	}
	return s;
}

int TForm1::walkingDistance() {
	char transf[16], transf_r[16];
	switch (Type->ItemIndex) {

	case 0: // zerobased
		for (int i = 0; i < 16; i++)
			transf[i] = fromzerobased[puz[fromzerobased[i]]];
		break;
	case 1: // zero-terminated
		for (int i = 0; i < 16; i++)
			transf_r[i] = (16 - puz[15 - i]) % 16;
		for (int i = 0; i < 16; i++)
			transf[i] = fromzerobased[transf_r[fromzerobased[i]]];
		break;
	}
	reflect(transf, transf_r);
	return prunwd[wdindex(transf)] + prunwd[wdindex(transf_r)];
}

int TForm1::_78Distance() {
	char transf[16], transf_r[16];
	if (prun17 != NULL && prun815 != 0 && prun17[0] != 0 && prun815[0] != 0) {
		switch (Type->ItemIndex) {

		case 0: // zerobased
			for (int i = 0; i < 16; i++)
				transf[i] = fromzerobased[puz[fromzerobased[i]]];
			break;
		case 1: // zero-terminated
			for (int i = 0; i < 16; i++)
				transf_r[i] = (16 - puz[15 - i]) % 16;
			for (int i = 0; i < 16; i++)
				transf[i] = fromzerobased[transf_r[fromzerobased[i]]];
			break;
		}
		reflect(transf, transf_r);
		int d1 = prun17[index(transf, 1, 7)] + prun815[index(transf, 8, 15)];
		int d2 = prun17[index(transf_r, 1, 7)] +
			prun815[index(transf_r, 8, 15)];
		if (d1 > d2)
			return d1;
		else
			return d2;
	}
	else
		return -1;
}

int TForm1::_663Distance() {
	char transf[16], transf_r[16];
	if (prun16 != NULL && prun1015 != 0 && prun16[0] != 0 && prun1015[0] != 0) {
		switch (Type->ItemIndex) {

		case 0: // zerobased
			for (int i = 0; i < 16; i++)
				transf[i] = fromzerobased[puz[fromzerobased[i]]];
			break;
		case 1: // zero-terminated
			for (int i = 0; i < 16; i++)
				transf_r[i] = (16 - puz[15 - i]) % 16;
			for (int i = 0; i < 16; i++)
				transf[i] = fromzerobased[transf_r[fromzerobased[i]]];
			break;
		}
		reflect(transf, transf_r);
		int d1 = prun16[index(transf, 1, 6)] + prun79[index(transf, 7, 9)] +
			prun1015[index(transf, 10, 15)];
		int d2 = prun16[index(transf_r, 1, 6)] + prun79[index(transf_r, 7, 9)] +
			prun1015[index(transf_r, 10, 15)];
		if (d1 > d2)
			return d1;
		else
			return d2;
	}
	else
		return -1;
}

int TForm1::_555Distance() {
	char transf[16], transf_r[16];
	if (prun15 != NULL && prun610 != 0 && prun15[0] != 0 && prun610[0] != 0) {
		switch (Type->ItemIndex) {

		case 0: // zerobased
			for (int i = 0; i < 16; i++)
				transf[i] = fromzerobased[puz[fromzerobased[i]]];
			break;
		case 1: // zero-terminated
			for (int i = 0; i < 16; i++)
				transf_r[i] = (16 - puz[15 - i]) % 16;
			for (int i = 0; i < 16; i++)
				transf[i] = fromzerobased[transf_r[fromzerobased[i]]];
			break;
		}
		reflect(transf, transf_r);
		int d1 = prun15[index(transf, 1, 5)] + prun610[index(transf, 6, 10)] +
			prun1115[index(transf, 11, 15)];
		int d2 = prun15[index(transf_r, 1, 5)] + prun610[index(transf_r, 6, 10)]
			+ prun1115[index(transf_r, 11, 15)];
		if (d1 > d2)
			return d1;
		else
			return d2;
	}
	else
		return -1;
}

void __fastcall TForm1::FormCloseQuery(TObject *Sender, bool &CanClose) {
	if (MessageDlg("Do you really want to quit?", mtConfirmation,
		TMsgDlgButtons() << mbYes << mbNo, 0) == mrNo)
		CanClose = false;
	else
		exit(EXIT_SUCCESS); // sofortiger Abbruch
}
// ---------------------------------------------------------------------------

// Performance test. Button2 is not visible by default
// ---------------------------------------------------------------------------
void __fastcall TForm1::Button2Click(TObject *Sender) {
	__int64 f, t0, t1;
	__int64 totaltime = 0;
	char puz[16];

	QueryPerformanceFrequency((LARGE_INTEGER *)&f);
	for (int i = 0; i < 1000; i++) {
		randompuz(puz, (puztype)Type->ItemIndex);
		int l = IDASearch_(puz, (puztype)Type->ItemIndex,
			(heuristic)Heuristic->ItemIndex);
		totaltime += timeStop - timeStart;
	}
	totaltime = totaltime * 1000 / f;
	Memo->Lines->Add(IntToStr(totaltime));
}
// ---------------------------------------------------------------------------
