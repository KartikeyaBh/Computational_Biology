// Viterbi_Algorithm.cpp : Defines the entry point for the console application.
//

#define _USE_MATH_DEFINES

#include "stdafx.h"
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <cmath>
#include <ctime>
#include <algorithm>
#include <vector>
#include <map>
using namespace std;

void FormatFna(string& content)
{
    for (int i = 0; i < content.length(); i++)
    {
        char& contenti = content[i];
        if (contenti != 'a' && contenti != 'g' && contenti != 'c' && contenti != 't')
        {
            content[i] = 't';
        }
    }
}

string ReadGenomeFromFna()
{
    ifstream inFile;
    inFile.open("GCF_000091665.1_ASM9166v1_genomic.fna");

    std::string line, content;
    int count = 0;
    while (std::getline(inFile, line).good() && count <= 1) 
    {
        if (line[0] != '>')
        {
            content += line;
        }
        else
        {
            count++;
        }
    }
    std::transform(content.begin(), content.end(), content.begin(), ::tolower);
    FormatFna(content);
    return content;
}

class Viterbi
{
public:
    Viterbi(string genome):
        m_Genome(genome)
    {
        m_Emissions['a'] = {0.25, 0.20};
        m_Emissions['g'] = {0.25, 0.30};
        m_Emissions['c'] = {0.25, 0.30};
        m_Emissions['t'] = {0.25, 0.20};
    }

    void StartTask();

private:

    void ComputeViterbiPath();
    void ViterbiTrain();
    void ViterbiPath();

private:
    string m_Genome;
    string m_ViterbiPath;
    map<char, vector<double>> m_Emissions;
    vector<vector<double>> m_Transitions = {{0.9999,0.0001}, {0.01, 0.99}};
    vector<vector<double>> m_dp;
    vector<vector<double>> m_Traceback;
};

void Viterbi::StartTask()
{
    int numScans = 0;
    while(numScans < 10)
    {
        ComputeViterbiPath();
        ViterbiPath();
        ViterbiTrain();
        numScans++;
    }
}

void Viterbi::ComputeViterbiPath()
{
    // Initialize our memo
    m_dp = vector<vector<double>>(2, vector<double>(m_Genome.length() + 1));
    m_Traceback = vector<vector<double>>(2, vector<double>(m_Genome.length() + 1));

    // Begin state has probability 1
    m_dp[0][0] = log(0.9999 * m_Emissions[m_Genome[0]][0]);
    m_dp[1][0] = log(0.0001 * m_Emissions[m_Genome[0]][1]);

    // Iterate over position of the whole genome
    for (int position = 1; position < m_Genome.length(); position++)
    {
        // Iterate over all the states to find the maximum path ending in "state"
        for (int endState = 0; endState < 2; endState++)
        {
            int maxState = -1;
            double maxStateValue = INT_MIN;
            for (int prevState = 0; prevState < 2; prevState++)
            {
                double tempStateValue = m_dp[prevState][position - 1] + 
                                            log(m_Transitions[prevState][endState] * m_Emissions[m_Genome[position]][endState]);
                if (tempStateValue > maxStateValue)
                {
                    maxStateValue = tempStateValue;
                    maxState = prevState;
                }
            }

            // The max probability of ending at this state.
            m_dp[endState][position] = maxStateValue;
            m_Traceback[endState][position] = maxState;
        }
    }
}

void Viterbi::ViterbiTrain()
{
    // Count transitions
    // count transitions for 0 to 0,1
    vector<double> temp = {0, 0};
    m_Emissions['a'] = temp, m_Emissions['g'] = temp, m_Emissions['c'] = temp, m_Emissions['t'] = temp;
    int totalChars[2] = {0, 0};

    m_Transitions = { { 0,0 },{ 0, 0 } };
    double totalTransitions[2] = {0, 0};

    double total = 0;
    for (int i = 0; i < m_Genome.length(); i++)
    {
        m_Emissions[m_Genome[i]][m_ViterbiPath[i] - '0']++;
        totalChars[m_ViterbiPath[i] - '0']++;
        if (i != 0)
        {
            m_Transitions[m_ViterbiPath[i-1] - '0'][m_ViterbiPath[i] - '0']++;
            totalTransitions[m_ViterbiPath[i-1] - '0']++;
        }
    }

    // Compute the next set of transition matrix
    // i to j / all from i
    for (int i = 0; i < 2; i++)
        for (int j = 0; j < 2; j++)
            m_Transitions[i][j] = m_Transitions[i][j]/totalTransitions[i];

    // Compute the emission matrix
    m_Emissions['a'][0] /= totalChars[0];
    m_Emissions['g'][0] /= totalChars[0];
    m_Emissions['c'][0] /= totalChars[0];
    m_Emissions['t'][0] /= totalChars[0];
    m_Emissions['a'][1] /= totalChars[1];
    m_Emissions['g'][1] /= totalChars[1];
    m_Emissions['c'][1] /= totalChars[1];
    m_Emissions['t'][1] /= totalChars[1];
}

void Viterbi::ViterbiPath()
{
    m_ViterbiPath = "";
    // Find the cell with max probability
    int endState = -1;
    int maxEndState = -INT_MAX;
    for (int i = 0; i < 2; i++) 
    { 
        if (maxEndState < m_dp[i][m_Genome.length() - 1])
        {
            maxEndState = m_dp[i][m_Genome.length() - 1];
            endState = i;
        }
    }

    // Trace the path.
    // create the strings when we are in state 2.
    vector<string> results;
    vector<pair<int,int>> positionsOfResults;
    string temp;
    for (int position = m_Genome.length() -1 ; position >= 0; position--)
    {
        m_ViterbiPath += to_string(endState);
        endState = m_Traceback[endState][position];
    }

    reverse(m_ViterbiPath.begin(), m_ViterbiPath.end());

    int startIndex = 0;
    int endIndex = 0;
    for (int position = 0; position < m_Genome.length(); position-- )
    {
        if (position != 0 && m_ViterbiPath[position] == '1' && m_ViterbiPath[position-1] == '0')
        {
            startIndex = position;
        }
        else if (position != 0 && m_ViterbiPath[position] == '0' && m_ViterbiPath[position - 1] == '1')
        {
            endIndex = position;
            if(temp!="")
            {
                results.push_back(temp);
                positionsOfResults.push_back(make_pair(startIndex,endIndex));
            }
        }

        if (m_ViterbiPath[position] == '1') temp += m_Genome[position];
    }

    for (int i = 0; i < positionsOfResults.size(); i++) cout<< positionsOfResults[i].first << "   " << positionsOfResults[i].second<<endl;
}
int main()
{
    Viterbi viterbi(ReadGenomeFromFna());
    viterbi.StartTask();

    getchar();
    getchar();
    return 0;
}

