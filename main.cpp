#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <map>
#include <iomanip>

using namespace std;

// --- STRUCTURI ---
struct Doctor
{
  string nume, orar, zileLibere, telefon, specializare;
};

struct Programare
{
  string numeP, prenumeP, emailP, telefonP, boala, medicNume, medicTel, medicSpec;
  int luna, zi;
  string ora;
};

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
  vector<Programare> programari;

  const string F_BOLI = "specializari.txt";
  const string F_MEDICI = "medici.txt";
  const string F_USERI = "utilizatori.txt";
  const string F_PROG = "programari.txt";

  bool esteZiBlocataGeneric(int zi)
  {
    return (zi % 3 == 0 || zi == 7 || zi == 14 || zi == 22);
  }

  // --- FUNCTIE MODIFICATA: RESCRIE FISIERUL SI FORMATEAZA GMAIL ---
  void trimiteEmailConfirmare(Programare p)
  {
    // Observatie: Am scos ios::app pentru a sterge continutul vechi
    ofstream mailFile("MAIL_SERVER.txt");
    mailFile << "==========================================" << endl;
    mailFile << "DE LA: server-notificari@spital-central.ro" << endl;
    mailFile << "CATRE: " << p.emailP << " (GMAIL)" << endl;
    mailFile << "SUBIECT: Confirmare Programare Medicala - 2026" << endl;
    mailFile << "------------------------------------------" << endl;
    mailFile << "Stimate " << p.numeP << " " << p.prenumeP << "," << endl
             << endl;
    mailFile << "Programarea dumneavoastra a fost inregistrata cu succes." << endl;
    mailFile << "Detalii consultatie:" << endl;
    mailFile << "- Specializare: " << p.medicNume << " (" << p.medicSpec << ")" << endl;
    mailFile << "- Telefon Medic: " << p.medicTel << endl;
    mailFile << "- Boala declarata: " << p.boala << endl;
    mailFile << "- Data: " << p.zi << "." << p.luna << ".2026" << endl;
    mailFile << "- Ora: " << p.ora << endl
             << endl;
    mailFile << "Va rugam sa aveti la dumneavoastra cardul de sanatate." << endl;
    mailFile << "==========================================" << endl;
    mailFile.close();
    cout << "\n[SMTP] Gmail-ul a fost trimis! (Continutul anterior din MAIL_SERVER.txt a fost sters)." << endl;
  }

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
      string s, n, o, zl, tel;
      getline(ss, s, ';');
      getline(ss, n, ';');
      getline(ss, o, ';');
      getline(ss, zl, ';');
      getline(ss, tel, ';');
      if (!s.empty())
        bazaMedici[s].push_back({n, o, zl, tel, s});
    }
    fm.close();

    programari.clear();
    ifstream fp(F_PROG.c_str());
    string np, pp, ep, tp, b, m, ora, mTel, mSpec;
    int ln, zi;
    // Citire extinsa pentru a include datele medicului salvate anterior daca e cazul
    while (fp >> np >> pp >> ep >> tp >> b >> m >> ln >> zi >> ora)
    {
      Programare p = {np, pp, ep, tp, b, m, "", "", ln, zi, ora};
      programari.push_back(p);
    }
    fp.close();

    staff.clear();
    ifstream fu(F_USERI.c_str());
    string un, upr, ue, upass, ur;
    while (fu >> un >> upr >> ue >> upass >> ur)
      staff.push_back(Utilizator(un, upr, ue, upass, ur));
    fu.close();
  }

  bool esteOcupat(string m, int l, int z, string ora = "")
  {
    if (ora == "" && esteZiBlocataGeneric(z))
      return true;
    for (size_t i = 0; i < programari.size(); i++)
    {
      if (programari[i].medicNume == m && programari[i].luna == l && programari[i].zi == z)
      {
        if (ora == "" || programari[i].ora == ora)
          return true;
      }
    }
    return false;
  }

  void afiseazaCalendar(int luna, string numeL, int start, int zile, string m)
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

public:
  SpitalManager() { incarcaDate(); }

  void meniuPacient()
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
      if (esteOcupat(d.nume, lA, zA))
      {
        cout << "\n[!] ZI OCUPATA [XX]. Alegeti alta data!\n";
      }
      else
        break;
    }

    cout << "\n--- SLOTURI ORARE LIBERE (30 MIN) ---\n";
    string ore[] = {"08:00", "08:30", "09:00", "09:30", "10:00", "10:30", "11:00", "11:30", "12:00", "12:30", "13:00"};
    vector<string> libere;
    for (int i = 0; i < 11; i++)
    {
      if (!esteOcupat(d.nume, lA, zA, ore[i]))
      {
        libere.push_back(ore[i]);
        cout << libere.size() << ". " << ore[i] << endl;
      }
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
    cout << "Gmail: ";
    cin >> emP;
    cout << "Tel: ";
    cin >> tlP;

    // Salvare in baza de date
    ofstream f(F_PROG.c_str(), ios::app);
    f << nP << " " << prP << " " << emP << " " << tlP << " " << bA << " " << d.nume << " " << lA << " " << zA << " " << oraFinala << endl;
    f.close();

    // Trimitere Gmail (Rescrie fisierul)
    Programare p = {nP, prP, emP, tlP, bA, d.nume, d.telefon, d.specializare, lA, zA, oraFinala};
    trimiteEmailConfirmare(p);

    incarcaDate();
    cout << "\n[OK] Programare finalizata cu succes!" << endl;
  }

  void cauta()
  {
    string n;
    cout << "Nume familie: ";
    cin >> n;
    for (size_t i = 0; i < programari.size(); i++)
      if (programari[i].numeP == n)
        cout << "[*] Data: " << programari[i].zi << "/" << programari[i].luna << " ora " << programari[i].ora << endl;
  }

  void signup()
  {
    string n, p, e, pass, r;
    cout << "Nume: ";
    cin >> n;
    cout << "Gmail: ";
    cin >> e;
    cout << "Pass: ";
    cin >> pass;
    cout << "Rol: ";
    cin >> r;
    ofstream f(F_USERI.c_str(), ios::app);
    f << n << " " << n << " " << e << " " << pass << " " << r << endl;
    f.close();
    incarcaDate();
  }

  void login()
  {
    string e, p;
    cout << "Email: ";
    cin >> e;
    cout << "Pass: ";
    cin >> p;
    for (size_t i = 0; i < staff.size(); i++)
      if (staff[i].email == e && staff[i].parola == p)
        cout << "\nSalut, " << staff[i].nume << endl;
  }

  void admin()
  {
    string p;
    cout << "Parola Admin: ";
    cin >> p;
    if (p == "admin2026")
      for (size_t i = 0; i < programari.size(); i++)
        cout << programari[i].numeP << " | Medic: " << programari[i].medicNume << " | " << programari[i].ora << endl;
  }
};

int main()
{
  SpitalManager manager;
  int opt;
  while (true)
  {
    cout << "\n1. PACIENT | 2. CAUTARE | 3. LOGIN | 4. SIGNUP | 5. ADMIN | 0. IESIRE\nAlegere: ";
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
      manager.cauta();
      break;
    case 3:
      manager.login();
      break;
    case 4:
      manager.signup();
      break;
    case 5:
      manager.admin();
      break;
    }
  }
  return 0;
}