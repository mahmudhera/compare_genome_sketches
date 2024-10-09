#include <iostream>
#include <zlib.h>
#include <string>
#include <list>
#include <unordered_map>
#include <vector>
#include <queue>
#include <algorithm>
#include <unordered_set>
#include <fstream>

#include "json.hpp"

using json = nlohmann::json;
using namespace std;

typedef unsigned long long int hash_t;


class HashValueMembersOf {
public:
    hash_t key; // Key member variable
    vector<int> members_of; // List of integers
    bool appears_in_query = false;

    // Constructor to initialize key and optionally initialize the list
    HashValueMembersOf(hash_t k, int member) : key(k) {
        members_of.push_back(member);
    }

    // Add a member to the list
    void addMember(int member) {
        members_of.push_back(member);
    }

    // Print key and members_of
    void printMembers() const {
        std::cout << "Key: " << key << " Members of: ";
        for (const auto& val : members_of) {
            std::cout << val << " ";
        }
        std::cout << std::endl;
    }
};

// Custom comparator to make the priority_queue act as a min-heap
struct Compare {
    bool operator()(const HashValueMembersOf* lhs, const HashValueMembersOf* rhs) const {
        return lhs->key > rhs->key; // Min-heap based on key
    }
};

class MinHeap {
private:
    priority_queue<HashValueMembersOf*, vector<HashValueMembersOf*>, Compare> heap;
    unordered_map<hash_t, HashValueMembersOf*> lookup; // Hash map to keep track of keys

public:
    // Insert function: If the key exists, append to members_of; if not, create a new object
    void insert(hash_t key, int member_of) {
        if (lookup.find(key) == lookup.end()) {
            // Key not found, create new MyClass object and insert it into the heap
            HashValueMembersOf* newObject = new HashValueMembersOf(key, member_of);
            heap.push(newObject);
            lookup[key] = newObject; // Track the new object in the map
        } else {
            // Key found, append member_of to the existing object
            lookup[key]->addMember(member_of);
        }
    }

    // Pop the top (min) element from the heap
    HashValueMembersOf* pop() {
        if (heap.empty()) {
            return nullptr;
        }

        HashValueMembersOf* top = heap.top();
        heap.pop();
        lookup.erase(top->key); // Remove from the lookup map
        return top;
    }

    // Check if the heap is empty
    bool isEmpty() const {
        return heap.empty();
    }

    // Print the entire heap (min-heap order cannot be guaranteed)
    void printHeap() {
        std::priority_queue<HashValueMembersOf*, std::vector<HashValueMembersOf*>, Compare> tempHeap = heap;
        
        std::cout << "Heap keys in lookup" << std::endl;
        for (const auto& pair : lookup) {
            std::cout << pair.first << " ";
        }
        std::cout << std::endl;

        std::cout << "Heap contents: " << std::endl;
        while (!tempHeap.empty()) {
            HashValueMembersOf* top = tempHeap.top();
            top->printMembers();
            tempHeap.pop();
        }
    }

    void memberAppearsInQuery(hash_t key) {
        if (lookup.find(key) != lookup.end()) {
            lookup[key]->appears_in_query = true;
        } else {
            std::cout << "Key not found in lookup" << std::endl;
        }
    }
};

string decompressGzip(const std::string& filename) {
    // Open file
    gzFile file = gzopen(filename.c_str(), "rb");
    if (!file) {
        throw runtime_error("Failed to open gzip file.");
    }

    // Buffer for decompressed data
    const size_t bufferSize = 8192;
    vector<char> buffer(bufferSize);
    string decompressedData;

    int bytesRead;
    while ((bytesRead = gzread(file, buffer.data(), bufferSize)) > 0) {
        decompressedData.append(buffer.data(), bytesRead);
    }

    gzclose(file);
    return decompressedData;
}


vector<hash_t> read_min_hashes(const string& json_filename) {
    auto jsonData = json::parse(decompressGzip(json_filename));
    std::vector<hash_t> min_hashes = jsonData[0]["signatures"][0]["mins"];
    return min_hashes;
}

vector<vector<hash_t>> read_sketches(vector<string> sketch_names)
{
    vector<vector<hash_t>> sketches;
    for (const auto& sketch_name : sketch_names) {
        vector<hash_t> min_hashes = read_min_hashes(sketch_name);
        sketches.push_back(min_hashes);
    }
    return sketches;
}

vector<string> get_sketch_names(const string& filelist) {
    // the filelist is a file, where each line is a path to a sketch file
    ifstream file(filelist);
    vector<string> sketch_names;
    string line;
    while (getline(file, line)) {
        sketch_names.push_back(line);
    }
    return sketch_names;
}

unordered_map<size_t, size_t> compare_against_query(int query_index, vector<vector<hash_t>> sketches) {
    unordered_map<size_t, size_t> results;

    // create an array of indices to keep track of the current element of each list
    std::vector<size_t> indices(sketches.size(), 0);

    // create a minheap to store the first element of all these lists
    MinHeap minHeap;

    for (int i = 0; i < sketches.size(); i++) {
        if (!sketches[i].empty()) {
            minHeap.insert(sketches[i][0], i);
            if (i == query_index) {
                minHeap.memberAppearsInQuery(sketches[i][0]);
            }
            indices[i] = 1;
        }
    }

    // while the heap is not empty
    while (!minHeap.isEmpty()) {

        // pop the min element from the heap
        HashValueMembersOf* minElement = minHeap.pop();

        // get the lists that contain this element using the member_of list
        std::vector<int> list_ids_where_this_element_a_member = minElement->members_of;

        // if the element appears in the query, increment the count for each list that contains this element
        if (minElement->appears_in_query) {
            for (int i = 0; i < list_ids_where_this_element_a_member.size(); i++) {
                int list_id = list_ids_where_this_element_a_member[i];
                if (results.find(list_id) == results.end()) {
                    results[list_id] = 1;
                } else {
                    results[list_id]++;
                }
            }
        }

        // for each list that contains this element
        for (int i = 0; i < list_ids_where_this_element_a_member.size(); i++) {
            int list_id1 = list_ids_where_this_element_a_member[i];
            
            // if the index is less than the size of the list, insert the element at that index into the heap
            if (indices[list_id1] < sketches[list_id1].size()) {
                auto next_element = sketches[list_id1][indices[list_id1]];
                minHeap.insert(next_element, list_id1);
                if (list_id1 == query_index) {
                    minHeap.memberAppearsInQuery(next_element);
                }

                // increment the index for that list
                indices[list_id1]++; 

            }
        }

        // delete the min element
        delete minElement;

    }    

    return results;
}


int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " filelist outputfile" << std::endl;
        return 1;
    }

    auto now = chrono::system_clock::now();

    // get the sketch names
    vector<string> sketch_names = get_sketch_names(argv[1]);

    // read the sketches
    vector<vector<hash_t>> sketches = read_sketches(sketch_names);

    int query_index = 0;
    unordered_map<size_t, size_t> results = compare_against_query(query_index, sketches);

    // write the results to stdout
    for (const auto& result : results) {
        cout << result.first << " " << result.second << std::endl;
    }

    auto end = chrono::system_clock::now();

    // show time needed
    chrono::duration<double> elapsed_seconds = end - now;
    cout << "elapsed time: " << elapsed_seconds.count() << "s\n";

    return 0;

}