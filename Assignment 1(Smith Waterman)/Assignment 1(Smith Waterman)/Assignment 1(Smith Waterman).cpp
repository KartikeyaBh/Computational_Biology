// Assignment 1(Smith Waterman).cpp : Defines the entry point for the console application.
//

// Implements Smith Waterman
#include "stdafx.h"
#include <iostream>
#include <windows.h>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <ctime>
#include <ppl.h>
#include <ctype.h>
#include <map>
using namespace std;
using namespace concurrency;

// initialize Blosum 62 matrix as static
static int blosum62[20][20] = {
    { 4, -1, -2, -2,  0, -1, -1,  0, -2, -1, -1, -1, -1, -2, -1,  1,  0, -3, -2,  0 },
    { -1,  5,  0, -2, -3,  1,  0, -2,  0, -3, -2,  2, -1, -3, -2, -1, -1, -3, -2, -3 },
    { -2,  0,  6,  1, -3,  0,  0,  0,  1, -3, -3,  0, -2, -3, -2,  1,  0, -4, -2, -3 },
    { -2, -2,  1,  6, -3,  0,  2, -1, -1, -3, -4, -1, -3, -3, -1,  0, -1, -4, -3, -3 },
    { 0, -3, -3, -3,  9, -3, -4, -3, -3, -1, -1, -3, -1, -2, -3, -1, -1, -2, -2, -1 },
    { -1,  1,  0,  0, -3,  5,  2, -2,  0, -3, -2,  1,  0, -3, -1,  0, -1, -2, -1, -2 },
    { -1,  0,  0,  2, -4,  2,  5, -2,  0, -3, -3,  1, -2, -3, -1,  0, -1, -3, -2, -2 },
    { 0, -2,  0, -1, -3, -2, -2,  6, -2, -4, -4, -2, -3, -3, -2,  0, -2, -2, -3, -3 },
    { -2,  0,  1, -1, -3,  0,  0, -2,  8, -3, -3, -1, -2, -1, -2, -1, -2, -2,  2, -3 },
    { -1, -3, -3, -3, -1, -3, -3, -4, -3,  4,  2, -3,  1,  0, -3, -2, -1, -3, -1,  3 },
    { -1, -2, -3, -4, -1, -2, -3, -4, -3,  2,  4, -2,  2,  0, -3, -2, -1, -2, -1,  1 },
    { -1,  2,  0, -1, -3,  1,  1, -2, -1, -3, -2,  5, -1, -3, -1,  0, -1, -3, -2, -2 },
    { -1, -1, -2, -3, -1,  0, -2, -3, -2,  1,  2, -1,  5,  0, -2, -1, -1, -1, -1,  1 },
    { -2, -3, -3, -3, -2, -3, -3, -3, -1,  0,  0, -3,  0,  6, -4, -2, -2,  1,  3, -1 },
    { -1, -2, -2, -1, -3, -1, -1, -2, -2, -3, -3, -1, -2, -4,  7, -1, -1, -4, -3, -2 },
    { 1, -1,  1,  0, -1,  0,  0,  0, -1, -2, -2,  0, -1, -2, -1,  4,  1, -3, -2, -2 },
    { 0, -1,  0, -1, -1, -1, -1, -2, -2, -1, -1, -1, -1, -2, -1,  1,  5, -2, -2,  0 },
    { -3, -3, -4, -4, -2, -2, -3, -2, -2, -3, -2, -3, -1,  1, -4, -3, -2, 11,  2, -3 },
    { -2, -2, -2, -3, -2, -1, -2, -3,  2, -1, -1, -2, -1,  3, -3, -2, -2,  2,  7, -1 },
    { 0, -3, -3, -3, -1, -2, -2, -3, -3,  3,  1, -2,  1, -1, -2, -2,  0, -3, -1,  4 } };

static int getIndexBlosum(char a) {
    // check for upper and lowercase characters
    switch (toupper(a)) {
    case 'A': return 0;
    case 'R': return 1;
    case 'N': return 2;
    case 'D': return 3;
    case 'C': return 4;
    case 'Q': return 5;
    case 'E': return 6;
    case 'G': return 7;
    case 'H': return 8;
    case 'I': return 9;
    case 'L': return 10;
    case 'K': return 11;
    case 'M': return 12;
    case 'F': return 13;
    case 'P': return 14;
    case 'S': return 15;
    case 'T': return 16;
    case 'W': return 17;
    case 'Y': return 18;
    case 'V': return 19;
    default: return 0;
    }
}


// Calls the provided work function and returns the number of milliseconds 
// that it takes to call that function.
template <class Function>
DWORD time_call(Function&& f)
{
    DWORD begin = GetTickCount();
    f();
    return GetTickCount() - begin;
}

// Define our smithWaterman class
class SmithWaterman
{
    // Specify the direction enum.
    // This will be useful for traceback. Each cell in the dp would have a direction where it takes the max from.
    enum class direction : int
    {
        up = 0,
        diagonal = 1,
        left = 2
    };

public:

    // Constructor of our class.
    // It takes as input the two protein sequences.
    SmithWaterman(string protein1, string protein2):
    m_protein1(protein1),
    m_protein2(protein2)
    {
        // Initialize the memoization matrix
        m_dp = vector<vector<int>>(m_protein1.length() + 1, vector<int>(m_protein2.length() + 1, 0));
        m_traceback = vector<vector<int>>(m_protein1.length() + 1, vector<int>(m_protein2.length() + 1, 0));
    }

    // Public functions to perform various tasks related to smithWaterman.

    // Compute the score of the local alignment
    int ComputeScore();

    // Print the optimal alignment using the traceback algorithm
    void PrintOptimalAlignment();

    // Calculate the empirical p value by calculating the score "count" times.
    void CalculateEmpiricalPValue(int count = 999);

    // Userful in displaying the output in Blast format.
    inline void SetAccessionQuery(string accession) { m_accession1 = accession;}
    inline void SetAccessionSubject(string accession) { m_accession2 = accession; }

private:
    // Returns the similarity score between two proteins.
    int similarityScore(int i, int j)
    {
        return blosum62[getIndexBlosum(m_protein1[i])][getIndexBlosum(m_protein2[j])];
    }

    // Generates a permutation for a given sequence.
    void GeneratePermutation(string& s)
    {
        for (int i = s.length() - 1; i > 0; i--)
        {
            int usableRandomMax = ((RAND_MAX) / (i + 1));
            usableRandomMax = usableRandomMax * (i+1);
            int randomIndex = RAND_MAX;
            while (randomIndex > usableRandomMax) { randomIndex = rand();}
            randomIndex = randomIndex % (i+1);
            swap(s[i], s[randomIndex]);
        }
    }

    int penalty() { return -4;}

    int findMax(vector<int> v, pair<int, int> index)
    {
        int result = 0;
        int dir = -1;
        for (unsigned int i = 0; i < v.size(); i++)
        {
            if (result < v[i])
            {
                result = v[i];
                m_traceback[index.first][index.second] = i;
            }
        }
        return result;
    }

private:
    string m_protein1;
    string m_protein2;
    string m_accession1 = {};
    string m_accession2 = {};
    vector<vector<int>> m_dp;
    vector<vector<int>> m_traceback;
    pair<int,int> m_resultIndices;
    int m_score;
    int dir[3][2] = {{-1,0}, {-1,-1}, {0,-1}};
};

int SmithWaterman::ComputeScore()
{
    int length1 = m_protein1.length();
    int length2 = m_protein2.length();

    // Calculate the score matrix
    for (auto i = 1; i <= length1; i++)
    {
        for (auto j = 1; j <= length2; j++)
        {
            vector<int> v = { m_dp[i - 1][j] + penalty(), m_dp[i - 1][j - 1] + similarityScore(i - 1,j - 1), m_dp[i][j - 1] + penalty(), 0 };
            m_dp[i][j] = findMax(v, pair<int,int>(i,j));
        }
    }

    // If you want to print the score matrix uncomment the code and compile

    // print the scoring matrix
    //for (auto i = 0; i <= length1; i++)
    //{
    //    for (auto j = 0; j <= length2; j++)
    //    {
    //        cout << m_dp[i][j]<< " ";
    //    }
    //    cout<<endl;
    //}

    // find the max
    auto result = 0;
    for (auto i = 0; i <= length1; i++)
    {
        for (auto j = 0; j <= length2; j++)
        {
            if (result < m_dp[i][j])
            {
                result = m_dp[i][j];
                m_resultIndices.first = i;
                m_resultIndices.second = j;
            }
        }
    }

    // Store the score. This can be useful in calculating empirical value.
    m_score = result;

    return result;
}

void SmithWaterman::PrintOptimalAlignment()
{
    string result1;
    int row = m_resultIndices.first, col = m_resultIndices.second;

    // Start indices of the local alignment
    int startIndex1 = 0, startIndex2 = 0;

    // Calculate the string for protein 1.
    while (m_dp[row][col] != 0)
    {
        int direction = m_traceback[row][col];
        if(direction == 0 || direction == 1)
        {
            result1 += m_protein1[row-1];
        }
        else
        {
            result1 += '-';
        }

        row += dir[direction][0];
        col += dir[direction][1];

        startIndex1 = row;
        startIndex2 = col;
    }

    // Calculate the string for protein2
    string result2;
    row = m_resultIndices.first, col = m_resultIndices.second;
    while (m_dp[row][col] != 0)
    {
        int direction = m_traceback[row][col];
        if (direction == 2 || direction == 1)
        {
            result2 += m_protein2[col - 1];
        }
        else
        {
            result2 += '-';
        }

        row += dir[direction][0];
        col += dir[direction][1];
    }

    {
        // Reverse as the strings as we had appended them in a bottom up manner
        reverse(result1.begin(), result1.end());
        reverse(result2.begin(), result2.end());

        // Calculate the middle row
        // It is the character if character matches
        // '+' if blosum score is positive
        // gap otherwise
        string insight;
        for (int i = 0; i < result1.length(); i++)
        {
            if (result1[i] == result2[i])
            {
                //cout << blosum62[getIndexBlosum(result1[i])][getIndexBlosum(result2[i])];
                insight += result1[i];
            }
            else if (blosum62[getIndexBlosum(result1[i])][getIndexBlosum(result2[i])] > 0)
            {
                //cout<< blosum62[getIndexBlosum(result1[i])][getIndexBlosum(result2[i])];
                insight += "+";
            }
            else
            {
                insight += " ";
            }
        }

        // Print everything in the blast format.
        int pos1 = 0, pos2 = 0, pos3 = 0;
        while (pos1 < result1.length() && pos2 < result2.length() && pos3 < insight.length())
        {
            string intro1 = m_accession1 + " :    " + to_string(startIndex1 + 1);
            for (int i = intro1.length(); i < 20; i++) intro1 += " ";
            cout<<intro1;
            for (int i = 0; i < 60 && pos1 < result1.length(); i++)
            {
                if (result1[pos1] != '-')
                {
                    startIndex1++;
                }
                cout<<result1[pos1++];
            }
            cout<<endl;

            for (int  i = 0; i < 20 ; i++) { cout<<" ";}
            for (int i = 0; i < 60 && pos3 < insight.length(); i++)
            {
                cout << insight[pos3++];
            }
            cout<<endl;

            string intro2 = m_accession2 + " :    " + to_string(startIndex2 + 1);
            for (int i = intro2.length(); i < 20; i++) intro2 += " ";
            cout << intro2;
            for (int i = 0; i < 60 && pos2 < result2.length(); i++)
            {
                if (result2[pos2] != '-')
                {
                    startIndex2++;
                }
                cout << result2[pos2++];
            }
            cout << endl<<endl;
        }
    }

    // Comment this out if you dont want to print it.
    if (m_protein1.length() < 15 && m_protein2.length() < 15)
    {
        // print the scoring matrix
        for (auto i = 0; i <= m_protein1.length(); i++)
        {
            for (auto j = 0; j <= m_protein2.length(); j++)
            {
                cout << m_dp[i][j]<< " ";
            }
            cout<<endl;
        }
    }
}

void SmithWaterman::CalculateEmpiricalPValue(int count)
{
    cout<<endl<<count<<endl;

    double greaterScore = 0;
    // We just searched protein 1 and found protein 2 to be a good match.
    // Use the for_each algorithm to compute the results parallely.

    // We use visual studio parallel program library.
    DWORD elapsedParallel;
    elapsedParallel = time_call([&]
    {
        parallel_for(0, count, [&](int i) {
            string permutation = m_protein2;
            GeneratePermutation(m_protein2);
            SmithWaterman smithWaterman(m_protein1, permutation);
            if (m_score <= smithWaterman.ComputeScore())
            {
                greaterScore++;
            }
        });
    });
    wcout << L"parallel time: " << elapsedParallel << L" ms" << endl;

    //DWORD elapsedSerial;
    //elapsedSerial = time_call([&]
    //{
    //    for(int i = 0; i < count; i++) {
    //        string permutation = m_protein2;
    //        GeneratePermutation(m_protein2);
    //        SmithWaterman smithWaterman(m_protein1, permutation);
    //        smithWaterman.ComputeScore();
    //    }
    //});
    //wcout << L"serial time: " << elapsedSerial << L" ms" << endl;

    double pValue = (greaterScore + 1) / static_cast<double>(count + 1);
    cout<<endl<<"Empirical pValue: "<<pValue<<endl;
}

string FastaDecoder(ifstream& input)
{
    std::string line, name, content;
    while (std::getline(input, line).good()) {
        if (line.empty() || line[0] == '>') { // Identifier marker
            if (!name.empty()) { // Print out what we read from the last entry
                std::cout << name << " : " << content << std::endl;
                name.clear();
            }
            if (!line.empty()) {
                name = line.substr(1);
            }
            content.clear();
        }
        else if (!name.empty()) {
            if (line.find(' ') != std::string::npos) { // Invalid sequence--no spaces allowed
                name.clear();
                content.clear();
            }
            else {
                content += line;
            }
        }
    }
    //if (!name.empty()) { // Print out what we read from the last entry
    //    std::cout << name << endl<< content << std::endl;
    //}

    return content;
}

int PerformTask(string accession1, string accession2, bool fCalculateEmpirical = false)
{
    ifstream inFileProtein1, inFileProtein2;
    /*cout << "Finding the sequence of " << accession1 << "....." << endl << endl;*/
    inFileProtein1.open(accession1 + ".fasta");
    string aminoAcidSequence1 = FastaDecoder(inFileProtein1);

    /*cout << endl << endl << "Finding the sequence of " << accession2 << "....." << endl << endl;*/
    inFileProtein2.open(accession2 + ".fasta");
    string aminoAcidSequence2 = FastaDecoder(inFileProtein2);

    SmithWaterman smithWaterman(aminoAcidSequence1, aminoAcidSequence2);
    smithWaterman.SetAccessionQuery(accession1);
    smithWaterman.SetAccessionSubject(accession2);

    int score = smithWaterman.ComputeScore();
    cout << endl <<"Score: "<< score << endl<<endl;
    smithWaterman.PrintOptimalAlignment();
    if (fCalculateEmpirical)
    {
        smithWaterman.CalculateEmpiricalPValue(9999);
    }

    cout << endl;
    return score;
}

int main()
{
    // This is to set system clock and needs to be done once per program execution
    std::srand(std::time(0));

    cout<<"Do you want to enter two accession numbers or results for all n(n+1)?"<<endl;
    cout<<"0: for accession numbers(empirical value calculated)"<<endl<<"1: for everything(empirical value skipped"<<endl<<"2: for protein strings(empirical value calculated)"<<endl;

    int choice = {-1};
    cin>>choice;
    if( choice == 0)
    {
        // initialize the protein sequences
        cout<<"Input the accession numbers of the two protein sequences:"<<endl;
        string protein1;
        string protein2;
        cin>>protein1>>protein2;

        PerformTask(protein1, protein2, true);
    }
    else if(choice == 1)
    {
        ifstream inFile;
        inFile.open("Proteins.txt");
        vector<string> proteinList;
        string proteinAccession;
        while (getline(inFile, proteinAccession).good())
        {
            proteinList.emplace_back(proteinAccession);
        }

        auto scoreMatrix = vector<vector<int>>(proteinList.size(), vector<int>(proteinList.size() + 1, 0));
        for (int i = 0; i < proteinList.size(); i++)
        {
            for (int j = i + 1; j < proteinList.size(); j++)
            {
                cout<<endl<<proteinList[i]<<"        "<<proteinList[j]<<endl;
                scoreMatrix[i][j] = PerformTask(proteinList[i], proteinList[j]);
                cout<<"*********************************************"<<endl;
            }
        }

        for (int i = 0; i < proteinList.size(); i++)
        {
            for(int j = 0; j< proteinList.size(); j++)
                cout<<scoreMatrix[i][j]<<"  ";

            cout<<endl;
        }
    }
    else if (choice == 2)
    {
        string protein1;
        string protein2;
        cin >> protein1 >> protein2;

        SmithWaterman smithWaterman(protein1, protein2);
        int score = smithWaterman.ComputeScore();
        cout << endl << "Score: " << score << endl << endl;

        smithWaterman.PrintOptimalAlignment();
        smithWaterman.CalculateEmpiricalPValue(999);
    }

    getchar();
    getchar();
    return 0;
}

