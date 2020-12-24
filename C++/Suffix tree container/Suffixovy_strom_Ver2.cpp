#include <iostream>
#include <string>
#include "Suffix_Tree.h"
#include <unordered_set>
#include <algorithm>
#include <chrono>

using namespace std;



int main()
{
	
	//auto start = chrono::high_resolution_clock::now();
	SuffixTree strom;

	string line;
	while (cin >> line)
	{
	strom.insert(line);
	}
	//auto stop = chrono::high_resolution_clock().now();
	
	
	

	cout << strom.LCS() << '\n';

	
	
	
	//auto duration = chrono::duration_cast<chrono::milliseconds>(stop - start);
	//cout << duration.count() << endl;



	//cout << strom.get_concString().size();

	//auto start2 = chrono::high_resolution_clock::now();
	
	/*try
	{
		strom.printAllSufixes();
	}
	catch (const std::exception& e)
	{
		cout << e.what() << endl;
	}*/

	//auto stop2 = chrono::high_resolution_clock().now();
	//auto duration2 = chrono::duration_cast<chrono::milliseconds>(stop2 - start2);


	//cout << strom.isSubstring("orbi") << endl;
	//cout << strom.get_concString().size() << endl << endl;

	

	/*try
	{
	strom.printAllSufixes(true);
	}
	catch (const std::exception& e)
	{
	cout << e.what() << endl;
	}*/

	//cout << duration2.count() << endl;

	SuffixTree strom2(strom);
	strom2.insert("Suspendis");
	strom2.insert("diar");
	strom2.insert("mattes");
	try {
		strom2.printAllSufixes(true);
	}
	catch (const std::exception& e) {
		cout << e.what() << endl;
	}

	return 0;
}

