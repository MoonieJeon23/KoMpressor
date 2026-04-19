#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <map>

// Modèle de base pour un arbre binaire (Huffman)
struct Noeud {
    char caractere; // Caractère stocké
    int frequence;  // Nombre d'apparitions

    Noeud* gauche; // Enfant gauche (0)
    Noeud* droite; // Enfant droite (1)

    // Constructeur pour initialiser un noeud
    Noeud(char c, int f) : caractere(c), frequence(f), gauche(nullptr), droite(nullptr) {}
};

// --- BLOC : Parcours Récursif (Génération des codes) ---
void genererCodes(Noeud* n, std::string code, std::map<char, std::string>& tableDeCodes) {
    if (n == nullptr) return;

    // Si c'est une feuille, on enregistre son code binaire
    if (n->gauche == nullptr && n->droite == nullptr) {
        tableDeCodes[n->caractere] = code;
    }

    // On descend à gauche (ajoute 0) et à droite (ajoute 1)
    genererCodes(n->gauche, code + "0", tableDeCodes);
    genererCodes(n->droite, code + "1", tableDeCodes);
}

// --- BLOC : Nettoyage de la RAM ---
void effacerArbre(Noeud* n) {
    if (n == nullptr) return;
    effacerArbre(n->gauche);
    effacerArbre(n->droite);
    delete n;
}

// --- BLOC : Encodeur de Fichier (Compression Binaire Pro) ---
void compresserFichier(std::string cheminOriginal, std::map<char, std::string>& tableDeCodes, int frequences[256]) {
    std::ifstream entree(cheminOriginal, std::ios::binary);
    std::ofstream sortie(cheminOriginal + ".huff", std::ios::binary);

    if (!entree.is_open() || !sortie.is_open()) {
        std::cout << "Erreur lors de la compression !" << std::endl;
        return;
    }

    // 1. ÉCRITURE DE L'ENTÊTE (HEADER) - La "recette" pour décompresser
    // Signature magique pour identifier le fichier
    sortie.write("KOMP", 4); 

    // Calcul du total et des briques uniques
    long long total = 0;
    int nbBriques = 0;
    for(int i = 0; i < 256; i++) {
        total += frequences[i];
        if(frequences[i] > 0) nbBriques++;
    }

    // Écriture brute (Binaire) du total et du nombre de briques
    sortie.write(reinterpret_cast<const char*>(&total), sizeof(total));
    sortie.write(reinterpret_cast<const char*>(&nbBriques), sizeof(nbBriques));

    // Écriture de la table des fréquences (1 char + 4 octets par brique)
    for(int i = 0; i < 256; i++) {
        if(frequences[i] > 0) {
            char c = (char)i;
            sortie.write(&c, 1);
            sortie.write(reinterpret_cast<const char*>(&frequences[i]), sizeof(int));
        }
    }

    // 2. BIT PACKING - On transforme les '0' et '1' (string) en vrais bits
    unsigned char buffer = 0; // Notre octet de stockage (8 bits)
    int nbBits = 0; 
    char c;

    std::cout << "Compression en cours..." << std::endl;
    while (entree.get(c)) {
        std::string code = tableDeCodes[c];
        for (char bit : code) {
            buffer <<= 1; // On décale pour faire de la place
            if (bit == '1') buffer |= 1; // On ajoute le bit 1
            nbBits++;

            if (nbBits == 8) { // Si l'octet est plein, on l'écrit
                sortie.put(buffer);
                buffer = 0; nbBits = 0;
            }
        }
    }

    // 3. PADDING - On vide le dernier buffer s'il n'est pas plein
    if (nbBits > 0) {
        buffer <<= (8 - nbBits); 
        sortie.put(buffer);
    }

    entree.close(); sortie.close();
}

// --- BLOC : Décompresseur (Restauration exacte) ---
void decompresserFichier(std::string cheminCompresse, Noeud* racine, std::string extension) {
    std::ifstream entree(cheminCompresse, std::ios::binary);
    std::string cheminSortie = cheminCompresse.substr(0, cheminCompresse.find_last_of(".")) + "_restaure" + extension;
    std::ofstream sortie(cheminSortie, std::ios::binary);

    if (!entree.is_open()) return;

    // 1. Lecture de l'entête binaire
    char sig[4]; entree.read(sig, 4);
    long long total; entree.read(reinterpret_cast<char*>(&total), sizeof(total));
    int briques; entree.read(reinterpret_cast<char*>(&briques), sizeof(briques));

    // 2. On saute la table des fréquences pour arriver aux données (5 octets par brique)
    entree.seekg(briques * 5, std::ios::cur);

    // 3. Décodage bit à bit en parcourant l'arbre
    Noeud* actuel = racine;
    unsigned char buffer;
    long long ecrits = 0;

    while (ecrits < total && entree.read(reinterpret_cast<char*>(&buffer), 1)) {
        for (int i = 7; i >= 0 && ecrits < total; i--) {
            int bit = (buffer >> i) & 1;
            actuel = (bit == 0) ? actuel->gauche : actuel->droite;

            if (actuel->gauche == nullptr && actuel->droite == nullptr) {
                sortie.put(actuel->caractere);
                ecrits++;
                actuel = racine; // Retour au sommet de l'arbre
            }
        }
    }
    entree.close(); sortie.close();
}

// --- MAIN : Gestion des arguments et Pipeline ---
int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: .\\KoMpressor.exe <fichier>" << std::endl;
        return 1;
    }

    std::string nomFichier = argv[1];
    // On récupère l'extension pour la restauration (ex: .jpg)
    std::string ext = nomFichier.substr(nomFichier.find_last_of("."));

    // --- ANALYSE STATISTIQUE BINAIRE ---
    int frequences[256] = {0};
    std::ifstream Myreader(nomFichier, std::ios::binary); // FLAG BINAIRE CRUCIAL

    if (!Myreader.is_open()) {
        std::cout << "Erreur d'ouverture de : " << nomFichier << std::endl;
        return 1;
    }

    char c;
    while (Myreader.get(c)) {
        frequences[static_cast<unsigned char>(c)]++;
    }
    Myreader.close();

    // --- CONSTRUCTION DE L'ARBRE (Algorithme Glouton) ---
    std::vector<Noeud*> maListe;
    for(int i = 0; i < 256; i++) {
        if (frequences[i] > 0) {
            maListe.push_back(new Noeud(static_cast<char>(i), frequences[i]));
        }
    }

    // Boucle de fusion des noeuds
    while (maListe.size() > 1) {
        std::sort(maListe.begin(), maListe.end(), [](Noeud* a, Noeud* b) {
            return a->frequence < b->frequence;
        });

        Noeud* g = maListe[0];
        Noeud* d = maListe[1];
        Noeud* parent = new Noeud('\0', g->frequence + d->frequence);
        parent->gauche = g; parent->droite = d;

        maListe.erase(maListe.begin(), maListe.begin() + 2);
        maListe.push_back(parent);
    }

    if (!maListe.empty()) {
        Noeud* racine = maListe[0];
        std::map<char, std::string> table;
        genererCodes(racine, "", table);

        std::cout << "Frequence totale detectee : " << racine->frequence << " octets." << std::endl;

        // Compression et Décompression immédiate pour test
        compresserFichier(nomFichier, table, frequences);
        decompresserFichier(nomFichier + ".huff", racine, ext);

        effacerArbre(racine);
        std::cout << "Memoire liberee. Travail fini !" << std::endl;
    }

    return 0;
}