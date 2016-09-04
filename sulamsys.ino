
#include <TaskScheduler.h>
#include <Wire.h>
#include <RTClib.h>
#include <LiquidCrystal.h>
#include <Bounce.h>

RTC_DS1307 rtc;

LiquidCrystal lcd(7, 6, 5, 4, 3, 2);

struct PNM {
	unsigned idpin : 3, pin : 4, autosu : 1,
		manual : 1, kapat : 1, ilk : 1, periyodik : 1,
		aktif : 1, pinstate : 1, ytarih : 1;
};
struct PBC {
	unsigned long int bekle, calis;
}mpbc[] = { {86400,2},{86400,600},{86400,300} };
unsigned int awake = 0;
unsigned int MenuGoster = 0;
unsigned long int msayac[] = {0,0,0};
//unsigned int mpnuma[] = {0x1d1c,0x2c00,0x3b04};
struct PNM mpnuma[] = { { 1,13,0,0,0,1,1,1,0,0 },{ 2,12,0,0,0,0,0,1,0,0 },{ 3,11,0,0,0,0,0,1,0,0 } };
DateTime mtarih[] = { DateTime(2016, 8, 22, 13, 1, 0), 
					  DateTime(2016, 8, 22, 14, 2, 0),
					  DateTime(2016, 8, 22, 15, 3, 0) };

Scheduler tsu;

DateTime TimeUpdate(void);

void CallBayrak();

void t0CalisCallback(void);
void t1CalisCallback(void);
void t2CalisCallback(void);

Task Sayac(1000, TASK_FOREVER, &CallBayrak, &tsu, true);
Task t0Bekle(30000, TASK_FOREVER, &t0CalisCallback, &tsu);
Task t1Bekle(30000, TASK_FOREVER, &t1CalisCallback, &tsu);
Task t2Bekle(30000, TASK_FOREVER, &t2CalisCallback, &tsu);

class Su
{
public:
	Su(struct PNM *_mpnuma) :pnuma(_mpnuma) {
		for (this->id = 0; this->id < arraysize(mpnuma) ; this->id++) {	this->setup(pnuma[id].idpin);}
		this->id = 0;
		this->pnuma[0] = { 1,13,0,0,0,0,1,1,0,0 };
		this->pnuma[1] = { 2,12,0,0,0,0,0,1,0,0 };
		this->pnuma[2] = { 3,11,0,0,0,0,0,1,0,0 };
	}
	template <typename T, unsigned S>
	inline unsigned arraysize(const T(&v)[S]) { return S; }

	struct PNM* gpnuma(void) { return this->pnuma; }
	unsigned int pin(void) { return this->pnuma[this->id].pin; }

	void service(unsigned int _id)
	{
		this->id = _id;
		this->state = !this->state;
		digitalWrite(this->pnuma[id].pin, this->state);
	}
	virtual void setup(unsigned int _id)
	{
		this->id = _id;
		pinMode(this->pnuma[id].pin, OUTPUT);
		this->state = false;
		digitalWrite(this->pnuma[id].pin, this->state);
	}
	virtual void cleanup(unsigned _id)
	{
		this->id = _id;
		this->state = false;
		pinMode(this->pnuma[id].pin, INPUT);
	}

	~Su() {};
protected:
	unsigned int id;
	bool state = false;
	struct PNM *pnuma;
};


class Humus:public Su
{
public:
	int mOkRead() { return this->mOk.read(); }
	bool mOkUpdate() { return this->mOk.update(); }
	long mOkDuration() { return this->mOk.duration(); }

	Humus(struct PNM *_pnuma,unsigned long int *_sayac, DateTime *_tarih,struct PBC *_pinBC) 
		:Su(_pnuma), sayac ( _sayac), tarih(_tarih),pinBC(_pinBC) {
		this->id = 0;
		this->pnuma[1] = { 2,12,0,0,0,0,0,0,0,0 };
		this->tarih[0] = DateTime(2016, 8, 22, 17, 0, 0);
		this->sayac[0] = 0;
	}
	~Humus() {};
	unsigned long int* gbayrak(void) { return this->sayac; }
	DateTime gtarih(unsigned int _id) { return (this->tarih[_id]); }
	unsigned int pinid(unsigned int _id) { return (this->pnuma[_id].pin); }
	void Bvana(void) {
		lcd.setCursor(0, 1);
		lcd.print("V: ");
		for (int i = 0; i < this->arraysize(mpnuma); i++) {
			lcd.setCursor(3 + i, 1);
			(this->pnuma[i].aktif) ? lcd.print(this->pnuma[i].idpin) : lcd.print("_");
		}
	}
	void Bsaat(void) {
		lcd.setCursor(0, 0);
		lcd.print(this->Dig2(TimeUpdate().hour()));
		lcd.print(".");
		lcd.print(this->Dig2(TimeUpdate().minute()));
		lcd.print("     ");
		lcd.print(this->Dig2(TimeUpdate().day()));
		lcd.print("/");
		lcd.print(this->cdays[TimeUpdate().dayOfWeek()]);
	}
	bool Bcals(void) { this->bpuls = !this->bpuls; return bpuls; }
	String Dig2(int _dd) {
		if (_dd >= 0 && _dd < 10) {
			return (String(_dd) + " ");
		}
		else { return String(_dd); }
	}
	void Auto() {
		if ((this->pnuma[this->id].aktif) && (this->pnuma[this->id].periyodik)) {//TekrE -1 ve Cilk -1 ise
			this->pnuma[this->id].manual = 0;
			this->pnuma[this->id].kapat = 0; 
			this->pnuma[this->id].autosu = 1;
		}
	}
	void Manual() { this->pnuma[this->id].autosu = 0; this->pnuma[this->id].manual = 1; }
	void Kapat() {(this->pnuma[this->id].manual) ? this->pnuma[this->id].kapat = 1 : this->pnuma[this->id].kapat = 0;}
	void CilkE() { this->pnuma[this->id].ilk = 1; }
	void CilkH() { this->pnuma[this->id].ilk = 0; }
	void TekrE() { this->pnuma[this->id].periyodik = 1; }
	void TekrH() { this->pnuma[this->id].periyodik = 0; }
	void Aktif() { this->pnuma[this->id].aktif = 1; }
	void Pasif() { this->pnuma[this->id].aktif = 0; }
	void DurmOn() { this->pnuma[this->id].pinstate = 1; }
	void DurmOf() { this->pnuma[this->id].pinstate = 0; }
	void GncTE() { this->pnuma[this->id].ytarih = 1; }
	void GncTH() { this->pnuma[this->id].ytarih = 0; }
	void GnceT(bool _tguncel) { (_tguncel) ? this->pnuma[this->id].ytarih = 1 : this->pnuma[this->id].ytarih = 0; }
	bool Akt_Pas() { return this->pnuma[this->id].aktif ; }
	//bool GncEH(void) { return this->pnuma[this->id] & 0x0001; }
	//bool GncEH(int _sayi) { return this->pnuma[_sayi >> 12] & 0x0001; }
	void Bekle(unsigned long _bekle) {
		this->pinBC[this->id].bekle = _bekle;
	}
	void Calis(unsigned long int _calis) {
		//this->calis = _calis;
		this->pinBC[this->id].calis = _calis;
		this->sayac[this->id] = _calis;
	}
	long Degistir(bool _sayi, String _msg) {
		String _mm = "";
		_mm = _msg;
		if (_sayi == false) { (this->EcHs) ? _mm = this->baslik[1] : _mm = this->baslik[0]; }
		long y = this->U0.ilkdeger;
		this->Ekran(_mm, 0);
		while (1) {
			lcd.setCursor(this->U0.sutun, this->U0.satir);
			//lcd.cursor();
			if (this->mBack.update()) {
				if (this->mBack.read() == HIGH && (y > this->U0.altlimit)) {
					y -= this->U0.artim;
					if (y <= 0) y = this->U0.altlimit;
					lcd.setCursor(this->U0.sutun, this->U0.satir);
					//lcd.cursor();
					if (_sayi) {
						lcd.print(this->Dig2(y));
					}
					else {
						TimeSpan ts2 = y;
						this->PEkran(ts2);
					}
				}
			}
			if (this->mNext.update()) {
				if (this->mNext.read() == HIGH && (y < this->U0.ustlimit)) {
					y += this->U0.artim;
					if (y >= this->U0.ustlimit) y = this->U0.ustlimit;
					lcd.setCursor(this->U0.sutun, this->U0.satir);
					//lcd.cursor();
					if (_sayi) {
						lcd.print(this->Dig2(y));
					}
					else {
						TimeSpan ts2 = y;
						this->PEkran(ts2);
					}
				}
			}
			if (this->mOk.update()) {
				if (this->mOk.read() == HIGH) {
					//lcd.noCursor();
					return y;
				}
			}
		}
	}
	int iDegistir(bool _secim,String _msg) {
		long y = this->iu0.ilkdeger;
		lcd.setCursor(this->iu0.sutun, this->iu0.satir);
		lcd.print(y+1);
		this->Ekran(_msg, 0);
		while (1) {
			lcd.setCursor(this->iu0.sutun, this->iu0.satir);
			//lcd.cursor();
			if (this->mBack.update()) {
				if (this->mBack.read() == HIGH && (y >= this->iu0.altlimit)) {
					y -= this->iu0.artim;
					if (y <= 0) y = this->iu0.altlimit;
					lcd.setCursor(this->iu0.sutun, this->iu0.satir);
					//lcd.cursor();
					(_secim)?lcd.print(y+1):lcd.print(this->aktifpasif[y]);
				}
			}
			if (this->mNext.update()) {
				if (this->mNext.read() == HIGH && (y <= this->iu0.ustlimit)) {
					y += this->iu0.artim;
					if (y >= this->iu0.ustlimit) y = this->iu0.ustlimit;
					lcd.setCursor(this->iu0.sutun, this->iu0.satir);
					//lcd.cursor();
					(_secim) ? lcd.print(y+1) : lcd.print(this->aktifpasif[y]);
				}
			}
			if (this->mOk.update()) {
				if (this->mOk.read() == HIGH) {
					//lcd.noCursor();
					return y;
				}
			}
		}
	}
	void PEkran(TimeSpan& ts) {
		String _un = "", _at = "";
		int uun, uaa;
		String df = "";
		if (this->EcHs) {
			_un = String(ts.hours());
			_at = String(ts.minutes());
			df = this->baslik[1];
		}
		else {
			_un = String(ts.days());
			_at = String(ts.hours());
			df = this->baslik[0];
		}
		uun = _un.length();
		uaa = _at.length();
		while (uun < 3) {
			_un = " " + _un;
			uun = _un.length();
		}
		while (uaa < 3) {
			_at = _at + " ";
			uaa = _at.length();
		}
		lcd.setCursor(0, 0);
		lcd.print(df);
		lcd.setCursor(3, 1);
		lcd.print((EcHs) ? "      " + _un + ":" + _at : "     " + _un + "/" + _at + " ");
	}
	void Ekran(String _Msg, int _satir) {
		int uzMsg = _Msg.length();
		lcd.setCursor((16 - uzMsg) / 2, _satir);
		lcd.print(_Msg);
	}
	void iTarih(void) {
		String AltMsg = String(this->id+1)+". Vana"+" Tarih  ";
		DateTime ts = 0;
		int ustlimitgun;
		this->Ekran(AltMsg, 0);
		this->TEkran(this->tarih[this->id], 6, 1);
		this->U0 = { this->tarih[this->id].year(), 2015, 2099, 1, 12, 1 };
		int tmyil = this->Degistir(true, AltMsg);
		this->U0 = { this->tarih[this->id].month(), 1, 12, 1, 9, 1 };
		int tmay = this->Degistir(true, AltMsg);
		this->tarih[this->id]=DateTime(tmyil, tmay, this->tarih[this->id].day(), this->tarih[this->id].hour(), this->tarih[this->id].minute(), 0);
		if ((this->tarih[this->id].year() % 4 == 0) && (this->tarih[this->id].month() == 2)) { ustlimitgun = 29; }
		else { ustlimitgun = this->cmdays[tmay - 1]; }
		this->U0 = { this->tarih[this->id].day(), 1, ustlimitgun, 1, 6, 1 };
		long tmgun = this->Degistir(true, AltMsg);
		ts = DateTime(tmyil, tmay, tmgun, this->tarih[this->id].hour(), this->tarih[this->id].minute(), 0);
		this->tarih[this->id] = ts;
	}
	void TEkran(DateTime idt, int i, int j) {
		int iyear = idt.year();
		int imonth = idt.month();
		int iday = idt.day();
		lcd.setCursor(i, j);
		lcd.print(this->Dig2(iday));
		lcd.print("/");
		lcd.print(this->Dig2(imonth));
		lcd.print("/");
		lcd.print(iyear);
	}
	void DtEkran(DateTime idt, bool gsaat, bool gtarih, int i, int j) {
		int iyear = idt.year();
		int imonth = idt.month();
		int iday = idt.day();
		int ihour = idt.hour();
		int iminute = idt.minute();
		int isecond = idt.second();
		int ihaftagun = idt.dayOfWeek();

		if (gsaat && gtarih) {
			lcd.setCursor(0, j);
			lcd.print(this->Dig2(ihour));
			lcd.print(".");
			lcd.print(this->Dig2(iminute));
			lcd.print("     ");
			lcd.print(this->Dig2(iday));
			lcd.print("/");
			lcd.print(this->cdays[ihaftagun]);
		}
		else {
			if (gtarih) {
				lcd.setCursor(i, j);
				lcd.print(this->Dig2(iday));
				lcd.print("|");
				lcd.print(this->cmonths[imonth - 1]);
				lcd.print("|");
				lcd.print(this->cdays[ihaftagun]);
			}
			if (gsaat) {
				lcd.setCursor(i, j);
				lcd.print(this->Dig2(ihour));
				lcd.print(":");
				lcd.print(this->Dig2(iminute));
			}
		}
	}
	void iSaat(void) {
		String AltMsg =String(this->id+1)+". "+"Baslama Saati";
		DateTime ts = 0;
		this->Ekran(AltMsg, 0);
		lcd.setCursor(11, 1);
		lcd.print("     ");
		this->DtEkran(this->Tarih(), true, false, 6, 1);
		this->U0 = { this->Tarih().hour(),0, 23, 1, 6, 1 };
		int bmsaat = this->Degistir(true, AltMsg);
		this->U0 = { this->Tarih().minute(), 0, 59, 1, 9, 1 };
		int bmdak = this->Degistir(true, AltMsg);
		ts = DateTime(this->Tarih().year(), this->Tarih().month(), this->Tarih().day(), bmsaat, bmdak, 0);
		this->tarih[this->id] = ts;
	}
	unsigned long int iBekCal(void) {
		unsigned long int _deger;
		lcd.setCursor(0, 0);
		this->Ekran((this->EcHs) ? this->baslik[1] : this->baslik[0], 0);
		DateTime d0 = DateTime(2000, 6, 15, 0, 0, 0);
		DateTime d1 = DateTime(d0.unixtime() + this->U0.ilkdeger);
		TimeSpan ts3 = d1 - d0;
		this->PEkran(ts3);
		unsigned long int k = this->Degistir(false, (this->EcHs) ? this->baslik[1] : this->baslik[0]);
		DateTime s1 = DateTime(d0.unixtime() + k);
		TimeSpan ts2 = s1 - d0;
		_deger = ts2.days() * 86400 + ts2.hours() * 3600 + ts2.minutes() * 60;
		return _deger;
	}
	DateTime Tarih(void) {
		return this->tarih[this->id];
	}
	void Tarih(DateTime _ts) {
		DateTime ts0 = this->tarih[this->id];
		this->tarih[this->id] = _ts;
		if (this->Akt_Pas()) {
			this->GncTE();
			this->TekrH();
			if (ts0.unixtime() == _ts.unixtime()) {
				this->pnuma[this->id].ytarih = 0;
				this->TekrE();
			}
			//else { this->Vana_Off(); }
		}
	}
	unsigned int Kurulum(void) {
		lcd.clear();
		String  _msg = "    Vana Sec    ";
		lcd.setCursor(0, 1);
		//lcd.print("V:");
		this->iu0 = { this->id,0,this->arraysize(mpnuma) - 1,1,15,0 };
		this->id = this->iDegistir(true, _msg);
		lcd.clear();
		_msg = String(this->id+1)+"  Vana Durum ";
		lcd.setCursor(0, 0);
		//lcd.print("V:");
		this->iu0 = { this->id,0,2,1,1,1 };
		unsigned int id0 = this->iDegistir(false,_msg);
		switch (id0) {
		case 0:
			this->pnuma[this->id].aktif = 1;
			return id0;
			break;
		case 1:
			this->pnuma[this->id].aktif = 0;
			return id0;
			break;
		default:
			return 2;
			break;
		}
		
	}
	
	void Secim() {
		if (this->Kurulum() == 0) {
			lcd.setCursor(0, 0);
			lcd.clear();
			this->iTarih();
			this->iSaat();
			this->EcHs = true;
			this->U0 = { this->pinBC[this->id].calis, 300, 86400, 300, 4, 1 };
			this->calis = this->iBekCal();
			this->EcHs = false;
			this->U0 = { this->pinBC[this->id].bekle, 3600, 2592000, 3600, 4, 1 };
			this->bekle = this->iBekCal();
			this->sayac[this->id] = this->calis;
			this->pinBC[this->id].calis = this->calis;
			this->pinBC[this->id].bekle = this->bekle;
			this->pnuma[this->id].periyodik = 1;
		}
		lcd.clear();
	}
	
protected:
	unsigned long int *sayac,calis,bekle;
	bool bpuls=false, EcHs = true;
	DateTime *tarih;
	struct PBC *pinBC;
	struct iBirim {
		long ilkdeger,
			altlimit,
			ustlimit,
			artim;
		unsigned int sutun, satir;
	} U0;
	struct iBir {
		int ilkdeger,
			altlimit,
			ustlimit,
			artim,
			sutun,
			satir;
	} iu0;

	Bounce mBack = Bounce(10, 200);
	Bounce mOk = Bounce(9, 200);
	Bounce mNext = Bounce(8, 200);
	char* aktifpasif[3] = { "    aktif","    pasif","    iptal" };
	char* baslik[3] = { "sonraki Gun/Saat" ,"Calisma Saat:Dak","Tarih gg/aa/yyyy" };
	const char* cdays[7] = { "Paz","Pzt", "Sal", "Car","Per","Cum", "Cmt" };
	const char* cmonths[12] = { "Oca", "Sub", "Mart", "Nis", "May", "Haz","Tem", "Agu","Eyl", "Eki", "Kas", "Ara" };
	const int cmdays[12] = { 31,28,31,30,31,30,31,31,30,31,30,31 };
};
/*	
	
*/

Humus tere(mpnuma, msayac, mtarih,mpbc );

DateTime TimeUpdate() {
	DateTime t0 = rtc.now();
	return t0;

} 

void t0CalisCallback() {
	tere.gpnuma()[0].ilk = 1;
}
void t1CalisCallback() {
	tere.gpnuma()[1].ilk = 1;
}
void t2CalisCallback() {
	tere.gpnuma()[2].ilk = 1;
}

void setup()
{
	Wire.begin();
	rtc.begin();
	lcd.begin(16, 2);
	awake = tere.mOkRead();
	lcd.clear();
	lcd.setCursor(0, 0);
	tere.gbayrak()[0] = 100;
}

void CallBayrak(void) {
	for (unsigned int i = 0; i < tere.arraysize(mpnuma); i++) {
		if ((tere.gpnuma()[i].ilk) && (tere.gpnuma()[i].periyodik) && (tere.gpnuma()[i].aktif)) {
			if (tere.gbayrak()[i] != 0) {
				tere.gbayrak()[i]--;   // = tere.gbayrak()[i] - 1;
				lcd.setCursor(3+i, 1);
				if (MenuGoster == 2) {
					(tere.Bcals()) ? lcd.print(tere.gpnuma()[i].idpin) : lcd.print(" ");
				}
				if ((tere.gpnuma()[i].pinstate) == 0) {
					tere.service(i);
					tere.gpnuma()[i].pinstate = 1;	
				}
			}
			else {
				tere.service(i);
				tere.gpnuma()[i].pinstate = 0;
				tere.gpnuma()[i].ilk = 0;
			}
		}
	}
}

/*
void CallBayrak() {
	tere.SecVa(tere.Vana()[0]);
	if (tere.GncEH()) {
		if (Bayrak[0] != 0) { Bayrak[0]--; tere.Vana_Off(); }
		else { t0Bekle.enable(); tere.GncTH(); tere.CilkE(); tere.Auto(); }
	}
	tere.SecVa(tere.Vana()[1]);
	if (tere.GncEH()) {
		if (Bayrak[1] != 0) { Bayrak[1]--; tere.Vana_Off(); }
		else { t1Bekle.enable(); tere.GncTH(); tere.CilkE(); tere.Auto(); }
	}
	tere.SecVa(tere.Vana()[2]);
	if (tere.GncEH()) {
		if (Bayrak[2] != 0) { Bayrak[2]--; tere.Vana_Off(); }
		else { t2Bekle.enable(); tere.GncTH(); tere.CilkE(); tere.Auto(); }
	}
}
*/

void loop()
{
	tsu.execute();
	tere.mOkUpdate();
	tere.Bsaat();
	lcd.setCursor(15, 1);
	lcd.print(tere.gbayrak()[0]);
	if (MenuGoster == 0) {
		tere.Bvana();
		MenuGoster = 2;
	}
	if (MenuGoster == 1) {
		lcd.clear();
		tere.Secim();
		MenuGoster = 0;
	}

	if (tere.mOkRead() != awake  && tere.mOkDuration() > 3000) {
		awake = tere.mOkRead();
		if (awake) {
			MenuGoster = 1;
			lcd.clear();
			while (awake) {
				//tere.Ekran(".....", 0);
				tere.mOkUpdate();
				awake = tere.mOkRead();
			}
		}
	}

}
