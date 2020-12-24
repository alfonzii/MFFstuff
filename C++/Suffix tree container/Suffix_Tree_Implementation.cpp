#include <vector>
#include <string>
#include <iostream>
#include <exception>
#include <utility>
#include <algorithm>
#include <queue>

#include "Suffix_Tree.h"

using namespace std;


//------------------------------------------------------EXCEPTIONS------------------------------------------------------------------------------
class EmptyTreeException : public exception
{
	virtual const char * what() const throw ()
	{
		return "No string inserted in SuffixTree.\n";
	}
};

//-------------------------------------------------------------------NODE--------------------------------------------------------------------------
Node::Node() : rodic(nullptr), suffixLink(nullptr), variableInt(0), variableString(""), ID(4294967295) {}
Node::Node(Node * r, char typ, unsigned int i_Endov) {
	Node();
	rodic = r;
	type = typ;
	if (typ == 'l')
		ID = i_Endov;
}

void SuffixTree::set_containAllStrings(bool t, Node * n) {
	n->containAllStrings = t;
}

void SuffixTree::set_ID(unsigned int i, Node * n) {
	n->ID = i;
}

SuffixTree::SuffixTree() : imInID_(false), firstDollarSeen(false), NumOfStrings_(0), remainder_(0), i(0), concString(""), indexEndov(0), numBrokenEdgesThisTurn(0), lastBrokenNodeThisTurn(nullptr) {
	root.type = 'r';
	ENDy.push_back(4294967295); //MAX UINT

	ActivePoint_.ac_Node = &(root);
	ActivePoint_.ac_Edge = NULL;
	ActivePoint_.ac_Length = 0;

}
SuffixTree::SuffixTree(const std::string& seedString) : SuffixTree() {
	insert(seedString);
}

SuffixTree::SuffixTree(const SuffixTree& old_suffixtree) : ENDy(old_suffixtree.ENDy) {
	imInID_ = false;
	firstDollarSeen = false;
	NumOfStrings_ = old_suffixtree.NumOfStrings_;
	concString = old_suffixtree.concString;
	i = old_suffixtree.i;
	remainder_ = old_suffixtree.remainder_;
	numBrokenEdgesThisTurn = old_suffixtree.numBrokenEdgesThisTurn;
	indexEndov = old_suffixtree.indexEndov;
	lastBrokenNodeThisTurn = nullptr;
	ActivePoint_ = old_suffixtree.ActivePoint_;
	root.rodic = nullptr;

	CopyConstruct(&old_suffixtree.root, &root);
	InsertSuffixLinks(&old_suffixtree.root, &root);
}

void SuffixTree::CopyConstruct(const Node * old_n, Node * new_n) {
	set_containAllStrings(old_n->get_containAllStrings(), new_n);
	//new_n->ID = old_n->ID;
	new_n->type = old_n->type;
	new_n->variableInt = old_n->variableInt;
	new_n->variableString = old_n->variableString;

	for (auto it = old_n->potomok.begin(); it != old_n->potomok.end(); ++it) {
		new_n->potomok.insert({ it->first, make_unique<Node>(new_n, it->second->type, it->second->get_ID()) }); //spravim vernu kopiu potomka
		stary_vs_novy_vrchol.insert({ it->second.get(), new_n->potomok[it->first].get() }); //spravime si zaznam do tabulky (kluc je stary vrchol, hodnota novy)

		if (it->second->type == 'l')
			new_n->intervalyHranL.insert({ it->first, old_n->intervalyHranL.at(it->first) });
		else
			new_n->intervalyHranV.insert({ it->first, old_n->intervalyHranV.at(it->first) });

		CopyConstruct(it->second.get(), new_n->potomok[it->first].get());
	}
}

void SuffixTree::InsertSuffixLinks(const Node * old_n, Node * new_n) { //Suffix linky vlozime na druhy prechod, teda ked budeme mat tabulku stary_vs_novy_vrchol naplnenu
	queue<Node*> dfs_struktura;										   //aby sme poznali 1:1 ktory vrchol v novom je reprezentovany starym
	for (auto it = old_n->potomok.begin(); it != old_n->potomok.end(); ++it)
		dfs_struktura.push(it->second.get());
	if (old_n->suffixLink == nullptr)
		new_n->suffixLink = nullptr;
	else {
		new_n->suffixLink = stary_vs_novy_vrchol[old_n->suffixLink];
	}
	while (dfs_struktura.size() != 0) {
		InsertSuffixLinks(dfs_struktura.front(), stary_vs_novy_vrchol[dfs_struktura.front()]);
		dfs_struktura.pop();
	}
}

const string& SuffixTree::get_concString() {
	return concString;
}



void SuffixTree::pridajNovuHranu(Node * vrchol, char c, char type) {
	vrchol->potomok.insert({ c, make_unique<Node>(vrchol, type, indexEndov) });
}

void SuffixTree::rozbiHranu(Node * vrchol, char c, unsigned int index_c) { //ak vieme, ze uz je hranu nutne rozbit, tak pouzijeme tuto metodu (unsafe)
	pair<unsigned int, unsigned int> p1, p2; //p1 je prva cast vetvy, p2/3 je pripojenie zbytku.
	pair<unsigned int, unsigned int *> p3; //p3 sa pouziva pri prepojeni ak rozbijam hranu iducu do listu
	unsigned int * ukazovatelNaEnd = &(ENDy[indexEndov]); //vyuzitie pri stavbe generalizovaneho 
	unsigned int IDcko; //v pripade ze rozbijam hranu listu, tak na zachovanie ID predosleho vrcholu

	if ((*(vrchol->potomok[ActivePoint_.ac_Edge])).type == 'l') {
		p1.first = vrchol->intervalyHranL[ActivePoint_.ac_Edge].first; //interval prvej casti
		p1.second = p1.first + ActivePoint_.ac_Length - 1;

		p3.first = p1.second + 1; //interval druhej ("uz znamej") casti
		p3.second = vrchol->intervalyHranL[ActivePoint_.ac_Edge].second; //nastavim mu taky END ktory mal (nie nutne ten, ktory je teraz aktualny)

		IDcko = vrchol->potomok[ActivePoint_.ac_Edge]->get_ID(); //ulozime ID z povodneho listu, pretoze sa bude vymazavat vrchol

		vrchol->intervalyHranL.erase(ActivePoint_.ac_Edge); //vymaz vrchol z tabulky hodnot
		vrchol->potomok.erase(ActivePoint_.ac_Edge); //vymaz vrchol z tabulky ze je sused (tym padom kedze to je uptr, tak by mal zaniknut)

		pridajNovuHranu(vrchol, ActivePoint_.ac_Edge, 'v'); //vytvori sa novy vnutorny vrchol
		vrchol->intervalyHranV.insert({ ActivePoint_.ac_Edge, p1 }); //priradim mu string (indexy) na hranu

		pridajNovuHranu(vrchol->potomok[ActivePoint_.ac_Edge].get(), concString[p3.first], 'l'); //vytvorim druhu cast hrany ktora bola rozbita (resp. vrchol)
		(*vrchol->potomok[ActivePoint_.ac_Edge]).intervalyHranL.insert({ concString[p3.first], p3 }); //priradil som hodnotu hrane
		set_ID(IDcko, vrchol->potomok[ActivePoint_.ac_Edge]->potomok[concString[p3.first]].get()); //nastavim nasmu listu spravne ID

		pridajNovuHranu(vrchol->potomok[ActivePoint_.ac_Edge].get(), c, 'l'); //vytvoril som novy vrchol
		vrchol->potomok[ActivePoint_.ac_Edge]->intervalyHranL.insert({ c,  make_pair(index_c, ukazovatelNaEnd) }); //priradil mu hodnoty na hranu

	}

	else { //ak hrana ktoru rozbijame vedie do vnutorneho vrcholu
		p1.first = vrchol->intervalyHranV[ActivePoint_.ac_Edge].first; //interval prvej casti
		p1.second = p1.first + ActivePoint_.ac_Length - 1;

		p2.first = p1.second + 1; //interval druhej ("uz znamej") casti
		p2.second = vrchol->intervalyHranV[ActivePoint_.ac_Edge].second;


		//ak pouzijeme '$' ako pomocny, tak si zvacsime moznost pouzitej abecedy o nejaky 1 znak, ale suffixovy strom sa bude chovat blbo
		//preto sme pouzili '#' a teda $ a # budu zakazane znaky (nemozno pouzit v stringoch)
		//# mi sluzi ako docasny znak, pod ktorym si ulozim potomka pretoze je vnutorny, takze ho nemozem vymazat a nanovo spravit
		//ak by tam bol $, tak by sa to mohlo rozbit napriklad pri 2 stringoch "abcabcabc" a "abca", kde by sa nevypisal uz ziadny zo suffixov cabc (ten by bol posledny z prveho).
		//prisli by sme totiz o informaciu kedze tam uz nejake suffixy koncia
		const char newKey = '#';
		const auto it = vrchol->potomok.find(ActivePoint_.ac_Edge);
		swap(vrchol->potomok[newKey], it->second);
		vrchol->potomok.erase(it);

		pridajNovuHranu(vrchol, ActivePoint_.ac_Edge, 'v');
		vrchol->intervalyHranV[ActivePoint_.ac_Edge] = p1;
		vrchol->potomok['#']->rodic = vrchol->potomok[ActivePoint_.ac_Edge].get(); //zmena rodica stareho vrcholu na novy


		vrchol->potomok[ActivePoint_.ac_Edge]->intervalyHranV[concString[p2.first]] = p2; //robi to to iste ako insert
		vrchol->potomok[ActivePoint_.ac_Edge]->potomok[concString[p2.first]] = move(vrchol->potomok['#']); //presunieme do noveho vrchola
		vrchol->potomok.erase('#'); //vymazeme z povodneho

		pridajNovuHranu(vrchol->potomok[ActivePoint_.ac_Edge].get(), c, 'l'); //vytvoril som novy vrchol z prave nacitaneho znaku (ktory nesiel nikde "pripisat")
		vrchol->potomok[ActivePoint_.ac_Edge]->intervalyHranL.insert({ c,  make_pair(index_c, ukazovatelNaEnd) }); //priradil mu hodnoty na hranu


	}
}

void SuffixTree::updateAc_point(unsigned int i_Spr) {
	//ak je dlzka rovna dlzke intervalu, tak iba skoc na dalsi vrchol a vynuluj sa. (ac_edge != NULL kvoli tomu aby metoda nebuchla, ked do roota skocim s tym, ze ac_Edge uz bude NULL).
	auto it = ActivePoint_.ac_Node->potomok.find(ActivePoint_.ac_Edge); //ak tam taka hrana neexistuje, tak nerob nic (nie je kde sa posuvat)
	if (it == ActivePoint_.ac_Node->potomok.end())
		return;
	if (ActivePoint_.ac_Edge == NULL)
		return;
	if ((*(*ActivePoint_.ac_Node).potomok[ActivePoint_.ac_Edge]).type == 'l')
		return;



	else if (ActivePoint_.ac_Length == (ActivePoint_.ac_Node->intervalyHranV[ActivePoint_.ac_Edge].second - ActivePoint_.ac_Node->intervalyHranV[ActivePoint_.ac_Edge].first + 1)) {
		ActivePoint_.ac_Length = 0;
		ActivePoint_.ac_Node = ActivePoint_.ac_Node->potomok[ActivePoint_.ac_Edge].get();
		ActivePoint_.ac_Edge = NULL;
	}

	//ak je dlzka vacsia ako interval, tak rekurzivne chod dalej (moze nastat pri skoku cez suffix link), ale vrchol do ktoreho ides musi byt vnutorny. Ak je to list, tak ani sa nemusis namahat, pretoze nikdy nevieme "predbehnut" list
	else if ((ActivePoint_.ac_Node->potomok[ActivePoint_.ac_Edge]->type == 'v') && (ActivePoint_.ac_Length > (ActivePoint_.ac_Node->intervalyHranV[ActivePoint_.ac_Edge].second - ActivePoint_.ac_Node->intervalyHranV[ActivePoint_.ac_Edge].first + 1))) {
		ActivePoint_.ac_Length -= (ActivePoint_.ac_Node->intervalyHranV[ActivePoint_.ac_Edge].second - ActivePoint_.ac_Node->intervalyHranV[ActivePoint_.ac_Edge].first + 1);
		ActivePoint_.ac_Node = ActivePoint_.ac_Node->potomok[ActivePoint_.ac_Edge].get(); //nastav novy vrchol
		ActivePoint_.ac_Edge = concString[i_Spr - ActivePoint_.ac_Length]; //nastav spravnu hranu
		updateAc_point(i_Spr); //rekurzivne sa zanor tak, aby ActivnePoint_.ac_Length bola nastavena spravne
	}
}

bool SuffixTree::mozemPridat(unsigned int i_Spr) {
	if (ActivePoint_.ac_Length == 0) { //ak som vo vrchole (teda niesom v polovicke hrany)
		auto it = ActivePoint_.ac_Node->potomok.find(concString[i_Spr]);
		if (it == ActivePoint_.ac_Node->potomok.end()) {
			return false;
		}
		else {
			ActivePoint_.ac_Edge = concString[i_Spr]; //musim nastavit spravnu hranu, nakolko ak som sem vstupil, tak je ac_Edge == NULL
			return true;
		}

	}
	else {
		unsigned int index;
		//najprv potrebujem zistit, ci vchadzam do vnutorneho vrcholu alebo do listu; potom zvolim pismeno, ktore nasleduje za length

		
		if (ActivePoint_.ac_Node->potomok[ActivePoint_.ac_Edge]->type == 'l')
			index = ActivePoint_.ac_Node->intervalyHranL[ActivePoint_.ac_Edge].first + ActivePoint_.ac_Length;
		else
			index = ActivePoint_.ac_Node->intervalyHranV[ActivePoint_.ac_Edge].first + ActivePoint_.ac_Length;

		if (concString[index] == concString[i_Spr]) //zistujem ci sa pismena rovnaju. Ak ano, tak viem pridat, ak nie, musim rozbit hranu
			return true;
		else
			return false;
	}

}

void SuffixTree::rekurzivneVyriesSuffixy(unsigned int i_Spr) {
	
	if (i_Spr > 1 && (imInID_ || concString[i_Spr - remainder_] == '$')) {
		imInID_ = true;
		firstDollarSeen = true;
		remainder_ = 0;
		numBrokenEdgesThisTurn = 0;
		lastBrokenNodeThisTurn = nullptr;
		return;
	}
	if (mozemPridat(i_Spr) == true) { //bazicka podmienka rekurzie -> ak viem pridat na hranu dany znak, tak ho implicitne "pridam" a return
		++ActivePoint_.ac_Length;
		updateAc_point(i_Spr);
		numBrokenEdgesThisTurn = 0;
		lastBrokenNodeThisTurn = nullptr;
		return;
	}
	else {
		if (ActivePoint_.ac_Length == 0) { //ak som vo vrchole a nie v hrane, tak pridaj novy list (novu hranu) z prave nacitaneho pismena   
			pridajNovuHranu(ActivePoint_.ac_Node, concString[i_Spr], 'l');
			(*ActivePoint_.ac_Node).intervalyHranL.insert({ concString[i_Spr], make_pair(i_Spr, &ENDy[indexEndov]) });
			if (remainder_ == 1) {	//bazicka podmienka rek. -> ak som v koreni a z posledneho znaku suff (cize prave spracovavany)
				--remainder_;		//som vytvoril hranu, tak skoncim rekurziu.
				numBrokenEdgesThisTurn = 0;
				lastBrokenNodeThisTurn = nullptr;
				return;
			}
		}
		else { //ak je vsak nutne rozbit hranu, tak ju rozbi a aplikuj patricne pravidla ktore sa aplikuju pri rozbijani hrany
			rozbiHranu(ActivePoint_.ac_Node, concString[i_Spr], i_Spr);
			if (numBrokenEdgesThisTurn >= 1) {
				lastBrokenNodeThisTurn->suffixLink = ActivePoint_.ac_Node->potomok[ActivePoint_.ac_Edge].get();
			}
			++numBrokenEdgesThisTurn;
			lastBrokenNodeThisTurn = ActivePoint_.ac_Node->potomok[ActivePoint_.ac_Edge].get();
		}
		--remainder_;
		if (ActivePoint_.ac_Node->suffixLink != nullptr) { //ak obsahujem suffixLink, tak sa po nom pustim a rekurzivne dalej vytvaram suff. strom
			ActivePoint_.ac_Node = ActivePoint_.ac_Node->suffixLink;
			updateAc_point(i_Spr);
			rekurzivneVyriesSuffixy(i_Spr);
		}
		else if (ActivePoint_.ac_Node == &root) {	//predtym nez zistim ze neobsahujem suffLink a skocim do roota zistim ci niesom v roote.
													//to kvoli tomu, aby som sa potom nezacyklil, lebo root ma tiez nullptr ako suffixLink
			--ActivePoint_.ac_Length;
			ActivePoint_.ac_Edge = concString[i_Spr - remainder_ + 1];
			updateAc_point(i_Spr);
			rekurzivneVyriesSuffixy(i_Spr);
		}
		else { //else if ((*ActivePoint_.ac_Node).suffixLink == nullptr)
			ActivePoint_.ac_Node = &root;
			
			if (remainder_ != 1) {
				ActivePoint_.ac_Length = remainder_ - 1;
				ActivePoint_.ac_Edge = concString[i_Spr - remainder_ + 1];
			}
			

			
			updateAc_point(i_Spr);
			rekurzivneVyriesSuffixy(i_Spr);
		}
	}
}

void SuffixTree::insert(const std::string& Slovo) {


	
	++NumOfStrings_;
	concString += Slovo;
	concString += "$";
	concString += to_string(NumOfStrings_ - 1);

	remainder_ = 0;
	imInID_ = false;
	firstDollarSeen = false;
	ActivePoint_.ac_Edge = NULL;
	ActivePoint_.ac_Length = 0;
	ActivePoint_.ac_Node = &root;


	for (; i < concString.size(); ++i) {	//prejdeme cez cely skonkatenovany string a postupne buildujeme suffixovy strom
		++remainder_;										//nacitali sme novy znak ktory treba priradit k suffixom
		++ENDy[indexEndov];									//vsetkym listom ho mozem ihned priradit


		if (i > 1 && (imInID_ || concString[i - remainder_] == '$')) {
			imInID_ = true;
			firstDollarSeen = true;
			remainder_ = 0;
		}

		else if (remainder_ == 1) {						//ak to plati, tak musim byt v koreni
			auto it = root.potomok.find(concString[i]);		//snazim sa pridat novonacitany znak
			if (it == root.potomok.end()) {					//ak neexistuje hrana s rovnakym znakom
				pridajNovuHranu(&root, concString[i], 'l');	//tak ju vytvor
				root.intervalyHranL.insert({ concString[i], make_pair(i, &ENDy[indexEndov]) }); //pridame zaznam do intervalovHranL
				--remainder_;								//a odpocitaj remainder (tzn. ze sme novy suffix spracovali)
			}
			else {											//naopak, ak hrana existuje
				ActivePoint_.ac_Node = &root;				//spravne nastav ActivePoint
				ActivePoint_.ac_Edge = concString[i];
				ActivePoint_.ac_Length = 1;
				updateAc_point(i);							//a ak nahodou ho bude treba upravit, tak ho uprav
			}												//s remainderom nerob nic (nespracovali sme suffix)
		}

		else {
			rekurzivneVyriesSuffixy(i);
		}

		//sluzi na to, ked skonci jeden string (som na poslednom znaku v ID v spracovavanom stringu) a potrebujem spravit spravne ukoncenie stringu, aby som nezapisoval dalej suffixy zbytocne
		if (i > 1 && concString[i - to_string(indexEndov).size()] == '$' && firstDollarSeen) { //spustame az po ulozeni $ a ID vsade tam, kde ho je treba na ukoncenie. Potom len na listy pripojime to co treba.
																							   
																							   
			ENDy.push_back(ENDy[indexEndov]);
			++indexEndov;

			
		}
	}



}

void SuffixTree::prepareAllSuffixes(const Node& vrchol, const string& prechodnyString, bool Sorted) {
	if (NumOfStrings_ == 0)
		throw EmptyTreeException();

	for (auto it = vrchol.potomok.begin(); it != vrchol.potomok.end(); ++it) {
		if ((*(*it).second).type == 'v') {
			string vstupnyString = prechodnyString + concString.substr(vrchol.intervalyHranV.at(it->first).first, vrchol.intervalyHranV.at(it->first).second - vrchol.intervalyHranV.at(it->first).first + 1);
			prepareAllSuffixes((*(*it).second), vstupnyString, Sorted);
		}
		else {
			string pomString = concString.substr(vrchol.intervalyHranL.at(it->first).first, (*(vrchol.intervalyHranL.at(it->first).second)) - vrchol.intervalyHranL.at(it->first).first + 1);
			if (Sorted == false)
				cout << prechodnyString << pomString << "\n";
			else {
				auto p = vystupSuffixov.insert(prechodnyString + pomString);
			}
		}
	}
}

void SuffixTree::printAllSufixes(bool Sorted) {
	try
	{
		prepareAllSuffixes(root, "", Sorted);
	}
	catch (...)
	{
		throw;
	}

	if (Sorted == true) {
		for (auto it : vystupSuffixov)
			cout << it << endl;
	}

	vystupSuffixov.clear();
}

unsigned int SuffixTree::nextChar() {
	if (ActivePoint_.ac_Edge == NULL)
		return 4294967295; //MAX UINT
	else {
		if (ActivePoint_.ac_Node->potomok[ActivePoint_.ac_Edge]->type == 'l') {
			return ActivePoint_.ac_Node->intervalyHranL[ActivePoint_.ac_Edge].first + ActivePoint_.ac_Length - 1;
		}
		else
			return ActivePoint_.ac_Node->intervalyHranV[ActivePoint_.ac_Edge].first + ActivePoint_.ac_Length - 1;
	}
}

bool SuffixTree::isSubstring(const std::string& substr) {
	if (substr.size() == 0)
		return true;
	else {
		auto it = root.potomok.find(substr[0]);
		if (it == root.potomok.end())
			return false;
		else {
			ActivePoint_.ac_Length = 1;
			ActivePoint_.ac_Edge = substr[0];
			ActivePoint_.ac_Node = &root;
			for (unsigned int i = 0; i < substr.size(); ++i) {
				if (ActivePoint_.ac_Edge == NULL) {
					it = ActivePoint_.ac_Node->potomok.find(substr[i]);
					if (it == ActivePoint_.ac_Node->potomok.end())
						return false;
					else {
						ActivePoint_.ac_Length = 1;
						ActivePoint_.ac_Edge = substr[i];
					}
				}
				unsigned int index = nextChar();
				if (concString[index] == substr[i]) {
					updateAc_point(index);
					++ActivePoint_.ac_Length;
					if (ActivePoint_.ac_Node->type == 'l' && (i + 1 != substr.size()))
						return false;
				}
				else
					return false;

			}
			return true;
		}
	}
}

string SuffixTree::LCS() {
	if (NumOfStrings_ == 0)
		throw EmptyTreeException();
	
	DFS_pre_LCS(&root);
	DFS2_pre_LCS(&root, "");
	return longest_common_substring;

}

std::unordered_set<unsigned int> SuffixTree::DFS_pre_LCS(Node * children) {
	if (children->type == 'l') {
		unordered_set<unsigned int> mnozina{ children->get_ID() };
		return mnozina;
	}
	else {
		queue<Node*> struktura_na_vrcholy;
		for (auto it = children->potomok.begin(); it != children->potomok.end(); ++it) {
			struktura_na_vrcholy.push(it->second.get());
		}
		unordered_set<unsigned int> mnozina;
		while (struktura_na_vrcholy.size() != 0) {
			mnozina = union_sets<unordered_set<unsigned int>>(DFS_pre_LCS(struktura_na_vrcholy.front()), mnozina);
			struktura_na_vrcholy.pop();
		}
		if (mnozina.size() == indexEndov)
			set_containAllStrings(true, children);
		return mnozina;
	}

}

void SuffixTree::DFS2_pre_LCS(Node * children, string prechodnyStr) {
	if (children->get_containAllStrings() == false)
		return;
	else {
		queue< pair<char, Node* > > struktura_na_vrcholy;
		for (auto it = children->potomok.begin(); it != children->potomok.end(); ++it) {
			if (it->second->type == 'v')
				struktura_na_vrcholy.push(make_pair(it->first, it->second.get()));
		}
		while (struktura_na_vrcholy.size() != 0) {
			DFS2_pre_LCS(struktura_na_vrcholy.front().second, prechodnyStr + concString.substr(children->intervalyHranV[struktura_na_vrcholy.front().first].first, children->intervalyHranV[struktura_na_vrcholy.front().first].second - children->intervalyHranV[struktura_na_vrcholy.front().first].first + 1));
			struktura_na_vrcholy.pop();
		}
		if (longest_common_substring.size() < prechodnyStr.size() || (longest_common_substring == "$" && prechodnyStr != "$"))
			longest_common_substring = prechodnyStr;
		return;
	}
}



