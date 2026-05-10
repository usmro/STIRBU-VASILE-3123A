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
#include "httplib.h"
#include "json.hpp"

using namespace std;
using json = nlohmann::json;

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
    if (!isdigit((unsigned char)tel[i]))
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

string toLower(const string &s)
{
  string r = s;
  for (size_t i = 0; i < r.size(); i++)
    r[i] = tolower((unsigned char)r[i]);
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

struct Internat
{
  string numeP, prenumeP, telefonP;
  // BUG FIX: boala si urgenta pot contine spatii – le stocam separat
  string boala, urgenta, medicNume, medicSpec;
  int camera, etaj;
  string cod;
};

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
public:
  map<string, vector<string>> bazaBoli;
  map<string, vector<Doctor>> bazaMedici;
  vector<Utilizator> staff;
  vector<Utilizator> conturiMedici;
  vector<Programare> programari;
  vector<Internat> internati;
  vector<Factura> facturi;
  vector<int> camereOcupate;

private:
  const string F_BOLI = "specializari.txt";
  const string F_MEDICI = "medici.txt";
  const string F_USERI = "utilizatori.txt";
  const string F_MEDICI_CNT = "medici_conturi.txt";
  const string F_PROG = "programari.txt";
  const string F_INTERNATI = "internati.txt";
  const string F_FACTURI = "facturi.txt";

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
    if (spec == "Ortopedie")
      return {3, 251, 300, "Salon Ortopedie    - Etaj 3, camere 251-300 | Echipamente: Rx ortopedic, Ghips, Masa ortopedica"};
    if (spec == "Oftalmologie")
      return {4, 301, 350, "Salon Oftalmologie - Etaj 4, camere 301-350 | Echipamente: Optometru, Laser retinian, Biomicroscop"};
    if (spec == "ORL")
      return {4, 351, 400, "Salon ORL          - Etaj 4, camere 351-400 | Echipamente: Audiometru, Endoscop nazal, Otoscop"};
    if (spec == "Gastroenterologie")
      return {5, 401, 450, "Salon Gastroent.   - Etaj 5, camere 401-450 | Echipamente: Endoscop, Colonoscop, Ecograf abdominal"};
    if (spec == "Oncologie")
      return {5, 451, 500, "Salon Oncologie    - Etaj 5, camere 451-500 | Echipamente: Chimioterapie, Radioterapie, PET-CT"};
    // Fallback – rezerva ATI
    return {3, 251, 300, "Salon General (Rezerva) - Etaj 3, camere 251-300 | Echipamente: Paturi standard, Perfuzii"};
  }

  // BUG FIX: nu mai adaugam in camereOcupate aici – se face la incarcare
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
    return -1;
  }

  // ============================================================
  // TRIAJ AUTOMAT
  // ============================================================
  string detecteazaSpecializare(const string &parteCorp, const string &simptome)
  {
    string combined = toLower(parteCorp) + " " + toLower(simptome);

    if (combined.find("inima") != string::npos || combined.find("piept") != string::npos ||
        combined.find("cardiac") != string::npos || combined.find("tensiune") != string::npos ||
        combined.find("palpita") != string::npos || combined.find("angina") != string::npos ||
        combined.find("infarct") != string::npos || combined.find("aritmie") != string::npos)
      return "Cardiologie";

    if (combined.find("creier") != string::npos || combined.find("neurolog") != string::npos ||
        combined.find("epilep") != string::npos || combined.find("avc") != string::npos ||
        combined.find("amorteal") != string::npos || combined.find("tremur") != string::npos ||
        combined.find("memorie") != string::npos || combined.find("parkinson") != string::npos ||
        combined.find("scleroza") != string::npos)
      return "Neurologie";

    if (combined.find("rinichi") != string::npos || combined.find("vezica") != string::npos ||
        combined.find("urina") != string::npos || combined.find("prostata") != string::npos ||
        combined.find("pietre") != string::npos || combined.find("cistit") != string::npos)
      return "Urologie";

    if (combined.find("dinte") != string::npos || combined.find("gura") != string::npos ||
        combined.find("gingii") != string::npos || combined.find("masea") != string::npos ||
        combined.find("dinti") != string::npos || combined.find("maxilar") != string::npos)
      return "Stomatologie";

    if (combined.find("piele") != string::npos || combined.find("rash") != string::npos ||
        combined.find("mancarime") != string::npos || combined.find("eczema") != string::npos ||
        combined.find("acnee") != string::npos || combined.find("psoriazis") != string::npos ||
        combined.find("eruptie") != string::npos || combined.find("alunita") != string::npos)
      return "Dermatologie";

    if (combined.find("os") != string::npos || combined.find("fractura") != string::npos ||
        combined.find("articulat") != string::npos || combined.find("genunchi") != string::npos ||
        combined.find("sold") != string::npos || combined.find("coloana") != string::npos)
      return "Ortopedie";

    if (combined.find("ochi") != string::npos || combined.find("vedere") != string::npos ||
        combined.find("retina") != string::npos || combined.find("glaucom") != string::npos)
      return "Oftalmologie";

    if (combined.find("gat") != string::npos || combined.find("ureche") != string::npos ||
        combined.find("nas") != string::npos || combined.find("sinuzita") != string::npos ||
        combined.find("amigdale") != string::npos)
      return "ORL";

    if (combined.find("stomac") != string::npos || combined.find("intestin") != string::npos ||
        combined.find("ficat") != string::npos || combined.find("colon") != string::npos ||
        combined.find("gastrita") != string::npos || combined.find("abdomen") != string::npos)
      return "Gastroenterologie";

    return "";
  }

  struct RezultatTriaj
  {
    string boala, urgenta, culoare;
  };

  RezultatTriaj determinaUrgenta(const string &spec, const string &simptome)
  {
    string sm = toLower(simptome);
    RezultatTriaj r;

    // COD ROSU
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

    // COD PORTOCALIU
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

    // COD VERDE – pe baza de specializare
    r.urgenta = "COD VERDE";
    r.culoare = "[V]";
    if (spec == "Cardiologie")
    {
      if (sm.find("hipertensiune") != string::npos)
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
  // BUG FIX: incarcaDate nu mai sterge camereOcupate daca e apelata
  // in mijlocul unei sesiuni cu camere deja alocate (fix major).
  // Solutia: separam incarcarea de date (din fisiere) de lista camerelor.
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

    // BUG FIX: programari.txt – campurile cu spatii (boala, medic)
    // Formatul: Nume Prenume email tel boala medic luna zi ora cod
    // Problema: boala poate fi "Recuperare post-operatorie" (spatii) → citim token cu token
    // Deci citim separat fiecare token; boala e un singur cuvant in fisier (fara spatii la scriere)
    programari.clear();
    ifstream fp(F_PROG.c_str());
    while (getline(fp, linie))
    {
      if (linie.empty())
        continue;
      stringstream ss(linie);
      string np, pp, ep, tp, b, m, oraS, cod;
      int ln = 4, zi = 1;
      string lnStr, ziStr;
      
      getline(ss, np, '\t');
      getline(ss, pp, '\t');
      getline(ss, ep, '\t');
      getline(ss, tp, '\t');
      getline(ss, b, '\t');
      getline(ss, m, '\t');
      getline(ss, lnStr, '\t');
      getline(ss, ziStr, '\t');
      getline(ss, oraS, '\t');
      getline(ss, cod);

      if (!np.empty()) {
        try {
          ln = stoi(lnStr);
          zi = stoi(ziStr);
        } catch (...) {}
        programari.push_back({np, pp, ep, tp, b, m, "", "", ln, zi, oraS, cod});
      }
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

    // BUG FIX MAJOR: internati.txt – campurile boala, urgenta, medicNume
    // pot contine spatii ("Recuperare post-operatorie", "COD ROSU", "Dr. Garda")
    // Solutia: salvam/incarcam cu separator TAB ('\t') in loc de spatiu
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
      // Folosim tab ca separator pentru campurile cu spatii
      getline(ss, np, '\t');
      getline(ss, pp, '\t');
      getline(ss, tp, '\t');
      getline(ss, b, '\t');
      getline(ss, urg, '\t');
      getline(ss, mn, '\t');
      getline(ss, ms, '\t');
      string camStr, etStr;
      getline(ss, camStr, '\t');
      getline(ss, etStr, '\t');
      getline(ss, cod);
      if (np.empty())
        continue;
      try
      {
        cam = stoi(camStr);
        et = stoi(etStr);
      }
      catch (...)
      {
        continue;
      }
      internati.push_back({np, pp, tp, b, urg, mn, ms, cam, et, cod});
      camereOcupate.push_back(cam);
    }
    fi.close();

    facturi.clear();
    ifstream ff(F_FACTURI.c_str());
    while (getline(ff, linie))
    {
      if (linie.empty())
        continue;
      stringstream ss(linie);
      string np, pp, cod, sumStr, achStr;
      double sum = 0;
      int achitat = 0;
      
      getline(ss, np, '\t');
      getline(ss, pp, '\t');
      getline(ss, cod, '\t');
      getline(ss, sumStr, '\t');
      getline(ss, achStr);

      if (!np.empty()) {
        try {
          sum = stod(sumStr);
          achitat = stoi(achStr);
        } catch (...) {}
        facturi.push_back({np, pp, cod, sum, (achitat == 1)});
      }
    }
    ff.close();
  }

  void salveazaFacturi()
  {
    ofstream f(F_FACTURI.c_str());
    for (size_t i = 0; i < facturi.size(); i++)
      f << facturi[i].numeP << '\t' << facturi[i].prenumeP << '\t'
        << facturi[i].codFactura << '\t' << facturi[i].suma << '\t'
        << (facturi[i].achitata ? 1 : 0) << endl;
    f.close();
  }

  void salveazaProgramari()
  {
    ofstream f(F_PROG.c_str());
    for (size_t i = 0; i < programari.size(); i++)
    {
      Programare &p = programari[i];
      f << p.numeP << '\t' << p.prenumeP << '\t' << p.emailP << '\t' << p.telefonP << '\t'
        << p.boala << '\t' << p.medicNume << '\t' << p.luna << '\t' << p.zi << '\t'
        << p.ora << '\t' << p.cod << endl;
    }
    f.close();
  }

  // BUG FIX: folosim TAB ca separator pentru a suporta spatii in campuri
  void salveazaInternati()
  {
    ofstream f(F_INTERNATI.c_str());
    for (size_t i = 0; i < internati.size(); i++)
    {
      Internat &n = internati[i];
      f << n.numeP << '\t' << n.prenumeP << '\t' << n.telefonP << '\t'
        << n.boala << '\t' << n.urgenta << '\t' << n.medicNume << '\t'
        << n.medicSpec << '\t' << n.camera << '\t' << n.etaj << '\t'
        << n.cod << endl;
    }
    f.close();
  }

  void salveazaMedici()
  {
    ofstream f(F_MEDICI.c_str());
    for (auto it = bazaMedici.begin(); it != bazaMedici.end(); ++it)
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
    if (ora.empty() && esteZiBlocataGeneric(z))
      return true;
    for (size_t i = 0; i < programari.size(); i++)
      if (programari[i].medicNume == m && programari[i].luna == l && programari[i].zi == z)
        if (ora.empty() || programari[i].ora == ora)
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

  void afiseazaHartaSpital()
  {
    cout << "\n";
    cout << "  +----------------------------------------------------------+" << endl;
    cout << "  |           SPITAL CENTRAL - HARTA ETAJE                   |" << endl;
    cout << "  +----------------------------------------------------------+" << endl;
    cout << "  |  ETAJ 5  | Gastroenterologie (401-450) | Oncologie (451-500) |" << endl;
    cout << "  +----------------------------------------------------------+" << endl;
    cout << "  |  ETAJ 4  | Oftalmologie (301-350) | ORL (351-400)        |" << endl;
    cout << "  +----------------------------------------------------------+" << endl;
    cout << "  |  ETAJ 3  | Dermatologie (201-250) | Ortopedie (251-300)  |" << endl;
    cout << "  +----------------------------------------------------------+" << endl;
    cout << "  |  ETAJ 2  | Urologie     (101-150) | Stomatologie (151-200) |" << endl;
    cout << "  +----------------------------------------------------------+" << endl;
    cout << "  |  ETAJ 1  | Cardiologie   ( 1-50)  | Neurologie (51-100)  |" << endl;
    cout << "  +----------------------------------------------------------+" << endl;
    cout << "  |  PARTER  | RECEPTIE  | URGENTE  | FARMACIA | LABORATOR   |" << endl;
    cout << "  +----------------------------------------------------------+" << endl;
  }

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

    int durataMin = rand() % 271 + 30;
    cout << "\nOperatia este in curs... (Durata estimata: " << durataMin << " minute)" << endl;

    double costOperatie = durataMin * 100.0;
    if (areAsigurare)
    {
      costOperatie /= 2.0;
      cout << "[i] Cost redus cu 50% datorita asigurarii de sanatate." << endl;
    }

    int sansa = rand() % 100;
    if (sansa < 2)
    {
      cout << "\n[REZULTAT]: Din nefericire, pacientul a decedat pe masa de operatie." << endl;
    }
    else if (sansa < 51)
    {
      cout << "\n[REZULTAT]: Operatie reusita! Pacientul este vindecat si poate fi externat." << endl;
    }
    else
    {
      cout << "\n[REZULTAT]: Operatie reusita! Pacientul necesita recuperare si ramane internat." << endl;
      string codInt = genereazaCod();
      // BUG FIX: alocaCamera deja actualizeaza camereOcupate, nu mai apelam incarcaDate()
      int cam = alocaCamera(251, 300);
      Internat intern = {nP, prP, "-", "Recuperare post-operatorie", "ROSU", "Dr. Garda", "ATI", cam, 3, codInt};
      internati.push_back(intern);
      salveazaInternati();
    }

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
    string codF;
    cout << "Introduceti codul facturii (#XXXXXX sau FACT-XXXXXX): ";
    cin >> codF;

    int idx = -1;
    for (size_t i = 0; i < facturi.size(); i++)
      if (facturi[i].codFactura == codF)
      {
        idx = (int)i;
        break;
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

    cout << "Factura: " << facturi[idx].numeP << " " << facturi[idx].prenumeP
         << " | Suma: " << facturi[idx].suma << " RON" << endl;
    cout << "Introduceti numarul cardului bancar (16 cifre): ";
    string card;
    cin >> card;

    // BUG FIX: card.length() e size_t (unsigned) → cast la int pentru comparatie
    if ((int)card.length() != 16)
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
  // MENIU PRINCIPAL PACIENT
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
      else if (optP == 1)
        sosireCuAmbulanta();
      else if (optP == 2)
        prezentareLaUrgenta();
      else if (optP == 3)
        programareNoua();
      else if (optP == 4)
        anuleazaProgramare();
      else if (optP == 5)
        plataFacturi();
      else if (optP == 6)
        afiseazaHartaSpital();
      else
        cout << "[!] Optiune invalida." << endl;
    }
  }

  // ============================================================
  // PREZENTARE LA URGENTA
  // ============================================================
  void prezentareLaUrgenta()
  {
    cout << "\n+------------------------------------------+" << endl;
    cout << "|     RECEPTIE URGENTE - TRIAJ INITIAL     |" << endl;
    cout << "+------------------------------------------+" << endl;

    string nP, prP, tlP;
    cout << "\nNume: ";
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

    cin.ignore();
    cout << "\n--- TRIAJ AVANSAT ---" << endl;
    string constient, sangerare, simptomeAcute, parteCorp;
    int nivelDurere;

    cout << "1. Pacientul este constient? (da/nu): ";
    getline(cin, constient);
    cout << "2. Exista sangerare masiva? (da/nu): ";
    getline(cin, sangerare);
    cout << "3. Nivel durere (1-10): ";
    cin >> nivelDurere;
    cin.ignore();
    cout << "4. Partea corpului afectata: ";
    getline(cin, parteCorp);
    cout << "5. Simptome principale: ";
    getline(cin, simptomeAcute);

    string spec = detecteazaSpecializare(parteCorp, simptomeAcute);
    if (spec.empty())
    {
      cout << "\n[!] Nu am putut identifica sectia automat. Alegeti manual:" << endl;
      vector<string> specs;
      for (auto it = bazaBoli.begin(); it != bazaBoli.end(); ++it)
        specs.push_back(it->first);
      for (size_t i = 0; i < specs.size(); i++)
        cout << "  " << i + 1 << ". " << specs[i] << endl;
      int optS;
      cin >> optS;
      if (optS >= 1 && optS <= (int)specs.size())
        spec = specs[optS - 1];
      else
        spec = "Cardiologie";
    }
    else
    {
      cout << "\n[OK] Sectie identificata: " << spec << endl;
    }

    RezultatTriaj triaj = determinaUrgenta(spec, simptomeAcute);

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

    Doctor *drGarda = nullptr;
    if (!bazaMedici[spec].empty())
      drGarda = &bazaMedici[spec][0];

    InfoEtaj infoEtaj = getInfoEtaj(spec);

    cout << "\n+--------------------------------------------------+" << endl;
    cout << "|              REZULTAT TRIAJ                      |" << endl;
    cout << "+--------------------------------------------------+" << endl;
    cout << "|  Pacient: " << setw(39) << left << (nP + " " + prP) << "|" << endl;
    cout << "|  Sectie:  " << setw(39) << left << spec << "|" << endl;
    cout << "|  Boala:   " << setw(39) << left << triaj.boala << "|" << endl;
    cout << "|  " << triaj.culoare << " " << setw(46) << left << triaj.urgenta << "|" << endl;
    cout << "+--------------------------------------------------+" << endl;
    cout << "\n  >> " << infoEtaj.salon << endl;

    string mnNume = drGarda ? drGarda->nume : "Dr. Garda";
    string mnSpec = drGarda ? drGarda->specializare : spec;

    if (triaj.urgenta == "COD ROSU")
    {
      int cam = alocaCamera(infoEtaj.cameraStart, infoEtaj.cameraEnd);
      string codInt = genereazaCod();
      cout << "\n  *** COD ROSU - INTERNAT IMEDIAT ***" << endl;
      if (cam == -1)
      {
        cam = alocaCamera(251, 300);
        cout << "  [!] Sectia plina! Redirectionat la Rezerva." << endl;
      }
      cout << "  Camera: " << cam << " (Etaj " << infoEtaj.etaj << ")" << endl;
      if (drGarda)
        cout << "  Medic garda: " << drGarda->nume << " | Tel: " << drGarda->telefon << endl;
      cout << "  Cod internare: " << codInt << endl;

      // BUG FIX: nu mai apelam incarcaDate() dupa salveazaInternati()
      // incarcaDate() sterge camereOcupate si reincarca din fisier, ceea ce e corect,
      // dar alocaCamera deja a adaugat camera in camereOcupate in memorie.
      // Apelam incarcaDate() DOAR la sfarsit pentru a sincroniza datele afisate.
      Internat intern = {nP, prP, tlP, triaj.boala, "ROSU", mnNume, mnSpec, cam, infoEtaj.etaj, codInt};
      internati.push_back(intern);
      salveazaInternati();
      cout << "\n  [OK] Pacient internat cu succes." << endl;
    }
    else if (triaj.urgenta == "COD PORTOCALIU")
    {
      int cam = alocaCamera(infoEtaj.cameraStart, infoEtaj.cameraEnd);
      string codInt = genereazaCod();
      cout << "\n  *** COD PORTOCALIU - INTERNARE URGENTA ***" << endl;
      if (cam == -1)
      {
        cam = alocaCamera(251, 300);
        cout << "  [!] Sectia plina! Redirectionat la Rezerva." << endl;
      }
      cout << "  Camera: " << cam << " (Etaj " << infoEtaj.etaj << ")" << endl;
      if (drGarda)
        cout << "  Medic garda: " << drGarda->nume << " | Tel: " << drGarda->telefon << endl;
      cout << "  Cod internare: " << codInt << endl;

      Internat intern = {nP, prP, tlP, triaj.boala, "PORTOCALIU", mnNume, mnSpec, cam, infoEtaj.etaj, codInt};
      internati.push_back(intern);
      salveazaInternati();
      cout << "\n  [OK] Camera rezervata." << endl;
      cout << "  Doriti si o programare formala? (1=Da / 0=Nu): ";
      int optProg;
      cin >> optProg;
      if (optProg == 1)
        programareNoua();
    }
    else
    {
      cout << "\n  *** COD VERDE - CONSULTATIE STANDARD ***" << endl;
      cout << "  Nu este necesara internarea." << endl;
      double costConsult = 150.0;
      string codFact = genereazaCod();
      facturi.push_back({nP, prP, codFact, costConsult, false});
      salveazaFacturi();
      trimiteEmailFactura(facturi.back(), emP, "Consultatie urgente (COD VERDE)");
      cout << "\n  [i] Factura: " << codFact << " | Suma: " << costConsult << " RON" << endl;
      cout << "  Continuati cu o programare? (1=Da / 0=Nu): ";
      int optProg;
      cin >> optProg;
      if (optProg == 1)
        programareNoua();
    }
  }

  // ============================================================
  // PROGRAMARE NOUA
  // ============================================================
  void programareNoua()
  {
    vector<string> specs;
    for (auto it = bazaBoli.begin(); it != bazaBoli.end(); ++it)
      specs.push_back(it->first);

    cout << "\n=== SELECTATI SECTIA ===\n";
    for (size_t i = 0; i < specs.size(); i++)
      cout << i + 1 << ". " << specs[i] << endl;
    int optS;
    cin >> optS;
    if (optS < 1 || optS > (int)specs.size())
    {
      cout << "[!] Optiune invalida.\n";
      return;
    }
    string sA = specs[optS - 1];

    cout << "\n--- BOLI TRATATE ---\n";
    for (size_t j = 0; j < bazaBoli[sA].size(); j++)
      cout << j + 1 << ". " << bazaBoli[sA][j] << endl;
    int optB;
    cin >> optB;
    if (optB < 1 || optB > (int)bazaBoli[sA].size())
    {
      cout << "[!] Optiune invalida.\n";
      return;
    }
    string bA = bazaBoli[sA][optB - 1];

    cout << "\n--- MEDICI DISPONIBILI ---\n";
    for (size_t k = 0; k < bazaMedici[sA].size(); k++)
    {
      Doctor &dr = bazaMedici[sA][k];
      cout << k + 1 << ". " << dr.nume << " | Tel: " << dr.telefon << " | " << dr.specializare << endl;
    }
    int optM;
    cin >> optM;
    if (optM < 1 || optM > (int)bazaMedici[sA].size())
    {
      cout << "[!] Optiune invalida.\n";
      return;
    }
    Doctor d = bazaMedici[sA][optM - 1];

    int lA, zA;
    while (true)
    {
      afiseazaCalendar(4, "Aprilie", 3, 30, d.nume);
      afiseazaCalendar(5, "Mai", 5, 31, d.nume);
      cout << "\nAlegeti Luna (4-5): ";
      cin >> lA;
      cout << "Alegeti Ziua: ";
      cin >> zA;
      try
      {
        valideazaData(lA, zA);
        if (esteOcupat(d.nume, lA, zA))
          cout << "\n[!] ZI OCUPATA. Alegeti alta data!\n";
        else
          break;
      }
      catch (const DataInvalidaException &e)
      {
        cout << e.what() << " Va rugam reincercati.\n";
      }
    }

    cout << "\n--- SLOTURI ORARE LIBERE ---\n";
    string ore[] = {"08:00", "08:30", "09:00", "09:30", "10:00", "10:30",
                    "11:00", "11:30", "12:00", "12:30", "13:00"};
    vector<string> libere;
    for (int i = 0; i < 11; i++)
      if (!esteOcupat(d.nume, lA, zA, ore[i]))
      {
        libere.push_back(ore[i]);
        cout << libere.size() << ". " << ore[i] << endl;
      }
    if (libere.empty())
    {
      cout << "[!] Nicio ora disponibila in ziua aleasa.\n";
      return;
    }
    int oOpt;
    cout << "Selectati slotul: ";
    cin >> oOpt;
    if (oOpt < 1 || oOpt > (int)libere.size())
    {
      cout << "[!] Optiune invalida.\n";
      return;
    }
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
        cout << e.what() << "\n";
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
        cout << e.what() << "\n";
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

    InfoEtaj info = getInfoEtaj(d.specializare);
    cout << "\n[OK] Programare finalizata!" << endl;
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

    string numeDoctor;
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
    for (auto it = bazaMedici.begin(); it != bazaMedici.end(); ++it)
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
      cout << "1. Toate programarile" << endl;
      cout << "2. Toti internati" << endl;
      cout << "3. Adauga cont medic" << endl;
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
            cout << e.what() << "\n";
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
        for (auto it = bazaMedici.begin(); it != bazaMedici.end(); ++it)
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
        for (auto it = bazaMedici.begin(); it != bazaMedici.end(); ++it)
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
// MAIN (Web Server)
// ============================================================
int main()
{
  SpitalManager manager;
  httplib::Server svr;

  svr.set_mount_point("/", "./");

  // GET /api/data – returneaza toate datele
  svr.Get("/api/data", [&](const httplib::Request &, httplib::Response &res)
          {
    json j;

    j["programari"] = json::array();
    for (const auto &p : manager.programari)
      j["programari"].push_back({
        {"nume",   p.numeP},   {"prenume", p.prenumeP},
        {"email",  p.emailP},  {"tel",     p.telefonP},
        {"boala",  p.boala},   {"medic",   p.medicNume},
        {"luna",   p.luna},    {"zi",      p.zi},
        {"ora",    p.ora},     {"cod",     p.cod},
        {"sectie", p.medicSpec}
      });

    j["internati"] = json::array();
    for (const auto &i : manager.internati)
      j["internati"].push_back({
        {"numeP",    i.numeP},    {"prenumeP",  i.prenumeP},
        {"boala",    i.boala},    {"urgenta",   i.urgenta},
        {"medicNume",i.medicNume},{"medicSpec", i.medicSpec},
        {"camera",   i.camera},   {"etaj",      i.etaj},
        {"cod",      i.cod}
      });

    j["facturi"] = json::array();
    for (const auto &f : manager.facturi)
      j["facturi"].push_back({
        {"numeP",      f.numeP},    {"prenumeP",   f.prenumeP},
        {"codFactura", f.codFactura},{"suma",       f.suma},
        {"achitata",   f.achitata}
      });

    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_content(j.dump(), "application/json"); });

  // POST /api/programare
  svr.Post("/api/programare", [&](const httplib::Request &req, httplib::Response &res)
           {
    try {
      auto body = json::parse(req.body);
      Programare p;
      p.numeP    = body["nume"].get<string>();
      p.prenumeP = body["prenume"].get<string>();
      p.emailP   = body["email"].get<string>();
      p.telefonP = body["tel"].get<string>();
      p.boala    = body["boala"].get<string>();
      p.medicNume= body["medic"].get<string>();
      p.luna     = body["luna"].get<int>();
      p.zi       = body["zi"].get<int>();
      p.ora      = body["ora"].get<string>();
      p.cod      = body["cod"].get<string>();
      p.medicSpec= body["sectie"].get<string>();
      // BUG FIX: cautam telefonul medicului din baza de date
      p.medicTel = "";
      for (auto &v : manager.bazaMedici[p.medicSpec])
        if (v.nume == p.medicNume) { p.medicTel = v.telefon; break; }

      manager.programari.push_back(p);
      manager.salveazaProgramari();

      // Adauga si factura
      if (body.contains("codFactura")) {
        Factura f;
        f.numeP      = p.numeP; f.prenumeP = p.prenumeP;
        f.codFactura = body["codFactura"].get<string>();
        f.suma       = 238.0;   f.achitata = false;
        manager.facturi.push_back(f);
        manager.salveazaFacturi();
      }

      res.set_header("Access-Control-Allow-Origin", "*");
      res.set_content("{\"status\":\"ok\"}", "application/json");
    } catch (const exception &e) {
      res.status = 400;
      res.set_content(string("{\"error\":\"") + e.what() + "\"}", "application/json");
    } });

  // POST /api/anulare
  svr.Post("/api/anulare", [&](const httplib::Request &req, httplib::Response &res)
           {
    try {
      auto body = json::parse(req.body);
      string n = body["nume"].get<string>();
      string p = body["prenume"].get<string>();
      string c = body["cod"].get<string>();
      bool found = false;
      for (size_t i = 0; i < manager.programari.size(); i++) {
        if (manager.programari[i].numeP == n &&
            manager.programari[i].prenumeP == p &&
            manager.programari[i].cod == c) {
          manager.programari.erase(manager.programari.begin() + i);
          manager.salveazaProgramari();
          found = true; break;
        }
      }
      res.set_header("Access-Control-Allow-Origin", "*");
      if (found) res.set_content("{\"status\":\"ok\"}", "application/json");
      else { res.status = 404; res.set_content("{\"error\":\"not found\"}", "application/json"); }
    } catch (const exception &e) {
      res.status = 400;
      res.set_content(string("{\"error\":\"") + e.what() + "\"}", "application/json");
    } });

  // POST /api/internare
  svr.Post("/api/internare", [&](const httplib::Request &req, httplib::Response &res)
           {
    try {
      auto body = json::parse(req.body);
      Internat i;
      i.numeP    = body["numeP"].get<string>();
      i.prenumeP = body["prenumeP"].get<string>();
      i.telefonP = body.value("telefonP", "-");
      i.boala    = body["boala"].get<string>();
      i.urgenta  = body["urgenta"].get<string>();
      i.medicNume= body["medicNume"].get<string>();
      i.medicSpec= body["medicSpec"].get<string>();
      i.camera   = body["camera"].get<int>();
      i.etaj     = body["etaj"].get<int>();
      i.cod      = body["cod"].get<string>();

      manager.internati.push_back(i);
      manager.salveazaInternati();
      manager.camereOcupate.push_back(i.camera);

      if (body.contains("factura")) {
        Factura f;
        f.numeP      = i.numeP; f.prenumeP = i.prenumeP;
        f.codFactura = body["factura"]["codFactura"].get<string>();
        f.suma       = body["factura"]["suma"].get<double>();
        f.achitata   = false;
        manager.facturi.push_back(f);
        manager.salveazaFacturi();
      }

      res.set_header("Access-Control-Allow-Origin", "*");
      res.set_content("{\"status\":\"ok\"}", "application/json");
    } catch (const exception &e) {
      res.status = 400;
      res.set_content(string("{\"error\":\"") + e.what() + "\"}", "application/json");
    } });

  // POST /api/factura – update sau creare factura
  svr.Post("/api/factura", [&](const httplib::Request &req, httplib::Response &res)
           {
    try {
      auto body = json::parse(req.body);
      string codF = body["codFactura"].get<string>();
      bool found = false;
      for (auto &f : manager.facturi) {
        if (f.codFactura == codF) {
          f.achitata = body.value("achitata", false);
          found = true; break;
        }
      }
      if (!found) {
        Factura f;
        f.numeP      = body.value("numeP", "");
        f.prenumeP   = body.value("prenumeP", "");
        f.codFactura = codF;
        f.suma       = body.value("suma", 0.0);
        f.achitata   = body.value("achitata", false);
        manager.facturi.push_back(f);
      }
      manager.salveazaFacturi();
      res.set_header("Access-Control-Allow-Origin", "*");
      res.set_content("{\"status\":\"ok\"}", "application/json");
    } catch (const exception &e) {
      res.status = 400;
      res.set_content(string("{\"error\":\"") + e.what() + "\"}", "application/json");
    } });

  // POST /api/doctor
  svr.Post("/api/doctor", [&](const httplib::Request &req, httplib::Response &res)
           {
    try {
      auto body = json::parse(req.body);
      string email = body["email"].get<string>();
      string pass = body["parola"].get<string>();
      string nume = body["nume"].get<string>();
      string spec = body["spec"].get<string>();

      // Adaugam cont medic
      ofstream fc("medici_conturi.txt", ios::app);
      fc << email << " " << pass << " " << nume << endl;
      fc.close();

      // Adaugam medic in lista
      Doctor d;
      d.nume = nume; d.specializare = spec;
      d.orar = "08:00-16:00"; d.zileLibere = "Sun";
      d.telefon = "0700000000"; d.salariu = 7000;
      manager.bazaMedici[spec].push_back(d);
      manager.salveazaMedici();
      manager.incarcaDate();

      res.set_header("Access-Control-Allow-Origin", "*");
      res.set_content("{\"status\":\"ok\"}", "application/json");
    } catch (const exception &e) {
      res.status = 400;
      res.set_content(string("{\"error\":\"") + e.what() + "\"}", "application/json");
    } });

  // OPTIONS preflight pentru CORS
  svr.Options(".*", [](const httplib::Request &, httplib::Response &res)
              {
    res.set_header("Access-Control-Allow-Origin",  "*");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type");
    res.set_content("", "text/plain"); });

  cout << "Serverul ruleaza pe http://localhost:8080..." << endl;
  svr.listen("0.0.0.0", 8080);
  return 0;
}