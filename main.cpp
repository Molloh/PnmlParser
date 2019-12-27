#include <iostream>
#include <string>
#include <vector>
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
Place *p_end;
Place *p_pre;
vector<Place> places;
vector<Transition> trans;
vector<Arc> arcs;
vector<vector<Transition*>> paths;
vector<Transition*> active_path;

void coverageTrans(Place *pre_place, Transition *t, vector<Transition*> &path);
void coveragePlaces(Place *p, vector<Transition*> &path);
void initVisitStatus(Transition *t);

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

void initVisitStatus(Transition *t) {
    t->visted = false;
    vector<Place*> next = t->next_p;
    for(auto p: next) {
        for(auto n: p->next_t) {
            initVisitStatus(n);
        }
    }
}

// get path after a transition
void coverageTrans(Place *pre_place, Transition *t, vector<Transition*> &path) {
    bool canMove = true;
    for(auto p: t->pre_p)
        if(!p->token)
            canMove = false;
    
    if(canMove) {
        initVisitStatus(t);
        t->visted = true;
        vector<Transition*> pre_path;
        pre_path.assign(path.begin(), path.end());
        path.push_back(t);
        vector<Place*> next = t->next_p;
        int next_size = next.size();
        if(next_size == 1) {
            // move to next place
            coveragePlaces(next[0], path);
            // trace back to the previous place
            coveragePlaces(pre_place, pre_path);
        }else {
            vector<Transition*> init_path;

        }
    }else {
        // trace back to parallel occurrence
    }
}

// get path after a place
void coveragePlaces(Place *p, vector<Transition*> &path) {
    p->token = 1;
    vector<Transition*> next = p->next_t;
    if(next.empty()) {
        paths.push_back(path);
    }else {
        for(int i = 0; i < next.size(); i ++) {
            if(!next[i]->visted) {
                coverageTrans(p, next[i], path);
                break;
            }
        }
    }
}



int main() {
    readPnml();
    generateGraph();
    
    vector<Transition*> path;
    coveragePlaces(p_start, path);

    // print all paths
    for(int i = 0; i < paths.size(); i ++){
        cout << "case" << i + 1 << ": ";
        for(int j = 0; j < paths[i].size(); j ++) {
            if(j == paths[i].size() - 1)
                cout << paths[i][j]->name << endl;
            else
                cout << paths[i][j]->name << "->";
        }
    }
}