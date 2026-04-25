// Graph_traversals.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <stack>
#include <fstream>
#include <algorithm>
#include <set>
#include <iomanip>

using namespace std;

class WebAnalyzer {
private:
    unordered_map<string, vector<string>> adj;
    unordered_map<string, vector<string>> revAdj; // Strongly connectedi hamar 
    vector<string> pages;

    void dfs1(const string& v, unordered_map<string, bool>& visited, stack<string>& s) {
        visited[v] = true;
        auto it = adj.find(v);
        if (it != adj.end()) {
            for (const auto& neighbor : it->second) {
                if (!visited[neighbor]) dfs1(neighbor, visited, s);
            }
        }
        s.push(v);
    }

    void dfs2(const string& v, unordered_map<string, bool>& visited, vector<string>& component) {
        visited[v] = true;
        component.push_back(v);
        auto it = revAdj.find(v);
        if (it != revAdj.end()) {
            for (const auto& neighbor : it->second) {
                if (!visited[neighbor]) dfs2(neighbor, visited, component);
            }
        }
    }

public:
    // Crawler, bfsi himan vra
    void crawl(const string& rootPage) {
        queue<string> q;
        set<string> visited;

        q.push(rootPage);
        visited.insert(rootPage);

        cout << "--- Crawl Order (BFS) ---\n";

        while (!q.empty()) {
            string curr = q.front();
            q.pop();
            pages.push_back(curr);
            cout << "Visited: " << curr << endl;

            
            ifstream file(curr);
            if (!file.is_open()) {
                cerr << "Warning: cannot open " << curr << " — treating as dangling (no outgoing links)\n";
                continue;
            }

            string link;
            while (file >> link) {
                
                if (link == curr) continue;

                adj[curr].push_back(link);
                revAdj[link].push_back(curr); 
                if (visited.find(link) == visited.end()) {
                    visited.insert(link);
                    q.push(link);
                }
            }
        }
    }

  
    void findHubs() {
        unordered_map<string, int> inDegree;

        
        for (const string& page : pages) {
            inDegree[page] = 0;
        }

   
        for (auto const& kv : adj) {
            const vector<string>& neighbors = kv.second;
            for (const string& destinationPage : neighbors) {
                if (destinationPage == kv.first) continue;
                inDegree[destinationPage]++;
            }
        }

  
        string hub = "None";
        int maxDegree = -1;

        for (auto const& kv : inDegree) {
            const string& page = kv.first;
            int count = kv.second;
            if (count > maxDegree) {
                maxDegree = count;
                hub = page;
            }
        }

        if (hub != "None") {
            cout << "\n Hub Analysis " << endl;
            cout << "Main Hub: " << hub << " (referenced by " << maxDegree << " pages)" << endl;
        }
        else {
            cout << "No links found to analyze." << endl;
        }
    }

    // SCC
    vector<vector<string>> computeSCCs(unordered_map<string, int>& pageToComp) {
        stack<string> s;
        unordered_map<string, bool> visited;

        // first pass
        for (auto const& page : pages) {
            if (!visited[page]) dfs1(page, visited, s);
        }

        // second pass
        visited.clear();
        vector<vector<string>> components;
        while (!s.empty()) {
            string v = s.top();
            s.pop();
            if (!visited[v]) {
                vector<string> component;
                dfs2(v, visited, component);
                components.push_back(component);
            }
        }

        
        pageToComp.clear();
        for (size_t i = 0; i < components.size(); ++i) {
            for (const string& p : components[i]) pageToComp[p] = static_cast<int>(i);
        }

        return components;
    }

    // print SCCs
    void printSCCs() {
        unordered_map<string, int> pageToComp;
        auto components = computeSCCs(pageToComp);
        cout << "\n--- Strongly Connected Components (" << components.size() << ") ---\n";
        for (size_t i = 0; i < components.size(); ++i) {
            cout << "Component " << i << ": { ";
            for (const auto& p : components[i]) cout << p << " ";
            cout << "}\n";
        }
    }

   
    vector<int> topologicalSortCondensed() {
        unordered_map<string, int> pageToComp;
        auto components = computeSCCs(pageToComp);
        int k = static_cast<int>(components.size());
        if (k == 0) return {};

        
        vector<unordered_set<int>> condAdjSet(k);
        vector<vector<int>> condAdj(k);
        vector<int> inDegree(k, 0);

        for (const auto& kv : adj) {
            const string& u = kv.first;
            const vector<string>& neighbors = kv.second;
            int cu = pageToComp[u];
            for (const string& v : neighbors) {
                int cv = pageToComp[v];
                if (cu == cv) continue; 
                if (condAdjSet[cu].insert(cv).second) {
                    condAdj[cu].push_back(cv);
                    inDegree[cv]++;
                }
            }
        }


        queue<int> q;
        for (int i = 0; i < k; ++i) if (inDegree[i] == 0) q.push(i);

        vector<int> topoOrder;
        while (!q.empty()) {
            int node = q.front(); q.pop();
            topoOrder.push_back(node);
            for (int nb : condAdj[node]) {
                if (--inDegree[nb] == 0) q.push(nb);
            }
        }

        if (static_cast<int>(topoOrder.size()) != k) {
            cerr << "Warning: topological sort on condensed graph did not include all components — cycle detected (unexpected)\n";
            return {};
        }

        // Print topological order
        cout << "\n--- Topological Order of Condensed SCC DAG ---\n";
        for (int compId : topoOrder) {
            cout << "Comp " << compId << ": { ";
            for (const auto& p : components[compId]) cout << p << " ";
            cout << "}\n";
        }

        return topoOrder;
    }

    // Bonus:  PageRank 
    void computePageRank(int iterations = 5, double damping = 0.85) {
        int n = static_cast<int>(pages.size());
        if (n == 0) {
            cout << "\n--- PageRank Results ---\nNo pages to rank.\n";
            return;
        }

        unordered_map<string, double> rank;
        for (const string& p : pages) rank[p] = 1.0 / n;

        for (int iter = 0; iter < iterations; ++iter) {
            unordered_map<string, double> nextRank;
            for (const string& p : pages) nextRank[p] = (1.0 - damping) / n;

            double danglingSum = 0.0;
            for (const string& u : pages) {
                auto it = adj.find(u);
                size_t outDegree = (it != adj.end()) ? it->second.size() : 0;
                if (outDegree == 0) {
                  
                    danglingSum += rank[u];
                }
                else {
                    double share = rank[u] / static_cast<double>(outDegree);
                    for (const string& v : it->second) {
                        if (v == u) continue;
                        nextRank[v] += damping * share;
                    }
                }
            }

            double danglingContribution = damping * danglingSum / n;
            for (const string& v : pages) nextRank[v] += danglingContribution;

            rank = std::move(nextRank);
        }

        // Sort and print rankings
        vector<pair<string, double>> sortedRanks(rank.begin(), rank.end());
        sort(sortedRanks.begin(), sortedRanks.end(), [](const pair<string, double>& a, const pair<string, double>& b) {
            if (a.second == b.second) return a.first < b.first;
            return a.second > b.second;
            });

        cout << "\nPageRank Results (iterations=" << iterations << ")\n";
        cout.setf(ios::fixed);
        cout.precision(6);
        for (const auto& p : sortedRanks) {
            cout << p.first << ": " << p.second << endl;
        }
    }

    // Complexity analysis
    void printComplexityAnalysis() {
        cout << "\nComplexity Analysis\n";
        cout << "BFS Crawl: Time O(V + E), Space O(V + E) (V = number of pages discovered, vertices , E = number of links read)\n";
        cout << "SCC detection (Kosaraju): Time O(V + E), Space O(V + E)\n";
        cout << "Topological sort on condensed DAG: Let V' = components, E' = edges between components.\n";
        cout << "  Time O(V' + E'), Space O(V' + E')\n";
        cout << "PageRank (simplified, t iterations): Time O(t * (V + E)), Space O(V + E)\n";
    }
};

int main() {
    WebAnalyzer crawler;

    
    crawler.crawl("page03.txt");

    crawler.findHubs();

    
    crawler.printSCCs();

    crawler.topologicalSortCondensed();

    crawler.computePageRank(10);

    crawler.printComplexityAnalysis();

    return 0;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu