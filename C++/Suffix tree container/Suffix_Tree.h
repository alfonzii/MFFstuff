//Created by Matus Madar
//For purpose of Zapoctovy program
//Charles University Faculty of Mathematics and Physics


#include <string>
#include <vector>
#include <memory>
#include <utility>
#include <unordered_map>
#include <set>
#include <unordered_set>


#ifndef Suff
#define Suff

//-------------------------------------------------------Gumove pole------------------------------------------------------------------------
template<typename T> class Pole { //Vektor bez invalidacie pointrov. Presne to, co potrebujem.
public:
	Pole(size_t chunk = 100) : chunk_(chunk), size_(0) {}
	Pole(const Pole& old_Pole) {
		chunk_ = old_Pole.chunk_;
		size_ = old_Pole.size_;
		resize(size_);
		size_t i = 0;
		for (i = 0; i < size_/chunk_; ++i) {
			for (size_t j = 0; j < chunk_; ++j)
				hrabe_[i][j] = old_Pole.hrabe_[i][j];
		}
		if (i == size_ / chunk_) {
			for (size_t j = 0; j < size_ % chunk_; ++j)
				hrabe_[i][j] = old_Pole.hrabe_[i][j];
		}
	}

	void push_back(const T& x) { resize(++size_); (*this)[size_ - 1] = x; }
	T& operator[] (size_t i) { return hrabe_[i / chunk_][i%chunk_]; }
	
private:
	
	void resize(size_t i) {
		for (size_t k = hrabe_.size(); k <= i / chunk_; ++k)
			hrabe_.push_back(make_unique< T[]>(chunk_));
	}
	std::size_t chunk_;
	std::size_t size_;
	std::vector< std::unique_ptr<T[]>> hrabe_;
};


template<typename S> //spajanie mnozin. Vyuzivame to pri LCS pri vnorovacom DFS.
S union_sets(const S& s1, const S& s2)
{
	S result = s1;

	result.insert(s2.cbegin(), s2.cend());

	return result;
}



class Node {
public:
	Node();
	
	Node(Node * r, char type, unsigned int index_ENDov); //vytvori novy vrchol so zadanym rodicom
							   
	Node * rodic;
	char type;
	std::unordered_map< char, std::unique_ptr<  Node>> potomok;	//potomok a intervaly hran idu 1:1, teda napr. intervalyHranL['c'] reprezentuje hranu co vedie do potomok['c']
	std::unordered_map< char, std::pair< unsigned int, unsigned int>> intervalyHranV; //sem su vkladane indexy pre potomkov, ktory su vnutorny  
	std::unordered_map< char, std::pair< unsigned int, unsigned int *>> intervalyHranL; //sem su vkladane indexy pre potomkov, ktory su listy

	

	std::string variableString; //mozeme si ulozit stringovu hodnotu do vrcholu
	int variableInt; //obdobne aj int hodnotu

	//container robime pre bezneho pouzivatela (programatora) preto chceme aby bol co najblbuvzdornejsi a teda aby si nemohol menit ID ani containAllStrings
	friend class SuffixTree; //nastavovanie ID a containAllStrings bude prebiehat v SuffixTree cez "settery"
	unsigned int get_ID() { return ID; }
	bool get_containAllStrings() { return containAllStrings; }
	unsigned int get_ID() const{ return ID; }
	bool get_containAllStrings() const{	return containAllStrings; }

	Node * get_suffixLink() { return suffixLink; }
	Node * get_suffixLink() const { return suffixLink; }

private:
	unsigned int ID; //relevantne iba ked je vrchol list
	bool containAllStrings;
	Node * suffixLink; //pointer ktory nam pomaha pri buildovani stromu v zlozitosti O(n)
};


class SuffixTree {
public:
	SuffixTree();
	SuffixTree(const std::string& seedString);
	SuffixTree(const SuffixTree&);
	void insert(const std::string& Slovo); //metoda na vlozenie slova do stromu 

	void printAllSufixes(bool Sorted = false); //vypise vsetky suffixy z postaveneho suffixoveho stormu (a pripadne ich usorti ak je parameter true)
	const std::string& get_concString();
	bool isSubstring(const std::string& substr);
	std::string LCS();

	Node root;	//koren stromu je viditelny aj pre "vonkajsi svet" kvoli tomu, ak by si niekto chcel implementovat nieco vlastne na postavenom suff. strome
				//root ma k dispozicii tak si v nom moze hladat rozne veci a robit co chce
private:

	std::string concString;

	void set_ID(unsigned int, Node *); //tu je deklaracia tych metod, kvoli ktorym som spravil tuto triedu friend v predchadzajucej
	void set_containAllStrings(bool, Node*);
	

	bool imInID_; //ak som v sekcii ID (text$....) teda za '$', tak sa nastavim na true
	bool firstDollarSeen; //ak som uz navstivil dolar tak sa nastavim na true
	unsigned int NumOfStrings_;
	unsigned int i; //iterator v for v inserte

	unsigned int numBrokenEdgesThisTurn; //pouzivame v metode rozbiHranu; kvoli O(n)
	Node * lastBrokenNodeThisTurn;
	void prepareAllSuffixes(const Node&, const std::string& pomocnyString, bool Sort);


	struct size_compare { //funktor na porovnavanie podla dlzky suffixu
		bool operator() (const std::string& lhs, const std::string& rhs) const {
			std::string::size_type lhsDollarPos = lhs.rfind("$");
			std::string::size_type rhsDollarPos = rhs.rfind("$");
			if (stoi(lhs.substr(lhsDollarPos + 1, lhs.size() - lhsDollarPos - 1)) == stoi(rhs.substr(rhsDollarPos + 1, rhs.size() - rhsDollarPos - 1)))
				return lhs.size() > rhs.size();
			else
				return stoi(lhs.substr(lhsDollarPos + 1, lhs.size() - lhsDollarPos - 1)) < stoi(rhs.substr(rhsDollarPos + 1, rhs.size() - rhsDollarPos - 1));
		}
	};

	std::multiset<std::string, size_compare> vystupSuffixov; //mnozina, do ktorej vklad·m prvky ak ich chcem usporiadaù

	Pole<unsigned int> ENDy; //Tieto ENDy sluzia na odlisenie viacerych stringov pri stavani generalizovaneho suf. stromu
	unsigned int indexEndov;
	unsigned int remainder_;

	struct Active_Point
	{
		Node * ac_Node;
		char ac_Edge;
		unsigned int ac_Length;
	} ActivePoint_;

	std::string longest_common_substring; //privatna premenna, ktoru bude vracat metoda LCS
	std::unordered_map<Node*, Node*> stary_vs_novy_vrchol; //pouziva sa pri copy-constructore (1:1 reprezentacia "stareho" k "novemu" vrcholu)

	void rozbiHranu(Node * vrchol, char c, unsigned int index_c); //na zaklade activepoint a remaindera rozbijame hranu, ak char c nie je nasledujuci na hrane
	void pridajNovuHranu(Node * vrchol, char c, char type); //unsafe metoda, treba si hlidat, ze nepridavam hranu tam, kde uz znak ulozeny v c existuje
														   //Oba metody maju iba referenciu (nie const), pretoze ich upravujem

	void rekurzivneVyriesSuffixy(unsigned int i_Spracuvavaneho);
	void updateAc_point(unsigned int i_Spracuvavaneho); //updatnem AC point a to tak, ze ak je moja length akurat taka ze mam byt vo vrchole, tak pojdem do vrcholu a nebudem mat dlhsiu length ako je hrana
	bool mozemPridat(unsigned int i_Spracuvavaneho); //metoda sluzi pri druhej casti algoritmu, ak potrebujem riesit ked remainder neni == 1, teda som niekde zanoreny
													 //ci tam mozem pridat pismeno alebo nie. Ulahcuje to takto pracu a sprehladnuje kod algoritmu

	unsigned int nextChar(); //vrati mi nasledujuci znak (NULL v pripade ze som vo vrchole, teda nenasleduje ziadny znak);
	std::unordered_set<unsigned int> DFS_pre_LCS(Node * children); //pomocna funkcia (DFS) pre hladanie longest common substring
	void DFS2_pre_LCS(Node * children, std::string); //druha pomocna funkcia DFS, ktora prehlada strom druhykrat a najde LCS, podla uz nastavenych offsetov (containAllStrings)

	void CopyConstruct(const Node * old_node, Node * new_node);
	void InsertSuffixLinks(const Node * old_node, Node * new_node);
};







#endif // !Suff