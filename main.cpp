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
// --- FUNCTII DE VALIDARE ---
// ============================================================
void valideazaEmail(const string &email)
{
  string sufix = "@gmail.com";
  if (email.size() < sufix.size() ||
      email.substr(email.size() - sufix.size()) != sufix)
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
                                " nu exista in luna " + to_string(luna) +
                                " (max " + to_string(zileLunii) + " zile).");
}

// Genereaza cod unic de forma #XXXXXX
string genereazaCod()
{
  string cod = "#";
  for (int i = 0; i < 6; i++)
    cod += to_string(rand() % 10);
  return cod;
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
  string ora;
  string cod;
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

  const string F_BOLI = "specializari.txt";
  const string F_MEDICI = "medici.txt";
  const string F_USERI = "utilizatori.txt";
  const string F_MEDICI_CNT = "medici_conturi.txt";
  const string F_PROG = "programari.txt";

  // --------------------------------------------------------
  bool esteZiBlocataGeneric(int zi)
  {
    return (zi % 3 == 0 || zi == 7 || zi == 14 || zi == 22);
  }

  string determinaCodUrgenta(const string &boala)
  {
    if (boala == "Infarct" || boala == "AVC" || boala == "Abces" || boala == "Melanom")
      return "(o) COD ROSU - Urgenta Critica";
    if (boala == "Fractura" || boala == "Diabet" || boala == "Psoriazis" || boala == "Litiaza")
      return "(o) COD PORTOCALIU - Urgenta Ridicata";
    return "(o) COD VERDE - Consultatie Standard";
  }

  // --------------------------------------------------------
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
    mailFile << "Detalii consultatie:" << endl;
    mailFile << "- Specializare: " << p.medicNume << " (" << p.medicSpec << ")" << endl;
    mailFile << "- Telefon Medic: " << p.medicTel << endl;
    mailFile << "- Boala declarata: " << p.boala << endl;
    mailFile << "- Data: " << p.zi << "." << p.luna << ".2026" << endl;
    mailFile << "- Ora: " << p.ora << endl
             << endl;
    mailFile << "Cod anulare programare: " << p.cod << endl
             << endl;
    mailFile << "Va rugam sa aveti la dumneavoastra cardul de sanatate." << endl;
    mailFile << "==========================================" << endl;
    mailFile.close();
    cout << "\n[SMTP] Gmail-ul a fost trimis! (Continutul anterior din MAIL_SERVER.txt a fost sters)." << endl;
  }

  // --------------------------------------------------------
  void incarcaDate()
  {
    // --- Boli ---
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

    // --- Medici (format: Spec;Nume;Orar;ZiLibera;Tel;Salariu) ---
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

    // --- Programari (numeP prenumeP emailP telP boala medicNume luna zi ora cod) ---
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

    // --- Conturi admin (utilizatori.txt: nume prenume email parola rol) ---
    staff.clear();
    ifstream fu(F_USERI.c_str());
    string un, upr, ue, upass, ur;
    while (fu >> un >> upr >> ue >> upass >> ur)
      staff.push_back(Utilizator(un, upr, ue, upass, ur));
    fu.close();

    // --- Conturi medici (medici_conturi.txt: email parola numeDoctor) ---
    conturiMedici.clear();
    ifstream fc(F_MEDICI_CNT.c_str());
    string me, mpass, mnume;
    while (fc >> me >> mpass >> mnume)
      conturiMedici.push_back(Utilizator(mnume, "", me, mpass, "medic"));
    fc.close();
  }

  // --------------------------------------------------------
  bool esteOcupat(const string &m, int l, int z, const string &ora = "")
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

  // --------------------------------------------------------
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

  // --------------------------------------------------------
  void salveazaProgramari()
  {
    ofstream f(F_PROG.c_str());
    for (size_t i = 0; i < programari.size(); i++)
    {
      Programare &p = programari[i];
      f << p.numeP << " " << p.prenumeP << " " << p.emailP << " "
        << p.telefonP << " " << p.boala << " " << p.medicNume << " "
        << p.luna << " " << p.zi << " " << p.ora << " "
        << p.cod << endl;
    }
    f.close();
  }

  // --------------------------------------------------------
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

  // ============================================================
public:
  SpitalManager()
  {
    srand((unsigned)time(0));
    incarcaDate();
  }

  // ============================================================
  // MENIU PACIENT
  // ============================================================
  void meniuPacient()
  {
    int optP;
    cout << "\n=== MENIU PACIENT ===" << endl;
    cout << "1. Programare noua" << endl;
    cout << "2. Anulare programare" << endl;
    cout << "0. Inapoi" << endl;
    cout << "Alegere: ";
    cin >> optP;

    if (optP == 0)
      return;
    if (optP == 2)
    {
      anuleazaProgramare();
      return;
    }
    if (optP != 1)
      return;

    // ---- PROGRAMARE NOUA ----
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
      cout << k + 1 << ". " << dr.nume << " | Tel: " << dr.telefon
           << " | " << dr.specializare << endl;
    }
    int optM;
    cin >> optM;
    Doctor d = bazaMedici[sA][optM - 1];

    // --- ALEGERE DATA ---
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

    // --- SLOTURI ORARE ---
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

    // --- DATE PACIENT ---
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

    // --- GENEREAZA COD UNIC ---
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

    // --- SALVEAZA ---
    Programare p = {nP, prP, emP, tlP, bA, d.nume, d.telefon, d.specializare,
                    lA, zA, oraFinala, codAnulare};
    programari.push_back(p);
    salveazaProgramari();
    trimiteEmailConfirmare(p);
    incarcaDate();

    cout << "\n[OK] Programare finalizata cu succes!" << endl;
    cout << ">>> Codul tau de anulare este: " << codAnulare
         << " (retine-l!)" << endl;
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
      if (programari[i].numeP == nP &&
          programari[i].prenumeP == prP &&
          programari[i].cod == cod)
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
  // MENIU MEDIC (cu login)
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
      cout << "\n[!] Contul exista dar medicul nu a fost gasit in baza de date." << endl;
      return;
    }

    cout << "\n=== Bun venit, " << dr->nume << " ===" << endl;

    int optM;
    while (true)
    {
      cout << "\n--- MENIU MEDIC ---" << endl;
      cout << "1. Programarile mele" << endl;
      cout << "2. Informatii personale (orar, zi libera, salariu)" << endl;
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
        cout << "\n=== INFORMATII PERSONALE ===" << endl;
        cout << "  Nume:         " << dr->nume << endl;
        cout << "  Specializare: " << dr->specializare << endl;
        cout << "  Orar:         " << dr->orar << endl;
        cout << "  Zi libera:    " << dr->zileLibere << endl;
        cout << "  Telefon:      " << dr->telefon << endl;
        cout << "  Salariu:      " << dr->salariu << " RON" << endl;
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
      cout << "2. Adauga cont medic (signup)" << endl;
      cout << "3. Modifica salariu medic" << endl;
      cout << "4. Modifica orar medic" << endl;
      cout << "0. Inapoi" << endl;
      cout << "Alegere: ";
      cin >> optA;

      if (optA == 0)
        break;

      // --- 1. Toate programarile ---
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

      // --- 2. Signup medic ---
      else if (optA == 2)
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

      // --- 3. Modifica salariu ---
      else if (optA == 3)
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

      // --- 4. Modifica orar ---
      else if (optA == 4)
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
    cout << "\n1. PACIENT | 2. MEDIC | 3. ADMIN | 0. IESIRE\nAlegere: ";
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
