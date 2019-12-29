#include <iostream>
#include <string>
#include <vector>
#include <stack>
#include "tinyxml2.h"

using namespace std;
using namespace tinyxml2;

struct Place;
struct Transition;
struct Arc;

struct Place {
    string id; 
    string name;
    int level;
    int token = 0;
    vector<Transition*> next_t;
    vector<Transition*> pre_t;
};

struct Transition {
    string id;
    string name;
    int token = 0;
    bool visted = false;
    vector<Place*> next_p;
    vector<Place*> pre_p;
};

struct Arc {
    string id;
    string source;
    string target;
};

// global variables
Place *p_start;
Transition *t_parallel_end;
Place *p_parallel;
vector<Place> places;
vector<Transition> trans;
vector<Arc> arcs;
vector<vector<Transition*>> paths;

void readPnml();
void generateGraph();
void initVisitStatus(Transition *t);
int generatePlaceLevel(Place *p);
void insertToParallel(int start, vector<Transition*> &t, vector<vector<Transition*>> &res);
vector<vector<Transition*>> arrangePath(vector<vector<Transition*>> &paths);
void coveragePlaces(Place *p, vector<Transition*> &path, vector<vector<Transition*>> &parallels);
void coverageTrans(Place *pre_place, Transition *t, vector<Transition*> &path, vector<vector<Transition*>> &parallels);

// resolve pnml file
void readPnml() {
    XMLDocument doc;
    doc.LoadFile("Model1.pnml");
    XMLElement *net = doc.RootElement()->FirstChildElement("net");
    XMLElement *node = net->FirstChildElement("place");
    while(node != NULL) {
        if(strcmp(node->Name(), "place") == 0) {
            Place p = {
                node->Attribute("id"),
                node->FirstChildElement("name")->FirstChildElement("text")->GetText(),
                0
            };
            places.push_back(p);
        } else if(strcmp(node->Name(), "transition") == 0) {
            Transition t = {
                node->Attribute("id"),
                node->FirstChildElement("name")->FirstChildElement("text")->GetText()
            };
            trans.push_back(t);
        }else {
            Arc a = {
                node->Attribute("id"),
                node->Attribute("source"),
                node->Attribute("target")
            };
            arcs.push_back(a);
        }
        node = node->NextSiblingElement();
    }
    
    // print all palces, transitions and arcs
    // for(auto n: places) 
    //     cout << "places " << "id: " << n.id << " name: " << n.name << endl;
    // for(auto n: trans) 
    //     cout << "transitions " << "id: " << n.id << " name: " << n.name << endl;
    // for(auto n: arcs)
    //     cout << "arcs " << "id: " << n.id << " source: " << n.source << " target: " << n.target << endl;
}

// generate petri graph
void generateGraph() {
    int p_size = places.size();
    int t_size = trans.size();
    for(auto v: arcs) {
        string source = v.source;
        string target = v.target;
        int s, t;
        bool source_is_place = true;
        for(int i = 0; i < p_size; i ++) {
            if(places[i].id == source) {
                s = i;
                source_is_place = true;
                break;
            }
            if(places[i].id == target) {
                t = i;
                source_is_place = false;
                break;
            }
        }
        for(int i = 0; i < t_size; i ++) {
            if(trans[i].id == source) {
                s = i;
                source_is_place = false;
                break;
            }
            if(trans[i].id == target) {
                t = i;
                source_is_place = true;
                break;
            }
        }
        // add pre nodes and next nodes for each node
        if(source_is_place) {
            places[s].next_t.push_back(&trans[t]);
            trans[t].pre_p.push_back(&places[s]);
        }else {
            trans[s].next_p.push_back(&places[t]);
            places[t].pre_t.push_back(&trans[s]);
        }
    }
    
    for(int i = 0; i < p_size; i ++)  {
        if(places[i].pre_t.empty())
            p_start = &places[i];

        // add visit level for each place
        places[i].level = generatePlaceLevel(&places[i]);
    }
    
}

// reset the later transitions visit status if a transition has not been visited
void initVisitStatus(Transition *t) {
    t->visted = false;
    vector<Place*> next = t->next_p;
    for(auto p: next) {
        for(auto n: p->next_t) {
            initVisitStatus(n);
        }
    }
}

void initVisitStatus(Transition *t, Place *exclude_p) {
    t->visted = false;
    vector<Place*> next = t->next_p;
    for(auto p: next) {
        if(p->id != exclude_p->id) {
            for(auto n: p->next_t) {
                initVisitStatus(n);
            }            
        }
    }
}

// generate level for each place
int generatePlaceLevel(Place *p) {
    if(p->next_t.empty())
        return 0;
    else {
        int level = p->next_t.size();
        for(auto t: p->next_t) {
            for(auto n: t->next_p) {
                level += generatePlaceLevel(n);
            }
        }
        return level;
    }
}

// used by arrangePath
void insertToParallel(int start, vector<Transition*> &t, vector<vector<Transition*>> &res) {
    while(start > -1) {
        vector<vector<Transition*>> tmp;
        for(auto p: res) {
            int pos = 0;
            for(int k = 0; k < p.size(); k ++) {
                if(p[k]->id == t[start + 1]->id) {
                    pos = k;
                    break;
                }
            }
            for(int m = -1; m < pos; m ++) {
                vector<Transition*> tmp_p;
                tmp_p.assign(p.begin(), p.end());
                tmp_p.insert(tmp_p.begin() + m + 1, t[start]);
                tmp.push_back(tmp_p);
            }
        }
        res.assign(tmp.begin(), tmp.end());
        start --;
    }
}

// permutation
void permutation(vector<vector<Transition*>> &paths, vector<Transition*> &path, int begin, int end) {
    int i;
    if(begin == end) {
        paths.push_back(path);
    }else {
        for(i = begin; i <= end; i ++) {
            swap(path[i], path[begin]);
            permutation(paths, path, begin + 1, end);
            swap(path[i], path[begin]);
        }
    }
}

// arrange parallel paths
vector<vector<Transition*>> arrangePath(vector<vector<Transition*>> &paths) {
    vector<vector<Transition*>> res;
    vector<Transition*> fi;
    for(auto path: paths) {
        int i = path.size() - 1;
        fi.push_back(path[i]);
    }
    permutation(res, fi, 0, fi.size() - 1);

    for(auto path: paths) {
        insertToParallel(path.size() - 2, path, res);
    }

    return res;
}

// judge if the next transition can be reached
int canBeMoved(Transition *t) {
    int token_sum = 0;
    for(auto p: t->pre_p)
        token_sum += p->token;

    if(token_sum == t->pre_p.size() - 1 && token_sum > 0) {
        t_parallel_end = t;
        return 0;
    }else if(token_sum == t->pre_p.size()) {
        return 1;
    }else {
        return 0;
    }
}

Transition *t_parallel_start;

// get path after a transition
void coverageTrans(Place *pre_place, Transition *t, vector<Transition*> &path, vector<vector<Transition*>> &parallels) {
    initVisitStatus(t);
    t->visted = true;
    // remove previous places' tokens
    for(auto p: t->pre_p) {
        p->token = 0;
    }

    vector<Transition*> pre_path;
    pre_path.assign(path.begin(), path.end());
    path.push_back(t);
    vector<Place*> next = t->next_p;
    int next_size = next.size();
    if(next_size == 1) {
        // move to next place
        coveragePlaces(next[0], path, parallels);
        // trace back to the previous place
        coveragePlaces(pre_place, pre_path, parallels);
    }else {
        t_parallel_start = t;
        // save current path
        vector<vector<Transition*>> ts;
        ts.assign(parallels.begin(), parallels.end());
        ts.push_back(path);
        for(auto n: next) {
            vector<Transition*> empty_path;
            coveragePlaces(n, empty_path, ts);
        }
        // trace back to the previous place
        coveragePlaces(pre_place, pre_path, parallels);        
    }
}

void parallelTrans(Transition *t, Place *exclude_place, vector<vector<Transition*>> &parallels) {
    initVisitStatus(t, exclude_place);
    t->visted = true;
    // remove previous places' tokens
    for(auto p: t->pre_p) {
        p->token = 0;
    }

    for(auto n: t->next_p) {
        if(n != exclude_place) {
            vector<Transition*> empty_path;
            coveragePlaces(n, empty_path, parallels);
        }
    }
}

// get path after a place
void coveragePlaces(Place *p, vector<Transition*> &path, vector<vector<Transition*>> &parallels) {
    p->token = 1;
    vector<Transition*> next = p->next_t;
    if(next.empty()) {
        paths.push_back(path);
        // cout << endl;
        // for(auto x: path) {
        //     cout << x->name << "->";
        // }
        // cout << endl;
    }else {
        for(int i = 0; i < next.size(); i ++) {
            if(!next[i]->visted) {
                int flag = canBeMoved(next[i]);
                if(flag == 1) {
                    if(next[i] == t_parallel_end) {
                        // parallel end
                        parallels.push_back(path);
                        // arrange parallel paths
                        vector<vector<Transition*>> to_arrange_paths;
                        to_arrange_paths.assign(parallels.begin() + 1, parallels.end());
                        vector<vector<Transition*>> arranged = arrangePath(to_arrange_paths);
                        vector<Transition*> prefix = parallels[0];
                        for(auto item: arranged) {
                            vector<Transition*> pre_path = parallels[0];                     
                            pre_path.insert(pre_path.end(), item.begin(), item.end());
                            coverageTrans(p, next[i], pre_path, parallels);
                        }
                        parallels.clear();
                        parallels.push_back(prefix);
                    }else {
                        coverageTrans(p, next[i], path, parallels);
                    }
                }else {
                    if(next[i] == t_parallel_end) {
                        vector<vector<Transition*>> back_parallel;
                        back_parallel.push_back(parallels[0]);

                        back_parallel.push_back(path);
                        Place *start_place;
                        for(auto p: t_parallel_start->next_p) {
                            for(auto t: p->next_t) {
                                if(t->id == path[0]->id)
                                    start_place = p;
                            }
                        }
                        parallelTrans(t_parallel_start, start_place, back_parallel);
                    }else {
                        // save parallel paths
                        parallels.push_back(path);
                    }
                }
                break;
            }
        }
    }
}

int main() {
    readPnml();
    generateGraph();
    
    vector<Transition*> path;
    vector<vector<Transition*>> parallels;
    coveragePlaces(p_start, path, parallels);

    // print all paths
    for(int i = 0; i < paths.size(); i ++){
        cout << "case" << i + 1 << ": ";
        for(int j = 0; j < paths[i].size(); j ++) {
            if(j == paths[i].size() - 1)
                cout << paths[i][j]->name << endl;
            else
                cout << paths[i][j]->name;
        }
    }

    // test arrange path method
    // Transition t1; t1.id = "A"; t1.name = "A";
    // Transition t2; t2.id = "B"; t2.name = "B";
    // Transition t3; t3.id = "C"; t3.name = "C";
    // Transition t4; t4.id = "D"; t4.name = "D";
    // Transition t5; t5.id = "E"; t5.name = "E";
    // Transition t6; t6.id = "F"; t6.name = "F";
    // vector<Transition*> p1, p2;
    // p1.push_back(&t1); p1.push_back(&t2); p1.push_back(&t3);
    // p2.push_back(&t4); p2.push_back(&t5); p2.push_back(&t6);
    // vector<vector<Transition*>> tmp;
    // tmp.push_back(p1); tmp.push_back(p2);
    // vector<vector<Transition*>> res = arrangePath(tmp);
    // for(int i = 0; i < res.size(); i ++){
    //     cout << "case" << i + 1 << ": ";
    //     for(int j = 0; j < res[i].size(); j ++) {
    //         if(j == res[i].size() - 1)
    //             cout << res[i][j]->name << endl;
    //         else
    //             cout << res[i][j]->name << "->";
    //     }
    // }

}