#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <map>


// Modèle de base pour un arbre binaire (utilisable pour Huffman, généalogie, dossiers, etc.)
struct Noeud {//Structure to represent a node in the Huffman tree
    char caractere;//Character stored in the node
    int frequence;//Frequency of the character

    Noeud* gauche; //Pointer to the left child node
    Noeud* droite;//Pointer to the right child node

    Noeud(char c, int f) { //Constructor to initialize the node with a character and its frequency
         caractere= c;
         frequence = f;
         gauche = nullptr;//Initialize left and right pointers to nullptr
         droite = nullptr;
         }
} ;

//Le Parcours Récursif de l'Arbre (Tree Traversal)
void genererCodes(Noeud* n, std::string code, std::map<char, std::string>& tableDeCodes) { // updated
   if (n == nullptr) return; // Sécurité : if the node is null, return

    // if the node is a leaf (it has a character), print the character and its corresponding code
   if (n->gauche == nullptr && n->droite == nullptr) {
        tableDeCodes[n->caractere] = code; // updated
        std::cout << "Lettre: " << n->caractere << " => Code: " << code << std::endl;
    }

    // Recursively generate codes for the left and right subtrees
    genererCodes(n->gauche, code + "0", tableDeCodes); // updated
    genererCodes(n->droite, code + "1", tableDeCodes); // updated
}

//  Destructeur Récursif (Nettoyage de la RAM)
void effacerArbre(Noeud* n) {
    if (n == nullptr) return;

    // (post-order traversal)
    effacerArbre(n->gauche);
    effacerArbre(n->droite);

    
    delete n;
}

// Encodeur de Fichier (Compression)//updated
// Réutilisable pour :
// BLOC : Encodeur de Fichier avec Entête (Header) // updated
// Réutilisabilité : Permet de stocker la "recette" de traduction directement dans le fichier/ Tout projet de traduction de données (Chiffrement, Conversion de format)
void compresserFichier(std::string cheminOriginal, std::map<char, std::string>& tableDeCodes, int frequences[256]) { // updated : ajout de l'array frequences
    std::ifstream entree(cheminOriginal);
    std::ofstream sortie(cheminOriginal + ".huff", std::ios::binary); // updated : extension .huff et mode binaire

    if (!entree.is_open() || !sortie.is_open()) {
        std::cout << "Erreur lors de la compression !" << std::endl;
        return;
    }

    // 1. Écriture de l'Entête (Header) // updated
    // On compte combien de caractères ont une fréquence > 0
    int nbCaracteresUnique = 0;
    for(int i = 0; i < 256; i++) if(frequences[i] > 0) nbCaracteresUnique++;

    // On écrit ce nombre en premier dans le fichier
    sortie << nbCaracteresUnique << std::endl;

    // On écrit chaque couple (Caractère : Fréquence)
    for(int i = 0; i < 256; i++) {
        if(frequences[i] > 0) {
            sortie << (char)i << " " << frequences[i] << std::endl;
        }
    }

    // 2. Écriture du contenu compressé en BINAIRE // updated
    // Réutilisabilité : Bloc de "Bit Packing" standard pour toute compression binaire.
    unsigned char buffer = 0; // Notre octet de stockage (8 bits)
    int nbBits = 0;           // Compteur de bits dans le buffer
    char c;

    std::cout << "Compression binaire en cours..." << std::endl;

    while (entree.get(c)) {
        std::string code = tableDeCodes[c];

        for (char bit : code) {
            // On décale le buffer vers la gauche de 1 pour faire de la place
            buffer <<= 1; 

            // Si le bit est '1', on l'ajoute à droite avec l'opérateur OR (|)
            if (bit == '1') {
                buffer |= 1;
            }

            nbBits++;

            // Dès qu'on a 8 bits, on écrit l'octet et on réinitialise
            if (nbBits == 8) {
                sortie.put(buffer);
                buffer = 0;
                nbBits = 0;
            }
        }
    }

    // 3. Gestion du PADDING (Le final) // updated
    // Si à la fin il reste des bits qui n'ont pas formé un octet complet
    if (nbBits > 0) {
        // On décale vers la gauche pour que les bits soient en haut de l'octet
        buffer <<= (8 - nbBits); 
        sortie.put(buffer);
    }

    entree.close();
    sortie.close();
    std::cout << "Fichier .huff REELLEMENT compresse avec succes !" << std::endl;
 }

// BLOC : Décompresseur de Fichier (Restauration) // updated
// Réutilisabilité : Logique inverse de parcours d'arbre pour décodage.
void decompresserFichier(std::string cheminCompresse, Noeud* racine) {
    std::ifstream entree(cheminCompresse, std::ios::binary);
    
    // On crée le nom du fichier restauré (ex: test.txt.huff -> test_restaure.txt)
    std::string cheminBase = cheminCompresse.substr(0, cheminCompresse.find_last_of("."));
    std::ofstream sortie(cheminBase + "_restaure.txt");

    if (!entree.is_open() || !sortie.is_open()) {
        std::cout << "Erreur lors de la decompression !" << std::endl;
        return;
    }

    // 1. Sauter l'entête // updated
    // Puisque nous avons déjà la 'racine' passée en paramètre, 
    // on va simplement sauter les lignes de l'entête pour arriver aux données binaires.
    std::string ligne;
    int nbLignesHeader;
    entree >> nbLignesHeader;
    entree.ignore(); // Sauter le \n
    for(int i = 0; i < nbLignesHeader; i++) {
        std::getline(entree, ligne); // On passe les lignes de fréquences
    }

    // 2. Décodage des bits // updated
    Noeud* actuel = racine;
    unsigned char buffer;
    
    std::cout << "Restauration du fichier en cours..." << std::endl;

    while (entree.read(reinterpret_cast<char*>(&buffer), 1)) {
        for (int i = 7; i >= 0; i--) {
            // On extrait le bit n°i (en partant de la gauche)
            int bit = (buffer >> i) & 1;

            if (bit == 0) actuel = actuel->gauche;
            else actuel = actuel->droite;

            // Si on atteint une feuille (un caractère)
            if (actuel->gauche == nullptr && actuel->droite == nullptr) {
                sortie.put(actuel->caractere);
                actuel = racine; // On repart du sommet pour le prochain code
            }
        }
    }

    entree.close();
    sortie.close();
    std::cout << "Fichier restaure avec succes : " << cheminBase << "_restaure.txt" << std::endl;
}

// BLOC : Gestion des Arguments CLI (Command Line Interface) // updated
// Réutilisabilité : Indispensable pour transformer n'importe quel script en outil professionnel
// utilisable via terminal (ex: mon_outil.exe fichier_source).
int main(int argc, char* argv[]) { // updated

    // Vérification de sécurité // updated
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <nom_du_fichier>" << std::endl;
        std::cout << "Exemple: .\\KoMpressor.exe ../data/test.txt" << std::endl;
        return 1;
    }

    // Récupération du nom du fichier passé en argument // updated
    std::string nomFichierOriginal = argv[1];

    // BLOC : Analyse Statistique (Fréquences) // updated
    //Variable to read the file
    std::fstream Myreader;
    char c;
    int frequences[256] = {0}; //Array to store the frequency of each character (ASCII characters)

    //Open the file dynamique // updated
    Myreader.open (nomFichierOriginal);

    //Lecture Sécurisée de Fichier
    if (!Myreader.is_open()) {

        //If the file cannot be opened, print an error message and exit
        std::cout << "Error opening file: " << nomFichierOriginal << " !" << std::endl;

        return 1;

    }

    else {

        //Analyse Statistique
        while (Myreader.get(c)) {//Read the file character by character

            //Increment the frequency of the character read
            frequences[static_cast<unsigned char>(c)]++;

        }

        Myreader.close();
        //Close the file 

    }

    for (int i = 0; i < 256; i++) {//Iterate through the frequency array and print the characters and their frequencies

        if (frequences[i] > 0) {//Only print characters that appear in the file

            //Print the character and its frequency
            std::cout << "Character: '" << static_cast<char>(i) << "' Frequency: " << frequences[i] << std::endl;

        }

    }

    // BLOC : Data Extraction (Initialisation des Noeuds) // updated
    //Data Extraction and Node Creation
    //Create a list of nodes for the Huffman tree based on the frequencies of the characters
    std::vector<Noeud*> maListeDeNoeuds;//Vector to store the nodes of the Huffman tree
    for(int i = 0; i < 256; i++) {
        if (frequences[i] > 0) {

            //Create a new node for each character that appears in the file and add it to the list
            maListeDeNoeuds.push_back(new Noeud(static_cast<char>(i), frequences[i])); //Create a new node with the character and its frequency and add it to the list
        }
    }
    std::cout << "Nombre de briques pretes pour l'arbre : " << maListeDeNoeuds.size() << std::endl; //Print the number of nodes created

    //Construction de l'Arbre de Huffman par Approche Gloutonne (Greedy Approach)
   // BLOC : Tri avec Prédicat Customisé (Lambda) // updated
   // 1.Tri avec Prédicat Customisé (ou plus simplement, un Tri par Lambda
    std::sort(maListeDeNoeuds.begin(), maListeDeNoeuds.end(), [](Noeud* a, Noeud* b) {
        return a->frequence < b->frequence;
    });

    
    std::cout << "--- Liste triee ---" << std::endl;
    for (Noeud* n : maListeDeNoeuds) {
        std::cout << n->caractere << " (" << n->frequence << ")" << std::endl;
    }

    // BLOC : Boucle de Réduction (Algorithme Glouton) // updated
    // 2.Construction de l'Arbre de Huffman
    //La Boucle de Réduction (Reduction Loop)
    while (maListeDeNoeuds.size() > 1) {
        Noeud* gauche = maListeDeNoeuds[0];
        Noeud* droite = maListeDeNoeuds[1];

    //La Fusion de Nœuds (Node Merging)
        Noeud* parent = new Noeud('\0', gauche->frequence + droite->frequence);
        parent->gauche = gauche;
        parent->droite = droite;

        maListeDeNoeuds.erase(maListeDeNoeuds.begin(), maListeDeNoeuds.begin() + 2);
        maListeDeNoeuds.push_back(parent);

        std::sort(maListeDeNoeuds.begin(), maListeDeNoeuds.end(), [](Noeud* a, Noeud* b) {
            return a->frequence < b->frequence;
        });
    }

    Noeud* racine = nullptr;

    //Affichage de la racine de l'arbre de Huffman
    if (!maListeDeNoeuds.empty()) {
        racine = maListeDeNoeuds[0]; // CORRECTION ICI : Pas de "Noeud*" devant racine
        std::cout << "Arbre construit ! Frequence totale : " << racine->frequence << std::endl;
    }

    
    // BLOC : Pipeline d'Exécution // updated 2
   if (racine != nullptr) {
    std::map<char, std::string> tableDeCodes;
    genererCodes(racine, "", tableDeCodes);
    
    // updated : on passe aussi le tableau 'frequences'
    compresserFichier(nomFichierOriginal, tableDeCodes, frequences); 

    // 2. On décompresse immédiatement pour tester (Optionnel mais recommandé)
     decompresserFichier(nomFichierOriginal + ".huff", racine);
}

   
    Noeud test('Z', 10);
    std::cout << "Test brique : " << test.caractere << " apparait " << test.frequence << " fois." << std::endl;
    

    //Libération de la mémoire
    if (racine != nullptr) {
        effacerArbre(racine);
        racine = nullptr; // Bonne pratique : on remet à zéro pour éviter les pointeurs fous
        std::cout << "Memoire liberee avec succes." << std::endl;
    }

    return 0;
}