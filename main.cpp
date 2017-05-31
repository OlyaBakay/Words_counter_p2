#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <algorithm>
#include <mutex>
#include <thread>
#include <vector>
#include "measuring_time.h"
#include <condition_variable>
#include <atomic>
#include <deque>
#include <ctype.h>

using namespace std;
mutex mx, mx_2, mx_3;
map<string, int> all_counted;
condition_variable cv;
atomic <bool> finished = {false};
deque<vector<string>> queue_of_vects;

bool diff_func(const pair<string, int> &a, const pair<string, int> &b){
    return a.second < b.second;
}

void alph_num_order(string f1, string f2) {
    ofstream f_a;
    f_a.open(f1);
    for (auto el : all_counted) f_a << el.first << " - " << el.second << endl;
    f_a.close();

    vector<pair<string, int> > vector_of_pairs(all_counted.begin(), all_counted.end());
    sort(vector_of_pairs.begin(), vector_of_pairs.end(), diff_func);
    ofstream f_n;
    f_n.open(f2);
    for (pair<string, int> item: vector_of_pairs){
        f_n << item.first << " - " << item.second << endl;
    }
    f_n.close();

}

map<string, string> read_config(string filename) {
    string line;
    ifstream myfile;
    map<string, string> mp;
    myfile.open(filename);

    if (myfile.is_open())
    {
        while (getline(myfile,line))
        {
            int pos = line.find("=");
            string key = line.substr(0, pos);
            string value = line.substr(pos + 1);
            mp[key] = value;
        }

        myfile.close();
    }
    else {
        cout << "Error with opening the file!" << endl;
    }
    return mp;
};

template <class T>
T get_param(string key, map<string, string> myMap) {
    istringstream ss(myMap[key]);
    T val;
    ss >> val;
    return val;
}

void check_word(string& word) {
    string new_word = "";
    transform(word.begin(), word.end(), word.begin(), ::tolower);
    for (auto i : word) {
        if (!ispunct(i) && !isdigit(i)) {
            new_word += i;
        }
    }
    word = new_word;
}

void data_producer(const string &file_name){
    fstream f(file_name);
    if(f.is_open()){
        string line;
        int size_of_block = 50;
        vector<string> all_lines;
        int num_of_lines = 0;
        while (f >> line){
            all_lines.push_back(line);
            if (size_of_block != num_of_lines + 1){
                ++num_of_lines;
            }
            else {
                {
                    lock_guard<mutex> locker(mx);
                    queue_of_vects.push_back(all_lines);
                }
                cv.notify_one();
                all_lines.clear();
                num_of_lines = 0;
            }
        }

        if (all_lines.size() != 0){
            {
                lock_guard<mutex> locker(mx_3);
                queue_of_vects.push_back(all_lines);
            }
            cv.notify_one();
        }
        cv.notify_all();
        finished = true;
    }
    else{
        cerr<< "Error with opening the file!" << endl;
    }
}

void data_consumer(){
    while(!finished){
        unique_lock<mutex> locker(mx);
        if(queue_of_vects.empty()){
            cv.wait(locker);
        }
        else
        {
            vector<string> data = queue_of_vects.front();
            queue_of_vects.pop_front();
            locker.unlock();
            for(auto i : data) {
                check_word(i);
                if (!i.empty()) {
                    lock_guard<mutex> lockGuard(mx_2);
                    ++all_counted[i];
                }
            }
        }
    }
}

int main(){
    string filename;
    cout << "Please enter name of configuration file with extension '.txt':>";
    cin >> filename;    // config.txt
    auto start_time = get_current_time_fenced();
    map<string, string> mp = read_config(filename);
    
    if (!mp.empty()) {
        string infile = get_param<string>("infile", mp);
        string out_by_a = get_param<string>("out_by_a", mp);
        string out_by_n = get_param<string>("out_by_n", mp);
        int num_thr = get_param<int>("threads", mp);

        auto start_reading = get_current_time_fenced();
        thread thr = thread(data_producer, infile);
        auto finish_reading = get_current_time_fenced();

        auto start_thr = get_current_time_fenced();
        thread threads[num_thr];
        for (int i = 0; i < num_thr; ++i){
            threads[i] = thread(data_consumer);
        }
        thr.join();
        
        for (int j = 0; j < num_thr; ++j){
            threads[j].join();
        }
        auto finish_thr = get_current_time_fenced();
        all_counted.erase("");
        alph_num_order(out_by_a, out_by_n);
        auto final_time = get_current_time_fenced();

        chrono::duration<double, milli> total_time = final_time - start_time;
        chrono::duration<double, milli> reading_time = finish_reading - start_reading;
        chrono::duration<double, milli> analyzing_time = finish_thr - start_thr;

        cout << "Total: " << total_time.count() << " ms" << endl;
        cout << "Reading time: " << reading_time.count() << " ms" << endl;
        cout << "Analyzing: " << analyzing_time.count() << " ms" << endl;
    }

}
