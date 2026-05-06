#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <map>
#include <iomanip>
#include <stdexcept>
#include <cstdlib>
#include <ctime>
#include <algorithm>

using namespace std;

// ============================================================
// --- EXCEPTII PERSONALIZATE ---
// ============================================================
class DataInvalidaException : public exception
{
  string mesaj;

public:
  DataInvalidaException(string m) : mesaj(m) {}
  const char *what() const noexcept override { return mesaj.c_str(); }
};
class EmailInvalidException : public exception
{
  string mesaj;

public:
  EmailInvalidException(string m) : mesaj(m) {}
  const char *what() const noexcept override { return mesaj.c_str(); }
};
class TelefonInvalidException : public exception
{
  string mesaj;

public:
  TelefonInvalidException(string m) : mesaj(m) {}
  const char *what() const noexcept override { return mesaj.c_str(); }
};

// ============================================================
// --- FUNCTII VALIDARE ---
// ============================================================
void valideazaEmail(const string &email)
{
  string sufix = "@gmail.com";
  if (email.size() < sufix.size() || email.substr(email.size() - sufix.size()) != sufix)
    throw EmailInvalidException("[!] Email invalid! Adresa trebuie sa se termine cu '@gmail.com'.");
}
void valideazaTelefon(const string &tel)
{
  if (tel.size() != 10)
    throw TelefonInvalidException("[!] Telefon invalid! Numarul trebuie sa aiba exact 10 cifre.");
  for (size_t i = 0; i < tel.size(); i++)
    if (!isdigit(tel[i]))
      throw TelefonInvalidException("[!] Telefon invalid! Numarul trebuie sa contina doar cifre.");
  if (tel.substr(0, 2) != "07")
    throw TelefonInvalidException("[!] Telefon invalid! Numarul trebuie sa inceapa cu '07'.");
}
void valideazaData(int luna, int zi)
{
  if (luna < 4 || luna > 5)
    throw DataInvalidaException("[!] Data invalida! Luna trebuie sa fie intre 4 (Aprilie) si 5 (Mai).");
  int zileLunii = (luna == 4) ? 30 : 31;
  if (zi < 1 || zi > zileLunii)
    throw DataInvalidaException("[!] Data invalida! Ziua " + to_string(zi) +
                                " nu exista in luna " + to_string(luna) + " (max " + to_string(zileLunii) + " zile).");
}

string genereazaCod()
{
  string cod = "#";
  for (int i = 0; i < 6; i++)
    cod += to_string(rand() % 10);
  return cod;
}

// Transforma string in lowercase
string toLower(const string &s)
{
  string r = s;
  for (size_t i = 0; i < r.size(); i++)
    r[i] = tolower(r[i]);
  return r;
}

// ============================================================
// --- STRUCTURI ---
// ============================================================
struct Doctor
{
  string nume, orar, zileLibere, telefon, specializare;
  int salariu;
};

struct Programare
{
  string numeP, prenumeP, emailP, telefonP, boala;
  string medicNume, medicTel, medicSpec;
  int luna, zi;
  string ora, cod;
};

// Structura pentru un pacient internat (urgenta rosie/portocalie)
struct Internat
{
  string numeP, prenumeP, telefonP;
  string boala, urgenta, medicNume, medicSpec;
  int camera, etaj;
  string cod;
};

// Structura pentru o factura emisa pacientilor
struct Factura
{
  string numeP, prenumeP, codFactura;
  double suma;
  bool achitata;
};

// ============================================================
// --- CLASE ---
// ============================================================
class Utilizator
{
public:
  string nume, prenume, email, parola, rol;
  Utilizator(string n, string p, string e, string pass, string r)
      : nume(n), prenume(p), email(e), parola(pass), rol(r) {}
};

class SpitalManager
{
private:
  map<string, vector<string>> bazaBoli;
  map<string, vector<Doctor>> bazaMedici;
  vector<Utilizator> staff;
  vector<Utilizator> conturiMedici;
  vector<Programare> programari;
  vector<Internat> internati;
  vector<Factura> facturi;

  // Camere ocupate: set de camere (1-300) deja alocate
  vector<int> camereOcupate;

  const string F_BOLI = "specializari.txt";
  const string F_MEDICI = "medici.txt";
  const string F_USERI = "utilizatori.txt";
  const string F_MEDICI_CNT = "medici_conturi.txt";
  const string F_PROG = "programari.txt";
  const string F_INTERNATI = "internati.txt";
  const string F_FACTURI = "facturi.txt";

  // ============================================================
  // HARTA SPITAL: specializare -> etaj + interval camere + salon
  // Etaj 1 (1-100):   Cardiologie (1-50),  Neurologie (51-100)
  // Etaj 2 (101-200): Urologie (101-150), Stomatologie (151-200)
  // Etaj 3 (201-300): Dermatologie (201-250), Rezerva (251-300)
  // ============================================================
  struct InfoEtaj
  {
    int etaj, cameraStart, cameraEnd;
    string salon;
  };

  InfoEtaj getInfoEtaj(const string &spec)
  {
    if (spec == "Cardiologie")
      return {1, 1, 50, "Salon Cardiologie  - Etaj 1, camere  1- 50  | Echipamente: EKG, Defibrilator, Monitor cardiac"};
    if (spec == "Neurologie")
      return {1, 51, 100, "Salon Neurologie   - Etaj 1, camere 51-100  | Echipamente: RMN, EEG, Tomograf"};
    if (spec == "Urologie")
      return {2, 101, 150, "Salon Urologie     - Etaj 2, camere 101-150 | Echipamente: Ecograf, Cistoscop, Masa operatorie"};
    if (spec == "Stomatologie")
      return {2, 151, 200, "Salon Stomatologie - Etaj 2, camere 151-200 | Echipamente: Fotoliu stomatologic, Rx dentar, Turbina"};
    if (spec == "Dermatologie")
      return {3, 201, 250, "Salon Dermatologie - Etaj 3, camere 201-250 | Echipamente: Dermatoscop, Laser, Fototerapie UV"};
    return {3, 251, 300, "Salon General      - Etaj 3, camere 251-300 | Echipamente: Paturi standard, Perfuzii"};
  }

  // Aloca prima camera libera in intervalul dat
  int alocaCamera(int start, int end)
  {
    for (int c = start; c <= end; c++)
    {
      bool ocupat = false;
      for (size_t i = 0; i < camereOcupate.size(); i++)
        if (camereOcupate[i] == c)
        {
          ocupat = true;
          break;
        }
      if (!ocupat)
      {
        camereOcupate.push_back(c);
        return c;
      }
    }
    return -1; // toate camerele ocupate
  }

  // ============================================================
  // TRIAJ AUTOMAT: parte corp -> specializare
  // ============================================================
  string detecteazaSpecializare(const string &parteCorp, const string &simptome)
  {
    string pc = toLower(parteCorp);
    string sm = toLower(simptome);
    string combined = pc + " " + sm;

    // Cardiologie
    if (combined.find("inima") != string::npos || combined.find("piept") != string::npos ||
        combined.find("cardiac") != string::npos || combined.find("tensiune") != string::npos ||
        combined.find("palpita") != string::npos || combined.find("angina") != string::npos ||
        combined.find("infarct") != string::npos || combined.find("aritmie") != string::npos)
      return "Cardiologie";

    // Neurologie
    if (combined.find("cap") != string::npos || combined.find("creier") != string::npos ||
        combined.find("minte") != string::npos || combined.find("neurolog") != string::npos ||
        combined.find("epilep") != string::npos || combined.find("avc") != string::npos ||
        combined.find("amorteal") != string::npos || combined.find("tremur") != string::npos ||
        combined.find("memorie") != string::npos || combined.find("parkinson") != string::npos ||
        combined.find("scleroza") != string::npos || combined.find("cap") != string::npos)
      return "Neurologie";

    // Urologie
    if (combined.find("rinichi") != string::npos || combined.find("vezica") != string::npos ||
        combined.find("urina") != string::npos || combined.find("prostata") != string::npos ||
        combined.find("urologica") != string::npos || combined.find("pietre") != string::npos ||
        combined.find("cislit") != string::npos || combined.find("abdomen inferior") != string::npos)
      return "Urologie";

    // Stomatologie
    if (combined.find("dinte") != string::npos || combined.find("gura") != string::npos ||
        combined.find("gingii") != string::npos || combined.find("masea") != string::npos ||
        combined.find("dinti") != string::npos || combined.find("stomatolog") != string::npos ||
        combined.find("maxilar") != string::npos || combined.find("abces dentar") != string::npos)
      return "Stomatologie";

    // Dermatologie
    if (combined.find("piele") != string::npos || combined.find("rash") != string::npos ||
        combined.find("mancarime") != string::npos || combined.find("ecze") != string::npos ||
        combined.find("acnee") != string::npos || combined.find("dermat") != string::npos ||
        combined.find("alunita") != string::npos || combined.find("psoriazis") != string::npos ||
        combined.find("eruptie") != string::npos || combined.find("prurit") != string::npos)
      return "Dermatologie";

    return ""; // necunoscut
  }

  // ============================================================
  // TRIAJ: simptome -> boala + urgenta
  // ============================================================
  struct RezultatTriaj
  {
    string boala, urgenta, culoare;
  };

  RezultatTriaj determinaUrgenta(const string &spec, const string &simptome)
  {
    string sm = toLower(simptome);
    RezultatTriaj r;

    // Boli critice (ROSU)
    if (sm.find("infarct") != string::npos || sm.find("atac de cord") != string::npos)
    {
      r.boala = "Infarct";
      r.urgenta = "COD ROSU";
      r.culoare = "[R]";
      return r;
    }
    if (sm.find("avc") != string::npos || sm.find("accident vascular") != string::npos)
    {
      r.boala = "AVC";
      r.urgenta = "COD ROSU";
      r.culoare = "[R]";
      return r;
    }
    if (sm.find("abces") != string::npos && spec == "Stomatologie")
    {
      r.boala = "Abces";
      r.urgenta = "COD ROSU";
      r.culoare = "[R]";
      return r;
    }
    if (sm.find("melanom") != string::npos || sm.find("cancer piele") != string::npos)
    {
      r.boala = "Melanom";
      r.urgenta = "COD ROSU";
      r.culoare = "[R]";
      return r;
    }

    // Boli urgente (PORTOCALIU)
    if (sm.find("fractura") != string::npos || sm.find("os rupt") != string::npos)
    {
      r.boala = "Fractura";
      r.urgenta = "COD PORTOCALIU";
      r.culoare = "[P]";
      return r;
    }
    if (sm.find("diabet") != string::npos || sm.find("glicemie") != string::npos)
    {
      r.boala = "Diabet";
      r.urgenta = "COD PORTOCALIU";
      r.culoare = "[P]";
      return r;
    }
    if (sm.find("psoriazis") != string::npos)
    {
      r.boala = "Psoriazis";
      r.urgenta = "COD PORTOCALIU";
      r.culoare = "[P]";
      return r;
    }
    if (sm.find("litiaza") != string::npos || sm.find("piatra la rinichi") != string::npos)
    {
      r.boala = "Litiaza";
      r.urgenta = "COD PORTOCALIU";
      r.culoare = "[P]";
      return r;
    }

    // Consultatie standard (VERDE) - pe baza de specializare
    r.urgenta = "COD VERDE";
    r.culoare = "[V]";
    if (spec == "Cardiologie")
    {
      if (sm.find("hipertensiune") != string::npos || sm.find("tensiune mare") != string::npos)
        r.boala = "Hipertensiune";
      else if (sm.find("aritmie") != string::npos)
        r.boala = "Aritmie";
      else if (sm.find("angina") != string::npos)
        r.boala = "Angina";
      else
        r.boala = "Hipertensiune";
    }
    else if (spec == "Neurologie")
    {
      if (sm.find("epilepsie") != string::npos)
        r.boala = "Epilepsie";
      else if (sm.find("parkinson") != string::npos)
        r.boala = "Parkinson";
      else if (sm.find("alzheimer") != string::npos)
        r.boala = "Alzheimer";
      else
        r.boala = "Epilepsie";
    }
    else if (spec == "Urologie")
    {
      if (sm.find("cistita") != string::npos)
        r.boala = "Cistita";
      else if (sm.find("prostatita") != string::npos)
        r.boala = "Prostatita";
      else
        r.boala = "Cistita";
    }
    else if (spec == "Stomatologie")
    {
      if (sm.find("carie") != string::npos)
        r.boala = "Caria";
      else if (sm.find("gingivita") != string::npos)
        r.boala = "Gingivita";
      else if (sm.find("parodontoza") != string::npos)
        r.boala = "Parodontoza";
      else if (sm.find("pulpita") != string::npos)
        r.boala = "Pulpita";
      else
        r.boala = "Caria";
    }
    else if (spec == "Dermatologie")
    {
      if (sm.find("eczema") != string::npos)
        r.boala = "Eczema";
      else if (sm.find("acnee") != string::npos)
        r.boala = "Acnee";
      else if (sm.find("dermatita") != string::npos)
        r.boala = "Dermatita";
      else
        r.boala = "Acnee";
    }
    else
    {
      r.boala = "Consultatie generala";
    }
    return r;
  }

  // ============================================================
  bool esteZiBlocataGeneric(int zi)
  {
    return (zi % 3 == 0 || zi == 7 || zi == 14 || zi == 22);
  }

  void trimiteEmailConfirmare(const Programare &p)
  {
    ofstream mailFile("MAIL_SERVER.txt");
    mailFile << "==========================================" << endl;
    mailFile << "DE LA: server-notificari@spital-central.ro" << endl;
    mailFile << "CATRE: " << p.emailP << " (GMAIL)" << endl;
    mailFile << "SUBIECT: Confirmare Programare Medicala - 2026" << endl;
    mailFile << "------------------------------------------" << endl;
    mailFile << "Stimate " << p.numeP << " " << p.prenumeP << "," << endl
             << endl;
    mailFile << "Programarea dumneavoastra a fost inregistrata cu succes." << endl;
    mailFile << "- Medic: " << p.medicNume << " (" << p.medicSpec << ")" << endl;
    mailFile << "- Telefon Medic: " << p.medicTel << endl;
    mailFile << "- Boala declarata: " << p.boala << endl;
    mailFile << "- Data: " << p.zi << "." << p.luna << ".2026 | Ora: " << p.ora << endl;
    mailFile << "- Cod anulare: " << p.cod << endl
             << endl;
    mailFile << "Va rugam sa aveti la dumneavoastra cardul de sanatate." << endl;
    mailFile << "==========================================" << endl;
    mailFile.close();
    cout << "\n[SMTP] Email trimis la " << p.emailP << endl;
  }

  void trimiteEmailFactura(const Factura &f, const string &email, const string &detalii)
  {
    ofstream mailFile("MAIL_SERVER.txt");
    mailFile << "==========================================" << endl;
    mailFile << "DE LA: facturare@spital-central.ro" << endl;
    mailFile << "CATRE: " << email << " (GMAIL)" << endl;
    mailFile << "SUBIECT: Factura Servicii Medicale" << endl;
    mailFile << "------------------------------------------" << endl;
    mailFile << "Stimate " << f.numeP << " " << f.prenumeP << "," << endl
             << endl;
    mailFile << "Va multumim pentru ca ati apelat la serviciile Spitalului Central." << endl;
    mailFile << "Detalii servicii prestate:" << endl;
    mailFile << " " << detalii << endl
             << endl;
    mailFile << "Total de plata: " << f.suma << " RON" << endl;
    mailFile << "Cod Factura pentru plata: " << f.codFactura << endl
             << endl;
    mailFile << "Va rugam sa achitati folosind meniul 'Plata Facturi'." << endl;
    mailFile << "==========================================" << endl;
    mailFile.close();
    cout << "\n[SMTP] Factura trimisa prin email la " << email << endl;
  }

  // ============================================================
  void incarcaDate()
  {
    bazaBoli.clear();
    ifstream fb(F_BOLI.c_str());
    string linie;
    while (getline(fb, linie))
    {
      size_t pos = linie.find(':');
      if (pos != string::npos)
      {
        string spec = linie.substr(0, pos);
        stringstream ss(linie.substr(pos + 1));
        string b;
        while (getline(ss, b, ','))
          bazaBoli[spec].push_back(b);
      }
    }
    fb.close();

    bazaMedici.clear();
    ifstream fm(F_MEDICI.c_str());
    while (getline(fm, linie))
    {
      if (linie.empty())
        continue;
      stringstream ss(linie);
      string s, n, o, zl, tel, salStr;
      getline(ss, s, ';');
      getline(ss, n, ';');
      getline(ss, o, ';');
      getline(ss, zl, ';');
      getline(ss, tel, ';');
      getline(ss, salStr, ';');
      int sal = salStr.empty() ? 0 : atoi(salStr.c_str());
      if (!s.empty())
        bazaMedici[s].push_back({n, o, zl, tel, s, sal});
    }
    fm.close();

    programari.clear();
    ifstream fp(F_PROG.c_str());
    while (getline(fp, linie))
    {
      if (linie.empty())
        continue;
      stringstream ss(linie);
      string np, pp, ep, tp, b, m, oraS, cod;
      int ln, zi;
      ss >> np >> pp >> ep >> tp >> b >> m >> ln >> zi >> oraS >> cod;
      if (!np.empty())
        programari.push_back({np, pp, ep, tp, b, m, "", "", ln, zi, oraS, cod});
    }
    fp.close();

    staff.clear();
    ifstream fu(F_USERI.c_str());
    string un, upr, ue, upass, ur;
    while (fu >> un >> upr >> ue >> upass >> ur)
      staff.push_back(Utilizator(un, upr, ue, upass, ur));
    fu.close();

    conturiMedici.clear();
    ifstream fc(F_MEDICI_CNT.c_str());
    string me, mpass, mnume;
    while (fc >> me >> mpass >> mnume)
      conturiMedici.push_back(Utilizator(mnume, "", me, mpass, "medic"));
    fc.close();

    // Incarca internati si reface lista camere ocupate
    internati.clear();
    camereOcupate.clear();
    ifstream fi(F_INTERNATI.c_str());
    while (getline(fi, linie))
    {
      if (linie.empty())
        continue;
      stringstream ss(linie);
      string np, pp, tp, b, urg, mn, ms, cod;
      int cam, et;
      ss >> np >> pp >> tp >> b >> urg >> mn >> ms >> cam >> et >> cod;
      if (!np.empty())
      {
        internati.push_back({np, pp, tp, b, urg, mn, ms, cam, et, cod});
        camereOcupate.push_back(cam);
      }
    }
    fi.close();

    facturi.clear();
    ifstream ff(F_FACTURI.c_str());
    while (getline(ff, linie))
    {
      if (linie.empty())
        continue;
      stringstream ss(linie);
      string np, pp, cod;
      double sum;
      int achitat;
      ss >> np >> pp >> cod >> sum >> achitat;
      if (!np.empty())
      {
        facturi.push_back({np, pp, cod, sum, (achitat == 1)});
      }
    }
    ff.close();
  }

  void salveazaFacturi()
  {
    ofstream f(F_FACTURI.c_str());
    for (size_t i = 0; i < facturi.size(); i++)
    {
      f << facturi[i].numeP << " " << facturi[i].prenumeP << " "
        << facturi[i].codFactura << " " << facturi[i].suma << " "
        << (facturi[i].achitata ? 1 : 0) << endl;
    }
    f.close();
  }

  void salveazaProgramari()
  {
    ofstream f(F_PROG.c_str());
    for (size_t i = 0; i < programari.size(); i++)
    {
      Programare &p = programari[i];
      f << p.numeP << " " << p.prenumeP << " " << p.emailP << " " << p.telefonP << " "
        << p.boala << " " << p.medicNume << " " << p.luna << " " << p.zi << " " << p.ora << " " << p.cod << endl;
    }
    f.close();
  }

  void salveazaInternati()
  {
    ofstream f(F_INTERNATI.c_str());
    for (size_t i = 0; i < internati.size(); i++)
    {
      Internat &n = internati[i];
      f << n.numeP << " " << n.prenumeP << " " << n.telefonP << " " << n.boala << " "
        << n.urgenta << " " << n.medicNume << " " << n.medicSpec << " "
        << n.camera << " " << n.etaj << " " << n.cod << endl;
    }
    f.close();
  }

  void salveazaMedici()
  {
    ofstream f(F_MEDICI.c_str());
    for (map<string, vector<Doctor>>::iterator it = bazaMedici.begin(); it != bazaMedici.end(); ++it)
      for (size_t i = 0; i < it->second.size(); i++)
      {
        Doctor &d = it->second[i];
        f << d.specializare << ";" << d.nume << ";" << d.orar << ";"
          << d.zileLibere << ";" << d.telefon << ";" << d.salariu << endl;
      }
    f.close();
  }

  bool esteOcupat(const string &m, int l, int z, const string &ora = "")
  {
    if (ora == "" && esteZiBlocataGeneric(z))
      return true;
    for (size_t i = 0; i < programari.size(); i++)
      if (programari[i].medicNume == m && programari[i].luna == l && programari[i].zi == z)
        if (ora == "" || programari[i].ora == ora)
          return true;
    return false;
  }

  void afiseazaCalendar(int luna, const string &numeL, int start, int zile, const string &m)
  {
    cout << "\n  --- CALENDAR " << numeL << " ---" << endl;
    cout << "  Sun  Mon  Tue  Wed  Thu  Fri  Sat" << endl;
    for (int i = 0; i < start; i++)
      cout << "     ";
    for (int zi = 1; zi <= zile; zi++)
    {
      if (esteOcupat(m, luna, zi))
        cout << " [XX]";
      else
        cout << setw(4) << zi << " ";
      if ((zi + start) % 7 == 0)
        cout << endl;
    }
    cout << endl;
  }

  // Afiseaza harta grafica a spitalului
  void afiseazaHartaSpital()
  {
    cout << "\n";
    cout << "  +----------------------------------------------------------+" << endl;
    cout << "  |           SPITAL CENTRAL - HARTA ETAJE                   |" << endl;
    cout << "  +----------------------------------------------------------+" << endl;
    cout << "  |  ETAJ 3  | Dermatologie (201-250) | Rezerva  (251-300)   |" << endl;
    cout << "  +----------------------------------------------------------+" << endl;
    cout << "  |  ETAJ 2  | Urologie     (101-150) | Stomato  (151-200)   |" << endl;
    cout << "  +----------------------------------------------------------+" << endl;
    cout << "  |  ETAJ 1  | Cardiologie  (  1- 50) | Neurolog (  51-100)  |" << endl;
    cout << "  +----------------------------------------------------------+" << endl;
    cout << "  |  PARTER  | RECEPTIE  | URGENTE  | FARMACIA | LABORATOR   |" << endl;
    cout << "  +----------------------------------------------------------+" << endl;
  }

  // ============================================================
public:
  SpitalManager()
  {
    srand((unsigned)time(0));
    incarcaDate();
  }

  // ============================================================
  // SOSIRE CU AMBULANTA
  // ============================================================
  void sosireCuAmbulanta()
  {
    cout << "\n+------------------------------------------+" << endl;
    cout << "|         URGENTA MAJORA - AMBULANTA       |" << endl;
    cout << "+------------------------------------------+" << endl;
    string nP, prP, asigurareStr;
    cout << "Nume pacient: ";
    cin >> nP;
    cout << "Prenume pacient: ";
    cin >> prP;
    cout << "Pacientul are asigurare de sanatate? (da/nu): ";
    cin >> asigurareStr;
    bool areAsigurare = (toLower(asigurareStr) == "da");

    cout << "\n[!] PACIENT PRELUAT DIRECT: COD ROSU" << endl;
    cout << "[!] TRIMITERE IMEDIATA IN SALA DE OPERATIE..." << endl;

    // Generare durata operatie intre 30 si 300 minute
    int durataMin = rand() % 271 + 30;
    cout << "\nOperatia este in curs... (Durata estimata: " << durataMin << " minute)" << endl;

    // Calcul cost operatie (ex: 100 RON pe minut)
    double costOperatie = durataMin * 100.0;
    if (areAsigurare)
    {
      costOperatie /= 2.0;
      cout << "[i] Cost redus cu 50% datorita asigurarii de sanatate." << endl;
    }

    // Probabilitati rezultat
    int sansa = rand() % 100;
    string starePacient;
    if (sansa < 2)
    {
      starePacient = "Din nefericire, pacientul a decedat pe masa de operatie.";
      cout << "\n[REZULTAT]: " << starePacient << endl;
    }
    else if (sansa < 51)
    {
      starePacient = "Operatie reusita! Pacientul este vindecat si poate fi externat.";
      cout << "\n[REZULTAT]: " << starePacient << endl;
    }
    else
    {
      starePacient = "Operatie reusita! Pacientul necesita recuperare si ramane internat.";
      cout << "\n[REZULTAT]: " << starePacient << endl;
      // Salvare ca internat la sectia de Terapie Intensiva / Rezerva
      string codInt = genereazaCod();
      int cam = alocaCamera(251, 300); // Rezerva
      Internat intern = {nP, prP, "-", "Recuperare post-operatorie", "ROSU", "Dr. Garda", "ATI", cam, 3, codInt};
      internati.push_back(intern);
      salveazaInternati();
    }

    // Generare Factura
    string codFact = genereazaCod();
    facturi.push_back({nP, prP, codFact, costOperatie, false});
    salveazaFacturi();

    cout << "\n[i] A fost emisa o factura in valoare de " << costOperatie << " RON." << endl;
    cout << "Cod factura: " << codFact << " (poate fi achitata din meniul principal)" << endl;
  }

  // ============================================================
  // PLATA FACTURI
  // ============================================================
  void plataFacturi()
  {
    cout << "\n+------------------------------------------+" << endl;
    cout << "|              PLATA FACTURI               |" << endl;
    cout << "+------------------------------------------+" << endl;
    string codF, card;
    cout << "Introduceti codul facturii (#XXXXXX): ";
    cin >> codF;

    int idx = -1;
    for (size_t i = 0; i < facturi.size(); i++)
    {
      if (facturi[i].codFactura == codF)
      {
        idx = i;
        break;
      }
    }

    if (idx == -1)
    {
      cout << "[!] Factura nu a fost gasita." << endl;
      return;
    }

    if (facturi[idx].achitata)
    {
      cout << "[i] Aceasta factura a fost deja achitata." << endl;
      return;
    }

    cout << "Factura gasita: " << facturi[idx].numeP << " " << facturi[idx].prenumeP << " | Suma: " << facturi[idx].suma << " RON" << endl;
    cout << "Introduceti numarul cardului bancar (16 cifre): ";
    cin >> card;

    if (card.length() != 16)
    {
      cout << "[!] Numar de card invalid. Trebuie sa aiba exact 16 cifre." << endl;
      return;
    }

    cout << "\nSe proceseaza plata..." << endl;
    facturi[idx].achitata = true;
    salveazaFacturi();
    cout << "[OK] Plata confirmata! Va multumim." << endl;
  }

  // ============================================================
  // MENIU PRINCIPAL PACIENT (prezentare la spital)
  // ============================================================
  void meniuPacient()
  {
    int optP;
    while (true)
    {
      cout << "\n+------------------------------+" << endl;
      cout << "|      MENIU PACIENT           |" << endl;
      cout << "+------------------------------+" << endl;
      cout << "| 1. Sosire cu Ambulanta       |" << endl;
      cout << "| 2. Prezentare la urgenta     |" << endl;
      cout << "| 3. Programare (consultatie)  |" << endl;
      cout << "| 4. Anulare programare        |" << endl;
      cout << "| 5. Plata Facturi             |" << endl;
      cout << "| 6. Harta spital              |" << endl;
      cout << "| 0. Inapoi                    |" << endl;
      cout << "+------------------------------+" << endl;
      cout << "Alegere: ";
      cin >> optP;

      if (optP == 0)
        return;
      if (optP == 1)
        sosireCuAmbulanta();
      if (optP == 2)
        prezentareLaUrgenta();
      if (optP == 3)
        programareNoua();
      if (optP == 4)
        anuleazaProgramare();
      if (optP == 5)
        plataFacturi();
      if (optP == 6)
        afiseazaHartaSpital();
    }
  }

  // ============================================================
  // PREZENTARE LA URGENTA (triaj automat)
  // ============================================================
  void prezentareLaUrgenta()
  {
    cout << "\n+------------------------------------------+" << endl;
    cout << "|     RECEPTIE URGENTE - TRIAJ INITIAL     |" << endl;
    cout << "+------------------------------------------+" << endl;

    string nP, prP, tlP;
    cout << "\nBuna ziua! Va rugam sa completati datele de mai jos." << endl;
    cout << "Nume: ";
    cin >> nP;
    cout << "Prenume: ";
    cin >> prP;

    while (true)
    {
      cout << "Telefon: ";
      cin >> tlP;
      try
      {
        valideazaTelefon(tlP);
        break;
      }
      catch (const TelefonInvalidException &e)
      {
        cout << e.what() << " Incercati din nou.\n";
      }
    }

    string emP;
    while (true)
    {
      cout << "Gmail: ";
      cin >> emP;
      try
      {
        valideazaEmail(emP);
        break;
      }
      catch (const EmailInvalidException &e)
      {
        cout << e.what() << " Incercati din nou.\n";
      }
    }

    // --- Triaj Avansat ---
    cin.ignore();
    cout << "\n--- TRIAJ AVANSAT ---" << endl;
    string constient, sangerare, simptomeAcute, parteCorp;
    int nivelDurere;

    cout << "1. Pacientul este constient? (da/nu): ";
    getline(cin, constient);
    cout << "2. Exista sangerare masiva sau probleme respiratorii severe? (da/nu): ";
    getline(cin, sangerare);
    cout << "3. Evaluati durerea pe o scara de la 1 la 10: ";
    cin >> nivelDurere;
    cin.ignore();
    cout << "4. Care parte a corpului este afectata? (ex: inima, cap, piele): ";
    getline(cin, parteCorp);
    cout << "5. Descrieti pe scurt simptomele (ex: febra, fractura, eruptie): ";
    getline(cin, simptomeAcute);

    // Detecteaza specializarea
    string spec = detecteazaSpecializare(parteCorp, simptomeAcute);
    if (spec.empty())
    {
      cout << "\n[!] Nu am putut identifica automat sectia. Alegeti manual:" << endl;
      vector<string> specs;
      for (map<string, vector<string>>::iterator it = bazaBoli.begin(); it != bazaBoli.end(); ++it)
        specs.push_back(it->first);
      for (size_t i = 0; i < specs.size(); i++)
        cout << "  " << i + 1 << ". " << specs[i] << endl;
      int optS;
      cin >> optS;
      spec = specs[optS - 1];
    }
    else
    {
      cout << "\n[OK] Sectie identificata automat: " << spec << endl;
    }

    // Determina urgenta si boala
    RezultatTriaj triaj = determinaUrgenta(spec, simptomeAcute);

    // Suprascrie urgenta pe baza intrebarilor
    if (toLower(constient) == "nu" || toLower(sangerare) == "da")
    {
      triaj.urgenta = "COD ROSU";
      triaj.culoare = "[R]";
    }
    else if (nivelDurere >= 8 || triaj.urgenta == "COD PORTOCALIU")
    {
      triaj.urgenta = "COD PORTOCALIU";
      triaj.culoare = "[P]";
    }
    else
    {
      triaj.urgenta = "COD VERDE";
      triaj.culoare = "[V]";
    }

    // Alege primul medic disponibil din sectia detectata
    Doctor *drGarda = nullptr;
    if (!bazaMedici[spec].empty())
      drGarda = &bazaMedici[spec][0];

    InfoEtaj infoEtaj = getInfoEtaj(spec);

    // ---- Afiseaza rezultatul triajului ----
    cout << "\n+--------------------------------------------------+" << endl;
    cout << "|              REZULTAT TRIAJ                      |" << endl;
    cout << "+--------------------------------------------------+" << endl;
    cout << "|  Pacient:     " << setw(35) << left << (nP + " " + prP) << "|" << endl;
    cout << "|  Sectie:      " << setw(35) << left << spec << "|" << endl;
    cout << "|  Boala:       " << setw(35) << left << triaj.boala << "|" << endl;
    // Am ajustat setw pentru a compensa lipsa simbolului special dacă e cazul
    cout << "|  " << triaj.culoare << " " << setw(46) << left << triaj.urgenta << "|" << endl;
    cout << "+--------------------------------------------------+" << endl;
    cout << "\n  >> " << infoEtaj.salon << endl;

    // ---- Logica pe culoare ----
    if (triaj.urgenta == "COD ROSU")
    {
      // Camera imediata
      int cam = alocaCamera(infoEtaj.cameraStart, infoEtaj.cameraEnd);
      string codInt = genereazaCod();

      cout << "\n  *** COD ROSU - INTERNAT IMEDIAT ***" << endl;
      if (cam != -1)
      {
        cout << "  Camera alocata: " << cam << "  (Etaj " << infoEtaj.etaj << ")" << endl;
      }
      else
      {
        cout << "  [!] Toate camerele sectiei sunt ocupate! Redirectionat la Rezerva." << endl;
        cam = alocaCamera(251, 300);
        cout << "  Camera alocata (rezerva): " << cam << "  (Etaj 3)" << endl;
      }
      if (drGarda)
        cout << "  Medic de garda: " << drGarda->nume << " | Tel: " << drGarda->telefon << endl;
      cout << "  Cod internare:  " << codInt << endl;

      // Salveaza internare
      string mnSpec = drGarda ? drGarda->specializare : spec;
      string mnNume = drGarda ? drGarda->nume : "Nedisponibil";
      string mnTel = drGarda ? drGarda->telefon : "-";
      Internat intern = {nP, prP, tlP, triaj.boala, "ROSU", mnNume, mnSpec, cam, infoEtaj.etaj, codInt};
      internati.push_back(intern);
      salveazaInternati();
      incarcaDate();

      cout << "\n  [OK] Pacientul a fost internat cu succes. Va rugam sa asteptati asistenta." << endl;
    }
    else if (triaj.urgenta == "COD PORTOCALIU")
    {
      // Camera + programare urgenta
      int cam = alocaCamera(infoEtaj.cameraStart, infoEtaj.cameraEnd);
      string codInt = genereazaCod();

      cout << "\n  *** COD PORTOCALIU - INTERNARE + PROGRAMARE URGENTA ***" << endl;
      if (cam != -1)
      {
        cout << "  Camera alocata: " << cam << "  (Etaj " << infoEtaj.etaj << ")" << endl;
      }
      else
      {
        cout << "  [!] Toate camerele sectiei sunt ocupate! Redirectionat la Rezerva." << endl;
        cam = alocaCamera(251, 300);
        cout << "  Camera alocata (rezerva): " << cam << "  (Etaj 3)" << endl;
      }
      if (drGarda)
        cout << "  Medic de garda: " << drGarda->nume << " | Tel: " << drGarda->telefon << endl;
      cout << "  Cod internare:  " << codInt << endl;

      string mnNume = drGarda ? drGarda->nume : "Nedisponibil";
      string mnSpec = drGarda ? drGarda->specializare : spec;
      Internat intern = {nP, prP, tlP, triaj.boala, "PORTOCALIU", mnNume, mnSpec, cam, infoEtaj.etaj, codInt};
      internati.push_back(intern);
      salveazaInternati();
      incarcaDate();

      cout << "\n  [OK] Camera rezervata. Va rugam sa asteptati consultatia de urgenta." << endl;
      cout << "  Puteti face si o programare formala (optional):" << endl;
      cout << "  0. Nu, multumesc  |  1. Da, fac si programare: ";
      int optProg;
      cin >> optProg;
      if (optProg == 1)
        programareNoua();
    }
    else
    {
      // COD VERDE: programare normala, fara camera
      cout << "\n  *** COD VERDE - CONSULTATIE STANDARD ***" << endl;
      cout << "  Nu este necesara internarea." << endl;

      // Facturare
      double costConsult = 150.0;
      string codFact = genereazaCod();
      facturi.push_back({nP, prP, codFact, costConsult, false});
      salveazaFacturi();
      trimiteEmailFactura(facturi.back(), emP, "Consultatie in cadrul departamentului de Urgente (COD VERDE)");

      cout << "\n  [i] S-a emis o factura de " << costConsult << " RON pentru consultatie." << endl;
      cout << "  Cod factura: " << codFact << endl;

      cout << "  Continuati cu o programare suplimentara? (1=Da / 0=Nu): ";
      int optProg;
      cin >> optProg;
      if (optProg == 1)
        programareNoua();
    }
  }

  // ============================================================
  // PROGRAMARE NOUA (consultatie planificata)
  // ============================================================
  void programareNoua()
  {
    vector<string> specs;
    for (map<string, vector<string>>::iterator it = bazaBoli.begin(); it != bazaBoli.end(); ++it)
      specs.push_back(it->first);

    cout << "\n=== SELECTATI SECTIA ===\n";
    for (size_t i = 0; i < specs.size(); i++)
      cout << i + 1 << ". " << specs[i] << endl;
    int optS;
    cin >> optS;
    string sA = specs[optS - 1];

    cout << "\n--- BOLI TRATATE ---\n";
    for (size_t j = 0; j < bazaBoli[sA].size(); j++)
      cout << j + 1 << ". " << bazaBoli[sA][j] << endl;
    int optB;
    cin >> optB;
    string bA = bazaBoli[sA][optB - 1];

    cout << "\n--- MEDICI DISPONIBILI (Nume | Telefon | Specializare) ---\n";
    for (size_t k = 0; k < bazaMedici[sA].size(); k++)
    {
      Doctor &dr = bazaMedici[sA][k];
      cout << k + 1 << ". " << dr.nume << " | Tel: " << dr.telefon << " | " << dr.specializare << endl;
    }
    int optM;
    cin >> optM;
    Doctor d = bazaMedici[sA][optM - 1];

    int lA, zA;
    while (true)
    {
      afiseazaCalendar(4, "Aprilie", 3, 30, d.nume);
      afiseazaCalendar(5, "Mai", 5, 31, d.nume);
      cout << "\nAlegeti Luna (4-5): ";
      cin >> lA;
      cout << "Alegeti Ziua Libera: ";
      cin >> zA;
      try
      {
        valideazaData(lA, zA);
        if (esteOcupat(d.nume, lA, zA))
          cout << "\n[!] ZI OCUPATA [XX]. Alegeti alta data!\n";
        else
          break;
      }
      catch (const DataInvalidaException &e)
      {
        cout << e.what() << " Va rugam introduceti o data valida.\n";
      }
    }

    cout << "\n--- SLOTURI ORARE LIBERE (30 MIN) ---\n";
    string ore[] = {"08:00", "08:30", "09:00", "09:30", "10:00", "10:30",
                    "11:00", "11:30", "12:00", "12:30", "13:00"};
    vector<string> libere;
    for (int i = 0; i < 11; i++)
      if (!esteOcupat(d.nume, lA, zA, ore[i]))
      {
        libere.push_back(ore[i]);
        cout << libere.size() << ". " << ore[i] << endl;
      }
    int oOpt;
    cout << "Selectati slotul: ";
    cin >> oOpt;
    string oraFinala = libere[oOpt - 1];

    string nP, prP, emP, tlP;
    cout << "Nume: ";
    cin >> nP;
    cout << "Prenume: ";
    cin >> prP;

    while (true)
    {
      cout << "Gmail: ";
      cin >> emP;
      try
      {
        valideazaEmail(emP);
        break;
      }
      catch (const EmailInvalidException &e)
      {
        cout << e.what() << " Incercati din nou.\n";
      }
    }
    while (true)
    {
      cout << "Tel: ";
      cin >> tlP;
      try
      {
        valideazaTelefon(tlP);
        break;
      }
      catch (const TelefonInvalidException &e)
      {
        cout << e.what() << " Incercati din nou.\n";
      }
    }

    string codAnulare;
    bool unic = false;
    while (!unic)
    {
      codAnulare = genereazaCod();
      unic = true;
      for (size_t i = 0; i < programari.size(); i++)
        if (programari[i].cod == codAnulare)
        {
          unic = false;
          break;
        }
    }

    Programare p = {nP, prP, emP, tlP, bA, d.nume, d.telefon, d.specializare, lA, zA, oraFinala, codAnulare};
    programari.push_back(p);
    salveazaProgramari();
    trimiteEmailConfirmare(p);
    incarcaDate();

    // Afiseaza si info salon
    InfoEtaj info = getInfoEtaj(d.specializare);
    cout << "\n[OK] Programare finalizata cu succes!" << endl;
    cout << ">>> Cod anulare: " << codAnulare << " (retine-l!)" << endl;
    cout << ">>> " << info.salon << endl;
  }

  // ============================================================
  // ANULARE PROGRAMARE
  // ============================================================
  void anuleazaProgramare()
  {
    string nP, prP, cod;
    cout << "\n=== ANULARE PROGRAMARE ===" << endl;
    cout << "Nume: ";
    cin >> nP;
    cout << "Prenume: ";
    cin >> prP;
    cout << "Cod (#XXXXXX): ";
    cin >> cod;

    for (size_t i = 0; i < programari.size(); i++)
    {
      if (programari[i].numeP == nP && programari[i].prenumeP == prP && programari[i].cod == cod)
      {
        cout << "\n[OK] Programare gasita:" << endl;
        cout << "  Medic: " << programari[i].medicNume << endl;
        cout << "  Data:  " << programari[i].zi << "." << programari[i].luna << ".2026" << endl;
        cout << "  Ora:   " << programari[i].ora << endl;
        programari.erase(programari.begin() + i);
        salveazaProgramari();
        incarcaDate();
        cout << "\n[OK] Programarea a fost anulata cu succes!" << endl;
        return;
      }
    }
    cout << "\n[!] Nu s-a gasit nicio programare cu datele introduse." << endl;
  }

  // ============================================================
  // MENIU MEDIC
  // ============================================================
  void meniuMedic()
  {
    string email, parola;
    cout << "\n=== LOGIN MEDIC ===" << endl;
    cout << "Email: ";
    cin >> email;
    cout << "Parola: ";
    cin >> parola;

    string numeDoctor = "";
    for (size_t i = 0; i < conturiMedici.size(); i++)
      if (conturiMedici[i].email == email && conturiMedici[i].parola == parola)
      {
        numeDoctor = conturiMedici[i].nume;
        break;
      }

    if (numeDoctor.empty())
    {
      cout << "\n[!] Email sau parola gresita!" << endl;
      return;
    }

    Doctor *dr = nullptr;
    for (map<string, vector<Doctor>>::iterator it = bazaMedici.begin(); it != bazaMedici.end(); ++it)
      for (size_t i = 0; i < it->second.size(); i++)
        if (it->second[i].nume == numeDoctor)
        {
          dr = &it->second[i];
          break;
        }

    if (!dr)
    {
      cout << "\n[!] Medicul nu a fost gasit in baza de date." << endl;
      return;
    }

    cout << "\n=== Bun venit, " << dr->nume << " ===" << endl;

    int optM;
    while (true)
    {
      cout << "\n--- MENIU MEDIC ---" << endl;
      cout << "1. Programarile mele" << endl;
      cout << "2. Pacienti internati in sectia mea" << endl;
      cout << "3. Informatii personale" << endl;
      cout << "4. Harta spital" << endl;
      cout << "0. Inapoi" << endl;
      cout << "Alegere: ";
      cin >> optM;
      if (optM == 0)
        break;

      if (optM == 1)
      {
        cout << "\n=== PROGRAMARILE MELE ===" << endl;
        bool gasit = false;
        for (size_t i = 0; i < programari.size(); i++)
          if (programari[i].medicNume == dr->nume)
          {
            cout << "  Pacient: " << programari[i].numeP << " " << programari[i].prenumeP
                 << " | Data: " << programari[i].zi << "." << programari[i].luna
                 << ".2026 | Ora: " << programari[i].ora
                 << " | Boala: " << programari[i].boala << endl;
            gasit = true;
          }
        if (!gasit)
          cout << "  Nu aveti programari inregistrate." << endl;
      }
      else if (optM == 2)
      {
        cout << "\n=== PACIENTI INTERNATI - SECTIA " << dr->specializare << " ===" << endl;
        bool gasit = false;
        for (size_t i = 0; i < internati.size(); i++)
          if (internati[i].medicSpec == dr->specializare)
          {
            cout << "  [" << internati[i].urgenta << "] "
                 << internati[i].numeP << " " << internati[i].prenumeP
                 << " | Camera: " << internati[i].camera
                 << " | Boala: " << internati[i].boala
                 << " | Cod: " << internati[i].cod << endl;
            gasit = true;
          }
        if (!gasit)
          cout << "  Nu sunt pacienti internati in sectia dumneavoastra." << endl;
      }
      else if (optM == 3)
      {
        InfoEtaj info = getInfoEtaj(dr->specializare);
        cout << "\n=== INFORMATII PERSONALE ===" << endl;
        cout << "  Nume:         " << dr->nume << endl;
        cout << "  Specializare: " << dr->specializare << endl;
        cout << "  Orar:         " << dr->orar << endl;
        cout << "  Zi libera:    " << dr->zileLibere << endl;
        cout << "  Telefon:      " << dr->telefon << endl;
        cout << "  Salariu:      " << dr->salariu << " RON" << endl;
        cout << "  Locatie:      " << info.salon << endl;
      }
      else if (optM == 4)
      {
        afiseazaHartaSpital();
      }
    }
  }

  // ============================================================
  // MENIU ADMIN
  // ============================================================
  void meniuAdmin()
  {
    string p;
    cout << "\nParola Admin: ";
    cin >> p;
    if (p != "1q2w3e")
    {
      cout << "\n[!] Parola gresita!" << endl;
      return;
    }

    int optA;
    while (true)
    {
      cout << "\n--- MENIU ADMIN ---" << endl;
      cout << "1. Vezi toate programarile" << endl;
      cout << "2. Vezi toti internati" << endl;
      cout << "3. Adauga cont medic (signup)" << endl;
      cout << "4. Modifica salariu medic" << endl;
      cout << "5. Modifica orar medic" << endl;
      cout << "6. Harta spital" << endl;
      cout << "0. Inapoi" << endl;
      cout << "Alegere: ";
      cin >> optA;
      if (optA == 0)
        break;

      if (optA == 1)
      {
        cout << "\n=== TOATE PROGRAMARILE ===" << endl;
        for (size_t i = 0; i < programari.size(); i++)
          cout << "  " << programari[i].numeP << " " << programari[i].prenumeP
               << " | Medic: " << programari[i].medicNume
               << " | " << programari[i].zi << "." << programari[i].luna
               << ".2026 | " << programari[i].ora
               << " | Cod: " << programari[i].cod << endl;
      }
      else if (optA == 2)
      {
        cout << "\n=== TOTI PACIENTII INTERNATI ===" << endl;
        for (size_t i = 0; i < internati.size(); i++)
          cout << "  [" << internati[i].urgenta << "] "
               << internati[i].numeP << " " << internati[i].prenumeP
               << " | Camera: " << internati[i].camera
               << " Etaj: " << internati[i].etaj
               << " | Sectie: " << internati[i].medicSpec
               << " | Medic: " << internati[i].medicNume
               << " | Boala: " << internati[i].boala
               << " | Cod: " << internati[i].cod << endl;
      }
      else if (optA == 3)
      {
        string numeM, emailM, parolaM;
        cout << "\n=== ADAUGARE CONT MEDIC ===" << endl;
        cout << "Numele medicului (ex: Dr.Vasilescu): ";
        cin >> numeM;
        while (true)
        {
          cout << "Gmail medic: ";
          cin >> emailM;
          try
          {
            valideazaEmail(emailM);
            break;
          }
          catch (const EmailInvalidException &e)
          {
            cout << e.what() << " Incercati din nou.\n";
          }
        }
        cout << "Parola: ";
        cin >> parolaM;
        ofstream fc(F_MEDICI_CNT.c_str(), ios::app);
        fc << emailM << " " << parolaM << " " << numeM << endl;
        fc.close();
        incarcaDate();
        cout << "[OK] Cont medic adaugat cu succes!" << endl;
      }
      else if (optA == 4)
      {
        string numeM;
        int salNou;
        cout << "\nNume medic: ";
        cin >> numeM;
        cout << "Salariu nou (RON): ";
        cin >> salNou;
        bool gasit = false;
        for (map<string, vector<Doctor>>::iterator it = bazaMedici.begin(); it != bazaMedici.end(); ++it)
          for (size_t i = 0; i < it->second.size(); i++)
            if (it->second[i].nume == numeM)
            {
              it->second[i].salariu = salNou;
              gasit = true;
            }
        if (gasit)
        {
          salveazaMedici();
          incarcaDate();
          cout << "[OK] Salariul actualizat!" << endl;
        }
        else
          cout << "[!] Medicul nu a fost gasit." << endl;
      }
      else if (optA == 5)
      {
        string numeM, orarNou;
        cout << "\nNume medic: ";
        cin >> numeM;
        cout << "Orar nou (ex: 08:00-16:00): ";
        cin >> orarNou;
        bool gasit = false;
        for (map<string, vector<Doctor>>::iterator it = bazaMedici.begin(); it != bazaMedici.end(); ++it)
          for (size_t i = 0; i < it->second.size(); i++)
            if (it->second[i].nume == numeM)
            {
              it->second[i].orar = orarNou;
              gasit = true;
            }
        if (gasit)
        {
          salveazaMedici();
          incarcaDate();
          cout << "[OK] Orarul actualizat!" << endl;
        }
        else
          cout << "[!] Medicul nu a fost gasit." << endl;
      }
      else if (optA == 6)
      {
        afiseazaHartaSpital();
      }
    }
  }
};

// ============================================================
// MAIN
// ============================================================
int main()
{
  SpitalManager manager;
  int opt;
  while (true)
  {
    cout << "\n+===========================================+" << endl;
    cout << "|         SPITAL CENTRAL - RECEPTIE         |" << endl;
    cout << "+===========================================+" << endl;
    cout << "|  1. PACIENT      2. PORTAL MEDIC          |" << endl;
    cout << "|  3. PORTAL ADMIN 0. IESIRE                |" << endl;
    cout << "+===========================================+" << endl;
    cout << "Alegere: ";
    if (!(cin >> opt))
    {
      cin.clear();
      cin.ignore(1000, '\n');
      continue;
    }
    if (opt == 0)
      break;
    switch (opt)
    {
    case 1:
      manager.meniuPacient();
      break;
    case 2:
      manager.meniuMedic();
      break;
    case 3:
      manager.meniuAdmin();
      break;
    }
  }
  return 0;
}
