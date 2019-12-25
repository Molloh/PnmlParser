#include "tinyxml2.h"
#include <iostream>
#include <string>
#include <vector>

using namespace std;
using namespace tinyxml2;

struct Place {
    string id;
    string name;
};

struct Transition {
    string id;
    string name;
};

struct Arc {
    string id;
    string source;
    string target;
};

vector<Place> places;
vector<Transition> trans;
vector<Arc> arcs;

int main() {
    XMLDocument doc;
    doc.LoadFile("Model1.pnml");
    XMLElement *net = doc.RootElement()->FirstChildElement("net");
    XMLElement *node = net->FirstChildElement("place");
    while(node != NULL) {
        // cout << node->Name() << " " << node->Attribute("id") << endl;
        if(strcmp(node->Name(), "place") == 0) {
            // cout << node->FirstChildElement("name")->FirstChildElement("text")->GetText() << endl;
            Place p = {
                node->Attribute("id"),
                node->FirstChildElement("name")->FirstChildElement("text")->GetText()
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
    
    for(auto n: places) 
        cout << "places " << "id: " << n.id << " name: " << n.name << endl;
    for(auto n: trans) 
        cout << "transitions " << "id: " << n.id << " name: " << n.name << endl;
    for(auto n: arcs)
        cout << "arcs " << "id: " << n.id << " source: " << n.source << " target: " << n.target << endl;
}